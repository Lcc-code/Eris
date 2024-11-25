#include "DMAcontext.h"
#include "packet.h"
#include "request.h"
#include "mlx5.h"
#include "HashTable.cc"

#include <fstream>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>


#define USE_HASH_AGTR_INDEX 1
static int MAX_SEND_NUM = 20000;
static int MAX_KEY_NUM = 6000;
static const int batch_packet_num = 50;
static const int SAMPLE = 100;
int UsedSwitchAGTRcount = 40000;
std::chrono::high_resolution_clock::time_point** write_send_time;
std::chrono::high_resolution_clock::time_point** write_receive_time;
std::chrono::high_resolution_clock::time_point** read_send_time;
std::chrono::high_resolution_clock::time_point** read_receive_time;
std::vector<Request > requests_;
std::chrono::high_resolution_clock::time_point T_now;

HashTable** hash_table;

// static int send_from_trace = 
void main_loop(DMAcontext* dma, int thread_id, bool isServer, 
            std::chrono::high_resolution_clock::time_point* write_send_time, std::chrono::high_resolution_clock::time_point*read_send_time, 
            std::chrono::high_resolution_clock::time_point* write_receive_time, std::chrono::high_resolution_clock::time_point*read_receive_time)
{   
    std::vector<double> read_latency;
    std::vector<double> write_latency;
    std::vector<double> mix_latency;
    int cpu = thread_id +0;
    bindingCPU(cpu);
    printf("main_loop bindingCPU %d\n", cpu);

    uint32_t bitmap = 0;
    uint16_t num_worker = 3;
    uint32_t version = 0;
    uint32_t key = 1;
    uint32_t index = 1;
    int opcode = 1;
    int send_position = 0;
    // size_t req_position = 0;
    uint32_t send_num = 0;
    uint8_t num_log = 1;
    uint32_t read_dst_IP = htonl(0x0d073870);
    uint32_t write_dst_IP = htonl(0x0d07387f);
    uint32_t dst_IP = 0;
    int need_to_resend = 0;
    
    double timer_total_us = 0.0;
    
    int msgs_completed = 0;
    cqe_snapshot_t cur_snapshot;
    int recv_totoal_num = 0;
    int collision = 0;

    auto send_timer = std::chrono::high_resolution_clock::now();
    auto send_now = std::chrono::high_resolution_clock::now();
    // std::chrono::high_resolution_clock::time_point send_ting;
    
    // send_ting = std::chrono::high_resolution_clock::now();
    while (1) {
        
            
        if (send_num >= MAX_SEND_NUM) {
            break;
        }
        if (send_position + batch_packet_num > (my_send_queue_length))
            send_position = 0;
        for (int i = 0; i < batch_packet_num; i++)
        {
            if (requests_[send_num].opcode == 1)
                dst_IP = write_dst_IP;
            else
                dst_IP = read_dst_IP;
            make_aggr_layer_and_copy_to(dma->send_region + (send_position + i) * LAYER_SIZE, &bitmap, &num_worker, &version, requests_[send_num].opcode,
                                            &requests_[send_num].key, &requests_[send_num].index, &send_num, &num_log, 0, 0, &dst_IP);
            send_num = send_num + 1;
        }
        send_packet(dma, (LAYER_SIZE) * batch_packet_num, send_position, thread_id, isServer);
        send_now = std::chrono::high_resolution_clock::now();
        send_position = send_position + batch_packet_num;
        if (send_position + batch_packet_num > my_send_queue_length)
            send_position = 0;
        
        while (1){
            while (1)
            {
                msgs_completed = receive_packet(dma, &cur_snapshot, thread_id);
                if (msgs_completed != 0)
                {
                    break;
                }
            }
            for (int i = 0; i < msgs_completed; i++)
            {
                uint8_t* buf = &dma->mp_recv_ring[dma->ring_head * kAppRingMbufSize];
                agghdr* recve_header = reinterpret_cast<agghdr*>(buf + IP_ETH_UDP_HEADER_SIZE);
                uint32_t seq = ntohl(recve_header->seq);
                uint8_t  flag = recve_header->flags;
                if (flag == 8) { // collision
                    collision = collision +1;
                    // printf("collision %u\n", collision);
                    uint16_t new_index = hash_table[0]->predefine_agtr_list[(seq)% UsedSwitchAGTRcount];
                    
                    // if get any of AGTR from hash
                    // if (new_index != -1) {
                    //     printf("new_hash_agtr %d\n", new_index);
                    //     // requests_[seq].index = new_index;
                    // } else {
                    //     // if all of the AGTR is used, full
                    //     // keep original AGTR
                    //     printf("Change Agtr fail, full.\n");
                    //     // new_agtr = p4ml_header->agtr;
                    // }
                    make_aggr_layer_and_copy_to(dma->send_region + (send_position + need_to_resend) * LAYER_SIZE, &bitmap, &num_worker, &version, requests_[seq].opcode,
                                            &requests_[seq].key, &new_index, &seq, &num_log, 1, 0, &dst_IP);
                    
                    need_to_resend = need_to_resend + 1;
                    recv_totoal_num = recv_totoal_num - 1;
                } else {
                    if (requests_[seq].ack == 0) {
                        requests_[seq].ack = 1;
                        if (seq % SAMPLE == 2)
                        {
                            if (recve_header->opcode == 0)
                            {
                                read_send_time[seq / SAMPLE] = send_now;
                                read_receive_time[seq / SAMPLE] = std::chrono::high_resolution_clock::now();
                                // std::cout << "1 seq / SAMPLE " << seq << " "<< SAMPLE << std::endl;
                            }
                            else
                            {
                                write_send_time[seq / SAMPLE] = send_now;
                                write_receive_time[seq / SAMPLE] = std::chrono::high_resolution_clock::now();
                                // auto duration_write = std::chrono::duration_cast<std::chrono::nanoseconds>(write_receive_time[i] - write_send_time[i]);
                                // std::cout << "2 seq / SAMPLE " << seq << " "<< SAMPLE << " "<< duration_write << std::endl;
                                

                            }
                        }
                        if (DEBUG_PRINT_ALL_RECEIVING_PACKET)
                        {
                            // printf("num %d\n", thread_id);
                            p4ml_header_print(recve_header, "RECV");
                        }
                    } else
                    {
                        recv_totoal_num = recv_totoal_num - 1;
                        if (DEBUG_PRINT_ALL_DUP_PACKET)
                        {
                            // printf("num %d\n", thread_id);
                            p4ml_header_print(recve_header, "DULP");
                        }
                        
                    }
                }
                dma_postback(dma);
            }

            recv_totoal_num = recv_totoal_num + msgs_completed;
            dma_update_snapshot(dma, cur_snapshot);
            msgs_completed = 0;
            
            if (need_to_resend != 0)
            {
                send_packet(dma, (LAYER_SIZE) * need_to_resend, send_position, thread_id, isServer);
                send_position = send_position + need_to_resend;
                need_to_resend = 0;
                if (send_position + batch_packet_num > my_send_queue_length)
                    send_position = 0;
            }
            // printf("recv_totoal_num %d batch_packet_num %d\n")
            if (recv_totoal_num >= batch_packet_num)
            {
                recv_totoal_num = 0;
                break;
            }
            
        }

        // if (send_num % 10000 == 0)
        // {
        //     printf("send_num %d\n", send_num);
        //     auto send_ting_1 = std::chrono::high_resolution_clock::now();

        //     // send_ting = std::chrono::high_resolution_clock::now();
        //     auto duration_ting = std::chrono::duration_cast<std::chrono::nanoseconds>(send_ting_1 - send_ting);
        //     double Throughput_Mbps = 10000 * ALL_PACKET_SIZE * 8 / double(duration_ting.count() / 1000) * 1000 * 1000 / 1024 / 1024; // us * 1000 * 1000 * 1000 / 1024 / 1024;
        //     double Throughput_Mops = Throughput_Mbps / (ALL_PACKET_SIZE * 8);
        //     // std::cout << "Throughput: " <<  Throughput_Mbps << " mbps" << std::endl;
        //     std::cout << "Throughput: " <<  Throughput_Mops << " mops" << std::endl;
        //     send_ting = send_ting_1;
        // }
    }
    auto send_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(send_end - send_timer);
    timer_total_us += double(duration.count() / 1000); // us
    
    double Throughput_Mbps = MAX_SEND_NUM * ALL_PACKET_SIZE * 8 / timer_total_us * 1000 * 1000 / 1024 / 1024; // us * 1000 * 1000 * 1000 / 1024 / 1024;
    double Throughput_Mops = Throughput_Mbps / (ALL_PACKET_SIZE * 8);
    // double Throughput_Mops = MAX_SEND_NUM / timer_total_us * 1000 * 1000 / 1024 / 1024;
    // double Throughput_Mops = MAX_SEND_NUM / timer_total_us * 1000 * 1000 / 1024 / 1024;
    // double Throughput_Mbps = Throughput_Mops * ALL_PACKET_SIZE * 8;
    
    std::cout << "Send " << MAX_SEND_NUM<< " " << timer_total_us << " us" << " collision " << collision<< std::endl;
    std::cout << "Throughput: " <<  Throughput_Mbps << " mbps" << std::endl;
    std::cout << "Throughput: " <<  Throughput_Mops << " mops" << std::endl;


    std::this_thread::sleep_for(std::chrono::nanoseconds(10000));

    double WIRTE_SUM = 0.0;
    int send_num_ = 0;
    double READ_SUM = 0.0;
    int read_num_ = 0;
    double ALL_SUM = 0.0;
    for (int i = 0; i < MAX_KEY_NUM; i++)
    {
        if (write_receive_time[i] != T_now && write_send_time[i] != T_now)
        {
            auto duration_write = std::chrono::duration_cast<std::chrono::nanoseconds>(write_receive_time[i] - write_send_time[i]);
            if (double(duration_write.count()) != 0)
            {
                
                write_latency.push_back(duration_write.count());
                mix_latency.push_back(duration_write.count());
                // std::cout << i << " write_latency " << duration_write.count() << "ns " << std::endl;
                WIRTE_SUM += double(duration_write.count() / 1000);
                send_num_ += 1;
            }
        }
        if (read_receive_time[i] != T_now && read_send_time[i] != T_now)
        {
            auto duration_read = std::chrono::duration_cast<std::chrono::nanoseconds>(read_receive_time[i] - read_send_time[i]);
            if (double(duration_read.count()) != 0)
            {
                read_latency.push_back(duration_read.count());
                mix_latency.push_back(duration_read.count());
                // std::cout << i << " read_latency " << duration_read.count() << "ns " << std::endl;
                READ_SUM += double(duration_read.count() / 1000);
                read_num_ += 1;
            }
        }
    }
    
    double AVG_W = WIRTE_SUM / (send_num_);
    double AVG_R = READ_SUM / (read_num_);
    double AVG_MIX = (READ_SUM + WIRTE_SUM) / (read_num_ + send_num_);

    std::sort(read_latency.begin(), read_latency.end());
    std::sort(write_latency.begin(), write_latency.end());
    std::sort(mix_latency.begin(), mix_latency.end());

    if (write_latency.size() != 0)
    {    
        std::cout << "write 90th percentile latency is " << write_latency[send_num_*90/100] / 1000 << "us " << std::endl;   
        std::cout << "write 95th percentile latency is " << write_latency[send_num_*95/100] / 1000 << "us " << std::endl;
        std::cout << "write 99th percentile latency is " << write_latency[send_num_*99/100] / 1000 << "us " << std::endl;
    }
    if (read_latency.size() != 0)
    {
        std::cout << "Read 90th percentile latency is " << read_latency[read_num_*90/100] / 1000<< "us " << std::endl;
        std::cout << "Read 95th percentile latency is " << read_latency[read_num_*95/100] / 1000<< "us " << std::endl;
        std::cout << "Read 99th percentile latency is " << read_latency[read_num_*99/100] / 1000<< "us " << std::endl;
    }
        std::cout << "Mix 90th percentile latency is " << mix_latency[(read_num_+send_num_)*90/100] / 1000<< "us " << std::endl;
        std::cout << "Mix 95th percentile latency is " << mix_latency[(read_num_+send_num_)*95/100] / 1000<< "us " << std::endl;
        std::cout << "Mix 99th percentile latency is " << mix_latency[(read_num_+send_num_)*99/100] / 1000<< "us " << std::endl;
    std::cout << "thread_id" << thread_id << " send_num_ " << send_num_ << " sum write latency: " << WIRTE_SUM<< " avg latency : " << AVG_W << "us" << std::endl;
    std::cout << "thread_id" << thread_id << "read_num_ "<< read_num_ << " sum read latency: " << READ_SUM<< " avg latency : " << AVG_R << "us" << std::endl;
    std::cout << "thread_id" << thread_id << " sum Mix latency: " << (WIRTE_SUM+READ_SUM) << " avg Mix latency : " << AVG_MIX << "us" << std::endl;
    std::cout << "==================================================================================" << std::endl;
}

