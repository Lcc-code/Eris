/* -*- P4_16 -*- */

#include <core.p4>
#include <tna.p4>
// #include "headers.p4"
#include "register.p4"

const bit<3> PKTGEN_APP_ID  = 1;
const bit<9> PKTGEN_PORT_ID = 68;
#define VALUE_SIZE 32w0x0400 //32 * 4 * 8 = 1024
typedef bit<9>   portid_t;           /* Port ID -- ingress or egress port  */

/*************************************************************************
 **************  I N G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

    /***********************  P A R S E R  **************************/
parser IngressParser(
        packet_in pkt,
        out header_t hdr,
        out metadata_t meta,
        out ingress_intrinsic_metadata_t ig_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
     state start {
        pkt.extract(ig_intr_md);
        pkt.advance(PORT_METADATA_SIZE);//32w64 bit
        transition init_meta;
    }

    state init_meta {
        // meta.need_reset = 0;
        meta.is_timer = 0;
        pktgen_base_header_t pktgen_base_hdr=pkt.lookahead<pktgen_base_header_t>();
        transition select(pktgen_base_hdr.app_id) {
        // transition select(ig_intr_md.ingress_port) {
            // PKTGEN_PORT_ID : parse_pktgen_timer;
            PKTGEN_APP_ID : parse_pktgen_timer;
            default : parse_ethernet;
        }
    }
    state parse_pktgen_timer{
        pkt.extract(meta.timer);
        meta.is_timer = 1;
        transition parse_pktgen_hdr;
    }

    state parse_pktgen_hdr {
        pkt.extract(hdr.ethernet);       
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_PKTG:  parse_pktg;
            default: reject;
        }
    }

    state parse_pktg {
        pkt.extract(hdr.pktgen);       
        transition accept;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);       
        transition select(hdr.ethernet.ether_type) {
            // ETHERTYPE_IPV4  :  parse_ipv4;
            ETHERTYPE_IPV4  :  parse_ipv4;
            // ETHERTYPE_IPKV  :  parse_ipv4_kv;
            ETHERTYPE_PKTG  :  parse_pktg;
            default: reject;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition select(hdr.ipv4.protocol) {
            0xff : parse_kv;
            default: accept;
        }
    }

    state parse_kv {
        pkt.extract(hdr.kv);
        transition select(hdr.kv.log_index) {
            0 : parse_value_0;
            1 : parse_value;
            2 : parse_value;
            3 : parse_value;
            4 : parse_value;
            5 : parse_value;
            6 : parse_value;
            7 : parse_value;
            default: accept;
        }
    }
    
    state parse_value_0 {
        pkt.extract(hdr.value);
        transition accept;
    }
    state parse_value {
        pkt.advance(VALUE_SIZE);
        pkt.extract(hdr.value);
        transition accept;
    }
}


    /***************** M A T C H - A C T I O N  *********************/

