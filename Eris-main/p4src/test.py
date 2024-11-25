################################################################################
# BAREFOOT NETWORKS CONFIDENTIAL & PROPRIETARY
#
# Copyright (c) 2018-2019 Barefoot Networks, Inc.

# All Rights Reserved.
#
# NOTICE: All information contained herein is, and remains the property of
# Barefoot Networks, Inc. and its suppliers, if any. The intellectual and
# technical concepts contained herein are proprietary to Barefoot Networks,
# Inc.
# and its suppliers and may be covered by U.S. and Foreign Patents, patents in
# process, and are protected by trade secret or copyright law.
# Dissemination of this information or reproduction of this material is
# strictly forbidden unless prior written permission is obtained from
# Barefoot Networks, Inc.
#
# No warranty, explicit or implicit is provided, unless granted under a
# written agreement with Barefoot Networks, Inc.
#
#
###############################################################################


"""
Simple PTF test for simple_l3_mcast (and simple_l3_mcast_checksum)
"""


######### STANDARD MODULE IMPORTS ########
import logging 
import grpc   
import pdb
import time

######### PTF modules for BFRuntime Client Library APIs #######
import ptf
from ptf.testutils import *
from ptf.dataplane import *

from bfruntime_client_base_tests import BfRuntimeTest
import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2
import bfrt_grpc.client as gc

######## PTF modules for Fixed APIs (Thrift) ######
import pd_base_tests
from ptf.thriftutils   import *
from res_pd_rpc.ttypes import *       # Common data types
from mc_pd_rpc.ttypes  import *       # Multicast-specific data types

def make_port(pipe, local_port):
    return (pipe << 7) | local_port

def pgen_port(pipe_id):
    """
    Given a pipe return a port in that pipe which is usable for packet
    generation.  Note that Tofino allows ports 68-71 in each pipe to be used for
    packet generation while Tofino2 allows ports 0-7.  This example will use
    either port 68 or port 6 in a pipe depending on chip type.
    """
    # if g_is_tofino:
    pipe_local_port = 68
    # if g_is_tofino2:
    #     pipe_local_port = 6
    return make_port(pipe_id, pipe_local_port)

########## Basic Initialization ############
class BaseProgramTest(BfRuntimeTest):
    
    #
    # This is a special class that will provide us with the interface to
    # the fixed APIs via the corresponding Thrift bindings. This particular
    # class provides access to all fixed APIs, but the cleanup routine is
    # implemented for multicast objects only
    #
    class FixedAPIs(pd_base_tests.ThriftInterfaceDataPlane):
        def __init__(self, p4names):
            pd_base_tests.ThriftInterfaceDataPlane.__init__(self, p4names)
            
        def setUp(self):
            pd_base_tests.ThriftInterfaceDataPlane.setUp(self)
            self.dev = 0
            self.mc_sess  = self.mc.mc_create_session()
            print("Opened MC Session {:#08x}".format(self.mc_sess))
            
        def cleanUp(self):
            try:
                print("  Deleting Multicast Groups")
                mgrp_count = self.mc.mc_mgrp_get_count(self.mc_sess, self.dev)
                if mgrp_count > 0:
                    mgrp_list  = [self.mc.mc_mgrp_get_first(self.mc_sess, self.dev)]
                    if mgrp_count > 1:
                        mgrp_list.extend(self.mc.mc_mgrp_get_next_i(
                            self.mc_sess, self.dev,
                            mgrp_list[-1], mgrp_count - 1))
                    
                    for mgrp in mgrp_list:
                        self.mc.mc_mgrp_destroy(self.mc_sess, self.dev, mgrp)
                    
                print("  Deleting Multicast ECMP Entries")
                ecmp_count = self.mc.mc_ecmp_get_count(self.mc_sess, self.dev)
                if ecmp_count > 0:
                    ecmp_list  = [self.mc.mc_ecmp_get_first(self.mc_sess, self.dev)]
                    if ecmp_count > 1:
                        ecmp_list.extend(self.mc.mc_ecmp_get_next_i(
                            self.mc_sess, self.dev,
                            ecmp_list[-1], ecmp_count - 1))
                    
                    for ecmp in ecmp_list:
                        self.mc.mc_ecmp_destroy(self.mc_sess, self.dev, ecmp)

                print("  Deleting Multicast Nodes")
                node_count = self.mc.mc_node_get_count(self.mc_sess, self.dev)
                if node_count > 0:
                    node_list  = [self.mc.mc_node_get_first(self.mc_sess, self.dev)]
                    if node_count > 1:
                        node_list.extend(self.mc.mc_node_get_next_i(
                            self.mc_sess, self.dev,
                            node_list[-1], node_count - 1))
                    
                    for node in node_list:
                        self.mc.mc_node_destroy(self.mc_sess, self.dev, node)
                    
                print("  Clearing Multicast LAGs")
                for lag in range(0, 255):
                    self.mc.mc_set_lag_membership(
                        self.mc_sess, self.dev,
                        hex_to_byte(lag), devports_to_mcbitmap([]))

                print("  Clearing Port Pruning Table")
                for yid in range(0, 288):
                    self.mc.mc_update_port_prune_table(
                        self.mc_sess, self.dev,
                        hex_to_i16(yid), devports_to_mcbitmap([]));

            except:
                print("  Error while cleaning up multicast objects. ")
                print("  You might need to restart the driver")
            finally:
                self.mc.mc_complete_operations(self.mc_sess)

        def runTest(self):
            pass
        
        def tearDown(self):
            self.mc.mc_destroy_session(self.mc_sess)
            print("  Closed MC Session %#08x" % self.mc_sess)
            pd_base_tests.ThriftInterfaceDataPlane.tearDown(self)
            
    # The setUp() method is used to prepare the test fixture. Typically
    # you would use it to establich connection to the gRPC Server
    #
    # You can also put the initial device configuration there. However,
    # if during this process an error is encountered, it will be considered
    # as a test error (meaning the test is incorrect),
    # rather than a test failure
    #
    # Here is the stuff we set up that is ready to use
    #  client_id
    #  p4_name
    #  bfrt_info
    #  dev
    #  dev_tgt
    #  allports
    #  tables    -- the list of tables
    #     Individual tables of the program with short names
    #     ipv4_host
    #     ipv4_lpm
    def register_entry_add(self, reg_name, index, value):
        """Reads one register entry"""

        table = self.bfrt_info.table_get(reg_name)

        _keys = table.make_key([gc.KeyTuple("$REGISTER_INDEX", index)])
        # data_name = table.info.data_dict_allname["f1"]
        _data = table.make_data([gc.DataTuple(reg_name + ".f1", value)])
        table.entry_add(self.dev_tgt, [_keys], [_data])

    def register_entry_range_add(self, reg_name, list_index, list_value):
        """Reads one register entry"""
        keys=[]
        vals=[]
        table = self.bfrt_info.table_get(reg_name)
        # for idx in list_index:
        #     _keys = table.make_key([gc.KeyTuple("$REGISTER_INDEX", idx)])
        #     keys.append(_keys)
        # for val in list_value:
        #     _data = table.make_data([gc.DataTuple(reg_name + ".f1", val)])
        #     vals.append(_data)
        for idx in range(1, len(list_index)):
            _keys = table.make_key([gc.KeyTuple("$REGISTER_INDEX", list_index[idx])])
            keys.append(_keys)
            _data = table.make_data([gc.DataTuple(reg_name + ".f1", list_value[idx])])
            vals.append(_data)

        table.entry_add(self.dev_tgt, keys, vals)


    def PortStatTest(self, ports, loopback, loopback_1, loopback_2):
