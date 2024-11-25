#ifndef SLIDING_W_H
#define SLIDING_W_H

#include "packet.h"
#define RESEND_TRIGGER 1

class WindowManager {
    public:
        bool* isACKed;
        /* This three variable is completely useless, but
        when deleting it, the performance will drop from 46Gbps to 40Gbps.. */
        bool* isSent;
        std::chrono::high_resolution_clock::time_point* send_time;
        std::chrono::high_resolution_clock::time_point* receive_time;
        /* */
        int total_ACK;
        int last_ACK;

        WindowManager() {
            last_ACK = 0;
        }

        bool inline UpdateWindow(uint16_t* seq_num)
        {
            bool isLastAckUpdated = false;
            // printf("update seq_num %lu last_ACK %d\n", (*seq_num), last_ACK);
            isACKed[*seq_num] = true;
            // printf("2222\n");
            while (isACKed[last_ACK + 1]) {
                // printf("last_ACK + 1 : %d\n", last_ACK + 1);
                last_ACK++;
                isLastAckUpdated = true;
            }
            return isLastAckUpdated;
        }

        int inline Reset(int packet_total)
        {
            last_ACK = 0;
            total_ACK = packet_total;
            for (int i = 0; i < packet_total; i++)
            {
                isACKed[i] = (bool)0;
            }
        }
};

#endif