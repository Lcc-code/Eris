/* CREDITS:
This boiler plate code is heavily adapted from Intel Connectivity
Academy course ICA-1132: "Barefoot Runtime Interface & PTF"
*/

/* Standard Linux/C++ includes go here */
#include <bf_rt/bf_rt_common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>
#include <bf_rt/bf_rt_info.hpp>
#include <bf_rt/bf_rt_init.hpp>
#include <bf_rt/bf_rt_session.hpp>
#include <bf_rt/bf_rt_table.hpp>
#include <bf_rt/bf_rt_table_data.hpp>
#include <bf_rt/bf_rt_table_key.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef __cplusplus
extern "C" {
#endif
/* All fixed function API includes go here */
#include <bf_switchd/bf_switchd.h>

#ifdef __cplusplus
}
#endif

/*
 * Convenient defines that reflect SDE conventions
 */
#ifndef SDE_INSTALL
#error "Please add -DSDE_INSTALL=\"$SDE_INSTALL\" to CPPFLAGS"
#endif

#ifndef PROG_NAME
#error "Please add -DPROG_NAME=\"optkv\" to CPPFLAGS"
#endif

#define CONF_FILE_DIR "share/p4/targets/tofino"
#define CONF_FILE_PATH(prog) \
    SDE_INSTALL "/" CONF_FILE_DIR "/" prog ".conf"

#define INIT_STATUS_TCP_PORT 7777
#define BFSHELL SDE_INSTALL "/bin/bfshell"  // macro string concat

#define DEV_TGT_ALL_PIPES 0xFFFF
#define DEV_TGT_ALL_PARSERS 0xFF
#define CHECK_BF_STATUS(status) __check_bf_status__(status, __FILE__, __LINE__)
bf_status_t status;

void __check_bf_status__(bf_status_t status, const char *file, int lineNumber);
std::vector<uint64_t> parse_string_to_key(std::string str);

struct message {
    long msg_type;
    uint32_t srcIP;
};

static inline bool sortAscByVal(const std::pair<uint64_t, uint64_t> &a, const std::pair<uint64_t, uint64_t> &b) {
    return (a.second < b.second);
}

int NUM = 500;
std::vector<std::unique_ptr<bfrt::BfRtTableKey>> keys(NUM*2);
std::vector<std::unique_ptr<bfrt::BfRtTableData>> datas(keys.size());
bfrt::BfRtTable::keyDataPairs key_data_pairs;
std::vector<uint64_t> reg_data_vector(0,NUM * 4);
static inline void addEntry(std::shared_ptr<bfrt::BfRtSession> &session, bf_rt_target_t &dev_tgt, const bfrt::BfRtTable *reg_register, std::string &reg) {

    // CHECK_BF_STATUS(status);
    bf_rt_id_t reg_index_key_id;
    bf_rt_id_t data_id;
    status = reg_register->keyFieldIdGet("$REGISTER_INDEX", &reg_index_key_id);
    // CHECK_BF_STATUS(status);
    status = reg_register->dataFieldIdGet(reg, &data_id);
    // CHECK_BF_STATUS(status);
    
    for (int i = 0; i < NUM; i++)
    {
        reg_register->keyAllocate(&keys[i]);
        reg_register->dataAllocate(&datas[i]);
        keys[i]->setValue(reg_index_key_id, static_cast<uint64_t>(i));
        datas[i]->setValue(data_id, static_cast<uint64_t>(i));

        status = reg_register->tableEntryAdd(*session, dev_tgt,
                                   *keys[i].get(), *datas[i].get());
        CHECK_BF_STATUS(status);
        // printf("setKey:%lu value:%lu\n", uint64_t(i), uint64_t(i));
    }
}

