#include "DMAcontext.h"
#include "packet.h"
#include "mlx5.h"
#include "log.h"
#include <atomic>
#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include <rocksdb/comparator.h>
#include <rocksdb/memtablerep.h>
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
using rocksdb::DB;
using rocksdb::Options;
using rocksdb::PinnableSlice;
using rocksdb::ReadOptions;
using rocksdb::Status;
using rocksdb::WriteBatch;
using rocksdb::WriteOptions;
rocksdb::WriteOptions* opts;
std::string kDBPath_0 = "/home/RockDB_0";
std::string kDBPath_1 = "/home/RockDB_1";
std::string kDBPath_2 = "/home/RockDB_2";

// parallism options
#define GLOBAL_MAX_FLUSH_THREAD_NUM 12
#define GLOBAL_MAX_COMPACTION_THREAD_NUM 12
#define BLOCK_SIZE 4 * 1024
#define BLOCKCACHE_SIZE 1 * 1024 * 1024 * 1024
#define BLOCKCACHE_SHARDBITS 4
#define TABLECACHE_SHARDBITS 6

#define MEMTABLE_SIZE 64 * 1024 * 1024
#define MAX_MEMTABLE_IMMUTABLE_NUM 40
#define MIN_IMMUTABLE_FLUSH_NUM 16
#define LEVEL0_SST_NUM 10

#define LEVEL1_SST_SIZE 512 * 1024 * 1024
#define LEVEL1_SST_NUM 10
#define LEVEL_TOTAKSIZE_MULTIPLIER 10
#define LEVEL_SSTSIZE_MULTIPLIER 1
#define LEVEL_NUM 7

#define WAL_BYTES_PER_SYNC 2 * 1024 * 1024
#define SYNC_WRITE false
#define DISABLE_WALL false
#define BLOOMFILTER_BITS_PER_KEY 10

static uint32_t bitmap_0 = htonl(0x1);
static int host_bitmap_0 = 0x1;
static uint32_t bitmap_1 = htonl(0x2);
static int host_bitmap_1 = 0x2;
static uint32_t bitmap_2 = htonl(0x4);
static int host_bitmap_2 = 0x4;
// static uint32_t bitmap_0 = htonl(0x2);
// static int host_bitmap_0 = 0x2;
// static uint32_t bitmap_1 = htonl(0x4);
// static int host_bitmap_1 = 0x4;
static uint32_t bitmap = 0;
static int host_bitmap = 0;
uint16_t num_worker = htonl(0x3);

int mlx_i = 0;
DB *db;
rocksdb::WriteBatch w_b;
rocksdb::Options options;
#include <rocksdb/comparator.h>
#include <rocksdb/memtablerep.h>
#include <rocksdb/db.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include "rocksdb/universal_compaction.h"
#define ENABLE_ROCKSDB true
#define ENABLE_VERSION_CHECK true
std::vector<std::atomic<int>> VERSION_VEC_CUR(30000); // 
std::vector<std::atomic<int>> VERSION_VEC_PED(30000); // 
std::vector<int> VERSION_VEC_INDEX(30000);

const int batch_num =50;
const int WRITE_BATCH = 50;
static int write_num = 0;


std::string inline uint32_to_string(uint32_t *value, size_t size) {
    std::string result;
    for (int i = 0; i < size; i++)
    {
        char buf[11];
        snprintf(buf, sizeof(buf), "%lu", value[i]);
        result.append(buf);
    }
    // std::cout << result.size() << std::endl;
    return result;
}
std::string inline uint32_to_string(uint32_t *value) {
    std::string result;

    char buf[11];
    snprintf(buf, sizeof(buf), "%lu", *value);
    
    return std::string(buf);
}

uint32_t string_to_uint32(const std::string& str) {
    char* endptr;
    uint32_t value = static_cast<uint32_t>(strtol(str.c_str(), &endptr, 10));
    
    if (endptr == str.c_str() || *endptr != '\0') {
        return 0;
    }
    return value;
}

void inline DB_READ(uint32_t *key)
{
    std::string value;
    auto stats = db->Get(ReadOptions(), uint32_to_string(key), &value);
    if (!stats.ok())
        {std::cout <<" key "<<  *key << std::endl;
        std::cout <<" key "<<  htonl(*key) << std::endl;}
    assert(stats.ok());
    // std::cout << "value : " << value << std::endl;
}