control Ingress(
        inout header_t hdr,
        inout metadata_t meta,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_intr_prsr_md,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_tm_md)
{
    // old_version port 
    action sendBack() {
        ig_intr_tm_md.ucast_egress_port = ig_intr_md.ingress_port;
    }

    action send(PortId_t port) {
        hdr.kv.flags = 0x2;
        ig_intr_tm_md.ucast_egress_port = port;
    }

    action drop() {
        ig_intr_dprsr_md.drop_ctl = 1;
    }

    action multicast(MulticastGroupId_t mcast_grp) {
        ig_intr_tm_md.mcast_grp_a = mcast_grp;
    }

    // action randomRout(bit<8> tmp_rand) {
    //     if (tmp_rand <= 0x80){ // [0, 255]-> [0,85] (85, 170] (170, 255]
    //         hdr.kv.flags = 0x2; // 010 -> 20
    //         multicast(93);
    //     }
    // }

    table ipv4_host {
        key = {
            meta.tmp_rand   : ternary;
            meta.isMatch    : exact;
            hdr.kv.bitmap   : exact;
            hdr.kv.opcode   : exact;
            hdr.kv.ack      : exact;
            hdr.kv.flags    : exact; // 
        }
        actions = {
            send; drop; multicast; sendBack;
        }
        size = IPV4_HOST_SIZE;
        default_action = drop;
    }

    table ipv4_ack_bitmap {
        key = {
            // hdr.kv.opcode   : exact;
            hdr.kv.ack      : exact;
        }
        actions = {
            send;drop;
        }
        size = 1;
        default_action = drop;
    }


    table ipv4_index_0 {
        key = {
            // hdr.kv.num_log  : exact; 
            hdr.kv.log_index: exact; // 0, 1, 2, 3, 4,5,6,7
        }
        actions = {
            send; drop; multicast;
        }
        size = 48;
        default_action = drop;
    }
    table ipv4_index_0_nl {
        key = {
            // hdr.kv.num_log  : exact; 
            hdr.kv.log_index: exact; // 0, 1, 2, 3, 4,5,6,7
        }
        actions = {
            send; drop; multicast;
        }
        size = 48;
        default_action = drop;
    }


    action timer_multicast(MulticastGroupId_t mcast_grp) {
        ig_intr_tm_md.mcast_grp_a = mcast_grp;
    }

    table ipv4_timer_resubmit {
        key = {
            hdr.pktgen.need_reset  : exact;
        }
        actions = {
            send; drop;
        }
        size = IPV4_HOST_SIZE;
        default_action = drop;
    }
    table ipv4_timer_mc {
        key = {
            meta.isTimeOut  : exact;
            hdr.pktgen.out_bitmap : exact;
            meta.AckNum     : exact;
        }
        actions = {
            send; drop; timer_multicast;
        }
        size = IPV4_HOST_SIZE;
        default_action = drop;
    }
    Register<version_t, bit<16>>(size=1, initial_value=0) Global_version;
    RegisterAction<version_t, bit<16>, version_t>(Global_version)
    global_version_record = {
        void apply(inout version_t register_data, out version_t result) {
            register_data = register_data +1;
            result = register_data;
        }
    };
    Write_process(num_boxes = NUM_CACHE) write_process;

    @pragma stage 1
    Register<index_t, bit<1>>(1) Key_index_record;
    RegisterAction<index_t, bit<1>, index_t>(Key_index_record)    
    key_index_record = {
        void apply(inout index_t register_data, out index_t result) {
            if (register_data < NUM_CACHE) {
                register_data = register_data + 1;
                result = register_data;
            } else
            {
                register_data = 0;
            }
            
        }
    };

    Register<bit<8>, index_t>(65535) Seq_record;
    RegisterAction<bit<8>, index_t, bit<8>>(Seq_record)
    seq_record = {
        void apply(inout bit<8> register_data, out bit<8> result) {
            result = register_data;
            if (register_data == 0)
                register_data = 2;
        }
    };
    RegisterAction<bit<8>, index_t, bit<8>>(Seq_record)
    Seq_debug = {
        void apply(inout bit<8> register_data, out bit<8> result) {
            register_data = 0;
        }
    };
    // RegisterAction<index_t, bit<1>, index_t>(Key_index_record)
    // key_index_reset = {
    //     void apply(inout index_t register_data, out index_t result) {
    //         if (register_data != 0)
    //         {
    //             register_data = 0;
    //         }
    //     }
    // };
    
    Random<bit<8>>() random;
    
    apply {
        meta.tmp_rand = random.get(); // 4095-> [0,1365] [1366, 2730] [2730, 4095]
        meta.timestamp0=ig_intr_md.ingress_mac_tstamp[47:16];
        // if (ig_intr_md.resubmit_flag == 0)
        if (ig_intr_md.ingress_port != 40) // resubmit_flag not work，
        {
            if (hdr.pktgen.isValid())
            {
                meta.key_index = key_index_record.execute(0); // get key index
            } else if (hdr.kv.isValid())
            {
                meta.key_index = hdr.kv.index;
                if (hdr.kv.opcode == 1 && hdr.kv.ack == 0)
                    hdr.kv.version = global_version_record.execute(0);
                    // meta.global_version = global_version_record.execute(0);
            }
            if (hdr.kv.isValid() || hdr.pktgen.isValid()) {
                // multicast
                write_process.apply(hdr.kv, hdr.value, hdr.pktgen, meta.key_index, meta);
                if (meta.is_timer == 1)
                {
                    ipv4_timer_mc.apply();   
                    // ipv4_timer_resubmit.apply();
                } else
                {
                    // ipv4_host.apply();
                    // Debugging uses the following fixed logical forwarding
                    // and the dynamic configuration of the flow table is completed later.
                        if (hdr.kv.ack == 0) {
                            if (hdr.kv.opcode == 1) {
                                if (meta.isMatch != 0) {
                                    multicast(98); // WRITE MULTICAST
                                } else {
                                    hdr.kv.flags = 0x8; // hash collision
                                    sendBack();
                                }
                            } else {
                                if (meta.isMatch != 0) { // partly inconsistent read
                                    // 101/ 011/ 110/ 100/ 010/ 001
                                    // 001 -> 12 | 010 -> 20 | 100->
                                    if (hdr.kv.bitmap == 0x0) {
                                        hdr.kv.flags = 0x1;
                                        multicast(99); // [12, 20, 0] // after write read im . read scheduling 
                                    } else if (hdr.kv.bitmap == 0x1) {
                                        hdr.kv.flags = 0x1;
                                        multicast(99); // [12, 20, 0] // but only send 12
                                    } else if (hdr.kv.bitmap == 0x2) { 
                                        hdr.kv.flags = 0x2;
                                        multicast(99); // [12, 20, 0] // but only send 20
                                    } else if (hdr.kv.bitmap == 0x4) {
                                        hdr.kv.flags = 0x4;
                                        multicast(99); // [12, 20, 0] // but only send 36
                                    } else if (hdr.kv.bitmap == 0x3) { // 011
                                        if (meta.tmp_rand <= 0x80){ // [0, 255]-> [0,85] (85, 170] (170, 255]
                                            hdr.kv.flags = 0x2; // 010 -> 20
                                            multicast(92);
                                        } else {
                                            hdr.kv.flags = 0x1; // 100 -> 
                                            multicast(91);
                                        }
                                    } else if (hdr.kv.bitmap == 0x6) {
                                        if (meta.tmp_rand <= 0x80){ // [0, 255]-> [0,85] (85, 170] (170, 255]
                                            hdr.kv.flags = 0x2; // 010 -> 20
                                            multicast(93);
                                        } else {
                                            hdr.kv.flags = 0x4; // 100 -> 
                                            multicast(91);
                                        }
                                    } else if (hdr.kv.bitmap == 0x5) {
                                        if (meta.tmp_rand <= 0x80){ // [0, 255]-> [0,85] (85, 170] (170, 255]
                                            hdr.kv.flags = 0x4; // 010 -> 20
                                            multicast(92);
                                        } else {
                                            hdr.kv.flags = 0x1; // 100 -> 
                                            multicast(93);
                                        }
                                    } else if (hdr.kv.bitmap == 0x7) {
                                        hdr.kv.flags = 0x2;
                                        send(20);
                                    }                    
                                } else { // consistent read -> scheduling
                                    if (meta.tmp_rand <= 0x55){ // [0, 255]-> [0,85] (85, 170] (170, 255]
                                        hdr.kv.flags = 0x2; // 010 -> 20
                                        send(20);
                                    } else if (meta.tmp_rand <= 0xaa){
                                        hdr.kv.flags = 0x1; // 001 -> 12
                                        send(12);
                                    } else {
                                        hdr.kv.flags = 0x4; // 100 -> 
                                        send(0);
                                    }
                                }
                            }
                        } else { // ack == 1
                            if (hdr.kv.opcode == 1) {
                                if (meta.isMatch != 0) {
                                    if (ig_intr_md.ingress_port != 56) {
                                        if (hdr.kv.bitmap == 0x7) { // full 
                                            send(56); // clear Key
                                        } else { // do nothing    
                                            drop();
                                        }
                                    } else {
                                        drop();
                                    }
                                } else {
                                    drop(); // do nothing 
                                }
                            } else { // ack read
                                if (meta.isMatch != 0) {
                                    if (ig_intr_md.ingress_port != 56) {
                                        if (hdr.kv.bitmap == 0x7) { // full 
                                            send(56); // clear Key
                                        } else { // do nothing
                                            if (hdr.kv.flags == 0){
                                                send(28);
                                            } else {
                                                drop();
                                            }
                                        }
                                    } else {
                                        if (hdr.kv.flags == 0)
                                            send(28);
                                        else
                                            drop();
                                    }
                                } else {
                                    if (hdr.kv.flags == 0){
                                        send(28);
                                    } else 
                                        drop();
                                }
                            }
                        }

                        // long value(exp)
                        /*
                        if (hdr.kv.log_index != 0)
                        {
                            if (hdr.kv.num_log != 0)
                            {
                                // if log_index == 2 send loopback2
                                // if log_index == 3 send loopback3
                                ipv4_index_0_nl.apply(); // if log_index == 1 multicast include first loopback
                            } else {
                                // log_index != 1 drop
                                ipv4_index_0.apply(); // if == 1 multicast not include first loopback
                            }
                        } else {
                            // if (ig_intr_md.resubmit_flag == 0)
                            if (hdr.kv.ack == 0) {
                                if (hdr.kv.opcode == 1) {
                                    if (meta.isMatch != 0) {
                                        multicast(98); // WRITE MULTICAST
                                    } else {
                                        hdr.kv.flags = 0x8; // hash collision
                                        sendBack();
                                    }
                                } else {
                                    if (meta.isMatch != 0) { // partly inconsistent read
                                        // 101/ 011/ 110/ 100/ 010/ 001
                                        // 001 -> 12 | 010 -> 20 | 100->
                                        if (hdr.kv.bitmap == 0x0) {
                                            hdr.kv.flags = 0x1;
                                            multicast(99); // [12, 20, 0] // after write read im . read scheduling 
                                        } else if (hdr.kv.bitmap == 0x1) {
                                            hdr.kv.flags = 0x1;
                                            multicast(99); // [12, 20, 0] // but only send 12
                                        } else if (hdr.kv.bitmap == 0x2) { 
                                            hdr.kv.flags = 0x2;
                                            multicast(99); // [12, 20, 0] // but only send 20
                                        } else if (hdr.kv.bitmap == 0x4) {
                                            hdr.kv.flags = 0x4;
                                            multicast(99); // [12, 20, 0] // but only send 36
                                        } else if (hdr.kv.bitmap == 0x3) { // 011
                                            if (tmp_rand <= 0x80){ // [0, 255]-> [0,85] (85, 170] (170, 255]
                                                hdr.kv.flags = 0x2; // 010 -> 20
                                                multicast(92);
                                            } else {
                                                hdr.kv.flags = 0x1; // 100 -> 
                                                multicast(91);
                                            }
                                        } else if (hdr.kv.bitmap == 0x6) {
                                            if (tmp_rand <= 0x80){ // [0, 255]-> [0,85] (85, 170] (170, 255]
                                                hdr.kv.flags = 0x2; // 010 -> 20
                                                multicast(93);
                                            } else {
                                                hdr.kv.flags = 0x4; // 100 -> 
                                                multicast(91);
                                            }
                                        } else if (hdr.kv.bitmap == 0x5) {
                                            if (tmp_rand <= 0x80){ // [0, 255]-> [0,85] (85, 170] (170, 255]
                                                hdr.kv.flags = 0x4; // 010 -> 20
                                                multicast(92);
                                            } else {
                                                hdr.kv.flags = 0x1; // 100 -> 
                                                multicast(93);
                                            }
                                        } else if (hdr.kv.bitmap == 0x7) {}
                                        
                                    } else { // consistent read -> scheduling
                                        if (tmp_rand <= 0x55){ // [0, 255]-> [0,85] (85, 170] (170, 255]
                                            hdr.kv.flags = 0x2; // 010 -> 20
                                            send(20);
                                        } else if (tmp_rand <= 0xaa){
                                            hdr.kv.flags = 0x1; // 001 -> 12
                                            send(12);
                                        } else {
                                            hdr.kv.flags = 0x4; // 100 -> 
                                            send(0);
                                        }
                                    }
                                }
                            } else { // ack == 1
                                if (hdr.kv.opcode == 1) {
                                    if (meta.isMatch != 0) {
                                        if (ig_intr_md.ingress_port != 56) {
                                            if (hdr.kv.bitmap == 0x7) { // full 
                                                send(56); // clear Key
                                            } else { // do nothing    
                                                drop();
                                            }
                                        } else {
                                            drop();
                                        }
                                    } else {
                                        drop(); // do nothing 
                                    }
                                } else { // ack read
                                    if (meta.isMatch != 0) {
                                        if (ig_intr_md.ingress_port != 56) {
                                            if (hdr.kv.bitmap == 0x7) { // full 
                                                send(56); // clear Key
                                            } else { // do nothing
                                                if (hdr.kv.flags == 0){
                                                    send(28);
                                                } else {
                                                    drop();
                                                }
                                            }
                                        } else {
                                            drop();
                                        }
                                    } else {
                                        if (hdr.kv.flags == 0){
                                            send(28);
                                        } else 
                                            drop();
                                    }
                                }
                            }
                        }*/

                    
                }
                    
            }
        } else {
            // if (hdr.pktgen.isValid() && hdr.pktgen.need_reset == 1)
            // {
            //     meta.key_index = key_index_reset.execute(0); // get key index
            //     // ig_intr_tm_md.ucast_egress_port = 28; //test
            // }
        }

    }
}

    /*********************  D E P A R S E R  ************************/

