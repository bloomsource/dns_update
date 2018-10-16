#ifndef UTIL_H
#define UTIL_H
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <pwd.h>
#include <netdb.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include "md5.h"
#include "aes.h"
#include "crc32.h"

#define TIME_DIFF  10

#define ERR ( strerror( errno ) )

extern char log_file[100];
extern char server[100];
extern int  cmd_port;
extern char auth_key[100];

typedef struct{
    unsigned int rnd;
    unsigned int checksum;
    unsigned int time_stamp;
}upd_pack_hdr;

int udp_create();

int udp_listen( char* ip, int port );

int udp_send( int fd, char* ip, int port, char* msg, int len );

int udp_recv( int fd, char* buf, int size, char* ip, int* port );

int resolve_host_ip( const char* host, char* ip );

int write_log( const char* fmt, ... );

int write_log_hex( char* buf, int buflen );

int hex2asc( char* hex, int len, char* buf, int buflen );

char *strlower( char *s );

int change_process_user( char* user );

int crypto_derive_key(const char *pass, unsigned char *key, int key_len);

int pack_update_pkg( char* passwd, char* buf, char* msg, int msglen );

int unpack_update_pkg( char* passwd, int time_diff, char* pkg, int pkglen );






#endif