void inline DB_WRITE(uint32_t *key, uint32_t *value, size_t len){
    w_b.Put(uint32_to_string(key), uint32_to_string(value, len));
    // auto stats = w_b.Put(*opts, uint32_to_string(key), uint32_to_string(value, len));
    // assert(stats.ok());
    // DB_READ(key);
}

void inline DB_Batch_WRITE() {
    if (write_num == WRITE_BATCH) {
        auto stats = db->Write(*opts, &w_b);
        assert(stats.ok());
        w_b.Clear();
        write_num = 0;
    }
}


void async_wirte_main_loop(DMAcontext* dma, Log* log, int thread_id, bool isWrite)
{
    int cpu = 0;
    if (mlx_i == 0)
        cpu = thread_id;
    else
        cpu = thread_id + 14;
    uint32_t dst_IP = htonl(0x0d07387f);
    
    bindingCPU(cpu);
    printf("async_wirte_main_loop :%d cpu %d\n", thread_id, cpu);
    int msgs_completed = 0;
    cqe_snapshot_t cur_snapshot;
    int totoal_num = 0;

    size_t value_len = MAX_ENTRIES_PER_PACKET;
    int send_position = 0;
    int N_PKT = htons(PKTGENPROTO);
    int N_OPTKVPROTO = htons(OPTKVPROTO);
    uint16_t out_of_version;
    int ret = 0;
        

    if (dma == nullptr)
    {
        printf("failed dma\n");
        return;
    }
    while (1) {
        while (1)
        {
            msgs_completed = receive_packet(dma, &cur_snapshot, thread_id);
            if (msgs_completed != 0)
            {
                // printf("isWrite %d msg:%d \n",isWrite,msgs_completed);
                break;
            }
        }
        while (msgs_completed > 0)
        {
            if (msgs_completed >= batch_num) {

                if (send_position + batch_num >= (my_send_queue_length))
                    send_position = 0;

                for (int i = 0; i < batch_num; i++)
                {
                    uint8_t* buf = &dma->mp_recv_ring[dma->ring_head * kAppRingMbufSize];
                    ether* ether_header = reinterpret_cast<ether*>(buf);
                    if (ether_header->h_proto == N_OPTKVPROTO){ // 0x0700 = 
                        agghdr* agg_header = reinterpret_cast<agghdr*>(buf + IP_ETH_UDP_HEADER_SIZE);
                        uint32_t key = ntohl(agg_header->key);
                        uint32_t version = ntohl(agg_header->version);
                        if (DEBUG_PRINT_ALL_RECEIVING_PACKET)
                            p4ml_header_print(agg_header, "RECV_WRITE");
                        // printf("isWrite %d opcode %u\n", isWrite, agg_header->opcode);
                        uint32_t pend_version = VERSION_VEC_PED[key].load(std::memory_order_relaxed);
                        if (ENABLE_VERSION_CHECK){
                            if (pend_version < version) {// if write version is higher than cur version
                                VERSION_VEC_PED[key].store(version, std::memory_order_release);
                                int index = log->append_write((&key), agg_header->opcode, version, value_len, (agg_header->flags), (&agg_header->seq), (&agg_header->index), (agg_header->num_log)); // ntohl(data) in append
                                // printf("1 index:%d\n", index);
                                VERSION_VEC_INDEX[key] = index;
                            }
                        }
                        

                        make_aggr_layer_and_copy_to(dma->send_region + (send_position + i) * LAYER_SIZE, &bitmap, &num_worker, &agg_header->version, agg_header->opcode, 
                                                    &agg_header->key, &agg_header->index, &agg_header->seq, &agg_header->num_log, 0, 1, &dst_IP);
                    } else if (ether_header->h_proto == N_PKT) //  0x1234
                    {
                        pktgen* pktgen_header = reinterpret_cast<pktgen*>(buf + IP_ETH_HEADER_SIZE);
                        if (DEBUG_CHECK_PKTGEN)
                            pktgen_header_print_h(pktgen_header, "RECV PKTGEN");
                    }
                    dma_postback(dma);
                }
                totoal_num = totoal_num + batch_num;
        
                send_packet(dma, (LAYER_SIZE) * batch_num, send_position, thread_id, 1);
                dma_update_snapshot(dma, cur_snapshot);
                send_position = send_position + batch_num;
                msgs_completed = msgs_completed - batch_num;
            } else {
                if (send_position + msgs_completed >= (my_send_queue_length))
                    send_position = 0;
                for (int i = 0; i < msgs_completed; i++)
                {
                    uint8_t* buf = &dma->mp_recv_ring[dma->ring_head * kAppRingMbufSize];
                    ether* ether_header = reinterpret_cast<ether*>(buf);
                    if (ether_header->h_proto == N_OPTKVPROTO){ // 0x0700 = 
                        agghdr* agg_header = reinterpret_cast<agghdr*>(buf + IP_ETH_UDP_HEADER_SIZE);
                        
                        uint32_t key = ntohl(agg_header->key);
                        uint32_t version = ntohl(agg_header->version);
                        if (DEBUG_PRINT_ALL_RECEIVING_PACKET)
                            p4ml_header_print(agg_header, "RECV_WRITE");
                        // printf("isWrite %d opcode %u\n", isWrite, agg_header->opcode);
                        
                        uint32_t pend_version = VERSION_VEC_PED[key].load(std::memory_order_relaxed);
                        // printf("wwwww111 pend_version:%lu k:%lu v:%lu\n", pend_version, key, version);
                        if (ENABLE_VERSION_CHECK){
                            if (pend_version < version) {// if write version is higher than cur version
                                VERSION_VEC_PED[key].store(version, std::memory_order_release);
                                pend_version = VERSION_VEC_PED[key].load(std::memory_order_relaxed);
                                // printf("wwwww2222222-- pend_version:%lu k:%lu v:%lu\n", pend_version, key, version);
                                
                                int index = log->append_write((&key), agg_header->opcode, version, value_len, (agg_header->flags), (&agg_header->seq), (&agg_header->index), (agg_header->num_log)); // ntohl(data) in append
                                // printf("2 index:%d\n", index);
                                VERSION_VEC_INDEX[key] = index;
                            }
                        }

                        make_aggr_layer_and_copy_to(dma->send_region + (send_position + i) * LAYER_SIZE, &bitmap, &num_worker, &agg_header->version, agg_header->opcode, 
                                                    &agg_header->key, &agg_header->index, &agg_header->seq, &agg_header->num_log, 0, 1, &dst_IP);
                    } else if (ether_header->h_proto == N_PKT) //  0x1234
                    {
                        pktgen* pktgen_header = reinterpret_cast<pktgen*>(buf + IP_ETH_HEADER_SIZE);
                        if (DEBUG_CHECK_PKTGEN)
                            pktgen_header_print_h(pktgen_header, "RECV PKTGEN");
                    }
                    dma_postback(dma);
                }
                totoal_num = totoal_num + msgs_completed;
                
                send_packet(dma, (LAYER_SIZE) * msgs_completed, send_position, thread_id, 1);
                dma_update_snapshot(dma, cur_snapshot);
                send_position = send_position + msgs_completed;
                msgs_completed = 0;
            }
        }

        if (totoal_num % 30000 == 0)
            printf("iswrite %d thread_id %d recv totoal_num %d\n", isWrite, thread_id, totoal_num);
    }
}

