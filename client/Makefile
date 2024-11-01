all:
	g++ -std=gnu++11 -g -O3 -I../include/ -c -o client.o client.cc
	# g++ -std=gnu++11 -g -O3 -I../include/ -c -o client.o client_throughput_latency.cc
	g++ -std=gnu++11 -g -O3 -c -o ../include/dma_common.o ../include/DMAcontext.cc
	g++ -std=gnu++11 -g -O3 -I../include/  -o lc_client_app client.o ../include/dma_common.o -lhiredis -lrdmacm -libverbs  -lmlx5 -lrt -lpthread -lhiredis -lm -fpermissive 
	
	
clean:
	# rm *.o
	rm lc_client_app *.o