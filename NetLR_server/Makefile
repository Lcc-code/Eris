all:
	# g++ -std=gnu++11 -g -O3 -I../include/ -c -o server.o server.cc
	g++ -std=gnu++17 -g -O3 -I../include/  -c -o server.o -lrocksdb  server.cc
	g++ -std=gnu++17 -g -O3 -c -o ../include/dma_common.o -lrocksdb ../include/DMAcontext.cc
	g++ -std=gnu++17 -g -O3 -I../include/  -o lc_server_app server.o ../include/dma_common.o -lrocksdb -lhiredis -lrdmacm -libverbs  -lmlx5 -lrt -lpthread -lhiredis -lm -fpermissive 
	
clean:
	# rm *.o
	rm app *.o
	