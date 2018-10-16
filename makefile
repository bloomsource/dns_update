
CFLAGS= -Wall 

all: dns_srv dns_update


dns_srv: dns.o util.o md5.o cjson.o rbtree.o aes.o crc32.o
	gcc -o $@ $^


dns_update: dns_update.o md5.o  cjson.o util.o aes.o crc32.o
	gcc -o $@ $^


clean:
	rm -f *.o *.log

c:clean