int app_dyso(bf_switchd_context_t *switchd_ctx) {
    (void)switchd_ctx;
    bf_rt_target_t dev_tgt;
    std::shared_ptr<bfrt::BfRtSession> session;
    const bfrt::BfRtInfo *bf_rt_info = nullptr;
    auto fromHwFlag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_HW;
    auto fromSwFlag = bfrt::BfRtTable::BfRtTableGetFlag::GET_FROM_SW;

    // to avoid unused compiler error
    printf("Flags - fromHwFlag=%ld, fromSwFlag=%ld\n",
           static_cast<uint64_t>(fromHwFlag), static_cast<uint64_t>(fromSwFlag));

    /* Prepare the dev_tgt */
    memset(&dev_tgt, 0, sizeof(dev_tgt));
    dev_tgt.dev_id = 0;
    dev_tgt.pipe_id = DEV_TGT_ALL_PIPES;

    /* Create BfRt session and retrieve BfRt Info */
    // Create a new BfRt session
    session = bfrt::BfRtSession::sessionCreate();
    if (session == nullptr) {
        printf("ERROR: Couldn't create BfRtSession\n");
        exit(1);
    }

    // Get ref to the singleton devMgr
    bfrt::BfRtDevMgr &dev_mgr = bfrt::BfRtDevMgr::getInstance();
    status = dev_mgr.bfRtInfoGet(dev_tgt.dev_id, PROG_NAME, &bf_rt_info);

    if (status != BF_SUCCESS) {
        printf("ERROR: Could not retrieve BfRtInfo: %s\n", bf_err_str(status));
        return status;
    }
    printf("Retrieved BfRtInfo successfully!\n");

    status = session->sessionCompleteOperations();
    CHECK_BF_STATUS(status);
    
    printf("Running DySO's control plane.....\n");


    // for measurement
    const bfrt::BfRtTable *reg_register0 = nullptr;
    const bfrt::BfRtTable *reg_register1 = nullptr;
    const bfrt::BfRtTable *reg_register2 = nullptr;
    const bfrt::BfRtTable *reg_register3 = nullptr;
    const bfrt::BfRtTable *reg_register4 = nullptr;
    const bfrt::BfRtTable *reg_register5 = nullptr;
    const bfrt::BfRtTable *reg_register6 = nullptr;
    const bfrt::BfRtTable *reg_register7 = nullptr;
    const bfrt::BfRtTable *reg_register8 = nullptr;
    const bfrt::BfRtTable *reg_register9 = nullptr;

    const bfrt::BfRtTable *reg_register10 = nullptr;
    const bfrt::BfRtTable *reg_register11 = nullptr;
    const bfrt::BfRtTable *reg_register12 = nullptr;
    const bfrt::BfRtTable *reg_register13 = nullptr;
    const bfrt::BfRtTable *reg_register14 = nullptr;
    const bfrt::BfRtTable *reg_register15 = nullptr;
    const bfrt::BfRtTable *reg_register16 = nullptr;
    const bfrt::BfRtTable *reg_register17 = nullptr;
    const bfrt::BfRtTable *reg_register18 = nullptr;
    const bfrt::BfRtTable *reg_register19 = nullptr;

    const bfrt::BfRtTable *reg_register20 = nullptr;
    const bfrt::BfRtTable *reg_register21 = nullptr;
    const bfrt::BfRtTable *reg_register22 = nullptr;
    const bfrt::BfRtTable *reg_register23 = nullptr;
    const bfrt::BfRtTable *reg_register24 = nullptr;
    const bfrt::BfRtTable *reg_register25 = nullptr;
    const bfrt::BfRtTable *reg_register26 = nullptr;
    const bfrt::BfRtTable *reg_register27 = nullptr;
    const bfrt::BfRtTable *reg_register28 = nullptr;
    const bfrt::BfRtTable *reg_register29 = nullptr;

    const bfrt::BfRtTable *reg_register30 = nullptr;
    std::vector<std::string> reg;
    for (int index = 0; index < 31; index++)
    {   
        reg.push_back("SwitchIngress.write_process.process_data" + std::to_string(index) + ".register" + std::to_string(index) + ".f1");

    }

    

    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data0.register0", &reg_register0);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data1.register1", &reg_register1);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data2.register2", &reg_register2);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data3.register3", &reg_register3);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data4.register4", &reg_register4);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data5.register5", &reg_register5);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data6.register6", &reg_register6);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data7.register7", &reg_register7);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data8.register8", &reg_register8);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data9.register9", &reg_register9);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data10.register10", &reg_register10);

    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data11.register11", &reg_register11);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data12.register12", &reg_register12);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data13.register13", &reg_register13);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data14.register14", &reg_register14);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data15.register15", &reg_register15);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data16.register16", &reg_register16);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data17.register17", &reg_register17);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data18.register18", &reg_register18);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data19.register19", &reg_register19);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data20.register20", &reg_register20);

    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data21.register21", &reg_register21);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data22.register22", &reg_register22);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data23.register23", &reg_register23);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data24.register24", &reg_register24);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data25.register25", &reg_register25);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data26.register26", &reg_register26);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data27.register27", &reg_register27);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data28.register28", &reg_register28);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data29.register29", &reg_register29);
    status = bf_rt_info->bfrtTableFromNameGet("SwitchIngress.write_process.process_data30.register30", &reg_register30);

    // auto init_time = std::chrono::system_clock::now();
    auto init_time = std::chrono::high_resolution_clock::now();
    addEntry(session, dev_tgt, reg_register0, reg[0]);
    addEntry(session, dev_tgt, reg_register1, reg[1]);
    addEntry(session, dev_tgt, reg_register2, reg[2]);
    addEntry(session, dev_tgt, reg_register3, reg[3]);
    addEntry(session, dev_tgt, reg_register4, reg[4]);
    addEntry(session, dev_tgt, reg_register5, reg[5]);
    addEntry(session, dev_tgt, reg_register6, reg[6]);
    addEntry(session, dev_tgt, reg_register7, reg[7]);
    addEntry(session, dev_tgt, reg_register8, reg[8]);
    addEntry(session, dev_tgt, reg_register9, reg[9]);

    addEntry(session, dev_tgt, reg_register10, reg[10]);
    addEntry(session, dev_tgt, reg_register11, reg[11]);
    addEntry(session, dev_tgt, reg_register12, reg[12]);
    addEntry(session, dev_tgt, reg_register13, reg[13]);
    addEntry(session, dev_tgt, reg_register14, reg[14]);
    addEntry(session, dev_tgt, reg_register15, reg[15]);
    addEntry(session, dev_tgt, reg_register16, reg[16]);
    addEntry(session, dev_tgt, reg_register17, reg[17]);
    addEntry(session, dev_tgt, reg_register18, reg[18]);
    addEntry(session, dev_tgt, reg_register19, reg[19]);



    addEntry(session, dev_tgt, reg_register20, reg[20]);
    addEntry(session, dev_tgt, reg_register21, reg[21]);
    addEntry(session, dev_tgt, reg_register22, reg[22]);
    addEntry(session, dev_tgt, reg_register23, reg[23]);
    addEntry(session, dev_tgt, reg_register24, reg[24]);
    addEntry(session, dev_tgt, reg_register25, reg[25]);
    addEntry(session, dev_tgt, reg_register26, reg[26]);
    addEntry(session, dev_tgt, reg_register27, reg[27]);
    addEntry(session, dev_tgt, reg_register28, reg[28]);
    addEntry(session, dev_tgt, reg_register29, reg[29]);

    addEntry(session, dev_tgt, reg_register30, reg[30]);


    
    // static inline bool addEntry(std::shared_ptr<bfrt::BfRtSession> &session, bf_rt_target_t &dev_tgt, const bfrt::BfRtTable *reg_register) {

    auto now = std::chrono::high_resolution_clock::now();
    auto duration_write = std::chrono::duration_cast<std::chrono::nanoseconds>(now - init_time);
    // auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - init_time).count() / 1000;
    printf("cost %lf ms\n", double(duration_write.count() / 1000 / 1000));




    // for (int i = 0; i < NUM; i++)
    // {
    //     key_data_pairs.push_back(std::make_pair(keys[i].get(), datas[i].get()));
    // }
    // std::unique_ptr<bfrt::BfRtTableKey> table_key;
    // std::unique_ptr<bfrt::BfRtTableData> table_data;
    // uint64_t key, value;
    // key = 1;
    // value = 1;
    

    // status = reg_register1->keyAllocate(&table_key);
    // CHECK_BF_STATUS(status);
    // status = reg_register1->dataAllocate(&table_data);
    // CHECK_BF_STATUS(status);

    // status = table_key->setValue(reg_index_key_id, key);
    // status = table_data->setValue(data_id, value);

    // status = reg_register1->tableEntryAdd(*session, dev_tgt,
    //                                *table_key.get(), *table_data.get());
    // CHECK_BF_STATUS(status);
    
    // // read
    // status = table_key->setValue(reg_index_key_id, key);
    // status = reg_register1->tableEntryGet(*session, dev_tgt, *table_key, fromHwFlag, table_data.get());
    // status = table_data->getValue(data_id, &reg_data_vector);

    // printf("optkv : register_Id %lu\n", reg_data_vector[0]);


    status = session->sessionDestroy();
    CHECK_BF_STATUS(status);
    return status;
}

