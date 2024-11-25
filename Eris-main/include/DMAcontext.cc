#include "DMAcontext.h"
#include "packet.h"
#include <atomic>

unsigned char PS_FILTER_TEMPLATE_R[] = { 0x05, 0x04, 0x03, 0x02, 0x01, 0xFF };
unsigned char WORKER_FILTER_TEMPLATE_R[] = { 0x77, 0x77, 0x77, 0x77, 0x77, 0xFF };

void snapshot_cqe(volatile mlx5_cqe64* cqe, cqe_snapshot_t& cqe_snapshot)
{
    while (true) {
        uint16_t wqe_id_0 = cqe->wqe_id;
        uint16_t wqe_counter_0 = cqe->wqe_counter;
        // std::cout << "wqe_id_0 " << ntohs(wqe_id_0) << " wqe_counter_0 "<< ntohs(wqe_counter_0) << std::endl;
        memory_barrier();
        uint16_t wqe_id_1 = cqe->wqe_id;
        uint16_t wqe_counter_1 = cqe->wqe_counter;

        if (likely(wqe_id_0 == wqe_id_1 && wqe_counter_0 == wqe_counter_1)) {
            cqe_snapshot.wqe_id = ntohs(wqe_id_0);
            cqe_snapshot.wqe_counter = ntohs(wqe_counter_0);
            return;
        }
    }
}

size_t get_cycle_delta(const cqe_snapshot_t& prev, const cqe_snapshot_t& cur)
{
    size_t prev_idx = prev.get_cqe_snapshot_cycle_idx();
    size_t cur_idx = cur.get_cqe_snapshot_cycle_idx();

    assert(prev_idx < kAppCQESnapshotCycle && cur_idx < kAppCQESnapshotCycle);
    // printf("prev_idx %d cur_idx %d\n", prev_idx, cur_idx);
    return ((cur_idx + kAppCQESnapshotCycle) - prev_idx) % kAppCQESnapshotCycle;
}

void dma_postback(DMAcontext *dma_context)
{
    dma_context->ring_head = (dma_context->ring_head + 1) % kAppNumRingEntries;
    dma_context->nb_rx_rolling++;
    if (dma_context->nb_rx_rolling == kAppStridesPerWQE)
    {
        dma_context->nb_rx_rolling = 0;

        struct mlx5_mprq_wqe *wqes = (struct mlx5_mprq_wqe*) dma_context->rwq->buf;
        struct mlx5_wqe_data_seg *scat;
        
        int index = dma_context->sge_idx;
        scat = &wqes[index].dseg;
        size_t mpwqe_offset = index * (kAppRingMbufSize * kAppStridesPerWQE);
        scat->addr = htobe64((uintptr_t)reinterpret_cast<uint64_t>(&dma_context->mp_recv_ring[mpwqe_offset]));
        scat->byte_count = htobe32(kAppRingMbufSize * kAppStridesPerWQE);
        scat->lkey = htobe32(dma_context->receive_mp_mr->lkey);
    
        dma_context->wqe_idx += 1;
        auto dbrec = (volatile uint32_t *)(dma_context->rwq->dbrec);
        (*dbrec) = htobe32(dma_context->wqe_idx);
        dma_context->sge_idx = (dma_context->sge_idx + 1) % kAppRQDepth;

    }
}

void dma_update_snapshot(DMAcontext *dma_context, cqe_snapshot_t new_snapshot)
{
    dma_context->prev_snapshot = new_snapshot;
    dma_context->cqe_idx = (dma_context->cqe_idx + 1) % kAppRecvCQDepth;
}

