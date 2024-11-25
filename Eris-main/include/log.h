#ifndef LOG_H
#define LOG_H
#include <vector>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sched.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <stdexcept>
#include <assert.h>
#include <atomic>
#define MAX_ENTRIES_PER_PACKET 5
#define DEBUG_CHECK_LOG 0
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
struct LogEntry {
        uint32_t key; // 1
        uint16_t index;
        uint8_t num_log;
        uint32_t seq; // 1
        uint8_t  opcode; // 1
        uint32_t version; // 1
        size_t value_len; // 1
        uint8_t flags; // 1
        uint32_t *value;

        // uint32_t value[MAX_ENTRIES_PER_PACKET]; // 4 * MAX_ENTRIES_PER_PACKET

        LogEntry() {
            this->value = (uint32_t *)malloc(sizeof(uint32_t)*MAX_ENTRIES_PER_PACKET);
            for (int i = 0; i < MAX_ENTRIES_PER_PACKET; i++)
            {
                    this->value[i] = 1;
                    // printf("i %d %d\t", i, value[i]);
            }
        }

        LogEntry(uint32_t *key, uint8_t opcode, uint32_t version, size_t value_len, 
                 uint8_t flags, uint32_t *seq, uint16_t *index, uint8_t *num_log)
                     : key((*key)), opcode(opcode), version(version), value_len(value_len),
                        flags(flags), seq((*seq)), index((*index)), num_log((*num_log))
        {
            if (value_len > 0)
            {
                this->value = (uint32_t *)malloc(sizeof(uint32_t)*value_len);
                for (int i = 0; i < value_len; i++)
                {
                    this->value[i] = 1;
                    // printf("i %d %d\t", i, value[i]);
                }
            }
        }


    };

class Log
{
public:
    Log(){
        this->write_head_ptr = -1; // start from 0;
        this->read_head_ptr = -1; // tail 0;
        this->write_tail_ptr = -1; // start from 0;
        this->read_tail_ptr = -1; // tail 0;
        entries_vec_read.reserve(MAX_LOG_ENTRY);
        entries_vec_write.reserve(MAX_LOG_ENTRY);

        for (int i = 0; i < MAX_LOG_ENTRY; i++)
        {
            entries_vec_read[i] = new LogEntry;
            entries_vec_write[i] =  new LogEntry;
        }
    }
    Log(int size_log){
        this->write_head_ptr = -1; // start from 0;
        this->read_head_ptr = -1; // tail 0;
        this->write_tail_ptr = -1; // start from 0;
        this->read_tail_ptr = -1; // tail 0;
        if (size_log>0)
        {
            entries_vec_read.reserve(size_log);
            entries_vec_write.reserve(size_log);
        }
        else
        {
            entries_vec_read.reserve(MAX_LOG_ENTRY);
            entries_vec_write.reserve(MAX_LOG_ENTRY);
        }
    }

    size_t append_write(uint32_t *key, uint8_t &opcode, uint32_t &version, size_t value_len, uint8_t &flags, uint32_t *seq, uint16_t *index, uint8_t num_log)
    {
        // if (DEBUG_CHECK_LOG)
        // printf("Append one entry-- this->tail_ptr %d MAX_LOG_ENTRY %d this->head_ptr:%d\n", this->tail_ptr, MAX_LOG_ENTRY, this->head_ptr);        
        
            size_t next_tail = this->write_tail_ptr.load(std::memory_order_relaxed) + 1;
            entries_vec_write[next_tail]->key = (*key);
            entries_vec_write[next_tail]->opcode = opcode;
            entries_vec_write[next_tail]->version = version;
            entries_vec_write[next_tail]->value_len = value_len;
            entries_vec_write[next_tail]->flags = flags;
            entries_vec_write[next_tail]->seq = (*seq);
            entries_vec_write[next_tail]->index = (*index);
            entries_vec_write[next_tail]->num_log = (num_log);
            this->write_tail_ptr.store(next_tail, std::memory_order_release);
            return next_tail;
    }
    size_t append_write(LogEntry *LE)
    {
        // if (DEBUG_CHECK_LOG)
        // printf("Append one entry-- this->tail_ptr %d MAX_LOG_ENTRY %d this->head_ptr:%d\n", this->tail_ptr, MAX_LOG_ENTRY, this->head_ptr);        
        
            size_t next_tail = this->write_tail_ptr.load(std::memory_order_relaxed) + 1;
            entries_vec_write[next_tail]->key = LE->key;
            entries_vec_write[next_tail]->opcode = LE->opcode;
            entries_vec_write[next_tail]->version = LE->version;
            entries_vec_write[next_tail]->value_len = LE->value_len;
            entries_vec_write[next_tail]->flags = LE->flags;
            entries_vec_write[next_tail]->seq = LE->seq;
            entries_vec_write[next_tail]->index = LE->index;
            entries_vec_write[next_tail]->num_log = LE->num_log;
            this->write_tail_ptr.store(next_tail, std::memory_order_release);
            return next_tail;
    }
    void append_read(uint32_t *key, uint8_t &opcode, uint32_t &version, size_t value_len, uint8_t &flags, uint32_t *seq, uint16_t *index, uint8_t num_log)
    {    
            size_t next_tail = this->read_tail_ptr.load(std::memory_order_relaxed) + 1;
            entries_vec_read[next_tail]->key = (*key);
            entries_vec_read[next_tail]->opcode = opcode;
            entries_vec_read[next_tail]->version = version;
            entries_vec_read[next_tail]->value_len = value_len;
            entries_vec_read[next_tail]->flags = flags;
            entries_vec_read[next_tail]->seq = (*seq);
            entries_vec_read[next_tail]->index = (*index);
            entries_vec_read[next_tail]->num_log = (num_log);
            this->read_tail_ptr.store(next_tail, std::memory_order_release);
              

        return;
    }

