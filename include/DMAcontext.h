#ifndef DMAcontext_H
#define DMAcontext_H
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rdma/rdma_cma.h>
#include <infiniband/verbs.h>
#include <infiniband/mlx5dv.h>

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

#include <hiredis/hiredis.h>
#include <hiredis/adapters/poll.h>
#include <hiredis/async.h>
#include <thread>
#include <cmath>
#include <chrono>
#include <iostream>
#include "packet.h"
#include "mlx5.h"

#define DEBUG_CHECK_CQ 0
const int my_send_queue_length = 2048;
const int my_recv_queue_length = my_send_queue_length * 8;

// static constexpr size_t kAppRecvCQDepth = 8;
// static constexpr size_t kAppRQDepth = 4; // Multi-packet RQ depth
static constexpr size_t kAppRecvCQDepth = 8; // 8个RQ，实际赋值4个，然后因为mlx5的驱动会产生8个cq
static constexpr size_t kAppRQDepth = 4; // Multi-packet RQ depth wq
// wq

static constexpr size_t kAppLogNumStrides = 9; // 2 ^ 9 = 512
static constexpr size_t kAppLogStrideBytes = 9; // 2 ^ 9 = 512
static constexpr size_t kAppMaxPostlist = 512;

/// Size of one ring message buffer
static constexpr size_t kAppRingMbufSize = (1ull << kAppLogStrideBytes);

/// Number of strides in one multi-packet RECV WQE
static constexpr size_t kAppStridesPerWQE = (1ull << kAppLogNumStrides);

/// Size of one sge
static constexpr size_t kAppSizePerSge = kAppRingMbufSize * kAppStridesPerWQE;

/// Total number of entries in the RX ring
static constexpr size_t kAppNumRingEntries = (kAppStridesPerWQE * kAppRQDepth);

static constexpr size_t kAppRingSize = (kAppNumRingEntries * kAppRingMbufSize);

/// Packets after which the CQE snapshot cycles
static constexpr size_t kAppCQESnapshotCycle = 65536 * kAppStridesPerWQE;

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
/// A consistent snapshot of CQE fields in host endian format
struct cqe_snapshot_t {
    uint16_t wqe_id;
    uint16_t wqe_counter;

    /// Return this packet's index in the CQE snapshot cycle
    size_t get_cqe_snapshot_cycle_idx() const
    {
        return wqe_id * kAppStridesPerWQE + wqe_counter;
    }

    // std::string to_string()
    // {
    //     printf("[ID %s, counter %s",
    //             std::to_string(wqe_id),
    //             std::to_string(wqe_counter));
    //     return ret.str();
    // }
};

struct DMAcontext {
    struct ibv_pd* pd;
    struct ibv_context* ctx;
    struct ibv_cq* receive_cq;
    struct ibv_cq* send_cq;
    struct ibv_mr* send_mr;
    void* send_region;
    struct ibv_qp* data_qp;
    struct ibv_qp* receive_qp;
    struct ibv_cq* receive_recv_cq;
    struct ibv_mr* receive_mp_mr;
    struct ibv_sge* receive_sge;
    uint8_t* mp_recv_ring;
    struct ibv_wq *receive_wq;
    struct ibv_recv_wr *recv_wr;
    unsigned cqe_idx;
    long long recv_buffer_index;
    int recv_to_post;
    int recv_index_to_post;
    cqe_snapshot_t prev_snapshot;
    size_t ring_head;
    size_t nb_rx_rolling;
    size_t sge_idx;
    uint32_t wqe_idx;
    struct mlx5dv_rwq* rwq;
    struct mlx5dv_cq *mlx_cq;
};


void snapshot_cqe(volatile mlx5_cqe64* cqe, cqe_snapshot_t& cqe_snapshot);
void dma_postback(DMAcontext *dma_context);
void dma_update_snapshot(DMAcontext *dma_context, cqe_snapshot_t new_snapshot);

DMAcontext* DMA_create(ibv_device* ib_dev, int thread_id, bool isServer, bool isWrite);

const char* ibv_wc_opcode_str(enum ibv_wc_opcode opcode);

void send_packet(DMAcontext *dma, int chunk_size, int offset, int thread_id, bool isServer);
// size_t receive_packet(DMAcontext *dma, cqe_snapshot_t * new_snapshot);
size_t receive_packet(DMAcontext *dma, cqe_snapshot_t * new_snapshot, int thread_id);

void snapshot_cqe(volatile mlx5_cqe64* cqe, cqe_snapshot_t& cqe_snapshot);
size_t get_cycle_delta(const cqe_snapshot_t& prev, const cqe_snapshot_t& cur);

#endif