void send_packet(DMAcontext *dma, int chunk_size, int offset, int thread_id, bool isServer)
{
    int ret;
    struct ibv_sge sg;
    struct ibv_send_wr wr, *bad_wr;
    
    memset(&sg, 0, sizeof(sg));    
    
    sg.addr = (uintptr_t)((char *)dma->send_region + offset * (LAYER_SIZE));

    sg.length = chunk_size;
    sg.lkey = dma->send_mr->lkey;

    char hdr[IP_ETH_UDP_HEADER_SIZE]; // ETH/IPv4/TCP header example
    if (isServer)
        memcpy(hdr, PS_IP_ETH_UDP_HEADER, IP_ETH_UDP_HEADER_SIZE); // Assuming that the header buffer was define before.
    else
        memcpy(hdr, WORKER_IP_ETH_UDP_HEADER, IP_ETH_UDP_HEADER_SIZE); // Assuming that the header buffer was define before.

    hdr[5] = thread_id;
    if (!isServer)
        hdr[11] = thread_id;
    
    memset(&wr, 0, sizeof(wr));
    
    wr.wr_id = 0;
    wr.sg_list = &sg;
    wr.num_sge = 1;
    wr.opcode = IBV_WR_TSO;
    wr.tso.mss = LAYER_SIZE;
    wr.tso.hdr_sz = IP_ETH_UDP_HEADER_SIZE;

    wr.send_flags |= IBV_SEND_SIGNALED;
    wr.tso.hdr = hdr;

    ret = ibv_post_send(dma->data_qp, &wr, &bad_wr);
    
    if (ret != 0)
    {
        printf("dma->send_region %lu\n", dma->send_region);
        fprintf(stderr, "failed to post send %d\n", ret);
        exit(1);
    }

    struct ibv_wc wc_send_cq[1024];
    ret = ibv_poll_cq(dma->send_cq, 1024, wc_send_cq);
    if (ret < 0) {
        printf("ibv_poll_cq() failed\n");
        return;
    }
}

size_t receive_packet(DMAcontext *dma, cqe_snapshot_t * new_snapshot, int thread_id)
{
    auto *recv_cqe_arr = reinterpret_cast<volatile mlx5_cqe64*>(dma->mlx_cq->buf);
    // printf("\ndma->cqe_idx:%d\n", dma->cqe_idx);
    snapshot_cqe(&recv_cqe_arr[dma->cqe_idx], *new_snapshot);
    const size_t delta = get_cycle_delta(dma->prev_snapshot, *new_snapshot);
    
    if (DEBUG_CHECK_CQ)
    {
        printf("delta %d thread_id:%d kAppNumRingEntries:%d \n", delta, thread_id,kAppNumRingEntries);
        std::cout << "thread_id " << thread_id << std::endl;
        for (size_t i = 0; i < 8; i++) {
        // Make it think that all buffers have been filled and we are about to fill up the first packet
        std::cout <<" i " << i <<  " wqe_id: "<< ntohs(recv_cqe_arr[i].wqe_id) << " wqe_counter: "<< ntohs(recv_cqe_arr[i].wqe_counter) << std::endl;
        //grab_snapshot(&recv_cqe_arr[i]);
        }
    }
    
    if (!(delta == 0 || delta >= kAppNumRingEntries))
    {
        return delta;
    } else
        return 0;
}

static unsigned long long get_recv_cycle(int id, int counter) {
  return id * kAppStridesPerWQE + counter;  
}

