#ifndef PACKET_H
#define PACKET_H
#include "DMAcontext.h"
#include "log.h"

// #define DEBUG_PRINT_ALL_SENDING_PACKET true
// #define DEBUG_PRINT_ALL_RECEIVING_PACKET true
// #define DEBUG_PRINT_ALL_DUP_PACKET true
// #define DEBUG_PRINT_ALL_RESENDING_PACKET true

#define DEBUG_PRINT_ALL_SENDING_PACKET false
#define DEBUG_PRINT_ALL_RECEIVING_PACKET false
#define DEBUG_PRINT_ALL_DUP_PACKET false
#define DEBUG_PRINT_ALL_RESENDING_PACKET false

#define DEBUG_CHECK_SEND_RECEIVE_TOTAL false
#define DEBUG_CHECK_PKTGEN false
#define ENABLE_LOSS_RESEND false
#define DEBUG_CHECK_CQ false


#define PS_FILTER_TEMPLATE 0x05, 0x04, 0x03, 0x02, 0x01, 0xFF
#define WORKER_FILTER_TEMPLATE 0x77, 0x77, 0x77, 0x77, 0x77, 0xFF

// #define SRC_MAC 0xb8, 0x59, 0x9f, 0x1d, 0x04, 0xf2 
#define SRC_MAC 0xe4, 0x1d, 0x2d, 0xf3, 0xdd, 0xcc
#define DST_MAC 0xb8, 0x59, 0x9f, 0x0b, 0x30, 0x72

#define ETH_TYPE 0x08, 0x00

// #define IP_HDRS 0x45, 0x00, 0x00, 0x54, 0x00, 0x00, 0x40, 0x00, 0x40, 0x01, 0xaf, 0xb6
#define IP_HDRS 0x45, 0x00, 0x00, 0x54, 0x00, 0x00, 0x40, 0x00, 0x40, 0xff, 0xaf, 0xb6

#define SRC_IP 0x0d, 0x07, 0x38, 0x66

#define DST_IP 0x0d, 0x07, 0x38, 0x7f

#define SRC_PORT 0x67, 0x67

#define DST_PORT 0x78, 0x78

#define UDP_HDRS 0x00, 0x00, 0x00, 0x00

const int PKTGENPROTO = 0x1234;
const int OPTKVPROTO = 0x0800;


// Only a template, DST_IP will be modified soon
// This one is for sending
const unsigned char PS_IP_ETH_UDP_HEADER[] = { WORKER_FILTER_TEMPLATE, SRC_MAC, ETH_TYPE, IP_HDRS, SRC_IP, DST_IP };
const unsigned char WORKER_IP_ETH_UDP_HEADER[] = { PS_FILTER_TEMPLATE, SRC_MAC, ETH_TYPE, IP_HDRS, SRC_IP, DST_IP };

// #define IP_ETH_UDP_HEADER_SIZE 34 // 6 * 2 + 2 = 14 ETH + 20 = 34
#define IP_ETH_UDP_HEADER_SIZE 30 // 6 * 2 + 2 = 14 ETH + 20 = 34 - 4

// // 21 + 4 * 5 = 41
// # 12 + 8 = 
// #define LAYER_SIZE 41
// #define IP_ETH_HEADER_SIZE 14 // 6 * 2 + 2 = 14 ETH + 20 = 34
// #define ALL_PACKET_SIZE 75 // 44 -> 64
// #define MAX_ENTRIES_PER_PACKET 5 // 5 * 4 = 20

// +IP
// # 4 + 23 + 4 * 31 = 
#define LAYER_SIZE 151
#define IP_ETH_HEADER_SIZE 14 // 6 * 2 + 2 = 14 ETH + 20 = 34
#define ALL_PACKET_SIZE 187 // 34 + 151 + 2= 185
// #define MAX_ENTRIES_PER_PACKET 5
#define MAX_ENTRIES_PER_PACKET 31

// #define LAYER_SIZE 269 // + 31 * 4 = 124
// #define IP_ETH_HEADER_SIZE 14 // 6 * 2 + 2 = 14 ETH + 20 = 34
// #define ALL_PACKET_SIZE 254 // 46 + 34 = 80
// // #define MAX_ENTRIES_PER_PACKET 5
// #define MAX_ENTRIES_PER_PACKET 62
// 4 + 2 + 2 + 4=12
#define ETH_ALEN 6

#pragma pack(push, 1)
    struct ether {
        unsigned char h_dest[ETH_ALEN]; /* destination eth addr */ 
        unsigned char h_source[ETH_ALEN]; /* source ether addr */ 
        unsigned short h_proto;  /* packet type ID field */ 
};
#pragma pack(pop)

#pragma pack(push, 1)
    struct pktgen {
        uint16_t need_reset;
        uint32_t key;
        uint32_t version;
        uint32_t out_bitmap;
};
#pragma pack(pop)

// -5
// 4 + 2 + 2 + 1 + 1 + 4 + 2 + 1 + 1 + 1 + 2 = 21
// 21 + 4 * 5 = 41
#pragma pack(push, 1)
    struct agghdr {
        uint32_t dst_IP;
        uint32_t bitmap; // 4
        uint16_t num_worker; // 2
        uint32_t version; // 4
        uint8_t  opcode; // 1
        uint8_t  ack;  // 1
        uint32_t key; // 4
        uint16_t index; // 2
        uint8_t  num_log; // num of log item  - 1
        uint8_t log_index; // num of log item - 1
        uint8_t flags; // 2 0: out_of_version - 1
        uint32_t seq; // - 2
        /* Current flasg
        // reserved       :  10;
        overflow       :  1;
        dataIndex      :  2;
        ECN            :  1;
        isResend       :  1;
        out_of_version :  1;
        */
        uint32_t value[MAX_ENTRIES_PER_PACKET]; // 4 * MAX_ENTRIES_PER_PACKET
};
#pragma pack(pop)