int main(int argc, char **argv)
{

    if (argc < 5) {
        printf("\nUsage %s [Thread_num] [isServer] [workload] [UsedSwitchAGTRcount]\n", argv[0]);
        printf("workload: 1->YCSB_load  2->YCSB_run  3->zipf_load default:zipf\n");
        exit(1);
    }

    int num_thread = atoi(argv[1]);
    int isServer = atoi(argv[2]);
    int workload = atoi(argv[3]);
    UsedSwitchAGTRcount = atoi(argv[4]);
    if (num_thread == 0)
        return -1;

    printf("Warm up.. UsedSwitchAGTRcount : %d\n", UsedSwitchAGTRcount);
    hash_table = new HashTable *[num_thread];
    // for (int i = 0; i < num_thread; i++)
    {
        hash_table[0] = new HashTable(UsedSwitchAGTRcount);
        
        for (int j = 0; j < UsedSwitchAGTRcount; j++)
        {
            if (USE_HASH_AGTR_INDEX)
                hash_table[0]->HashNew_crc(0, j);
            else 
                hash_table[0]->HashNew_linear(j);
            // printf("%d %d\n", j, hash_table[i]->hash_map[j]);
        }
        hash_table[0]->Clear_HashTable(UsedSwitchAGTRcount);
    }
    // return -1;

    int op,key,size,request_num,index;
    int OP_WRITE = 1;
    const std::string YCSB_load = "/home/lc427/lcc/Striding_RQ/parse_trace/YCSB_load_trace.txt";
    const std::string YCSB_run = "/home/lc427/lcc/Striding_RQ/parse_trace/YCSB_run_trace.txt";
    const std::string zipf_trace = "/home/lc427/lcc/Striding_RQ/parse_trace/zipf_trace.txt";
    std::string Workload;
    switch (workload)
    {
        case 1:Workload = YCSB_load; break;
        case 2:Workload = YCSB_run; break;
        case 3:Workload = zipf_trace; break;
        default:Workload = zipf_trace;
    }
    std::ifstream file(Workload);
    
    if (file.is_open()) {
        file >> MAX_SEND_NUM;
        
        for (int i = 0; i < MAX_SEND_NUM; i++)
        {
            // file>> op >> key >> index;
            // std:: cout << op << " " << key << " "<< index << std::endl;
            file>> op >> key;
            // if (op == OP_WRITE) file>>size;
            // std:: cout << op << " " << key << std::endl;
            
            Request new_request((uint8_t)op, uint32_t(key), uint32_t(hash_table[0]->hash_map[key % UsedSwitchAGTRcount]));
            
            requests_.push_back(new_request);
            
        }
        file.close(); // 关闭文件
    } else {
        std::cout << "无法打开文件" << std::endl;
    }

    // Request new_request((uint8_t)0, uint32_t(131), uint32_t(131));
    // requests_[].push_back(new_request);

    MAX_KEY_NUM = MAX_SEND_NUM / SAMPLE;
    // for (int i = 0; i < MAX_SEND_NUM; i++)
    // {
    //     printf("i %d\n", i);
    //     for (int j = 0; j < num_thread; j++){
    //         printRequest(&requests_[j][i]);
    //     }
    // }
    
    // return 1;
    printf("Warm finish.\n");
    

    DMAcontext** dmaContextQueue;
    std::thread** mainLoopThreadQueue;
    // std::thread** mainRecvLoopThreadQueue;

    dmaContextQueue = new DMAcontext *[num_thread];
    mainLoopThreadQueue = new std::thread*[num_thread];
    // mainRecvLoopThreadQueue = new std::thread*[num_thread];
    write_send_time = new std::chrono::high_resolution_clock::time_point *[num_thread];
    write_receive_time = new std::chrono::high_resolution_clock::time_point *[num_thread];
    read_send_time = new std::chrono::high_resolution_clock::time_point *[num_thread];
    read_receive_time = new std::chrono::high_resolution_clock::time_point *[num_thread];

    struct ibv_device** dev_list;
    struct ibv_device* ib_dev;
    dev_list = ibv_get_device_list(NULL);
    if (!dev_list)
    {
        perror("Failed to get devices list");
        exit(1);
    }
    ib_dev = dev_list[0];
    if (!ib_dev)
    {
        fprintf(stderr, "IB device not found\n");
        exit(1);
    }
    T_now = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_thread; i++)
    {
        dmaContextQueue[i] = DMA_create(ib_dev, i, isServer, 0);
        
        write_send_time[i] = new std::chrono::high_resolution_clock::time_point [MAX_KEY_NUM];
        write_receive_time[i] = new std::chrono::high_resolution_clock::time_point [MAX_KEY_NUM];
        read_send_time[i] = new std::chrono::high_resolution_clock::time_point [MAX_KEY_NUM];
        read_receive_time[i] = new std::chrono::high_resolution_clock::time_point [MAX_KEY_NUM];
        for (int j = 0; j < MAX_KEY_NUM; j++)
        {
            write_send_time[i][j] = T_now;
            write_receive_time[i][j] = T_now;
            read_send_time[i][j] = T_now;
            read_receive_time[i][j] = T_now;
        }

    }
    for (int i = 0; i < num_thread; i++)
    {
        mainLoopThreadQueue[i] = new std::thread(main_loop, dmaContextQueue[i], i, isServer, write_send_time[i],read_send_time[i], write_receive_time[i], read_receive_time[i]);
        // mainRecvLoopThreadQueue[i] = new std::thread(main_recv_loop, dmaContextQueue[i], i, isServer, write_send_time[i],read_send_time[i], write_receive_time[i], read_receive_time[i]);
    }


    while (1)
    {
        sleep(10000000);
    }
    printf("using: %s\n", ibv_get_device_name(ib_dev));
}