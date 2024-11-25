#include "headers.p4"
#include "data.p4"

control Write_process(
    inout   kv_h            kv,
    inout   value_h         VALUE,
    inout   pktgen_hdr_t    pktgen,
    in      index_t         index,
    inout   metadata_t      meta)
    (bit<32> num_boxes)
{

    Register<key_t, index_t>(num_boxes) Timer_record;
    RegisterAction<key_t, index_t, bit<8>>(Timer_record)
    timer_record = {
        void apply(inout number_t register_data, out bit<8> result) {
            // result = register_data;
            register_data = meta.timestamp0;
        }
    };

    RegisterAction<key_t, index_t, bit<8>>(Timer_record)
    timer_checker = {
        void apply(inout number_t register_data, out bit<8> result) {
            if (meta.timestamp0 - register_data >= 5) {
                register_data = meta.timestamp0;
                result = 1; // timeout
            } else 
            {
                result = 0;
            }
        }
    };






    
    Register<entry_t, index_t>(num_boxes) KV_R;
    RegisterAction<entry_t, index_t, bit<32>>(KV_R)
    KV_first_W = { // first arrive
        void apply(inout entry_t entry, out bit<32> result)
        {
            if (entry.key == 0 || kv.key == entry.key)
            {
                entry.key = kv.key;
                entry.version = kv.version; // 

                result = entry.version;
            } else {
                result = 0; // collision
            }
        }
    };
    RegisterAction<entry_t, index_t, bit<32>>(KV_R) // KV_Timer
    KV_get = { // first arrive
        void apply(inout entry_t entry, out bit<32> result)
        {
            if (kv.key == entry.key)
            {
                result = entry.version;
            } else {
                result = 0;
            }
        }
    };
    RegisterAction<entry_t, index_t, bit<32>>(KV_R)
    KV_ACK_WR = { // first arrive
        void apply(inout entry_t entry, out bit<32> result)
        {
            if (kv.key == entry.key && kv.version >= entry.version) 
            {
                entry.key = kv.key;
                entry.version = kv.version; // 

                result = entry.version;
            } else {
                result = 0;
            }
        }
    };
    RegisterAction<entry_t, index_t, bit<32>>(KV_R)
    KV_ACK_Clear = { // first arrive
        void apply(inout entry_t entry, out bit<32> result)
        {
            if (kv.key == entry.key && kv.version >= entry.version) 
            {
                entry.key = 0;
                entry.version = 0;
                result = 1;
            } else {
                result = 0; // collision
            }
        }
    };
    // RegisterAction<entry_t, index_t, bit<32>>(Key_Version)
    // KV_Timer = { // first arrive
    //     void apply(inout entry_t entry, out bit<32> result)
    //     {
    //         if (entry.key != 0)
    //         {
    //             result = entry.key;
    //         } else {
    //             result = 0; // collision
    //         }
    //     }
    // };

    Register<bitmap_t, index_t>(num_boxes) Bitmap_R;
    
    RegisterAction<bitmap_t, index_t, bitmap_t>(Bitmap_R)
    bitmap_first_W = {
        void apply(inout bitmap_t bitmap, out bitmap_t result) {
            bitmap = 0;
        }
    };
    RegisterAction<bitmap_t, index_t, bitmap_t>(Bitmap_R)
    bitmap_update = {
        void apply(inout bitmap_t bitmap, out bitmap_t result) {
            bitmap = kv.bitmap | bitmap;
            result = bitmap;
        }
    };
    RegisterAction<bitmap_t, index_t, bitmap_t>(Bitmap_R)
    bitmap_get = {
        void apply(inout bitmap_t bitmap, out bitmap_t result) {
            result = bitmap;
        }
    };
    RegisterAction<bitmap_t, index_t, bitmap_t>(Bitmap_R)
    bitmap_clear = {
        void apply(inout bitmap_t bitmap, out bitmap_t result) {
            bitmap = 0;
        }
    };

    
    

    // Register<key_t, index_t>(num_boxes) Key_R;
    // RegisterAction<key_t, index_t, bit<32>>(Key_R)
    // key_First_W = {
    //     void apply(inout key_t key, out bit<32> result) {
    //         if (key == kv.key || key == 0)
    //         {
    //             key = kv.key; // key -> reg
    //             result = key; // key match
    //         } else
    //         {
    //             result = 0; // key dismatch
    //         }
    //     }
    // };
    // RegisterAction<key_t, index_t, bit<32>>(Key_R) // for Read、Write_ack, Read_ack -> match
    // key_RW = {
    //     void apply(inout key_t key, out bit<32> result)
    //     {
    //         if (key == kv.key)
    //         {
    //             result = key;
    //         } else
    //             result = 0;
    //     }
    // };
    // RegisterAction<key_t, index_t, bit<32>>(Key_R) // for Write_ack, Read_ack clear
    // key_Clear = {
    //     void apply(inout key_t key, out bit<32> result)
    //     {
    //         result = key;
    //         if (key == kv.key)
    //         {
    //             key = 0;
    //         } 
            
    //     }
    // };
    // RegisterAction<key_t, index_t, bit<32>>(Key_R) // for Timer
    // key_Get = {
    //     void apply(inout key_t key, out bit<32> result)
    //     {
    //         if (key != 0)
    //         {
    //             result = key;
    //         }
    //     }
    // };

    // Register<version_t, index_t>(num_boxes) Version_R;
    // RegisterAction<version_t, index_t, version_t>(Version_R)
    // version_first_update = {
    //     void apply(inout version_t version, out version_t result) {
    //         if (kv.version >= version)
    //         {
    //             version = kv.version;
    //             result = 1;
    //         } else {
    //             result = 0;
    //         }
    //     }
    // };
    // RegisterAction<version_t, index_t, version_t>(Version_R)
    // version_get = {
    //     void apply(inout version_t version, out version_t result) {
    //         result = version;
    //     }
    // };
    // RegisterAction<version_t, index_t, version_t>(Version_R)
    // version_ack = {
    //     void apply(inout version_t version, out version_t result) {
    //         if (kv.version >= version)
    //             result = version;
    //         else {
    //             result = 0;
    //         }
            
    //     }
    // };


    // @pragma stage 4
    @pragma immediate 1
    ProcessData0() process_data0;
    ProcessData1() process_data1;
    ProcessData2() process_data2;
    ProcessData3() process_data3;
    ProcessData4() process_data4;
    ProcessData5() process_data5;
    ProcessData6() process_data6;
    ProcessData7() process_data7;
    ProcessData8() process_data8;
    ProcessData9() process_data9;
    ProcessData10() process_data10;
    ProcessData11() process_data11;
    ProcessData12() process_data12;
    ProcessData13() process_data13;
    ProcessData14() process_data14;
    ProcessData15() process_data15;
    ProcessData16() process_data16;
    ProcessData17() process_data17;
    ProcessData18() process_data18;
    ProcessData19() process_data19;
    ProcessData20() process_data20;
    ProcessData21() process_data21;
    ProcessData22() process_data22;
    ProcessData23() process_data23;
    ProcessData24() process_data24;
    ProcessData25() process_data25;
    ProcessData26() process_data26;
    ProcessData27() process_data27;

    ProcessData28() process_data28;
    ProcessData29() process_data29;
    ProcessData30() process_data30;
    apply {
        if (meta.is_timer == 1) { // timer gen
            // @stage(1)
            // {pktgen.key = key_Get.execute(index);}
            // if (pktgen.key == 0) // null key, need reset Key_index_record in egress// miss
            // {
            //     pktgen.need_reset = 1;
            // } else { // pktgen.key
            //     // meta.isTimeOut = timer_checker.execute(meta.key_index);
            //     // @stage (2)
            //     pktgen.version = version_get.execute(index);
            //     @stage(3)
            //     {pktgen.out_bitmap = bitmap_get.execute(meta.key_index);}
            //     pktgen.need_reset = 0;
            // }
        } else if (kv.ack == 1 && kv.bitmap == 0x7) { // for read/write resubmit // 111
            @stage(1)
            {meta.isMatch = KV_ACK_Clear.execute(index);}
            if (meta.isMatch != 0) {
                bitmap_clear.execute(index);
            }
        } else if (kv.ack == 0 && kv.opcode == 1) {
            @stage(1)
            {meta.isMatch = KV_first_W.execute(index);}
            if (meta.isMatch != 0) {
                bitmap_first_W.execute(index);
                kv.num_log = kv.num_log - 1;
                kv.log_index = kv.log_index + 1;    
            } else { // collision sendBack
                kv.flags = 0x8;
            }
        } else if (kv.ack == 0 && kv.opcode == 0){
            @stage(1)
            {meta.isMatch = KV_get.execute(index);}
            if (meta.isMatch != 0){
                kv.version = meta.isMatch;
                kv.bitmap = bitmap_get.execute(index);
            }
        } else if (kv.ack == 1 && kv.bitmap != 0x7) // 111
        {
            @stage(1)
            {meta.isMatch = KV_ACK_WR.execute(index);}
            if (meta.isMatch != 0) {
                kv.bitmap = bitmap_update.execute(index);
            }
        }
            
            
        // The following register operations need to be used with ipv4_host and are not enabled for debugging.
        // @stage(4)    
        // PROCESS_ENTRY
    }

    
}