    LogEntry* consume(bool isWrite)
    {
        if (isWrite)
        {
            size_t head = this->write_head_ptr.load(std::memory_order_relaxed);
            this->write_head_ptr.store(head + 1, std::memory_order_release);
            return entries_vec_write[this->write_head_ptr];
        } else
        {
            size_t head = this->read_head_ptr.load(std::memory_order_relaxed);
            this->read_head_ptr.store(head + 1, std::memory_order_release);
            return entries_vec_read[this->read_head_ptr];
        }
    }

    int isEmpty(bool isWrite)
    {
        
        if (isWrite)
        {
            if (this->write_tail_ptr < 0)
            {
                return -1;
            }
            size_t current_tail = this->write_tail_ptr.load(std::memory_order_relaxed);
            if (current_tail == write_head_ptr.load(std::memory_order_relaxed)) {
                return 0;
            } else {
                return this->write_tail_ptr.load(std::memory_order_relaxed) - this->write_head_ptr.load(std::memory_order_relaxed);
            }

        } else {
            if (this->read_tail_ptr < 0) {
                return -1;
            }

            size_t current_tail = this->read_tail_ptr.load(std::memory_order_relaxed);
            if (current_tail == read_head_ptr.load(std::memory_order_relaxed)) {
                return 0;
            } else {
                return this->read_tail_ptr.load(std::memory_order_relaxed) - this->read_head_ptr.load(std::memory_order_relaxed);
            }

        }
    }

    // LogEntry* pop()
    // {
    //     if (entries_queue.empty())
    //     {
    //         return nullptr;
    //     } else
    //         entries_queue.pop();
    // }

    void printLogEntry(LogEntry *LE)
    {
        printf("key %lu, opcode %d, version %lu"
                "value_len %d"
                "index %u flags %u"
                "seq %lu\n", 
                ntohl(LE->key), LE->opcode, LE->version, LE->value_len,
                ntohl(LE->index), LE->flags, ntohs(LE->seq));
        printf("data: ");
        for (int i = 0; i < LE->value_len; i++)
        {
            printf("%lu", LE->value[i]);
        }
        printf("\n");
    }

    void printLog()
    {
        // printf("write_head_ptr:%d write_tail_ptr:%d-------------------printLog\n",
        //         this->write_head_ptr, this->write_tail_ptr);
        for (int i = 0; i < write_tail_ptr; i++)
        {
            printLogEntry(entries_vec_write[i]);
        }
        // printf("read_head_ptr:%d read_tail_ptr:%d-------------------printLog\n",
        //         this->read_head_ptr, this->read_tail_ptr);
        for (int i = 0; i < read_tail_ptr; i++)
        {
            printf("read_tail_ptr %d\n", i);
            printLogEntry(entries_vec_read[i]);
        }
    }

LogEntry* find(size_t index) {
    // printf("pending index : %d\n", index);
    return entries_vec_write[index];
    

}

private:
    std::vector<LogEntry*> entries_vec_write;
    std::vector<LogEntry*> entries_vec_read;
    std::atomic<int>  write_head_ptr;
    std::atomic<int> write_tail_ptr;
    std::atomic<int>  read_head_ptr;
    std::atomic<int> read_tail_ptr;
    std::vector<int> VERSION_VEC_INDEX;
    // int write_head_ptr;
    // int write_tail_ptr;
    // int read_head_ptr;
    // int read_tail_ptr;
    const int MAX_LOG_ENTRY = 1000001;
    // std::queue<LogEntry> entries_queue; 
    // opcode_t start;
    // std::map<,> KeyMap;
};

// int main(void)
// {
//     Log *log= new Log();
//     uint32_t data[10] = {1,2,3,4,5,6,7,8,9,10};
//     uint32_t seq = 10;
//     // uint32_t &key, uint8_t opcode, uint16_t version, size_t value_len, uint32_t *value
//     uint32_t key = 1;
//     uint16_t index = 1;
//     log->append(&key,1,8,5, 0,&seq, &index, data);
//     log->append(&key,1,8,5, 0,&seq, &index, data);
//     log->printLog();
//     printf("----------------------------------------------\n");
//     log->printLogEntry(log->consume());
//     log->printLogEntry(log->consume());
//     log->printLog();
//     printf("lcc\n");
//     free(log);
//     return 1;
// }


#endif

