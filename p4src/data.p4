/* todo define the register values as signed */

typedef bit<32> const_t;
const const_t total_reg_cnt = 32w0x9fff;
typedef bit<16> total_value_idx_t;
typedef bit<32> data_type_t;

/* note that in the case of that the sume is already saturateed, 
        it is possible that the max positive + negative value;*/
// && value > -2147483647 should add this;
//&& register_lo > -2147483647
#define REG_TABLES(n)                                                                                           \
control ProcessData##n(                                                                                       \
                        in      bit<8>          is_timer,                                                       \
                        in      bit<8>          opcode,                                                         \
                        in      bit<8>          ack,                                                            \
                        inout   bit<32>         VALUE,                                                          \
                        in      index_t         index                                                           \
  ){                                                                                                            \
    Register<bit<32>, total_value_idx_t>(total_reg_cnt) register##n;                                            \
    RegisterAction<bit<32>, total_value_idx_t, data_type_t>(register##n) write_data_entry##n= {                 \
        void apply(inout bit<32> value, out data_type_t rv){                                                    \
            value = VALUE;                                                                                      \
        }                                                                                                       \
    };                                                                                                          \
    action write_entry##n() {                                                                                   \
        write_data_entry##n.execute(index);                                                   \
    }                                                                                                           \
                                                                                                                \
    RegisterAction<bit<32>, total_value_idx_t, data_type_t>(register##n) read_data_entry##n= {                  \
        void apply(inout bit<32> value, out data_type_t rv){                                                    \
            rv = value;                                                                                         \
        }                                                                                                       \
    };                                                                                                          \
    action read_entry##n() {                                                                                    \
        VALUE = read_data_entry##n.execute(index);                                                                      \
    }                                                                                                           \
    table processEntry##n {                                                                                     \
        key =  {                                                                                                \
            opcode : exact;                                                                                     \
            ack    : exact;                                                                                     \    
        }                                                                                                       \
        actions = {                                                                                             \
            read_entry##n;                                                                                      \
            write_entry##n;                                                                                     \
            NoAction;                                                                                           \
        }                                                                                                       \
        const default_action = NoAction;                                                                        \
        size = 2;                                                                                               \
    }                                                                                                           \
    apply{                                                                                                      \
        processEntry##n.apply();                                                                                \
    }                                                                                                           \
                                                                                                                \
};                                                                                                               \

#define PROCESS_ENTRY                                                                           \
process_data0.apply( meta.is_timer, kv.opcode, kv.ack, VALUE.value_0, meta.key_index);          \
process_data1.apply( meta.is_timer, kv.opcode, kv.ack, VALUE.value_1, meta.key_index);          \
process_data2.apply( meta.is_timer, kv.opcode, kv.ack, VALUE.value_2, meta.key_index);          \
process_data3.apply( meta.is_timer, kv.opcode, kv.ack, VALUE.value_3, meta.key_index);          \
process_data4.apply( meta.is_timer, kv.opcode, kv.ack, VALUE.value_4, meta.key_index);          \
process_data5.apply( meta.is_timer, kv.opcode, kv.ack, VALUE.value_5, meta.key_index);          \
process_data6.apply( meta.is_timer, kv.opcode, kv.ack, VALUE.value_6, meta.key_index);          \
process_data7.apply( meta.is_timer, kv.opcode, kv.ack, VALUE.value_7, meta.key_index);          \
process_data8.apply( meta.is_timer, kv.opcode, kv.ack, VALUE.value_8, meta.key_index);          \
process_data9.apply( meta.is_timer, kv.opcode, kv.ack, VALUE.value_9, meta.key_index);          \
process_data10.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_10, meta.key_index);          \
process_data11.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_11, meta.key_index);          \
process_data12.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_12, meta.key_index);          \
process_data13.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_13, meta.key_index);          \
process_data14.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_14, meta.key_index);          \
process_data15.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_15, meta.key_index);          \
process_data16.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_16, meta.key_index);          \
process_data17.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_17, meta.key_index);          \
process_data18.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_18, meta.key_index);          \
process_data19.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_19, meta.key_index);          \
process_data20.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_20, meta.key_index);          \
process_data21.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_21, meta.key_index);          \
process_data22.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_22, meta.key_index);          \
process_data23.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_23, meta.key_index);          \
process_data24.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_24, meta.key_index);          \
process_data25.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_25, meta.key_index);          \
process_data26.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_26, meta.key_index);          \
process_data27.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_27, meta.key_index);          \
process_data28.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_28, meta.key_index);          \
process_data29.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_29, meta.key_index);          \
process_data30.apply(meta.is_timer, kv.opcode, kv.ack, VALUE.value_30, meta.key_index);          \
 
// @stage(4)
REG_TABLES(0) 
@stage(4)
REG_TABLES(1) 
@stage(4)
REG_TABLES(2)
@stage(4)
REG_TABLES(3)

@stage(5)
REG_TABLES(4) 
@stage(5)
REG_TABLES(5) 
@stage(5)
REG_TABLES(6) 
@stage(5)
REG_TABLES(7) 

@stage(6)
REG_TABLES(8) 
@stage(6)
REG_TABLES(9) 
@stage(6)
REG_TABLES(10) 
@stage(6)
REG_TABLES(11) 

@stage(7)
REG_TABLES(12) 
@stage(7)
REG_TABLES(13) 
@stage(7)
REG_TABLES(14) 
@stage(7)
REG_TABLES(15) 

@stage(8)
REG_TABLES(16) 
@stage(8)
REG_TABLES(17) 
@stage(8)
REG_TABLES(18) 
@stage(8)
REG_TABLES(19) 

@stage(9)
REG_TABLES(20) 
@stage(9)
REG_TABLES(21) 
@stage(9)
REG_TABLES(22) 
@stage(9)
REG_TABLES(23) 

@stage(10)
REG_TABLES(24) 
@stage(10)
REG_TABLES(25) 
@stage(10)
REG_TABLES(26) 
@stage(10)
REG_TABLES(27) 

@stage(11)
REG_TABLES(28) 
@stage(11)
REG_TABLES(29) 
@stage(11)
REG_TABLES(30) 