void write_consume_loop(DMAcontext* dma, Log* log, int thread_id, int isServer, bool isWrite) {
    int cpu = 0;
    if (mlx_i == 0)
        cpu = thread_id + 4;
    else
        cpu = thread_id + 10;
    
    bindingCPU(cpu);
    printf("write_consume_loop threadid :%d cpu %d\n", thread_id, cpu);
    cqe_snapshot_t cur_snapshot;
    int totoal_num = 0;
    
    uint32_t version = 0;
    uint32_t key;
    uint16_t index;
    uint32_t seq;
    uint8_t flags;
    uint8_t opcode;
    
    int isACK = 1;
    int log_num = 0;
    int send_position = 0;
    
    while (1)
    {
        while (1)
        {
            log_num = log->isEmpty(isWrite);
            
            if (log_num > 0)
            {
                // printf("log_num %d isWrite:%d\n", log_num, isWrite);
                break;
            }
            // std::this_thread::sleep_for(std::chrono::nanoseconds(10));       
            
        }
    
        while (log_num > 0)
        {
            // printf("isWrite %d log_num %d\t", isWrite, log_num);
            if (log_num >= batch_num) {
                for (int i = 0; i < batch_num; i++)
                {
                    LogEntry *LE = log->consume(isWrite);
                    // uint32_t key = ntohl(LE->key);
                    // uint32_t version = ntohl(LE->version);
                    uint32_t cur_version = VERSION_VEC_CUR[LE->key].load(std::memory_order_relaxed);
                    if (ENABLE_VERSION_CHECK){
                        if (isWrite) {
                            // if (ENABLE_ROCKSDB)
                            //     DB_WRITE(&key, LE->value, LE->value_len);
                            if (cur_version < version) {// if write version is higher than cur version
                                // printf("w1 k:%lu cur_version:%u\n", key, cur_version);
                                // std::cout << "v[key]" << cur_version << std::endl;
                                VERSION_VEC_CUR[LE->key].store(LE->version, std::memory_order_release);
                                if (ENABLE_ROCKSDB) {
                                    DB_WRITE(&key, LE->value, LE->value_len);
                                    write_num = write_num + 1;
                                }
                            } else {
                                // printf("w2 k:%lu v:%u\n", key, version);
                                // std::cout << "v[key]" << cur_version << std::endl;
                            }
                        }
                    }
                    DB_Batch_WRITE();
                }
                totoal_num = totoal_num + batch_num;
                log_num = log_num - batch_num;

            } else {
                for (int i = 0; i < log_num; i++)
                {
                    LogEntry *LE = log->consume(isWrite);
                    // uint32_t key = ntohl(LE->key);
                    // uint32_t version = ntohl(LE->version);
                    uint32_t cur_version = VERSION_VEC_CUR[key].load(std::memory_order_relaxed);
                    if (ENABLE_VERSION_CHECK){
                        if (isWrite) {
                            // if (ENABLE_ROCKSDB)
                            //     DB_WRITE(&key, LE->value, LE->value_len);
                            if (cur_version < LE->version) {// if write version is higher than cur version
                                // printf("w1 k:%lu cur_version:%u\n", key, cur_version);
                                // std::cout << "v[key]" << cur_version << std::endl;
                                VERSION_VEC_CUR[LE->key].store(LE->version, std::memory_order_release);
                                if (ENABLE_ROCKSDB) {
                                    DB_WRITE(&key, LE->value, LE->value_len);
                                    write_num = write_num + 1;
                                }
                            } else {
                                // printf("w2 k:%lu v:%u\n", key, version);
                                // std::cout << "v[key]" << cur_version << std::endl;
                            }
                        }
                    }
                    DB_Batch_WRITE();
                    totoal_num = totoal_num + log_num;
                    log_num = 0;
                } 
            }
            

        }
    }

}