void inline pktgen_header_print_h(pktgen* pktgen_header, const char *caption)
{

    // std::lock_guard<std::mutex> lock(_packet_print_mutex);
    printf("[%s] need_reset: %u, key: %lu "
           "version: %lu out_bitmap %lu ",
        caption,
        ntohs(pktgen_header->need_reset), ntohl(pktgen_header->key), ntohl(pktgen_header->version),
        ntohl(pktgen_header->out_bitmap));
    printf("\n");

}


void inline p4ml_header_print_h(agghdr *p4ml_header)
{
    // std::lock_guard<std::mutex> lock(_packet_print_mutex);
    printf("bitmap: %lu, num_worker: %u version: %lu flags:%u "
           "opcode: %u ack %u "
           "key: %lu ",
        // caption,
        ntohl(p4ml_header->bitmap), ntohs(p4ml_header->num_worker), ntohl(p4ml_header->version), p4ml_header->flags,
        p4ml_header->opcode, p4ml_header->ack,
        ntohl(p4ml_header->key));
    printf("\n");
}


void inline p4ml_header_print(agghdr *p4ml_header, const char *caption)
{
    // std::lock_guard<std::mutex> lock(_packet_print_mutex);
    printf("[%s] bitmap: %lu version: %lu flags: %u "
           "opcode: %u ack %u "
           "key: %lu index : %u seq : %lu num_log: %u",
        caption,
        ntohl(p4ml_header->bitmap), ntohl(p4ml_header->version), p4ml_header->flags,
        p4ml_header->opcode, p4ml_header->ack,
        ntohl(p4ml_header->key), ntohs(p4ml_header->index), ntohl(p4ml_header->seq), p4ml_header->num_log);
    printf("\n");
}

void inline make_aggr_layer_and_copy_to(void* payload, uint32_t* bitmap, uint16_t* num_worker, uint32_t* version, uint8_t opcode, uint32_t *key, uint16_t *index, uint32_t *seq, uint8_t *num_log, uint8_t resend, uint8_t isServer, uint32_t *dst_IP)
{
    agghdr* agg_header = (agghdr*)payload;
    agghdr* p4ml_header = agg_header;
    if (isServer)
    {
        // 0x0d, 0x07, 0x38, 0x7f
        agg_header->dst_IP = (*dst_IP);
        agg_header->bitmap = *bitmap;
        agg_header->num_worker = *num_worker;
        agg_header->version = *version;
        agg_header->key = *key;
        agg_header->index = *index;
        agg_header->seq = *seq;
        agg_header->ack = 1;
    } else {
        if (opcode==1)
        {
            agg_header->dst_IP = (*dst_IP);
        } else
        {   
            agg_header->dst_IP = (*dst_IP);
        }
        agg_header->bitmap = *bitmap;
        agg_header->num_worker = 0;
        agg_header->version = 0;
        agg_header->key = htonl(*key);
        agg_header->index = htons(*index);
        agg_header->seq = htonl(*seq);
        agg_header->ack = 0;
    }
    agg_header->opcode = opcode;
    agg_header->num_log = *num_log;
    agg_header->log_index = 0;
    
    agg_header->flags = 0;

    if (DEBUG_PRINT_ALL_RESENDING_PACKET || DEBUG_PRINT_ALL_SENDING_PACKET)
    {
        if (resend)
            p4ml_header_print(agg_header, "RESEND");
        else
        {
            p4ml_header_print(agg_header, "SEND");
        }
            
    }    
}

void inline make_log_layer_and_copy_to(void* payload, LogEntry *LE, uint32_t* log_bitmap, uint16_t* num_worker, int isACK, int *host_bitmap)
{
    agghdr* agg_header = (agghdr*)payload;
    agghdr* p4ml_header = agg_header;
    
    agg_header->bitmap = *log_bitmap;
    // agg_header->num_worker = htons(*num_worker);

    agg_header->version = LE->version;
    agg_header->opcode = LE->opcode;
    agg_header->key = LE->key;
    agg_header->index = LE->index;
    agg_header->seq = LE->seq;
    agg_header->num_log = LE->num_log;
    if (LE->opcode == 1)
        agg_header->log_index = -1;
    agg_header->num_log = LE->num_log;

    if (isACK) 
        agg_header->ack = 1;
    else
        agg_header->ack = 0;

    
    if (LE->flags == *host_bitmap) {
        agg_header->flags = 0;
    } else {
        agg_header->flags = LE->flags;
    }
    if (DEBUG_PRINT_ALL_SENDING_PACKET)
        p4ml_header_print(agg_header, "SEND");
    if (LE->opcode == 0 && LE->num_log == 1)
    {
        for (int i = 0; i < MAX_ENTRIES_PER_PACKET; i++)
        {
            agg_header->value[i] = htonl(1);
        }
    }
    // int32_t* send_data;
    // if (*offset + MAX_ENTRIES_PER_PACKET > job_info->len) {
    //     int32_t* tmp = new int32_t[MAX_ENTRIES_PER_PACKET]();
    //     memcpy(tmp, used_data + *offset, sizeof(int32_t) * (job_info->len % MAX_ENTRIES_PER_PACKET));
    //     send_data = tmp;
    //     delete tmp;
    // } else {
    //     send_data = used_data + *offset;
    // }
    // if (DEBUG_PRINT_ALL_SENDING_PACKET)
    //     p4ml_header_print_h(agg_header, "Make");
}






#endif