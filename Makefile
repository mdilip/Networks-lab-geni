#make file to create executable of ospf

CC = g++
CFLAGS = -g
PTHREAD = -lpthread -lrt

ospf: ospf.cpp ospf.h
	$(CC) $(CFLAGS) -o $@ $< $(PTHREAD)

.PHONY: clean
clean:
	rm -f ospf