#init port
        self.port_table = self.bfrt_info.table_get("$PORT")
        for port in ports:
            self.port_table.entry_add(
                self.dev_tgt,
                [self.port_table.make_key([gc.KeyTuple('$DEV_PORT', port)])],
                [self.port_table.make_data([gc.DataTuple('$SPEED', str_val="BF_SPEED_100G"),
                                            gc.DataTuple('$FEC', str_val="BF_FEC_TYP_RS"),
                                            # gc.DataTuple('$AUTO_NEGOTIATION',str_val="PM_AN_DEFAULT"),
                                            gc.DataTuple('$PORT_ENABLE', bool_val=True)])])
            print("Binding add port {} 100G ENABLE".format(port))
            
        
        self.port_table.entry_mod(
            self.dev_tgt,
            [self.port_table.make_key([gc.KeyTuple('$DEV_PORT', loopback)])],
            [self.port_table.make_data([gc.DataTuple('$PORT_ENABLE', bool_val=True),
                                        gc.DataTuple('$LOOPBACK_MODE', str_val="BF_LPBK_MAC_NEAR")])])

        self.port_table.entry_mod(
            self.dev_tgt,
            [self.port_table.make_key([gc.KeyTuple('$DEV_PORT', loopback_1)])],
            [self.port_table.make_data([gc.DataTuple('$PORT_ENABLE', bool_val=True),
                                        gc.DataTuple('$LOOPBACK_MODE', str_val="BF_LPBK_MAC_NEAR")])])
        
        self.port_table.entry_mod(
            self.dev_tgt,
            [self.port_table.make_key([gc.KeyTuple('$DEV_PORT', loopback_2)])],
            [self.port_table.make_data([gc.DataTuple('$PORT_ENABLE', bool_val=True),
                                        gc.DataTuple('$LOOPBACK_MODE', str_val="BF_LPBK_MAC_NEAR")])])
        print("modify port {} LOOPBACK ENABLE".format(loopback))

    def setUp(self):
        self.checksum = test_param_get("checksum", False)        
        self.client_id = 0
        self.dev      = 0
        self.dev_tgt  = gc.Target(self.dev, pipe_id=0xFFFF)

        # self.p4_name = "pktgen_test"
        self.p4_name = "optkv"

        ###### FIXED API SETUP #########
        self.fixed = self.FixedAPIs([self.p4_name])
        self.fixed.setUp()
            
        print("\n")
        print("Test Setup")
        print("==========")
        ENABLE_TABLE_SETUP = 1
        
        
        loopback_2=56
        BfRuntimeTest.setUp(self, self.client_id, self.p4_name)
        self.bfrt_info = self.interface.bfrt_info_get(self.p4_name)
        # self.bfrt_info = self.interface.bfrt_info_get("optkv")

        self.PortStatTest(ports=[8,4,28,20,12,36,40,32,56,0], loopback=40, loopback_1=32, loopback_2=56)
        
        print("    Connected to Device: {}, Program: {}, ClientId: {}".format(
            self.dev, self.p4_name, self.client_id))
    
        # Since this class is not a test per se, we can use the setup method
        # for common setup. For example, we can have our tables and annotations
        # ready
        # self.register_entry_add('Ingress.write_process.Bitmap_R', 10, 1)
        # k = [i for i in range(1, 512)]
        # v = [i for i in range(1, 512)]
        # start_time = time.time()
        # self.register_entry_range_add('Ingress.write_process.Bitmap_R', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data0.register0', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data1.register1', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data2.register2', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data3.register3', k, v)

        # self.register_entry_range_add('Ingress.write_process.process_data4.register4', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data5.register5', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data6.register6', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data7.register7', k, v)

        # self.register_entry_range_add('Ingress.write_process.process_data8.register8', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data9.register9', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data10.register10', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data11.register11', k, v)
        
        # self.register_entry_range_add('Ingress.write_process.process_data12.register12', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data13.register13', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data14.register14', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data15.register15', k, v)

        # self.register_entry_range_add('Ingress.write_process.process_data16.register16', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data17.register17', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data18.register18', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data19.register19', k, v)

        # self.register_entry_range_add('Ingress.write_process.process_data20.register20', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data21.register21', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data22.register22', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data23.register23', k, v)

        # self.register_entry_range_add('Ingress.write_process.process_data24.register24', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data25.register25', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data26.register26', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data27.register27', k, v)

        # self.register_entry_range_add('Ingress.write_process.process_data28.register28', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data29.register29', k, v)
        # self.register_entry_range_add('Ingress.write_process.process_data30.register30', k, v)
        
        # end_time = time.time()
        # cost = end_time - start_time
        # print(cost)
        if ENABLE_TABLE_SETUP:
            # self.ipv4_host = self.bfrt_info.table_get("Ingress.ipv4_host")
            # self.ipv4_ack_bitmap = self.bfrt_info.table_get("Ingress.ipv4_ack_bitmap")
            # self.ipv4_index_0_nl = self.bfrt_info.table_get("Ingress.ipv4_index_0_nl")
            # self.ipv4_index_0 = self.bfrt_info.table_get("Ingress.ipv4_index_0")
            # self.process_data0 = self.bfrt_info.table_get("Ingress.write_process.process_data0.processEntry0")
            # self.process_data1 = self.bfrt_info.table_get("Ingress.write_process.process_data1.processEntry1")
            # self.process_data2 = self.bfrt_info.table_get("Ingress.write_process.process_data2.processEntry2")
            # self.process_data3 = self.bfrt_info.table_get("Ingress.write_process.process_data3.processEntry3")
            
            
            self.ipv4_timer_mc = self.bfrt_info.table_get("Ingress.ipv4_timer_mc")
            # self.ipv4_timer_resubmit = self.bfrt_info.table_get("Ingress.ipv4_timer_resubmit")
            # self.tables = [ self.ipv4_host, self.ipv4_timer_mc]



            # setup table MAU 
            is_timer_0    = test_param_get("opcode", 0)
            is_timer_1    = test_param_get("opcode", 1)

            version       = test_param_get("version", 1)
            version_mask  = test_param_get("version_mask", 0xffff)
            version_1     = test_param_get("version", 2)

            ipv4_dst      = test_param_get("ipv4_dst", "13.7.56.127")
            opcode_0      = test_param_get("opcode", 0)
            opcode_1      = test_param_get("opcode", 1)
            
            flag_0   = test_param_get("flags", 0)
            flag_1 = test_param_get("flags", 1)
            flag_2 = test_param_get("flags", 2)
            flag_8 = test_param_get("flags", 8) # collision
            
            log_index_0     = test_param_get("log_index", 0)
            log_index_1     = test_param_get("log_index", 1)
            log_index_2     = test_param_get("log_index", 2)
            log_index_3     = test_param_get("log_index", 3)
            log_index_4     = test_param_get("log_index", 4)
            log_index_5     = test_param_get("log_index", 5)
            log_index_6     = test_param_get("log_index", 6)
            log_index_7     = test_param_get("log_index", 7)

            
            ack_0         = test_param_get("ack", 0)
            ack_1         = test_param_get("ack", 1)

            port_0        = test_param_get("port", 28) # client
            port_1        = test_param_get("port", 20) # server1
            port_2        = test_param_get("port", 12) # server2
            port_3        = test_param_get("port", 36)  # server3
            port_4        = test_param_get("port", 32)  # server3
            port_5        = test_param_get("port", 56)  # server3

            ingress_port  = test_param_get("ingress_port", 0)
            mgrp_id_0     = test_param_get("mgrp_id_0",  99)
            mgrp_id_1     = test_param_get("mgrp_id_1",  98)
            
            # read multicast
            mod0_rid      = test_param_get("mod0_rid", 1)
            mod0_ports    = test_param_get("mod0_ports", [20, 12, 0]) # server 1/2/3
            # write multicast 
            mod1_rid      = test_param_get("mod1_rid", 2)
            mod1_ports    = test_param_get("mod1_ports", [4, 8, 28, 20, 12, 0]) # client 1 server1/2/3

            mgrp_id_6     = test_param_get("mgrp_id_6",  93)
            mgrp_id_7     = test_param_get("mgrp_id_7",  92)
            mgrp_id_8     = test_param_get("mgrp_id_8",  91)
            # 011
            mod6_rid      = test_param_get("mod6_rid", 6)
            mod6_ports    = test_param_get("mod6_ports", [20, 12]) # client 1 server1/2/3
            # 110
            mod7_rid      = test_param_get("mod7_rid", 7)
            mod7_ports    = test_param_get("mod7_ports", [20, 0]) # client 1 server1/2/3
            # 101
            mod8_rid      = test_param_get("mod8_rid", 8)
            mod8_ports    = test_param_get("mod8_ports", [12, 0]) # client 1 server1/2/3
            
            # isTimeOut     = test_param_get("isTimeOut", 0)
            isTimeOut_1   = test_param_get("isTimeOut", 1)

            out_bitmap    = test_param_get("hdr.pktgen.out_bitmap", 0) # all is timeout
            out_bitmap_1  = test_param_get("hdr.pktgen.out_bitmap", 1)
            out_bitmap_2  = test_param_get("hdr.pktgen.out_bitmap", 2)
            out_bitmap_3  = test_param_get("hdr.pktgen.out_bitmap", 3)
            out_bitmap_4  = test_param_get("hdr.pktgen.out_bitmap", 4)
            out_bitmap_5  = test_param_get("hdr.pktgen.out_bitmap", 5)
            out_bitmap_6  = test_param_get("hdr.pktgen.out_bitmap", 6)
            # out_bitmap_7  = test_param_get("hdr.pktgen.out_bitmap", 7)

            # AckNum = test_param_get("meta.AckNum", 3) # number of server is 3
            AckNum_1 = test_param_get("meta.AckNum", 1) # number of server is 2 -> 20 12
            AckNum_2 = test_param_get("meta.AckNum", 2) # number of server is 2 -> 20 12

            need_reset_0 = test_param_get("need_reset", 0)
            need_reset_1 = test_param_get("need_reset", 1)
            port_reseb   = test_param_get("port", 40)  # server3


            mgrp_id_2     = test_param_get("mgrp_id",  97)
            mgrp_id_3     = test_param_get("mgrp_id",  96)
            mgrp_id_4     = test_param_get("mgrp_id",  95)
            mgrp_id_5     = test_param_get("mgrp_id",  94)
            
            # timeout multicast -># 1
            # 20->w1 | 12->w2 | 4->w3
            mod2_rid      = test_param_get("mod0_rid", 3) # 001 -> w2 w3
            mod2_ports    = test_param_get("mod0_ports", [12, 4])
            # timeout multicast -># 2
            mod3_rid      = test_param_get("mod1_rid", 4) # 010 -> w1 w3
            mod3_ports    = test_param_get("mod1_ports", [20, 4])
            # timeout multicast -># 4
            mod4_rid      = test_param_get("mod1_rid", 5) # 100 -> w1 w2
            mod4_ports    = test_param_get("mod1_ports", [20, 12])

            # multicast include first loopback-># 4
            mod5_rid      = test_param_get("mod1_rid", 6) # first loopback 32
            mod5_ports    = test_param_get("mod1_ports", [28, 20, 12, 32])


            print("\n")
            print("Test Run")
            print("========")

            # Create the multicast group
            print("  Dev {} {}".format(self.dev, mgrp_id_0))
            mgrp0 = self.fixed.mc.mc_mgrp_create(self.fixed.mc_sess, self.dev, mgrp_id_0)
            print("  Created Multicast Group {}".format(mgrp_id_0))
            
            self.fixed.mc.mc_associate_node(
                self.fixed.mc_sess, self.dev,
                mgrp0,
                self.fixed.mc.mc_node_create(
                    self.fixed.mc_sess, self.dev,
                    mod0_rid,
                    devports_to_mcbitmap(mod0_ports),
                    lags_to_mcbitmap([])),
                xid=0, xid_valid=False)
            print("  Added the node with rid={} and ports {}".format(
                mod0_rid, mod0_ports))
            

            print("  Dev {} {}".format(self.dev, mgrp_id_1))
            mgrp1 = self.fixed.mc.mc_mgrp_create(self.fixed.mc_sess, self.dev, mgrp_id_1)
            print("  Created Multicast Group {}".format(mgrp_id_1))
            self.fixed.mc.mc_associate_node(
                self.fixed.mc_sess, self.dev,
                mgrp1,
                self.fixed.mc.mc_node_create(
                    self.fixed.mc_sess, self.dev,
                    mod1_rid,
                    devports_to_mcbitmap(mod1_ports),
                    lags_to_mcbitmap([])),
                xid=0, xid_valid=False)
            print("  Added the node with rid={} and ports {}".format(
                mod1_rid, mod1_ports))
            

            print("  Dev {} {}".format(self.dev, mgrp_id_2))
            mgrp2 = self.fixed.mc.mc_mgrp_create(self.fixed.mc_sess, self.dev, mgrp_id_2)
            self.fixed.mc.mc_associate_node(
                self.fixed.mc_sess, self.dev,
                mgrp2,
                self.fixed.mc.mc_node_create(
                    self.fixed.mc_sess, self.dev,
                    mod2_rid,
                    devports_to_mcbitmap(mod2_ports),
                    lags_to_mcbitmap([])),
                xid=0, xid_valid=False)
            print("  Added the node with rid={} and ports {}".format(
                mod2_rid, mod2_ports))
            
            print("  Dev {} {}".format(self.dev, mgrp_id_3))
            mgrp3 = self.fixed.mc.mc_mgrp_create(self.fixed.mc_sess, self.dev, mgrp_id_3)
            self.fixed.mc.mc_associate_node(
                self.fixed.mc_sess, self.dev,
                mgrp3,
                self.fixed.mc.mc_node_create(
                    self.fixed.mc_sess, self.dev,
                    mod3_rid,
                    devports_to_mcbitmap(mod3_ports),
                    lags_to_mcbitmap([])),
                xid=0, xid_valid=False)
            print("  Added the node with rid={} and ports {}".format(
                mod3_rid, mod3_ports))
            
            print("  Dev {} {}".format(self.dev, mgrp_id_4))
            mgrp4 = self.fixed.mc.mc_mgrp_create(self.fixed.mc_sess, self.dev, mgrp_id_4)
            self.fixed.mc.mc_associate_node(
                self.fixed.mc_sess, self.dev,
                mgrp4,
                self.fixed.mc.mc_node_create(
                    self.fixed.mc_sess, self.dev,
                    mod4_rid,
                    devports_to_mcbitmap(mod4_ports),
                    lags_to_mcbitmap([])),
                xid=0, xid_valid=False)
            print("  Added the node with rid={} and ports {}".format(
                mod4_rid, mod4_ports))

            print("  Dev {} {}".format(self.dev, mgrp_id_5))
            mgrp5 = self.fixed.mc.mc_mgrp_create(self.fixed.mc_sess, self.dev, mgrp_id_5)
            self.fixed.mc.mc_associate_node(
                self.fixed.mc_sess, self.dev,
                mgrp5,
                self.fixed.mc.mc_node_create(
                    self.fixed.mc_sess, self.dev,
                    mod5_rid,
                    devports_to_mcbitmap(mod5_ports),
                    lags_to_mcbitmap([])),
                xid=0, xid_valid=False)
            print("  Added the node with rid={} and ports {}".format(
                mod4_rid, mod5_ports))
            
            print("  Dev {} {}".format(self.dev, mgrp_id_6))
            mgrp6 = self.fixed.mc.mc_mgrp_create(self.fixed.mc_sess, self.dev, mgrp_id_6)
            self.fixed.mc.mc_associate_node(
                self.fixed.mc_sess, self.dev,
                mgrp6,
                self.fixed.mc.mc_node_create(
                    self.fixed.mc_sess, self.dev,
                    mod6_rid,
                    devports_to_mcbitmap(mod6_ports),
                    lags_to_mcbitmap([])),
                xid=0, xid_valid=False)
            print("  Added the node with rid={} and ports {}".format(
                mod6_rid, mod6_ports))
            
            print("  Dev {} {}".format(self.dev, mgrp_id_7))
            mgrp7 = self.fixed.mc.mc_mgrp_create(self.fixed.mc_sess, self.dev, mgrp_id_7)
            self.fixed.mc.mc_associate_node(
                self.fixed.mc_sess, self.dev,
                mgrp7,
                self.fixed.mc.mc_node_create(
                    self.fixed.mc_sess, self.dev,
                    mod7_rid,
                    devports_to_mcbitmap(mod7_ports),
                    lags_to_mcbitmap([])),
                xid=0, xid_valid=False)
            print("  Added the node with rid={} and ports {}".format(
                mod7_rid, mod7_ports))
            
            print("  Dev {} {}".format(self.dev, mgrp_id_8))
            mgrp8 = self.fixed.mc.mc_mgrp_create(self.fixed.mc_sess, self.dev, mgrp_id_8)
            self.fixed.mc.mc_associate_node(
                self.fixed.mc_sess, self.dev,
                mgrp8,
                self.fixed.mc.mc_node_create(
                    self.fixed.mc_sess, self.dev,
                    mod8_rid,
                    devports_to_mcbitmap(mod8_ports),
                    lags_to_mcbitmap([])),
                xid=0, xid_valid=False)
            print("  Added the node with rid={} and ports {}".format(
                mod8_rid, mod8_ports))
            
            self.fixed.mc.mc_complete_operations(self.fixed.mc_sess)
            #####################################################
            # for resubmit bitmap = 0x3
            # key_list = [
            #     self.ipv4_ack_bitmap.make_key([
            #         # gc.KeyTuple('hdr.kv.opcode', opcode_1),
            #         gc.KeyTuple('hdr.kv.ack', ack_1),
            #         ])
            #     ]
            # data_list = [    
            #         self.ipv4_ack_bitmap.make_data([
            #         gc.DataTuple('port', loopback_2)], "Ingress.send")
            #     ]
            # self.ipv4_ack_bitmap.entry_add(self.dev_tgt, key_list, data_list)
            # print("  Added an entry to ipv4_ack_bitmap: {} --> send({})".format(
            #     opcode_1, loopback_2))
            #####################################################
            # for read req
            # key_list = [
            #     self.ipv4_host.make_key([
            #         gc.KeyTuple('hdr.kv.opcode', opcode_0),
            #         gc.KeyTuple('hdr.kv.ack', ack_0),
            #         gc.KeyTuple('hdr.kv.flags', flag_0),
            #         # gc.KeyTuple('hdr.kv.log_index', log_index_0)
            #         ])
            #     ]
            # data_list = [
            #     self.ipv4_host.make_data([
            #         gc.DataTuple('mcast_grp', mgrp_id_0)], "Ingress.multicast")
            #     ]
            # self.ipv4_host.entry_add(self.dev_tgt, key_list, data_list)
            # print("  Added an entry to ipv4_host: {} ack{} --> multicast({})".format(
            #     opcode_0, ack_0, mgrp_id_0))            

            # key_list_1 = [
            #     self.ipv4_host.make_key([
            #         gc.KeyTuple('hdr.kv.opcode', opcode_1),
            #         gc.KeyTuple('hdr.kv.ack', ack_0),
            #         gc.KeyTuple('hdr.kv.flags', flag_0),
            #         # gc.KeyTuple('hdr.kv.log_index', log_index_1)
            #         ])
            #     ]
            # data_list_1 = [
            #     self.ipv4_host.make_data([
            #         gc.DataTuple('mcast_grp', mgrp_id_1)], "Ingress.multicast")
            #     ]
            # self.ipv4_host.entry_add(self.dev_tgt, key_list_1, data_list_1)
            # print("  Added an entry to ipv4_host: {} --> multicast({})".format(
            #     opcode_1, mgrp_id_1))

            # key_list_1 = [
            #     self.ipv4_host.make_key([
            #         gc.KeyTuple('hdr.kv.opcode', opcode_1),
            #         gc.KeyTuple('hdr.kv.ack', ack_0),
            #         gc.KeyTuple('hdr.kv.flags', flag_8),
            #         # gc.KeyTuple('hdr.kv.log_index', log_index_1)
            #         ])
            #     ]
            # data_list_1 = [
            #     self.ipv4_host.make_data([
            #         gc.DataTuple('port', port_0)], "Ingress.send")
            #     ]
            # self.ipv4_host.entry_add(self.dev_tgt, key_list_1, data_list_1)
            # print("  Added an entry to ipv4_host: {} flag_8 {}--> port_0({})".format(
            #     opcode_1, flag_8, port_0))

            # write ACK from server will be dropped
            # key_list_2 = [
            #     self.ipv4_host.make_key([
            #         gc.KeyTuple('hdr.kv.opcode', opcode_1),
            #         gc.KeyTuple('hdr.kv.ack', ack_1),
            #         gc.KeyTuple('hdr.kv.flags', flag_0) # flag_0 = 0
            #         ])
            #     ]
            # data_list_2 = [
            #     self.ipv4_host.make_data([
            #         gc.DataTuple('port', port_0)], "Ingress.send")
            #     ]
            # self.ipv4_host.entry_add(self.dev_tgt, key_list_2, data_list_2)
            # print("  Added an entry to ipv4_host: {} --> send({})".format(
            #     opcode_1, port_0))


            # key_list_3 = [
            #     self.ipv4_host.make_key([
            #         gc.KeyTuple('hdr.kv.opcode', opcode_0),
            #         gc.KeyTuple('hdr.kv.ack', ack_1),
            #         gc.KeyTuple('hdr.kv.flags', flag_0), # flag_0 = 0
            #         # gc.KeyTuple('hdr.kv.log_index', log_index_0)
            #         ])
            #     ]
            # data_list_3 = [
            #     self.ipv4_host.make_data([
            #         gc.DataTuple('port', port_0)], "Ingress.send")
            #     ]
            # self.ipv4_host.entry_add(self.dev_tgt, key_list_3, data_list_3)
            # print("  Added an entry to ipv4_host: {} ack_1 {} flag_0{} --> send({})".format(
            #     opcode_0, ack_1, flag_0, port_0))
            
            #################################### index ##########################
            # key_list_4 = [
            #     self.ipv4_index_0_nl.make_key([
            #         gc.KeyTuple('hdr.kv.log_index', 1),
            #         # gc.KeyTuple('hdr.kv.log_index', log_index_0)
            #         ])
            #     ]
            # data_list_4 = [
            #     self.ipv4_index_0_nl.make_data([
            #         gc.DataTuple('mcast_grp', mgrp_id_5)], "Ingress.multicast")
            #     ]
            # self.ipv4_index_0_nl.entry_add(self.dev_tgt, key_list_4, data_list_4)
            # print("  Added an entry log_index to ipv4_index_0: {} --> mgrp_id_5({})".format(
            #     1, mgrp_id_5))
            
            # key_list_4 = [
            #     self.ipv4_index_0_nl.make_key([
            #         gc.KeyTuple('hdr.kv.log_index', 2),
            #         # gc.KeyTuple('hdr.kv.log_index', log_index_0)
            #         ])
            #     ]
            # data_list_4 = [
            #     self.ipv4_index_0_nl.make_data([
            #         gc.DataTuple('port', port_4)], "Ingress.send")
            #     ]
            # self.ipv4_index_0_nl.entry_add(self.dev_tgt, key_list_4, data_list_4)
            # print("  Added an entry log_index to ipv4_index_0: {} --> port_4({})".format(
            #     2, port_4))
            
            # key_list_4 = [
            #     self.ipv4_index_0_nl.make_key([
            #         gc.KeyTuple('hdr.kv.log_index', 3),
            #         # gc.KeyTuple('hdr.kv.log_index', log_index_0)
            #         ])
            #     ]
            # data_list_4 = [
            #     self.ipv4_index_0_nl.make_data([
            #         gc.DataTuple('port', port_5)], "Ingress.send")
            #     ]
            # self.ipv4_index_0_nl.entry_add(self.dev_tgt, key_list_4, data_list_4)
            # print("  Added an entry log_index to ipv4_index_0: {} --> port_4({})".format(
            #     3, port_5))


            # key_list_3 = [
            #     self.ipv4_index_0.make_key([
            #         gc.KeyTuple('hdr.kv.log_index', 2),
            #         ])
            #     ]
            # data_list_3 = [
            #     self.ipv4_index_0.make_data([
            #         gc.DataTuple('port', port_3)], "Ingress.send")
            #     ]
            # self.ipv4_index_0.entry_add(self.dev_tgt, key_list_3, data_list_3)
            # print("  Added an entry to ipv4_index_0: {} --> send({})".format(
            #     2, port_3))

            # key_list_4 = [
            #     self.ipv4_index_0.make_key([
            #         gc.KeyTuple('hdr.kv.log_index', 1)
            #         ])
            #     ]
            # data_list_4 = [
            #     self.ipv4_index_0.make_data([
            #         gc.DataTuple('mcast_grp', mgrp_id_1)], "Ingress.multicast")
            #     ]
            # self.ipv4_index_0.entry_add(self.dev_tgt, key_list_4, data_list_4)
            # print("  Added an entry log_index to ipv4_index_0: {} --> mgrp_id_1({})".format(
            #     1, mgrp_id_1))


            #################################### timer ##########################

            # for timer 1
            key_list_5 = [
                self.ipv4_timer_mc.make_key([
                    gc.KeyTuple('meta.isTimeOut', isTimeOut_1),
                    gc.KeyTuple('hdr.pktgen.out_bitmap', out_bitmap_1),
                    gc.KeyTuple('meta.AckNum', AckNum_1)
                    ])
                ]
            data_list_5 = [
                self.ipv4_timer_mc.make_data([
                    gc.DataTuple('mcast_grp', mgrp_id_2)], "Ingress.timer_multicast")
                ]
            self.ipv4_timer_mc.entry_add(self.dev_tgt, key_list_5, data_list_5)
            print("  Added an entry to ipv4_timer: {} --> timer_multicast({})".format(
                out_bitmap_1, mgrp_id_2))


            # for timer 2
            key_list_6 = [
                self.ipv4_timer_mc.make_key([
                    gc.KeyTuple('meta.isTimeOut', isTimeOut_1),
                    gc.KeyTuple('hdr.pktgen.out_bitmap', out_bitmap_2),
                    gc.KeyTuple('meta.AckNum', AckNum_1)
                    ])
                ]
            data_list_6 = [
                self.ipv4_timer_mc.make_data([
                    gc.DataTuple('mcast_grp', mgrp_id_3)], "Ingress.timer_multicast")
                ]
            self.ipv4_timer_mc.entry_add(self.dev_tgt, key_list_6, data_list_6)
            print("  Added an entry to ipv4_timer: {} --> timer_multicast({})".format(
                out_bitmap_2, mgrp_id_3))
            
            # for timer 3
            key_list_7 = [
                self.ipv4_timer_mc.make_key([
                    gc.KeyTuple('meta.isTimeOut', isTimeOut_1),
                    gc.KeyTuple('hdr.pktgen.out_bitmap', out_bitmap_3),
                    gc.KeyTuple('meta.AckNum', AckNum_2)
                    ])
                ]
            data_list_7 = [
                self.ipv4_timer_mc.make_data([
                    gc.DataTuple('port', port_3)], "Ingress.send")
                ]
            self.ipv4_timer_mc.entry_add(self.dev_tgt, key_list_7, data_list_7)
            print("  Added an entry to ipv4_timer: {} --> timer_multicast({})".format(
                out_bitmap_3, port_3))
            
            # for timer 4
            key_list_9 = [
                self.ipv4_timer_mc.make_key([
                    gc.KeyTuple('meta.isTimeOut', isTimeOut_1),
                    gc.KeyTuple('hdr.pktgen.out_bitmap', out_bitmap_4),
                    gc.KeyTuple('meta.AckNum', AckNum_1)
                    ])
                ]
            data_list_9 = [
                self.ipv4_timer_mc.make_data([
                    gc.DataTuple('mcast_grp', mgrp_id_4)], "Ingress.timer_multicast")
                ]
            self.ipv4_timer_mc.entry_add(self.dev_tgt, key_list_9, data_list_9)
            print("  Added an entry to ipv4_timer: {} --> send({})".format(
                out_bitmap_4, mgrp_id_4))

            # for timer 5
            key_list_10 = [
                self.ipv4_timer_mc.make_key([
                    gc.KeyTuple('meta.isTimeOut', isTimeOut_1),
                    gc.KeyTuple('hdr.pktgen.out_bitmap', out_bitmap_5),
                    gc.KeyTuple('meta.AckNum', AckNum_2)
                    ])
                ]
            data_list_10 = [
                self.ipv4_timer_mc.make_data([
                    gc.DataTuple('port', port_2)], "Ingress.send")
                ]
            self.ipv4_timer_mc.entry_add(self.dev_tgt, key_list_10, data_list_10)
            print("  Added an entry to ipv4_timer: {} --> send({})".format(
                out_bitmap_5, port_2))
            

            # for timer 6
            key_list_5 = [
                self.ipv4_timer_mc.make_key([
                    gc.KeyTuple('meta.isTimeOut', isTimeOut_1),
                    gc.KeyTuple('hdr.pktgen.out_bitmap', out_bitmap_6),
                    gc.KeyTuple('meta.AckNum', AckNum_2)
                    ])
                ]
            data_list_5 = [
                self.ipv4_timer_mc.make_data([
                    gc.DataTuple('port', port_1)], "Ingress.send")
                ]
            self.ipv4_timer_mc.entry_add(self.dev_tgt, key_list_5, data_list_5)
            print("  Added an entry to ipv4_timer: {} --> timer_multicast({})".format(
                out_bitmap_6, port_1))


            
            # key_list_12 = [
            #     self.process_data0.make_key([
            #         # gc.KeyTuple('is_timer', is_timer_0),
            #         # gc.KeyTuple('need_reset', need_reset_0),
            #         gc.KeyTuple('opcode', opcode_1),
            #         gc.KeyTuple('ack', ack_0)
            #         # gc.KeyTuple('version', version, 0xffff),
            #         # gc.KeyTuple('$MATCH_PRIORITY', 0) # priority req for ternary
            #         ])
            #     ]
            # data_list_12 = [
            #     self.process_data0.make_data([],'Ingress.write_process.process_data0.write_entry0')
            #     ]
            # self.process_data0.entry_add(self.dev_tgt, key_list_12, data_list_12)
            # print("  Added an entry to process_data0: {} {} --> write()".format(
            #     opcode_1, ack_0))

            
            # key_list_12 = [
            #     self.process_data0.make_key([
            #         gc.KeyTuple('is_timer', is_timer_1),
            #         gc.KeyTuple('need_reset', need_reset_0),
            #         gc.KeyTuple('opcode', opcode_0),
            #         gc.KeyTuple('ack', ack_0),
            #         # gc.KeyTuple('version', version, 0xffff),
            #         # gc.KeyTuple('$MATCH_PRIORITY', 0) # priority req for ternary
            #         ])
            #     ]
            # data_list_12 = [
            #     self.process_data0.make_data([],'Ingress.write_process.process_data0.read_entry0')
            #     ]
            # self.process_data0.entry_add(self.dev_tgt, key_list_12, data_list_12)
            # print("  Added an entry to process_data0: {} --> ()".format(
            #     opcode_0))


        
        # Optional, but highly recommended
        # self.cleanUp()

    # Use Cleanup Method to clear the tables before and after the test starts
    # (the latter is done as a part of tearDown()
    def cleanUp(self):
        print("\n")
        print("Table Cleanup:")
        print("==============")

        try:
            for t in self.tables:
                print("  Clearing Table {}".format(t.info.name_get()))
                keys = []
                for (d, k) in t.entry_get(self.dev_tgt):
                    if k is not None:
                        keys.append(k)
                t.entry_del(self.dev_tgt, keys)
                # Not all tables support default entry
                try:
                    t.default_entry_reset(self.dev_tgt)
                except:
                    pass
        except Exception as e:
            print("Error cleaning up: {}".format(e))

        # CleanUp Fixed API objects
        self.fixed.cleanUp()

    # Use tearDown() method to return the DUT to the initial state by cleaning
    # all the configuration and clearing up the connection
    def tearDown(self):
        pass
        # print("\n")
        # print("Test TearDown:")
        # print("==============")

        # self.cleanUp()
        
        # # Call the Parent tearDown
        # BfRuntimeTest.tearDown(self)
        # self.fixed.tearDown()

