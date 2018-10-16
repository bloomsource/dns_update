#include "util.h"

char log_file[100];
char server[100];
int  cmd_port;
char auth_key[100];





int udp_create()
{
    return socket( AF_INET, SOCK_DGRAM, 0 );
}

int udp_listen( char* ip, int port )
{
/* ip 表示绑定在哪个ip 上，如果为NULL,表示绑定在所有ip 上，
如果不为null,则绑定在指定ip地址上: 
函数调用成功返回 socket fd, 否则返回 -1  */

    int fd;
    struct sockaddr_in addr;

    fd = socket( AF_INET, SOCK_DGRAM, 0 );
    if( fd == -1 )
    {
        return -1;
    }

    int reuse=1;
    setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse) );
    
    addr.sin_family = AF_INET;
    addr.sin_port   = htons( port );

    if(ip == NULL || ip[0] == '*' )
    {
        addr.sin_addr.s_addr = INADDR_ANY;
        if( bind( fd, (struct sockaddr*)&addr, sizeof(addr) ) == -1 )
        {
            close( fd );
            return -1;
        }
    }
    else /*绑定ip  */
    {
        addr.sin_addr.s_addr = inet_addr(ip);
        if( bind( fd, (struct sockaddr*)&addr, sizeof(addr) ) == -1 )
        {
            close( fd );
            return -1;
        }
    }

    return fd;

}


int udp_send( int fd, char* ip, int port, char* msg, int len )
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons( port );
    addr.sin_addr.s_addr = inet_addr(ip);
    
    return sendto( fd, msg, len, 0, (struct sockaddr*)&addr, sizeof(addr) );
}


int udp_recv( int fd, char* buf, int size, char* ip, int* port )
{
    struct sockaddr_in addr;
    socklen_t len;
    int rc;
    
    len = sizeof(addr);
    
    rc = recvfrom( fd, buf, size, 0, (struct sockaddr *)&addr, &len );
    if( rc > 0 )
    {
        *port=ntohs(addr.sin_port);
        sprintf(ip,"%s",inet_ntoa(addr.sin_addr));
    }
    
    return rc;
}

int resolve_host_ip( const char* host, char* ip )
{
    struct hostent* ent;
    char* addr;
    unsigned char b1,b2,b3,b4;

     ent = gethostbyname( host );
    if( ent == NULL )
        return -1;
    addr = *ent->h_addr_list;
    if( !addr )
        return -1;
    b1 = addr[0];
    b2 = addr[1];
    b3 = addr[2];
    b4 = addr[3];

    sprintf( ip, "%d.%d.%d.%d", b1, b2, b3, b4 );
    return 0;
}


int write_log( const char* fmt, ... )
{
    FILE* f;
    time_t t;
    struct tm* tm;
    va_list  ap;
    //struct timeval tmv;

    f = fopen( log_file, "a" );
    if( f == NULL )
        return -1;
    
    t = time( 0 );
    //gettimeofday( &tmv, NULL );
    
    tm = localtime( &t );
    
    if( *fmt )
    {
        va_start( ap, fmt );
        //fprintf(f,"[%02d-%02d %02d:%02d:%02d.%03d]  ",tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec,tmv.tv_usec / 1000 );
        fprintf( f, "[%02d-%02d %02d:%02d:%02d]  ", tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec );
        vfprintf( f, fmt, ap);
        fprintf( f, "\n" );
        va_end( ap );
    }
    
    fclose( f );
    return 0;
}

int write_log_hex( char* buf, int buflen )
{
  FILE *fp;
  int i,g,k;
  unsigned char *abuf,*hbuf;


  if ( ( fp = fopen(log_file,"a+")) == NULL) return(-1);

  hbuf = (unsigned char *)buf;
  abuf = (unsigned char *)buf;
  fprintf( fp, "  *** hex message(begin) ***\n" );
  fprintf( fp, "-----------|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|------------------\n" );
  for( i = 0, g = buflen/16; i < g; i++ )
  {
    fprintf( fp, "M(%6.6d)=< ", i*16 );
    for( k = 0; k < 16; k ++) fprintf( fp, "%02x ", *hbuf++ );
        fprintf( fp, "> " );
    for( k = 0; k < 16; k++, abuf++)
        fprintf( fp, "%c", (*abuf>32) ? ((*abuf<128) ? *abuf : '*') : '.');
    fprintf( fp, "\n" );
  }
  if( (i = buflen%16) > 0)
  {
    fprintf( fp, "M(%6.6d)=< ", buflen-buflen%16 );
    for( k = 0; k < i;  k++) fprintf( fp, "%02x ", *hbuf++);
    for( k = i; k < 16; k++) fprintf( fp, "   ");
        fprintf( fp, "> ");
    for( k = 0; k < i; k++, abuf++)
        fprintf( fp, "%c",(*abuf>32) ? ((*abuf<128) ? *abuf : '*') : '.');
    fprintf( fp, "\n");
  }
  fprintf( fp, "-----------|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|------------------\n");
  fprintf( fp, "  *** hex message(end) ***\n");
  fflush( fp );
  fclose( fp );
  return(0);
}