void sync_read_main_loop(DMAcontext* dma, Log* log, int thread_id, bool isWrite)
{
    int cpu = 0;
    if (mlx_i == 0)
        cpu = thread_id + 2;
    else
        cpu = thread_id + 8;
    
    bindingCPU(cpu);
    printf("sync_read_main_loop :%d cpu %d\n", thread_id, cpu);
    int msgs_completed = 0;
    cqe_snapshot_t cur_snapshot;
    int totoal_num = 0;

    size_t value_len = MAX_ENTRIES_PER_PACKET;
    int send_position = 0;
    int N_PKT = htons(PKTGENPROTO);
    int N_OPTKVPROTO = htons(OPTKVPROTO);
    uint16_t out_of_version;
    int ret = 0;

    if (dma == nullptr)
    {
        printf("failed dma\n");
        return;
    }
    while (1) {
        while (1)
        {
            if (DEBUG_CHECK_CQ)
                sleep(1);
            msgs_completed = receive_packet(dma, &cur_snapshot, thread_id);
            if (msgs_completed != 0)
            {
                // printf("isWrite %d msg:%d \n",isWrite,msgs_completed);
                break;
            }
        }

        for (int i = 0; i < msgs_completed; i++)
        {
            uint8_t* buf = &dma->mp_recv_ring[dma->ring_head * kAppRingMbufSize];
            ether* ether_header = reinterpret_cast<ether*>(buf);
            if (ether_header->h_proto == N_OPTKVPROTO){ // 0x0700 = 
                agghdr* agg_header = reinterpret_cast<agghdr*>(buf + IP_ETH_UDP_HEADER_SIZE);
                if (DEBUG_PRINT_ALL_RECEIVING_PACKET)
                    p4ml_header_print(agg_header, "RECV");
                // printf("isWrite %d opcode %u\n", isWrite, agg_header->opcode);
                log->append_read((&agg_header->key), agg_header->opcode, agg_header->version, value_len, (agg_header->flags), (&agg_header->seq), (&agg_header->index), (agg_header->num_log)); // ntohl(data) in append
            } else if (ether_header->h_proto == N_PKT) //  0x1234
            {
                pktgen* pktgen_header = reinterpret_cast<pktgen*>(buf + IP_ETH_HEADER_SIZE);
                if (DEBUG_CHECK_PKTGEN)
                    pktgen_header_print_h(pktgen_header, "RECV PKTGEN");
            }
            dma_postback(dma);
        }
        totoal_num = totoal_num + msgs_completed;
        msgs_completed = 0;
        
        dma_update_snapshot(dma, cur_snapshot);
        // if (totoal_num == 100)
        // {
        //     log->printLog();
        // }
        if (totoal_num % 30000 == 0)
            printf("iswrite %d thread_id %d recv totoal_num %d\n", isWrite, thread_id, totoal_num);
    }
}

