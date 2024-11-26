# Eris Benchmark

## Requirment - Run with Singal Swtich
In this experiment, 3 physical replicas, 1 physical client and 1 switch is used.

The Eris consists of [client](Eris-main/client), [server](Eris-main/server) and [p4src](Eris-main/p4src)

The IONIA* and NetLR* are the baselines.


## Getting Started
```
$ git clone git@github.com:Lcc-code/Eris.git
```

### Run Tofino Switch

#### Compile P4 Program and Start the Tofino Model (Terminal1)
We use the physical switch.
```
$ cd $SDE
```
```
$ $TOOLS/p4_build.sh ~/git/p4src/optkv.p4
```
#### Load Specified Switch Program (Terminal1)
```
$ ./run_switch.sh -p optkv
```
#### Enable Ports and Install Entries (Terminal2)
```
$ cd $SDE
```
```
clear && $SDE/run_p4_tests.sh -p optkv -t KV/  
```

#### Note that you should configure the port according to your local environment. Also the same as the server configuration.  
port   28->replica1  20->replica2  14->replica3;  
bitmap 001->replica1 010->replica2 100->replica3;

### Trace  
#### 1.Use ycsb to generate trace  
```
$ ./bin/ycsb.sh run basic -P workloads/workloada > run.txt  
```
#### 2.parser in parse_trace  
```
$ python parse.py  
```

### Run Replicas
#### Complie Replicas (Terminal3)
```
$ cd $Eris_REPO/server/
```
```
$ make
```
#### Run Replica1
```
# Usage %s [Thread_num] [isServer] [use mlx_[i]]
$ sudo ./lc_server_app 1 1 1
```
#### Run Replica2
```
# Usage %s [Thread_num] [isServer] [use mlx_[i]]
$ sudo ./lc_server_app 1 1 2
```
#### Run Replica3
```
# Usage %s [Thread_num] [isServer] [use mlx_[i]]
$ sudo ./lc_server_app 1 1 3
```
####
[server](Eris-main/server/server.cc)
`
static uint32_t bitmap_1 = htonl(0x1);

static int host_bitmap_1 = 0x1;

static uint32_t bitmap_0 = htonl(0x2);

static int host_bitmap_0 = 0x2;

static uint32_t bitmap_1 = htonl(0x4);

static int host_bitmap_1 = 0x4;

static uint32_t bitmap = 0;

static int host_bitmap = 0;

uint16_t num_worker = htonl(0x3);

`


### Run Client
#### Complie Clients
```
$ cd $Eris_REPO/client/
```
```
$ make
```
#### Run Client
```
$ Usage %s [Thread_num] [isServer] [workload] [UsedSwitchAGTRcount]  
$ sudo ./lc_client_app 1 0 1 2048  
```