DMAcontext* DMA_create(ibv_device* ib_dev, int thread_id, bool isServer, bool isWrite)
{
    ibv_context *context = ibv_open_device(ib_dev);
    if (!context) {
        fprintf(stderr, "Couldn't get context for %s\n",
            ibv_get_device_name(ib_dev));
        exit(1);
    }

    struct mlx5dv_context attrs_out;
  attrs_out.comp_mask = MLX5DV_CONTEXT_MASK_STRIDING_RQ;
  mlx5dv_query_device(context, &attrs_out);
//   printf("Attrs flags = %d Striding rq caps = %d\n", attrs_out.flags, attrs_out.striding_rq_caps.supported_qpts);

    ibv_pd* pd = ibv_alloc_pd(context);
    if (!pd) {
        fprintf(stderr, "Couldn't allocate PD\n");
        exit(1);
    }
    struct ibv_cq* rec_cq = ibv_create_cq(context, my_recv_queue_length + 1, NULL, NULL, 0);
        if (!rec_cq) {
        fprintf(stderr, "Couldn't create CQ %d\n", errno);
        exit(1);
    }

    struct ibv_cq* snd_cq = ibv_create_cq(context, my_send_queue_length + 1, NULL, NULL, 0);
    if (!snd_cq) {
        fprintf(stderr, "Couldn't create CQ %d\n", errno);
        exit(1);
    }

    struct ibv_qp *qp;
    struct ibv_qp_init_attr_ex* qp_init_attr = 
            (struct ibv_qp_init_attr_ex*)malloc(sizeof(struct ibv_qp_init_attr_ex));
    
    memset(qp_init_attr, 0, sizeof(*qp_init_attr));

    qp_init_attr->qp_type = IBV_QPT_RAW_PACKET;
    qp_init_attr->send_cq = snd_cq;
    qp_init_attr->recv_cq = rec_cq;

    qp_init_attr->cap.max_send_wr = my_send_queue_length + 1;
    qp_init_attr->cap.max_send_sge = 1;
    qp_init_attr->cap.max_recv_wr = my_recv_queue_length + 1;
    qp_init_attr->cap.max_recv_sge = 1;
    qp_init_attr->cap.max_inline_data = 512;
    qp_init_attr->pd = pd;
    qp_init_attr->sq_sig_all = 0;
    // qp_init_attr->comp_mask = IBV_QP_INIT_ATTR_PD;
    
    qp_init_attr->comp_mask = IBV_QP_INIT_ATTR_MAX_TSO_HEADER | IBV_QP_INIT_ATTR_PD;
    qp_init_attr->max_tso_header = IP_ETH_UDP_HEADER_SIZE;
    

    qp = ibv_create_qp_ex(context, qp_init_attr);
    if (!qp) {
        fprintf(stderr, "Couldn't create RSS QP %d\n");
        if (qp) {
            int ret = ibv_destroy_qp(qp);
            if (ret) {
                fprintf(stderr, "ibv_destroy_qp() failed, ret %d, errno %d: %s\n", ret, errno, strerror(errno));
            }
        }
        exit(1);
    }

    struct ibv_qp_attr qp_attr;
    int qp_flags;
    int ret;
    memset(&qp_attr, 0, sizeof(qp_attr));
    qp_flags = IBV_QP_STATE | IBV_QP_PORT;
    qp_attr.qp_state = IBV_QPS_INIT;
    qp_attr.port_num = 1;
    ret = ibv_modify_qp(qp, &qp_attr, qp_flags);
    if (ret < 0) {
        fprintf(stderr, "failed modify qp to init\n");
        if (qp) {
            ret = ibv_destroy_qp(qp);
            if (ret) {
                fprintf(stderr, "ibv_destroy_qp() failed, ret %d, errno %d: %s\n", ret, errno, strerror(errno));
            }
        }
        exit(1);

    }
    memset(&qp_attr, 0, sizeof(qp_attr));

    //  move the qp to receive
    qp_flags = IBV_QP_STATE;
    qp_attr.qp_state = IBV_QPS_RTR;
    ret = ibv_modify_qp(qp, &qp_attr, qp_flags);
    if (ret < 0) {
        fprintf(stderr, "failed modify qp to receive\n");
        if (qp) { 
            ret = ibv_destroy_qp(qp);
            if (ret) {
                fprintf(stderr, "ibv_destroy_qp() failed, ret %d, errno %d: %s\n", ret, errno, strerror(errno));
            }
        }
        exit(1);

    };

    //  move the qp to send
    qp_flags = IBV_QP_STATE;
    qp_attr.qp_state = IBV_QPS_RTS;
    ret = ibv_modify_qp(qp, &qp_attr, qp_flags);
    if (ret < 0) {
        fprintf(stderr, "failed modify qp to send\n");
        if (qp) {
            ret = ibv_destroy_qp(qp);
            if (ret) {
                fprintf(stderr, "ibv_destroy_qp() failed, ret %d, errno %d: %s\n", ret, errno, strerror(errno));
            }
        }
        exit(1);
        // exit(1);
    }

    int send_buf_size = (LAYER_SIZE + IP_ETH_UDP_HEADER_SIZE) * my_send_queue_length + 1;

    void *send_buf;
    ib_malloc(&send_buf, send_buf_size);
    if (!send_buf) {
        fprintf(stderr, "Coudln't allocate send memory\n");
        exit(1);
    }

    struct ibv_mr* send_mr;
    send_mr = ibv_reg_mr(pd, send_buf, send_buf_size, IBV_ACCESS_LOCAL_WRITE);
    if (!send_mr) {
        fprintf(stderr, "Couldn't register recv mr\n");
        if (send_buf) {
            free(send_buf);
        }
        exit(1);
    }
    /* Recv Queue*/
    struct ibv_cq_init_attr_ex cq_init_attr;
    memset(&cq_init_attr, 0, sizeof(cq_init_attr));
    cq_init_attr.parent_domain = pd;
    cq_init_attr.cqe = kAppRecvCQDepth / 2;  // 8个RQ，实际赋值4个，然后因为mlx5的驱动会产生8个cq
    cq_init_attr.flags = IBV_CREATE_CQ_ATTR_IGNORE_OVERRUN | IBV_CREATE_CQ_ATTR_SINGLE_THREADED;
    cq_init_attr.comp_mask = IBV_CQ_INIT_ATTR_MASK_FLAGS;
    // cq_init_attr.wc_flags = IBV_WC_STANDARD_FLAGS;

    // mlx5cq
    struct mlx5dv_cq_init_attr mlx_cq_init_attr;
    memset(&mlx_cq_init_attr, 0, sizeof(mlx_cq_init_attr));
    mlx_cq_init_attr.comp_mask = 0;

    struct ibv_cq* recv_cq = ibv_cq_ex_to_cq(mlx5dv_create_cq(context, &cq_init_attr, &mlx_cq_init_attr));
    assert(rec_cq != nullptr);

    // original cq
    // struct ibv_cq_ex *mp_recv_cq = ibv_create_cq_ex(context, &cq_init_attr);
    // assert(mp_recv_cq != nullptr);

    // using mlx5 API 
    struct ibv_wq_init_attr wq_init_attr = {};
    wq_init_attr.wq_type = IBV_WQT_RQ;
    wq_init_attr.max_wr = kAppRQDepth; // 8 = 4096 / (1 << single_wqe_log_num_of_strides)
    // wq_init_attr.max_wr = 16; // 4096 / single_wqe_log_num_of_strides
    wq_init_attr.max_sge = 1;
    wq_init_attr.pd = pd;
    wq_init_attr.cq = recv_cq;
    wq_init_attr.comp_mask = IBV_WQ_INIT_ATTR_FLAGS;
    
    struct mlx5dv_wq_init_attr mlx5_wq_attr;
    memset(&mlx5_wq_attr, 0, sizeof(mlx5dv_wq_init_attr));

    // Now for the strided stuff
    mlx5_wq_attr.comp_mask = MLX5DV_WQ_INIT_ATTR_MASK_STRIDING_RQ;
    // #define IBV_FRAME_SIZE_LOG (10ull) // ALL_PACKET_SIZE
    mlx5_wq_attr.striding_rq_attrs.single_stride_log_num_of_bytes = kAppLogStrideBytes; // one stride size = 2^.. 

    mlx5_wq_attr.striding_rq_attrs.single_wqe_log_num_of_strides = kAppLogNumStrides; // num_of strides peer wqe
    mlx5_wq_attr.striding_rq_attrs.two_byte_shift_en = 0;
    struct ibv_wq* recv_wq = mlx5dv_create_wq(context, &wq_init_attr, &mlx5_wq_attr);
    assert(recv_wq != nullptr);
    // struct ibv_wq *mp_wq = ibv_create_wq(context, &wq_init_attr);
    // assert(mp_wq != nullptr);

    struct ibv_wq_attr wq_attr;
    memset(&wq_attr, 0, sizeof(wq_attr));
    wq_attr.attr_mask = IBV_WQ_ATTR_STATE;
    wq_attr.wq_state = IBV_WQS_RDY;
    ret = ibv_modify_wq(recv_wq, &wq_attr);
    assert(ret == 0);

    struct mlx5dv_obj obj;
    struct mlx5dv_rwq *rwq = new mlx5dv_rwq();
    struct mlx5dv_cq *rx_cq_dv = new mlx5dv_cq();
    obj.rwq.in = recv_wq;
    obj.rwq.out = rwq;
    obj.cq.in = recv_cq;
    obj.cq.out = rx_cq_dv;

    ret = mlx5dv_init_obj(&obj, MLX5DV_OBJ_CQ | MLX5DV_OBJ_RWQ);
    assert(ret == 0);
    
    // // stride 是TMD的wqe的长度，
    // printf("rwq.wqe.cnt = %d\n", rwq->wqe_cnt);
    // printf("rwq.stride = %d\n", rwq->stride);
    // printf("rx_cq_dv.cqe_cnt = %d\n", rx_cq_dv.cqe_cnt);
    // printf("rx_cq_dv.cqe_size = %d\n", rx_cq_dv.cqe_size);
    // printf("rx_cq_dv.cqn = %d\n", rx_cq_dv.cqn);
    // printf("rx_cq_dv.comp_mask = %d\n", rx_cq_dv.comp_mask);



    // if (rwq.stride != sizeof(mlx5_mprq_wqe))
    // {
    //     ibv_destroy_wq(recv_wq);
    //     printf("multi-packet receive queue has unexpected stride\n");
    //     exit(1);
    // }


    struct ibv_rwq_ind_table_init_attr mp_ind_table = {0};
    mp_ind_table.log_ind_tbl_size = 0;
    mp_ind_table.ind_tbl = &recv_wq;
    mp_ind_table.comp_mask = 0;
    struct ibv_rwq_ind_table *mp_ind_tbl = ibv_create_rwq_ind_table(context, &mp_ind_table);
    assert(mp_ind_tbl != nullptr);

    // Create rx_hash_conf and indirection table for the QP
    uint8_t toeplitz_key[] = { 0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2,
        0x41, 0x67, 0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0,
        0xd0, 0xca, 0x2b, 0xcb, 0xae, 0x7b, 0x30, 0xb4,
        0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30, 0xf2, 0x0c,
        0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa };
    // uint8_t toeplitz_key[] = {0x2c, 0xc6, 0x81, 0xd1, 0x5b, 0xdb, 0xf4, 0xf7,
  //  0xfc, 0xa2, 0x83, 0x19, 0xdb, 0x1a, 0x3e, 0x94,
  //  0x6b, 0x9e, 0x38, 0xd9, 0x2c, 0x9c, 0x03, 0xd1,
  //  0xad, 0x99, 0x44, 0xa7, 0xd9, 0x56, 0x3d, 0x59,
  //  0x06, 0x3c, 0x25, 0xf3, 0xfc, 0x1f, 0xdc, 0x2a};
    const int TOEPLITZ_RX_HASH_KEY_LEN = sizeof(toeplitz_key) / sizeof(toeplitz_key[0]);

    struct ibv_rx_hash_conf rss_conf = {
        .rx_hash_function = IBV_RX_HASH_FUNC_TOEPLITZ,
        .rx_hash_key_len = TOEPLITZ_RX_HASH_KEY_LEN,
        .rx_hash_key = toeplitz_key,
        .rx_hash_fields_mask = 0,
        // .rx_hash_fields_mask = IBV_RX_HASH_DST_PORT_UDP,
    };

    unsigned char R_DST_MAC[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    if (isServer)
        memcpy(R_DST_MAC, PS_FILTER_TEMPLATE_R, sizeof(R_DST_MAC));
    else
        memcpy(R_DST_MAC, WORKER_FILTER_TEMPLATE_R, sizeof(R_DST_MAC));
    
    R_DST_MAC[5] = thread_id;

    uint32_t R_DST_IP = htonl(0x0D073870);
    if (isWrite)
        R_DST_IP = htonl(0x0D07387F);
    printf("isWrite %d R_DST_IP %lu \n", isWrite, R_DST_IP);

    struct raw_eth_flow_attr {
        struct ibv_flow_attr            attr;
        struct ibv_flow_spec_eth        spec_eth;
        struct ibv_flow_spec_ipv4       spec_ipv4;
    } __attribute__((packed));
    struct raw_eth_flow_attr flow_attr;
    if (isServer)
    {
        flow_attr.attr.comp_mask      = 0;
        flow_attr.attr.type           = IBV_FLOW_ATTR_NORMAL;
        flow_attr.attr.size           = sizeof(flow_attr);
        flow_attr.attr.priority       = 0;
        flow_attr.attr.num_of_specs   = 2;
        flow_attr.attr.port           = 1;
        flow_attr.attr.flags          = 0;

        flow_attr.spec_eth.type   = IBV_FLOW_SPEC_ETH,
        flow_attr.spec_eth.size   = sizeof(struct ibv_flow_spec_eth),
        flow_attr.spec_eth.val = {
                            // .dst_mac = {0x77, 0x77, 0x77, 0x77, 0x77, 0xFF},
                            .dst_mac = { R_DST_MAC[0], R_DST_MAC[1], R_DST_MAC[2], R_DST_MAC[3], R_DST_MAC[4], R_DST_MAC[5]},
                            .src_mac = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            .ether_type = 0,
                            .vlan_tag = 0,
                    };
        flow_attr.spec_eth.mask = {
                            // .dst_mac = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                            .dst_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                            .src_mac = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            .ether_type = 0,
                            .vlan_tag = 0,
                    };
        flow_attr.spec_ipv4.type   = IBV_FLOW_SPEC_IPV4;
        flow_attr.spec_ipv4.size   = sizeof(struct ibv_flow_spec_ipv4);
        flow_attr.spec_ipv4.val = {
                            .src_ip = 0,
                            .dst_ip = R_DST_IP,
                    };
        flow_attr.spec_ipv4.mask = {
                            .src_ip = 0,
                            .dst_ip = 0xFFFFFFFF,
                    };
    } else {
        flow_attr.attr.comp_mask      = 0;
        flow_attr.attr.type           = IBV_FLOW_ATTR_NORMAL;
        flow_attr.attr.size           = sizeof(flow_attr);
        flow_attr.attr.priority       = 0;
        flow_attr.attr.num_of_specs   = 1;
        flow_attr.attr.port           = 1;
        flow_attr.attr.flags          = 0;

        flow_attr.spec_eth.type   = IBV_FLOW_SPEC_ETH,
        flow_attr.spec_eth.size   = sizeof(struct ibv_flow_spec_eth),
        flow_attr.spec_eth.val = {
                            // .dst_mac = {0x77, 0x77, 0x77, 0x77, 0x77, 0xFF},
                            .dst_mac = { R_DST_MAC[0], R_DST_MAC[1], R_DST_MAC[2], R_DST_MAC[3], R_DST_MAC[4], R_DST_MAC[5]},
                            .src_mac = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            .ether_type = 0,
                            .vlan_tag = 0,
                    };
        flow_attr.spec_eth.mask = {
                            // .dst_mac = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                            .dst_mac = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                            .src_mac = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                            .ether_type = 0,
                            .vlan_tag = 0,
                    };
    }

    struct ibv_qp_init_attr_ex mp_qp_init_attr;
    memset(&mp_qp_init_attr, 0, sizeof(mp_qp_init_attr));
    mp_qp_init_attr.pd = pd;
    mp_qp_init_attr.qp_type = IBV_QPT_RAW_PACKET;
    mp_qp_init_attr.comp_mask = IBV_QP_INIT_ATTR_PD | IBV_QP_INIT_ATTR_IND_TABLE | IBV_QP_INIT_ATTR_RX_HASH |IBV_QP_INIT_ATTR_CREATE_FLAGS;
    mp_qp_init_attr.rx_hash_conf = rss_conf;
    mp_qp_init_attr.rwq_ind_tbl = mp_ind_tbl;

    struct ibv_qp* mp_recv_qp = ibv_create_qp_ex(context, &mp_qp_init_attr);
    if (!mp_recv_qp)
  {   
        fprintf(stderr, "ibv_create_qp_ex %d\n", errno);
        exit(1);
    }
    assert(mp_recv_qp != nullptr);
    
    struct ibv_flow *eth_flow;
    eth_flow = ibv_create_flow(mp_recv_qp, &(flow_attr.attr));
    if (!eth_flow)
    {   
        fprintf(stderr, "ibv_create_flow %d\n", errno);
        exit(1);
    }
  
    // Before we post the recvs, we have to arrange the recv_cq to be in the right "shape"
    // struct mlx5_cq* mcq = to_mcq(recv_cq);
    // struct mlx5_cq* mcq = reinterpret_cast<mlx5_cq*>(recv_cq);
    auto *recv_cqe_arr =reinterpret_cast<volatile mlx5_cqe64*>( rx_cq_dv->buf);
    assert(recv_cqe_arr != nullptr);
    // struct mlx5_cq* mcq;

    /* set ownership of cqes to "hardware" */
  // for (int i = 0; i < rx_cq_dv.cqe_cnt; i++)
  // {
    //     mlx5dv_set_cqe_owner(&recv_cqe_arr[i], 1);
    // }
    for (size_t i = 0; i < kAppRecvCQDepth; i++)
    {
        // make it think that all buffers have been filled and we are about to fill up the first packet
        
        /*
        * wqe_id is valid only for
        * Striding RQ (Multi-Packet RQ).
        * It provides the WQE index inside the RQ.
        */
        recv_cqe_arr[i].wqe_id = htons(UINT16_MAX);
        /*
        * In Striding RQ (Multi-Packet RQ) wqe_counter provides the WQE stride index (to calc pointer to start of the message)
        * 
        */
        recv_cqe_arr[i].wqe_counter = htons(kAppStridesPerWQE - (kAppRecvCQDepth - i));
        recv_cqe_arr[i].op_own = 0xf1;
        cqe_snapshot_t snapshot;
        snapshot_cqe(&recv_cqe_arr[i], snapshot);
        assert(snapshot.get_cqe_snapshot_cycle_idx() == kAppCQESnapshotCycle - (kAppRecvCQDepth - i));

    }

    size_t tx_ring_size = LAYER_SIZE * kAppMaxPostlist;
    uint8_t* mp_send_ring;
    ib_malloc((void **)&mp_send_ring, tx_ring_size);
    assert(mp_send_ring != nullptr);
    memset(mp_send_ring, 0, tx_ring_size);

    struct ibv_mr* mp_send_mr = ibv_reg_mr(pd, mp_send_ring, tx_ring_size, IBV_ACCESS_LOCAL_WRITE);
    assert(mp_send_mr != nullptr);
    
    
    // Register RX ring memory
    uint8_t* mp_recv_ring;
    ib_malloc((void **)&mp_recv_ring, kAppRingSize + 4096); // headroom
    mp_recv_ring = mp_recv_ring + 4096;
    assert(mp_recv_ring != nullptr);
    
    // mp_recv_ring = mp_recv_ring + 4096;
    memset(mp_recv_ring, 0, kAppRingSize);

    
    struct ibv_mr* mp_mr = ibv_reg_mr(pd, mp_recv_ring , kAppRingSize, IBV_ACCESS_LOCAL_WRITE |  IBV_ACCESS_HUGETLB);
    assert(mp_mr != nullptr);
    struct ibv_sge* mp_sge = reinterpret_cast<struct ibv_sge*>(malloc(sizeof(struct ibv_sge) * kAppRQDepth));
    struct ibv_recv_wr* recv_wr = reinterpret_cast<struct ibv_recv_wr*>(malloc(sizeof(struct ibv_recv_wr) * kAppRQDepth));
    
    struct mlx5_mprq_wqe *wqes = (struct mlx5_mprq_wqe*) rwq->buf;
    struct mlx5_wqe_data_seg *scat;
    for (size_t i = 0; i < kAppRQDepth; i++)
    {
        scat = &wqes[i].dseg;
        size_t mpwqe_offset = i * (kAppRingMbufSize * kAppStridesPerWQE);
        scat->addr = htobe64((uintptr_t)reinterpret_cast<uint64_t>(&mp_recv_ring[mpwqe_offset]));
        scat->byte_count = htobe32(kAppRingMbufSize * kAppStridesPerWQE);
        scat->lkey = htobe32(mp_mr->lkey);
    }
    *(rwq->dbrec) = htobe32(kAppRQDepth & 0xffff);
    // printf("thread_id :%lu rwq->dbrec:%lu\n", thread_id,rwq->dbrec);

    auto *cqe_arr = recv_cqe_arr;
    cqe_snapshot_t prev_snapshot;
    snapshot_cqe(&cqe_arr[kAppRecvCQDepth - 1], prev_snapshot);

    DMAcontext *dma = new DMAcontext{
        .pd = pd,
        .ctx = context,
        .receive_cq = rec_cq,
        .send_cq = snd_cq,
        .send_mr = send_mr,
        .send_region = send_buf,
        .data_qp = qp,
        .receive_qp = mp_recv_qp,
        .receive_recv_cq = recv_cq,
        .receive_mp_mr = mp_mr,
        .receive_sge = mp_sge,
        .mp_recv_ring = mp_recv_ring,
        .receive_wq = recv_wq,
        .recv_wr = recv_wr,
        .cqe_idx = 0,

        .recv_buffer_index = 0,
        .recv_to_post = 0,
        .recv_index_to_post = 0,
        .prev_snapshot = prev_snapshot,
        .wqe_idx = kAppRQDepth,
        .rwq = rwq,
        .mlx_cq =rx_cq_dv,
    };

    printf("kAppRingMbufSize=%lu, kAppStridesPerWQE=%lu, kAppRingSize=%lu, kAppRQDepth=%lu\n", kAppRingMbufSize, kAppStridesPerWQE, kAppRingSize, kAppRQDepth);
    return dma;
}