void read_consume_loop(DMAcontext* dma, Log* log, int thread_id, int isServer, bool isWrite) {
    int cpu = 0;
    if (mlx_i == 0)
        cpu = thread_id + 6;
    else
        cpu = thread_id + 12;
    bindingCPU(cpu);
    printf("read_consume_loop bingCPU threadid :%d cpu %d\n", thread_id, cpu);
    cqe_snapshot_t cur_snapshot;
    int totoal_num = 0;
    
    
    uint16_t num_worker = 2;
    uint32_t version = 0;
    uint32_t key;
    uint16_t index;
    uint32_t seq;
    uint8_t flags;
    uint8_t opcode;
    
    int isACK = 1;
    int log_num = 0;
    int send_position = 0;
    
    while (1)
    {
        while (1)
        {
            log_num = log->isEmpty(isWrite);
            
            if (log_num > 0)
            {
                // printf("log_num %d isWrite:%d\n", log_num, isWrite);
                break;
            }
            // std::this_thread::sleep_for(std::chrono::nanoseconds(10));       
            
        }
    
        while (log_num > 0)
        {
            // printf("isWrite %d log_num %d\t", isWrite, log_num);
            if (log_num >= batch_num)
            {
                
                if (send_position + batch_num >= (my_send_queue_length))
                    send_position = 0;
        
                for (int i = 0; i < batch_num; i++)
                {
                    LogEntry *LE = log->consume(isWrite);
                    uint32_t key = ntohl(LE->key);
                    uint32_t version = ntohl(LE->version);
                    
                    uint32_t pend_version = VERSION_VEC_PED[key].load(std::memory_order_relaxed);
                    uint32_t cur_version = VERSION_VEC_CUR[key].load(std::memory_order_relaxed);
                    // DB_READ(&key);
                    // printf("cur_version %lu pend_version version%lu\n",cur_version, pend_version, version);
                    // /*
                    if (ENABLE_VERSION_CHECK){
                        if (version == 0){
                            if (pend_version > cur_version) {
                                LogEntry* RLE = log->find(VERSION_VEC_INDEX[key]);
                            } else {
                                DB_READ(&key);
                            }
                        } else {
                                
                            if (pend_version < version) { // if read version is higher than cur version, Execute immediately, and return read ack
                                // printf("r3 k:%lu v:%lu pend_version :%lu, cur_version:%lu\n", key, version, pend_version, cur_version);
                                VERSION_VEC_PED[key].store(version, std::memory_order_release);
                                int index = log->append_write(LE);
                                LogEntry* RLE = log->find(index); // request has the newest value
                                // log->printLogEntry(RLE);
                            } else if (pend_version >= version) {
                                if (pend_version == cur_version) {
                                    // printf("r4 k:%lu v:%lu pend_version :%lu, cur_version:%lu\n", key, version, pend_version, cur_version);
                                    DB_READ(&key);
                                } else {
                                    // printf("r5 k:%lu v:%lu pend_version :%lu, cur_version:%lu\n", key, version, pend_version, cur_version);
                                    LogEntry* RLE = log->find(VERSION_VEC_INDEX[key]);
                                    // log->printLogEntry(RLE);
                                }
                                
                            }
                            
                        }
                        
                    }
                    // */

                    // if (LE == nullptr)
                    //     return;
                    make_log_layer_and_copy_to(dma->send_region + (send_position + i) * LAYER_SIZE, LE, &bitmap, &num_worker, isACK, &host_bitmap);
                }
                send_packet(dma, (LAYER_SIZE) * batch_num, send_position, thread_id, isServer);
                log_num = log_num - batch_num;
                send_position = send_position + batch_num;
                totoal_num = totoal_num + batch_num;
            } else  /// < batch_num 
            {
                if (send_position + log_num >= (my_send_queue_length))
                    send_position = 0;
                for (int i = 0; i < log_num; i++)
                {
                    LogEntry *LE = log->consume(isWrite);
                    uint32_t key = ntohl(LE->key);
                    uint32_t version = ntohl(LE->version);
                    
                    
                    uint32_t pend_version = VERSION_VEC_PED[key].load(std::memory_order_relaxed);
                    uint32_t cur_version = VERSION_VEC_CUR[key].load(std::memory_order_relaxed);
                    // DB_READ(&key);
                    // printf("cur_version %lu pend_version version%lu\n",cur_version, pend_version, version);
                    
                    if (ENABLE_VERSION_CHECK){
                        if (version == 0){
                            if (pend_version > cur_version) {
                                LogEntry* RLE = log->find(VERSION_VEC_INDEX[key]);
                            } else {
                                DB_READ(&key);
                            }
                        } else {
                            if (pend_version < version) { // if read version is higher than cur version, Execute immediately, and return read ack
                                // printf("r3 k:%lu v:%lu pend_version :%lu, cur_version:%lu\n", key, version, pend_version, cur_version);
                                VERSION_VEC_PED[key].store(version, std::memory_order_release);
                                int index = log->append_write(LE);
                                LogEntry* RLE = log->find(index); // request has the newest value
                                // log->printLogEntry(RLE);
                            } else if (pend_version >= version) {
                                if (pend_version == cur_version) {
                                    // printf("r4 k:%lu v:%lu pend_version :%lu, cur_version:%lu\n", key, version, pend_version, cur_version);
                                    DB_READ(&key);
                                } else {
                                    // printf("r5 k:%lu v:%lu pend_version :%lu, cur_version:%lu\n", key, version, pend_version, cur_version);
                                    LogEntry* RLE = log->find(VERSION_VEC_INDEX[key]);
                                    // log->printLogEntry(RLE);
                                }
                                
                            }
                            
                        }
                        
                    }
                    

                      
                    make_log_layer_and_copy_to(dma->send_region + (send_position + i) * LAYER_SIZE, LE, &bitmap, &num_worker, isACK, &host_bitmap);
                }
                send_packet(dma, (LAYER_SIZE) * log_num, send_position, thread_id, isServer);
                send_position = send_position + log_num;
                totoal_num = totoal_num + log_num;
                log_num = 0;
            }
        }

    }

}