#
# Helper functions for Tofino. We could've put them into a separate file, but
# that would complicate shipping. If their number grows, we'll do something
#
def to_devport(pipe, port):
    """
    Convert a (pipe, port) combination into a 9-bit (devport) number
    NOTE: For now this is a Tofino-specific method
    """
    return pipe << 7 | port

def to_pipeport(dp):
    """
    Convert a physical 9-bit (devport) number into (pipe, port) pair
    NOTE: For now this is a Tofino-specific method
    """
    return (dp >> 7, dp & 0x7F)

def devport_to_mcport(dp):
    """
    Convert a physical 9-bit (devport) number to the index that is used by 
    MC APIs (for bitmaps mostly)
    NOTE: For now this is a Tofino-specific method
    """
    (pipe, port) = to_pipeport(dp)
    return pipe * 72 + port

def mcport_to_devport(mcport):
    """
    Convert a MC port index (mcport) to devport
    NOTE: For now this is a Tofino-specific method
    """
    return to_devport(mcport / 72, mcport % 72)

def devports_to_mcbitmap(devport_list):
    """
    Convert a list of devports into a Tofino-specific MC bitmap
    """
    bit_map = [0] * ((288 + 7) / 8)
    for dp in devport_list:
        mc_port = devport_to_mcport(dp)
        bit_map[mc_port / 8] |= (1 << (mc_port % 8))
    return bytes_to_string(bit_map)