control IngressDeparser(
        packet_out pkt,
        inout header_t hdr,
        in metadata_t meta,
        in ingress_intrinsic_metadata_for_deparser_t ig_intr_dprsr_md)
{
    apply {
        pkt.emit(hdr);
        // pkt.emit(hdr.ethernet);
        // pkt.emit(hdr.ipv4);
        // pkt.emit(hdr.kv);
        // pkt.emit(hdr.value);
    }
    // apply{}
}


/*************************************************************************
 ****************  E G R E S S   P R O C E S S I N G   *******************
 *************************************************************************/

    /***********************  H E A D E R S  ************************/

struct my_egress_headers_t {
    ethernet_h   ethernet;
    ipv4_h       ipv4;
    kv_h         kv;
    value_h      value;
    pktgen_hdr_t pktgen;
}

    /********  G L O B A L   E G R E S S   M E T A D A T A  *********/

struct my_egress_metadata_t {
    bit<8>          is_timer;
    pktgen_base_header_t timer;
    bit<32>         key_index;
}

    /***********************  P A R S E R  **************************/

parser EgressParser(packet_in        pkt,
    /* User */
    out my_egress_headers_t          hdr,
    out my_egress_metadata_t         meta,
    /* Intrinsic */
    out egress_intrinsic_metadata_t  eg_intr_md)
{
    /* This is a mandatory state, required by Tofino Architecture */
    state start {
        pkt.extract(eg_intr_md);
        transition init_meta;
    }

    state init_meta {
        meta.is_timer = 0;
        pktgen_base_header_t pktgen_base_hdr=pkt.lookahead<pktgen_base_header_t>();
        transition select(pktgen_base_hdr.app_id) {
        // transition select(ig_intr_md.ingress_port) {
            // PKTGEN_PORT_ID : parse_pktgen_timer;
            PKTGEN_APP_ID : parse_pktgen_timer;
            default : parse_ethernet;
        }
    }

    state parse_pktgen_timer{
        pkt.extract(meta.timer);
        meta.is_timer = 1;
        transition parse_pktgen_hdr;
    }

    state parse_pktgen_hdr {
        pkt.extract(hdr.ethernet);       
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_PKTG:  parse_pktg;
            default: accept;
        }
    }

    state parse_pktg {
        pkt.extract(hdr.pktgen);       
        transition accept;
    }

    state parse_ethernet {
        pkt.extract(hdr.ethernet);        
        transition select(hdr.ethernet.ether_type) {
            ETHERTYPE_IPV4:  parse_ipv4_kv;
            // ETHERTYPE_IPKV:  parse_ipv4_kv;
            default: accept;
        }
    }

    state parse_ipv4 {
        pkt.extract(hdr.ipv4);
        transition accept;
    }

    state parse_ipv4_kv {
        pkt.extract(hdr.ipv4);
        transition parse_kv;
    }

    state parse_kv {
        pkt.extract(hdr.kv);
        transition accept;
    }
}

    /***************** M A T C H - A C T I O N  *********************/