/* Helper function to check bf_status */
void __check_bf_status__(bf_status_t status, const char *file, int lineNumber) {
    ;
    if (status != BF_SUCCESS) {
        printf("ERROR: CHECK_BF_STATUS failed at %s:%d\n", file, lineNumber);
        printf("   ==> with error: %s\n", bf_err_str(status));
        exit(status);
    }
}

bf_switchd_context_t *init_switchd() {
    bf_status_t status = 0;
    bf_switchd_context_t *switchd_ctx;

    /* Allocate switchd context */
    if ((switchd_ctx = (bf_switchd_context_t *)calloc(
             1, sizeof(bf_switchd_context_t))) == NULL) {
        printf("Cannot Allocate switchd context\n");
        exit(1);
    }

    /* Minimal switchd context initialization to get things going */
    switchd_ctx->install_dir = strdup(SDE_INSTALL);
    switchd_ctx->conf_file = strdup(CONF_FILE_PATH(PROG_NAME));
    switchd_ctx->running_in_background = true;
    switchd_ctx->dev_sts_thread = true;
    switchd_ctx->dev_sts_port = INIT_STATUS_TCP_PORT;

    /* Initialize the device */
    status = bf_switchd_lib_init(switchd_ctx);
    if (status != BF_SUCCESS) {
        printf("ERROR: Device initialization failed: %s\n", bf_err_str(status));
        exit(1);
    }

    return switchd_ctx;
}

int main(int argc, char **argv) {
    /* Not using cmdline params in this minimal boiler plate */
    (void)argc;
    (void)argv;

    bf_status_t status = 0;
    bf_switchd_context_t *switchd_ctx;

    /* Check if this CP program is being run as root */
    if (geteuid() != 0) {
        printf("ERROR: This control plane program must be run as root (e.g. sudo %s)\n", argv[0]);
        exit(1);
    }

    /* Initialize the switchd context */
    switchd_ctx = init_switchd();

    status = app_dyso(switchd_ctx);
    CHECK_BF_STATUS(status);

    /* Run Indefinitely */
    printf("Run indefinitely...\n");
    while (true) {
        sleep(1);
    }

    if (switchd_ctx) {
        free(switchd_ctx);
    }
    return status;
}