int hex2asc( char* hex, int len, char* buf, int buflen )
{
    char bits[] = "0123456789abcdef";
    
    unsigned char u;
    int i;

    if( buflen < len * 2 )
        return -1;
    buf[len*2]=0;
    
    for( i = 0; i < len; i++ )
    {
        u = hex[i];
        
        buf[i*2+0] = bits[ u >> 4 ];
        buf[i*2+1] = bits[ u & 0x0f ];
    }
    
    return 0;
    
}


char *strlower( char *s )
{
    int i, len = strlen(s);
    for( i = 0; i < len; i++ ){
        s[i] = ( s[i] >= 'A' && s[i] <= 'Z' ? s[i] +32 : s[i] );
    }
    return s;
}

int change_process_user( char* user )
{
    struct passwd *pw;
    
    pw = getpwnam( user );
    if( !pw )
        return -1;
        
    if( setgid(pw->pw_gid ) )
        return -1;    
    
    if( setuid( pw->pw_uid ) )
        return -1;

    return 0;
}

int crypto_derive_key(const char *pass, unsigned char *key, int key_len)
{
    int datal;
    datal = strlen((const char *)pass);
    md5_context ctx;



    unsigned char md_buf[16];
    int addmd;
    unsigned int i, j, mds;

    mds = 16;


    for (j = 0, addmd = 0; j < key_len; addmd++) {
        md5_starts(&ctx);
        if (addmd) {
            md5_update(&ctx, md_buf, mds);
        }
        md5_update(&ctx, (unsigned char *)pass, datal);
        md5_finish(&ctx, &(md_buf[0]));

        for (i = 0; i < mds; i++, j++) {
            if (j >= key_len)
                break;
            key[j] = md_buf[i];
        }
    }


    return key_len;
}

int pack_update_pkg( char* passwd, char* buf, char* msg, int msglen )
{
    aes_context ctx;
    
    upd_pack_hdr *hdr;
    char tmp[1024];
    unsigned char aes_key[32];
    unsigned char iv[16];
    unsigned int  checksum;
    unsigned int  timestamp;
    int offset = 0;
    int left = msglen + sizeof(upd_pack_hdr);
    int proc_len;
    
    int iv_off = 0;
    
    hdr = (upd_pack_hdr*)buf;
    
    crypto_derive_key( passwd, aes_key, 32 );
    md5( (unsigned char*)passwd, strlen( passwd ), iv );
    
    timestamp = time( NULL );
    hdr->time_stamp = htonl( timestamp );
    hdr->checksum   = 0;
    hdr->rnd        = rand();

    memcpy( buf + sizeof(upd_pack_hdr), msg, msglen );
    
    checksum = crc32( 0, buf, sizeof(upd_pack_hdr) + msglen );
    hdr->checksum = htonl( checksum );
    aes_setkey_enc( &ctx, aes_key, 256 );
    
    while( left )
    {
        proc_len = left > sizeof(tmp) ? sizeof(tmp) : left;
        
        aes_crypt_cfb128( &ctx, AES_ENCRYPT, proc_len, &iv_off, iv, (unsigned char*)buf + offset, (unsigned char*)tmp );
        memcpy( buf + offset, tmp, proc_len );
        
        left -= proc_len;
        offset += proc_len;
    }
    
    
    return 0;

}

int unpack_update_pkg( char* passwd, int time_diff, char* pkg, int pkglen )
{
    aes_context ctx;
    
    upd_pack_hdr *hdr;
    int offset = 0;
    int left = pkglen;
    int proc_len;    
    char tmp[1024];
    unsigned char aes_key[32];
    unsigned char iv[16];
    unsigned int  checksum;
    unsigned int  curtime;
    unsigned int  timestamp;
    int iv_off = 0;
    
    
    
    if( pkglen < sizeof(upd_pack_hdr) + 1 )
    {
        write_log( "[WRN] update pack length too small!" );
        return -1;
    }
    
    crypto_derive_key( passwd, aes_key, 32 );
    md5( (unsigned char*)passwd, strlen( passwd ), iv );
    aes_setkey_enc( &ctx, aes_key, 256 );
    
    while( left )
    {
        proc_len = left > sizeof(tmp) ? sizeof(tmp) : left;
        aes_crypt_cfb128( &ctx, AES_DECRYPT, proc_len, &iv_off, iv, (unsigned char*)pkg + offset, (unsigned char*)tmp );
        
        memcpy( pkg + offset, tmp, proc_len );
        
        left -= proc_len;
        offset += proc_len;
    }
    
    
    
    hdr = (upd_pack_hdr*)pkg;
    
    curtime = time( NULL );
    timestamp = ntohl( hdr->time_stamp );
    
    checksum = ntohl( hdr->checksum );
    hdr->checksum = 0;
    
    if( checksum != crc32( 0, pkg, pkglen ) )
    {
        write_log( "[WRN] update pack checksum incorrected!" );
        return -1;
    }
    
    if( abs( curtime - timestamp ) > time_diff )
    {
        write_log( "[WRN] update pack timestamp out of range!" );
        return -1;
    }
    
    return 0;
    
}


