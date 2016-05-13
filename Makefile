cc=g++
CXXFLAGS = -I/usr/include/libxml2 
LIBS = -lpthread -lrt -lxml2 -lnetsnmp

obj = handle_information.o utils.o ctdb_ip.o

collect_send: $(obj)
	$(cc) -o collect_send -g $(obj) $(LIBS)

.PHONY:clean

clean:
	rm -f *.o collect_send