int main(int argc, char **argv)
{

    options.create_if_missing = true;
    // options.enable_blob_files = false;

    // options.IncreaseParallelism();
    options.max_background_flushes = GLOBAL_MAX_FLUSH_THREAD_NUM;
    options.env->SetBackgroundThreads(GLOBAL_MAX_FLUSH_THREAD_NUM, rocksdb::Env::Priority::HIGH);
    options.max_background_compactions = GLOBAL_MAX_COMPACTION_THREAD_NUM;
    options.env->SetBackgroundThreads(GLOBAL_MAX_COMPACTION_THREAD_NUM, rocksdb::Env::Priority::LOW);

    // options.OptimizeLevelStyleCompaction();

    // general options
    rocksdb::BlockBasedTableOptions table_options;
    table_options.filter_policy = std::shared_ptr<const rocksdb::FilterPolicy>(rocksdb::NewBloomFilterPolicy(BLOOMFILTER_BITS_PER_KEY));
    table_options.block_cache = rocksdb::NewLRUCache(BLOCKCACHE_SIZE, BLOCKCACHE_SHARDBITS);
    table_options.block_size = BLOCK_SIZE;
    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
    options.max_open_files = -1;
    options.table_cache_numshardbits = TABLECACHE_SHARDBITS;

    // table_options.index_type = rocksdb::BlockBasedTableOptions::kBinarySearch;
    // table_options.index_type = rocksdb::BlockBasedTableOptions::kHashSearch;


    // flush options
    options.write_buffer_size = MEMTABLE_SIZE;
    options.max_write_buffer_number = MAX_MEMTABLE_IMMUTABLE_NUM;
    options.min_write_buffer_number_to_merge = MIN_IMMUTABLE_FLUSH_NUM;

    // level-style compaction
    options.level0_file_num_compaction_trigger = LEVEL0_SST_NUM;
    options.target_file_size_base = LEVEL1_SST_NUM;
    options.max_bytes_for_level_base = LEVEL1_SST_NUM * LEVEL1_SST_SIZE;
    options.max_bytes_for_level_multiplier = LEVEL_TOTAKSIZE_MULTIPLIER;
    options.target_file_size_multiplier = LEVEL_SSTSIZE_MULTIPLIER;

    options.num_levels = LEVEL_NUM;

    options.compaction_style = rocksdb::kCompactionStyleLevel;
    options.wal_bytes_per_sync = WAL_BYTES_PER_SYNC;

    opts = new rocksdb::WriteOptions;
    if (argc < 5) {
        printf("\nUsage %s [Thread_num] [isServer] [use mlx_[i]] [bitmap] \n\n", argv[0]);
        exit(1);
    }

    int num_thread = atoi(argv[1]);
    int isServer = atoi(argv[2]);
    mlx_i = atoi(argv[3]);
    int bit = atoi(argv[4]);


    if (num_thread == 0)
        return -1;
    std::string DBPath;
    if (bit == 0) {
        DBPath = kDBPath_0;
        bitmap = bitmap_0;
        host_bitmap = host_bitmap_0;
    } else if (bit == 1) {
        DBPath = kDBPath_1;
        bitmap = bitmap_1;
        host_bitmap = host_bitmap_1;
    } else 
    {
        DBPath = kDBPath_2;
        bitmap = bitmap_2;
        host_bitmap = host_bitmap_2;
    }
    printf("\nUsage [Thread_num %d] [isServer %d] [use mlx_[%d]] [bitmap %lu %d] \n\n", num_thread, isServer, mlx_i, bitmap, host_bitmap);
    Status stats = DB::Open(options, DBPath, &db);
    assert(stats.ok());

    // uint32_t insert_key = 0;
    
    // // for (int i = 0; i < 32; ++i) {
    // //     insert_value[i] = i;
    // // }
    // // uint32_t insert_value = 1;
    // uint32_t insert_value[32];
    // for (int i = 0; i < 32; ++i) {
    //     insert_value[i] = i;
    // }
    // options.statistics = rocksdb::CreateDBStatistics();
    
    // for (int i = 0; i < 50000; i++)
    // {
    //     // DB_WRITE(&insert_key, &insert_value, 1);
    //     auto stats = db->Put(*opts, uint32_to_string(&insert_key), uint32_to_string(&insert_value, 28));
    //     assert(stats.ok());
    //     // std::string b = std::to_string(i);
    //     // DB_READ(&insert_key);
    //     insert_key = insert_key + 1;    
    // }
    
    // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - send_end);
    // uint32_t insert_key_1 = 0;
    // for (int i = 0; i < 50000; i++)
    // {
    //     DB_READ(&insert_key_1);
    //     insert_key_1 = insert_key_1+1;
    // }
    
    
    // std::cout << options.statistics->ToString();
    // std::string stats;


    // std::cout << options.statistics->getHistogramString();
    // std::cout << options.statistics->getHistogramString(static_cast<uint32_t>(rocksdb::histogram_type::LATENCY))
    //           << std::endl;
    auto send_end = std::chrono::high_resolution_clock::now();

    uint32_t insert_key = 0;
    uint32_t insert_value = 1;
    std::string value;
    
    for (int i = 0; i < 40000; i++)
    {
        // DB_WRITE(&insert_key, &insert_value, 1);
        auto stats = db->Put(*opts, uint32_to_string(&insert_key), uint32_to_string(&insert_value, 1));
        assert(stats.ok());
        // std::string b = std::to_string(i);
        // DB_READ(&insert_key);
        insert_key = insert_key + 1;    
    }
    // uint32_t insert_key_1 = 0;
    
    // for (int i = 0; i < 50000; i++)
    // {
    //     DB_READ(&insert_key_1);
    //     insert_key_1 = insert_key_1+1;
    // }
    // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - send_end);
    // auto timer_total_us = double(duration.count() / 1000); // us
    // double Throughput_Mbps = 50000 * ALL_PACKET_SIZE * 8 / timer_total_us * 1000 * 1000 / 1024 / 1024; // us * 1000 * 1000 * 1000 / 1024 / 1024;
    // double Throughput_Mops = 50000 / (timer_total_us / 1000 / 1000) / 1000 / 1000;
    // std::cout << "Throughput: " <<  Throughput_Mbps << " mbps" << std::endl;
    // std::cout << "Throughput: " <<  Throughput_Mops << " mops" << std::endl;
    // std::string stat1s;
    // db->GetProperty("rocksdb.stats", &stat1s);
    // fprintf(stderr, "%s", stat1s.c_str());
    // return 1;

                    


    struct ibv_device** dev_list;
    struct ibv_device* ib_dev;

    DMAcontext** dmaContextQueue;
    Log** LogQueue;

    std::thread** mainLoopThreadQueue;
    std::thread** readConsumeLoopThreadQueue;
    std::thread** writeConsumeLoopThreadQueue;

    dmaContextQueue = new DMAcontext *[num_thread * 2];
    mainLoopThreadQueue = new std::thread*[num_thread * 2];
    readConsumeLoopThreadQueue = new std::thread*[1];
    writeConsumeLoopThreadQueue = new std::thread*[1];

    LogQueue = new Log*[num_thread];

    dev_list = ibv_get_device_list(NULL);
    if (!dev_list)
    {
        perror("Failed to get devices list");
        exit(1);
    }
    ib_dev = dev_list[mlx_i];
    if (!ib_dev)
    {
        fprintf(stderr, "IB device not found\n");
        exit(1);
    }
    int MAX_KEY_NUM = 6000;
    std::fill(VERSION_VEC_PED.begin(), VERSION_VEC_PED.end(), 0);
    std::fill(VERSION_VEC_CUR.begin(), VERSION_VEC_CUR.end(), 0);

    bool isWrite = 1;
    bool isRead = 0;
    for (int i = 0; i < 1; i++)
    {
        LogQueue[i] = new Log();
        
        dmaContextQueue[i] = DMA_create(ib_dev, i, isServer, isRead); // read
        mainLoopThreadQueue[i] = new std::thread(sync_read_main_loop, dmaContextQueue[i], LogQueue[i], i, isRead); // read
        readConsumeLoopThreadQueue[i] = new std::thread(read_consume_loop, dmaContextQueue[i], LogQueue[i], i, isServer, isRead);
        
        dmaContextQueue[i+1] = DMA_create(ib_dev, i, isServer, isWrite); // 2
        mainLoopThreadQueue[i+1] = new std::thread(async_wirte_main_loop, dmaContextQueue[i+1], LogQueue[i], i, isWrite); // write
        writeConsumeLoopThreadQueue[i] = new std::thread(write_consume_loop, dmaContextQueue[i+1], LogQueue[i], i, isServer, isWrite);
    }

    
    while (1)
    {
        // std::this_thread::sleep_for(std::chrono::seconds(100000000));
        sleep(100000000000000);
    }

    printf("using: %s\n", ibv_get_device_name(ib_dev));
    
}