def mcbitmap_to_devports(mc_bitmap):
    """
    Convert a MC bitmap of mcports to a list of devports
    """
    bit_map = string_to_bytes(mc_bitmap)
    devport_list = []
    for i in range(0, len(bit_map)):
        for j in range(0, 8):
            if bit_map[i] & (1 << j) != 0:
                devport_list.append(mcport_to_devport(i * 8 + j))
    return devport_list

def lags_to_mcbitmap(lag_list):
    """
    Convert a list of LAG indices to a MC bitmap
    """
    bit_map = [0] * ((256 + 7) / 8)
    for lag in lag_list:
        bit_map[lag / 8] |= (1 << (lag % 8))
    return bytes_to_string(bit_map)
    
def mcbitmap_to_lags(mc_bitmap):
    """
    Convert an MC bitmap into a list of LAG indices
    """
    bit_map = string_to_bytes(mc_bitmap)
    lag_list = []
    for i in range(0, len(bit_map)):
        for j in range(0, 8):
            if bit_map[i] & (1 << j) != 0:
                devport_list.append(i * 8 + j)
    return lag_list

############################################################################
################# I N D I V I D U A L    T E S T S #########################
############################################################################

class McTest1(BaseProgramTest):
    # This method represents the test itself. Typically you would want to
    # configure the device (e.g. by populating the tables), send some
    # traffic and check the results.
    #
    # For more flexible checks, you can import unittest module and use
    # the provided methods, such as unittest.assertEqual()
    #
    # Do not enclose the code into try/except/finally -- this is done by
    # the framework itself
    def runTest(self):
        ################################################
        # For pktgen
        timer_app_id = 1
        app_id = timer_app_id
        pktlen = 54
        pgen_pipe_id = 0
        src_port = pgen_port(pgen_pipe_id)
        buff_offset = 144
        # buff_offset = 0
        p_count = 100  # packets per batch
        b_count = 2

        my_pkt = Ether(
            # port 188
            # dst = '08:c0:eb:33:31:74',
            # port 184
            # dst = 'b8:ce:f6:83:b2:ea',
            dst = '05:04:03:02:01:ff',
            src = '00:06:07:08:09:0a',
            type = 0x1234
        )
        ENABLE_PKTGEN = 0
        if ENABLE_PKTGEN:
            my_pkt = my_pkt/(("0")*(pktlen-len(my_pkt)))

            print(my_pkt)

            pktgen_app_cfg_table = self.bfrt_info.table_get("$PKTGEN_APPLICATION_CFG")
            pktgen_pkt_buffer_table = self.bfrt_info.table_get("$PKTGEN_PKT_BUFFER")
            pktgen_port_cfg_table = self.bfrt_info.table_get("$PKTGEN_PORT_CFG")

            print(pktgen_app_cfg_table)
            print(pktgen_pkt_buffer_table)
            print(pktgen_port_cfg_table)

            pktgen_port_cfg_table.entry_add(
                self.dev_tgt,
                [pktgen_port_cfg_table.make_key(
                    [gc.KeyTuple('dev_port', src_port)]
                )],
                [pktgen_port_cfg_table.make_data(
                    [gc.DataTuple('pktgen_enable', bool_val=True)]
                )]
            )

            pktgen_app_cfg_data = pktgen_app_cfg_table.make_data([
                gc.DataTuple('timer_nanosec', 1),
                gc.DataTuple('app_enable', bool_val=False),
                gc.DataTuple('pkt_len', pktlen),
                gc.DataTuple('pkt_buffer_offset', buff_offset),
                gc.DataTuple('pipe_local_source_port', src_port),
                gc.DataTuple('increment_source_port', bool_val=False),
                gc.DataTuple('batch_count_cfg', b_count-1),
                gc.DataTuple('packets_per_batch_cfg', p_count-1),
                gc.DataTuple('ibg', 1000), #  Inter batch gap in nanoseconds
                gc.DataTuple('ibg_jitter', 0), 
                gc.DataTuple('ipg', 1000), #  Inter packet gap in nanoseconds
                gc.DataTuple('ipg_jitter', 500),
                gc.DataTuple('batch_counter', 0),
                gc.DataTuple('pkt_counter', 0),
                gc.DataTuple('trigger_counter', 0)
            ], '$PKTGEN_TRIGGER_TIMER_ONE_SHOT')

            pktgen_app_cfg_table.entry_add(
                self.dev_tgt,
                [pktgen_app_cfg_table.make_key(
                    [gc.KeyTuple('app_id', timer_app_id)]
                )],
                [pktgen_app_cfg_data]
            )
            resp = pktgen_app_cfg_table.entry_get(
                self.dev_tgt,
                [pktgen_app_cfg_table.make_key(
                    [gc.KeyTuple('app_id', timer_app_id)]
                )],
                {"from_hw": True},
                pktgen_app_cfg_table.make_data([
                    gc.DataTuple('batch_counter'),
                    gc.DataTuple('pkt_counter'),
                    gc.DataTuple('trigger_counter')
                ], '$PKTGEN_TRIGGER_TIMER_ONE_SHOT', get=True)
            )

            data_dict = next(resp)[0].to_dict()
            tri_value = data_dict["trigger_counter"]
            batch_value = data_dict["batch_counter"]
            pkt_value = data_dict["pkt_counter"]
            print(data_dict)

            pktgen_pkt_buffer_table.entry_add(
                self.dev_tgt,
                [pktgen_pkt_buffer_table.make_key([
                    gc.KeyTuple('pkt_buffer_offset', buff_offset),
                    gc.KeyTuple('pkt_buffer_size', pktlen)
                ])],
                [pktgen_pkt_buffer_table.make_data(
                    [gc.DataTuple('buffer', str(my_pkt))]
                )]
            )

            pktgen_app_cfg_table.entry_mod(
                self.dev_tgt,
                [pktgen_app_cfg_table.make_key(
                    [gc.KeyTuple('app_id', timer_app_id)]
                )],
                [pktgen_app_cfg_table.make_data(
                    [gc.DataTuple('app_enable', bool_val=True)],
                    '$PKTGEN_TRIGGER_TIMER_ONE_SHOT'
                )]
            )

            while (1):
                time.sleep(10)
                resp = pktgen_app_cfg_table.entry_get(
                        self.dev_tgt,
                        [pktgen_app_cfg_table.make_key(
                            [gc.KeyTuple('app_id', timer_app_id)]
                        )],
                        {"from_hw": True},
                        pktgen_app_cfg_table.make_data([
                            gc.DataTuple('batch_counter'),
                            gc.DataTuple('pkt_counter'),
                            gc.DataTuple('trigger_counter')
                        ], '$PKTGEN_TRIGGER_TIMER_ONE_SHOT', get=True)
                    )
                data_dict = next(resp)[0].to_dict()
                tri_value = data_dict["trigger_counter"]
                batch_value = data_dict["batch_counter"]
                pkt_value = data_dict["pkt_counter"]

                if batch_value * pkt_value > p_count * b_count:
                    print("Enabling pktgen...")
                    pktgen_app_cfg_table.entry_mod(
                        self.dev_tgt,
                        [pktgen_app_cfg_table.make_key([gc.KeyTuple('app_id', timer_app_id)])],
                        [pktgen_app_cfg_data]
                    )
                    pktgen_app_cfg_table.entry_mod(
                        self.dev_tgt,
                        [pktgen_app_cfg_table.make_key(
                            [gc.KeyTuple('app_id', timer_app_id)]
                        )],
                        [pktgen_app_cfg_table.make_data(
                            [gc.DataTuple('app_enable', bool_val=True)],
                            '$PKTGEN_TRIGGER_TIMER_ONE_SHOT'
                        )])
