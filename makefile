
CFLAGS= -Wall 

all: dns dns_update


dns: dns.o util.o md5.o cjson.o rbtree.o
	gcc -o $@ $^


dns_update: dns_update.o md5.o  cjson.o util.o
	gcc -o $@ $^


clean:
	rm -f *.o *.log

c:clean