control Egress(
    /* User */
    inout my_egress_headers_t                          hdr,
    inout my_egress_metadata_t                         meta,
    /* Intrinsic */    
    in    egress_intrinsic_metadata_t                  eg_intr_md,
    in    egress_intrinsic_metadata_from_parser_t      eg_prsr_md,
    inout egress_intrinsic_metadata_for_deparser_t     eg_dprsr_md,
    inout egress_intrinsic_metadata_for_output_port_t  eg_oport_md)
{
    apply {
        // ack of write req
        if (hdr.kv.opcode == 1)
        {
            if (eg_intr_md.egress_port == 28)
            {
                hdr.ethernet.dst_addr[47:16] = 0x77777777;
                hdr.ethernet.dst_addr[15:8] = 0x77;
                hdr.kv.ack = 1;
                // hdr.kv.value.setInvalid();
            } else {
            }  
        }
        if (hdr.kv.flags == 1)
        {
            hdr.ethernet.dst_addr[47:16] = 0x05040302;
            hdr.ethernet.dst_addr[15:8] = 0x01;
            hdr.ethernet.dst_addr[7:0] = hdr.ethernet.src_addr[7:0];

        }
    }
}

    /*********************  D E P A R S E R  ************************/

control EgressDeparser(packet_out pkt,
    /* User */
    inout my_egress_headers_t                       hdr,
    in    my_egress_metadata_t                      meta,
    /* Intrinsic */
    in    egress_intrinsic_metadata_for_deparser_t  eg_dprsr_md)
{
    apply {
        pkt.emit(hdr);
        // pkt.emit(hdr.ethernet);
        // pkt.emit(hdr.ipv4);
        // pkt.emit(hdr.kv);
        // pkt.emit(hdr.value);
    }
}


/************ F I N A L   P A C K A G E ******************************/
Pipeline(
    IngressParser(),
    Ingress(),
    IngressDeparser(),
    EgressParser(),
    Egress(),
    EgressDeparser()
) pipe;

Switch(pipe) main;