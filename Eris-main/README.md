# Eris
client and server are our contributions.
The IONIA* and NetLR* are the baselines.

# switch
## 1.build the source code。
$SDE/p4_build.sh ~/p4_programs/lcc/KV/optkv.p4  
## 2.run the project  
$SDE/run_switchd.sh -p optkv  
## 3.run the p4_test to configure the port and table.   
clear && ./run_p4_tests.sh -p optkv -t KV/  
## 4.Note that you should configure the port according to your local environment. Also the same as the server configuration.  
port   28->replica1  20->replica2  14->replica3;  
bitmap 001->replica1 010->replica2 100->replica3;  
  
# Client   
## 1.build the source code  
make  
## 2.run the project -> Usage %s [Thread_num] [isServer] [workload] [UsedSwitchAGTRcount]  
sudo ./lc_client_app 1 0 1 2048  

# Server  
## 1.build the source code  
make   
## 2.run the project -> Usage %s [Thread_num] [isServer] [use mlx_[i]]  
sudo ./lc_server_app 1 1 1  

# Trace  
## 1.Use ycsb to generate trace  
./bin/ycsb.sh run basic -P workloads/workloada > run.txt  
## 2.parser in parse_trace  
python parse.py  

