/*************************************************************************
 ************* C O N S T A N T S    A N D   T Y P E S  *******************
**************************************************************************/
const bit<16> ETHERTYPE_TPID = 0x8100;
const bit<16> ETHERTYPE_IPV4 = 0x0800;
const bit<16> ETHERTYPE_IPKV = 0x0700;
// const bit<16> ETHERTYPE_PKTG = 0x88cc;
const bit<16> ETHERTYPE_PKTG = 0x1234;
const int CLIENT_HOST = 28;

/* Table Sizes */
const int IPV4_HOST_SIZE = 256;
#define NUM_CACHE 40000

/*************************************************************************
 ***********************  H E A D E R S  *********************************
 *************************************************************************/

/*  Define all the headers the program will recognize             */
/*  The actual sets of headers processed by each gress can differ */

/* Standard ethernet header */
header ethernet_h {
    bit<48>   dst_addr;
    bit<48>   src_addr;
    bit<16>   ether_type;
}

header vlan_tag_h {
    bit<3>   pcp;
    bit<1>   cfi;
    bit<12>  vid;
    bit<16>  ether_type;
}

header ipv4_h {
    bit<4>   version;
    bit<4>   ihl;
    bit<8>   diffserv;
    bit<16>  total_len;
    bit<16>  identification;
    bit<3>   flags;
    bit<13>  frag_offset; 
    bit<8>   ttl;
    bit<8>   protocol;
    bit<16>  hdr_checksum;
    bit<32>  src_addr;
    bit<32>  dst_addr;
}

header kv_h {
    bit<32> bitmap;
    bit<16> num_worker;
    bit<32> version; /// need add
    bit<8>  opcode;
    bit<8>  ack;
    bit<32> key;
    bit<16> index; //16
    bit<8> num_log; // num of log item
    bit<8> log_index; // num of log item
    // bit<32> index;
    bit<8> flags; // 1 2 4 8 using 4 for scheduling reading
    bit<32> seq;
}
header value_h {
    bit<32> value_0;
    bit<32> value_1;
    bit<32> value_2;
    bit<32> value_3;

    bit<32> value_4;
    bit<32> value_5;
    bit<32> value_6;
    bit<32> value_7;

    bit<32> value_8;
    bit<32> value_9;
    bit<32> value_10;
    bit<32> value_11;

    bit<32> value_12;
    bit<32> value_13;
    bit<32> value_14;
    bit<32> value_15;

    bit<32> value_16;
    bit<32> value_17;
    bit<32> value_18;
    bit<32> value_19;

    bit<32> value_20;
    bit<32> value_21;
    bit<32> value_22;
    bit<32> value_23;
    
    bit<32> value_24;
    bit<32> value_25;
    bit<32> value_26;
    bit<32> value_27;
    
    bit<32> value_28;
    bit<32> value_29;
    bit<32> value_30;
    
    // bit<32> value_31;

}

header pktgen_base_header_t{
    bit<3>  _pad1;
    bit<2>  pipe_id;    // Pipe id
    bit<3>  app_id;     // Application id
    bit<8>  _pad2;
    bit<16> batch_id;   // Start at 0 and increment to a
                        // programmed number
    bit<16> packet_id;  // Start at 0 and increment to a
                        // programmed number
}

header pktgen_hdr_t {
    bit<16> need_reset;
    bit<32> key;
    bit<32> version;
    bit<32> out_bitmap;
    // bit<16> flags_3;
}

struct entry_t {
        bit<32> key;
        bit<32> version;
}

typedef bit<16> index_t;

typedef bit<32> key_t;
typedef bit<32> version_t;
typedef bit<32> bitmap_t;
typedef bit<32> number_t;
typedef bit<64> Ntest_t;
    /***********************  H E A D E R S  ************************/

struct header_t {
    ethernet_h   ethernet;
    ipv4_h       ipv4;
    kv_h         kv;
    value_h      value;
    pktgen_hdr_t pktgen;
}

    /******  G L O B A L   I N G R E S S   M E T A D A T A  *********/

struct metadata_t {
    bitmap_t        bitmap;
    version_t       key;
    version_t       version;
    bit<32>          isMatch;
    bit<32>         AckNum;
    bit<8>          is_timer;
    pktgen_base_header_t timer;
    index_t         key_index;
    bit<32>         timestamp0;
    bit<8>          isTimeOut;
    version_t         global_version;
    bit<8> tmp_rand;
}

