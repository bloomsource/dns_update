#include "util.h"
#include "md5.h"
#include "cjson.h"
#include "rbtree.h"

typedef struct{
    unsigned short id;
    unsigned short flags;
    unsigned short qry_cnt;
    unsigned short ans_cnt;
    unsigned short auth_cnt;
    unsigned short add_cnt;
    
}dns_hdr;

typedef struct{
    rbnode _;
    char domain[256];
    char ip[16];
} dns_node;

#define DNS_PORT   53
#define MAX_DOMAIN 255

rbtree domains;

int run = 1;

int dns_fd;
int cmd_fd;

int  domain_node_cmp( rbnode* n1, rbnode* n2 );
int  domain_value_cmp( void* val, rbnode* n );
void domain_node_swap( rbnode* n1, rbnode* n2 );

void sigproc( int sig );
int dns_resove_query( char* data, int len, char* name, int* query_type, int* query_class, int* query_len );
int dns_write_domain( char* data, int size, char* domain, int* write_len );

void proc_dns_query( char* data, int len, char* ip, int port );
void proc_cmd( char* data, int len, char* ip, int port );

int main( int argc, char* argv[] )
{
    int rc;
    struct pollfd pfds[2];
    char buf[1024];
    char ip[16];
    int  port;
    
    
    strcpy( log_file, "dns.log" );
    
    if( argc < 3 )
    {
        printf( "usage:dns_srv port auth_key [user]\n" );
        return 1;
    }
    
    cmd_port = atoi( argv[1] );
    snprintf( auth_key, sizeof(auth_key), "%s", argv[2] );
    
    dns_fd = udp_listen( NULL, DNS_PORT );
    if( dns_fd == -1 )
    {
        printf( "udp listen on dns port[%d] failed! err:%s\n", DNS_PORT, ERR );
        write_log( "[ERR] udp listen on dns port[%d] failed! err:%s", DNS_PORT, ERR );
        return 1;
    }
    
    cmd_fd = udp_listen( NULL, cmd_port );
    if( dns_fd == -1 )
    {
        printf( "udp listen on dns port[%d] failed! err:%s\n", cmd_port, ERR );
        write_log( "[ERR] udp listen on dns port[%d] failed! err:%s", cmd_port, ERR );
        return 1;
    }
    
    if( argc > 3 )
    {
        if( change_process_user( argv[3] ) )
        {
            printf( "process switch to user '%s' failed!\n", argv[3] );
            write_log( "[ERR] process switch to user '%s' failed!", argv[3] );
            return 1;
        }
    }
    
    signal( SIGINT, sigproc );
    signal( SIGTERM,sigproc );
    
    printf( "dns start up ok!\n" );
    write_log( "dns start up ok!" );
    write_log( "pid: %d", getpid() );
    
    rbtree_init( &domains, domain_node_cmp, domain_value_cmp, domain_node_swap );
    
    
    while( run )
    {
        pfds[0].fd = dns_fd;
        pfds[0].events = POLLIN;
        pfds[0].revents = 0;
        
        pfds[1].fd = cmd_fd;
        pfds[1].events = POLLIN;
        pfds[1].revents = 0;
        
        rc = poll( pfds, 2, 1000 );
        if( rc <= 0 )
            continue;
        
        if( pfds[0].revents )
        {
            rc = udp_recv( dns_fd, buf, sizeof(buf), ip, &port );
            if( rc > 0 )
            {
                proc_dns_query( buf, rc, ip, port );
            }
        }
        
        if( pfds[1].revents )
        {
            rc = udp_recv( cmd_fd, buf, sizeof(buf)-1, ip, &port );
            if( rc > 0 )
            {
                buf[rc] = 0;
                proc_cmd( buf, rc, ip, port );
            }
            
            
            
        }
        
        
    }
    
    write_log( "[WRN] dns stop." );
    
    return 0;
    
}


int  domain_node_cmp( rbnode* n1, rbnode* n2 )
{
    dns_node *node1, *node2;
    
    node1 = (dns_node*)n1;
    node2 = (dns_node*)n2;
    
    return strcmp( node1->domain, node2->domain );
    
}

int  domain_value_cmp( void* val, rbnode* n )
{
    dns_node *node;
    char* str;
    
    str = val;
    node = (dns_node*)n;
    
    return strcmp( str, node->domain );
}

void domain_node_swap( rbnode* n1, rbnode* n2 )
{
    dns_node node, *node1, *node2;
    
    node1 = (dns_node*)n1;
    node2 = (dns_node*)n2;
    
    strcpy( node.domain, node1->domain );
    strcpy( node.ip, node1->ip );
    
    strcpy( node1->domain, node2->domain );
    strcpy( node1->ip, node2->ip );
    
    strcpy( node2->domain, node.domain );
    strcpy( node2->ip, node.ip );
    
    
}

void sigproc( int sig )
{
    write_log( "[WRN] signal '%s' received, dns server will shutdown..", strsignal( sig ) );
    run = 0;
}

void fill_binary_ip( unsigned char bin[], char* ip )
{
    char tmp[20];
    char *p, *p1;
    
    strcpy( tmp, ip );
    p1 = tmp;
    
    p = strchr( p1, '.' );
    p[0] = 0;
    bin[0] = atoi( p1 );
    p1 = p+1;
    
    p = strchr( p1, '.' );
    p[0] = 0;
    bin[1] = atoi( p1 );
    p1 = p+1;

    p = strchr( p1, '.' );
    p[0] = 0;
    bin[2] = atoi( p1 );
    p1 = p+1;
    
    bin[3] = atoi( p1 );
    
}



void proc_dns_query( char* data, int len, char* ip, int port )
{
    dns_hdr req, rsp;
    char domain[MAX_DOMAIN+1];
    char buf[1024];
    int write_len;
    int qry_len;
    int qry_type, qry_class;
    int offset = 0;
    unsigned short slen;
    unsigned int   ilen;
    unsigned char bin_ip[4];
    int find;
    dns_node *node;
    
    //write_log( "recv dns data from %s:%d", ip, port );
    //write_log_hex( data, len );
    
    if( len < sizeof(dns_hdr) )
    {
        write_log( "recv dns data from %s:%d", ip, port );
        write_log_hex( data, len );
        write_log( "[WRN] dns pack size small, query abandon." );
        return;
    }
    
    memcpy( &req, data, sizeof(req) );
    req.flags   = ntohs( req.flags );
    req.qry_cnt = ntohs( req.qry_cnt );
    req.ans_cnt = ntohs( req.ans_cnt );
    req.auth_cnt= ntohs( req.auth_cnt );
    req.add_cnt = ntohs( req.add_cnt );
    
    if( req.flags & 0x8000 )
    {
        write_log( "recv dns data from %s:%d", ip, port );
        write_log_hex( data, len );
        write_log( "[WRN] dns query flag is not 1, query abandon." );
        return;
    }
    
    if( req.qry_cnt != 1 )
    {
        write_log( "recv dns data from %s:%d", ip, port );
        write_log_hex( data, len );
        write_log( "[WRN] dns query req cnt is not 1, query abandon." );
        return;
    }
    
    if( req.ans_cnt )
    {
        write_log( "recv dns data from %s:%d", ip, port );
        write_log_hex( data, len );
        write_log( "[WRN] dns ans_cnt not 0, query abandon." );
        return;
    }
    
    memset( domain, 0, sizeof(domain) );
    if( dns_resove_query( data + sizeof(req), len - sizeof(req), domain, &qry_type, &qry_class, &qry_len ) )
    {
        write_log( "recv dns data from %s:%d", ip, port );
        write_log_hex( data, len );
        write_log( "[WRN] dns query resolve domain failed, query abandon." );
        return ;
    }
    strlower( domain );
    
    write_log( "recv dns query from %-15s domain: %-20s type: %-3d class: %-3d", ip, domain, qry_type, qry_class );
    

    
    node = (dns_node*)rbtree_find( &domains, domain );
    if( node )
        find = 1;
    else
        find = 0;
    
    if( find == 0 )
    {
        write_log( "[WRN] domain %s not exist!", domain );
        return;
    }

    if( qry_type != 1 || qry_class != 1 ) //ipv4 only
    {
        write_log( "[WRN] dns query type/class not support." );
        find = 0;
    }
    
    

    rsp.id = req.id;
    rsp.flags = 0;
    rsp.flags |= 0x8000;
    if( !find )
        rsp.flags |= 0x03;
        
    rsp.flags = htons( rsp.flags );
    rsp.qry_cnt = htons( 1 );
    rsp.ans_cnt = find ? htons( 1 ) : 0;
    rsp.auth_cnt = 0;
    rsp.add_cnt = 0;
    
    /*------------ write response header ---------------*/
    memcpy( buf+offset, &rsp, sizeof(rsp) );
    offset+= sizeof(rsp );


    //-------------- write query ------------------------
    //write domain
    if( dns_write_domain( buf+offset, sizeof(buf)-sizeof(rsp), domain, &write_len ) )
        return;
    offset+= write_len;
    
    
    // write type
    slen = htons( qry_type );
    memcpy( buf + offset, &slen, sizeof(slen) );
    offset += sizeof(slen);
    
    //write class
    slen = htons( qry_class );
    memcpy( buf + offset, &slen, sizeof(slen) );
    offset += sizeof(slen);
    

    //--------------- write answer ------------------------
    if( !find )
        goto send_rsp;
        
    //answer name
    slen = htons( 0xc00c );
    memcpy( buf + offset, &slen, sizeof(slen) );
    offset += sizeof(slen);
    
    // write type
    slen = htons( 1 );
    memcpy( buf + offset, &slen, sizeof(slen) );
    offset += sizeof(slen);
    
    //write class
    slen = htons( 1 );
    memcpy( buf + offset, &slen, sizeof(slen) );
    offset += sizeof(slen);
    
    //write ttl
    ilen = htonl( 300 );
    memcpy( buf + offset, &ilen, sizeof(ilen) );
    offset += sizeof(ilen);
    
    //write address len
    slen = htons( 4 );
    memcpy( buf + offset, &slen, sizeof(slen) );
    offset += sizeof(slen);
    
    //write ip
    fill_binary_ip( bin_ip, node->ip );
    memcpy( buf + offset, bin_ip, 4 );
    offset += 4;
    
    
    send_rsp:
    //write_log( "dns response data:" );
    //write_log_hex( buf, offset );
    
    udp_send( dns_fd, ip, port, buf, offset );
    
}


int check_domain_char( char* domain, int len )
{
    int i;
    
    for( i = 0; i < len; i++ )
    {
        if( isalnum( domain[i] ) )
            continue;
        
        switch( domain[i] )
        {
            case '_':
            case '-':
                break;
            
               default:
                return -1;
        }
    }
    
    return 0;
    
}

int dns_resove_query( char* data, int len, char* domain, int* query_type, int* query_class, int* query_len )
{
    char* pdata;
    unsigned char *blen;
    int namelen= 0;
    int seclen;
    int left;
    char *pname;
    unsigned short shortlen;
    
    domain[0] = 0;
    pdata = data;
    pname = domain;
    left = len;
    
    if( left < 3+ 4 )
    {
        write_log( "[WRN] query length not enough." );
        return 1;
    }
    
    while( 1 )
    {
        blen = (unsigned char*)pdata;
        seclen = *blen;
        if( seclen == 0 )
        {
            if( namelen == 0 )
            {
                write_log( "[WRN] domain length is 0" );
                return 1;
            }
            
            if( domain[namelen-1] == '.' )
            {
                domain[namelen-1] = 0;
                namelen--;
            }
            pdata++;
            left --;
            break;
        }
        
        namelen += ( seclen + 1 );
        if( namelen > 255 )
        {
            write_log( "[WRN] domain name over length." );
            return 1;
        }
        
        if( seclen + 1 > left )
        {
            write_log( "[WRN] domain name not enough length." );
            return 1;
        }
        
        if( check_domain_char( pdata+1, seclen ) )
        {
            write_log( "[WRN] invalid domain char." );
            return 1;
        }
        
        memcpy( pname, pdata+1, seclen );
        pname[ seclen ] = '.';
        pname += ( seclen + 1 );
        left -= ( seclen+1 );
        pdata += ( seclen + 1 );
    }
    
    if( namelen == 0 )
    {
        write_log( "[WRN] domain name length is zero." );
    }
    
    if( left < sizeof(short)* 2 )
    {
        write_log( "[WRN] query length is not enough." );
        return 1;
    }
    
    memcpy( &shortlen, pdata, 2 );
    shortlen = ntohs( shortlen );
    *query_type = shortlen;
    
    memcpy( &shortlen, pdata+2, 2 );
    shortlen = ntohs( shortlen );
    *query_class = shortlen;
    
    left -= 4;
    
    *query_len = len - left;
    
    return 0;
}

int dns_write_domain( char* data, int size, char* domain, int* write_len )
{
    char* pdata;
    char* pdomain, *p;
    unsigned char *blen;
    int seclen;
    int left;
    
    left = size;
    if( strlen( domain ) > MAX_DOMAIN )
    {
        write_log( "[WRN] write domain failed, domain name too large." );
        return -1;
    }
    
    pdata = data;
    pdomain = domain;
    
    while( 1 )
    {
        if( pdomain[0] == 0 )
            break;
        
        p = strchr( pdomain, '.' );
        if( p )
        {
            seclen = p - pdomain;
            if( left <= seclen + 1 )
            {
                write_log( "[WRN] write domain failed, space not enough." );
                return 1;
            }
            
            blen = (unsigned char*)pdata;
            *blen = seclen;
            memcpy( pdata+1, pdomain, seclen );
            left -= ( seclen + 1 );
            pdata += ( seclen + 1 );
            pdomain = p+1;
        }
        else
        {
            seclen = strlen( pdomain );
            if( left < seclen + 2 )
            {
                write_log( "[WRN] write domain failed, space not enough." );
                return 1;
            }
            
            blen = (unsigned char*)pdata;
            *blen = seclen;
            memcpy( pdata+1, pdomain, seclen );
            blen = (unsigned char*)pdata + 1 + seclen;
            *blen = 0;
            left -= ( seclen + 2 );
            pdata += ( seclen + 2 );
            break;
        }
    }
    
    *write_len = size - left;
    
    return 0;
    
    
}

void proc_cmd( char* data, int len, char* ip, int port )
{
    cJSON *json_root, *json_domain, *json_ip, *json_cmd;
    char buf[1024];
    char json[1024];
    int json_len;
    int ilen;
    char domain[MAX_DOMAIN+1];
    dns_node* node;
    
    if( unpack_update_pkg( auth_key, TIME_DIFF, data, len ) )
    {
        write_log( "[WRN] recv invalid cmd pack!" );
        write_log_hex( data, len );
        return;
    }
    
    
    json_root = cJSON_Parse( data + sizeof(upd_pack_hdr) );
    if( !json_root )
    {
        write_log( "[WRN] recv invalid json cmd from %s:%d, parse failed.", ip, port );
        return;
    }
    
    json_cmd    = cJSON_GetObjectItem( json_root, "cmd" );
    json_domain = cJSON_GetObjectItem( json_root, "domain" );
    json_ip     = cJSON_GetObjectItem( json_root, "ip" );
    
    
    if( !json_cmd || !json_domain )
    {
        write_log( "[WRN] recv invalid json cmd from %s:%d, missing elements.", ip, port );
        cJSON_Delete( json_root );
        return;
    }
    
    ilen = strlen( json_domain->valuestring );
    if( ilen == 0 || ilen > MAX_DOMAIN )
    {
        write_log( "[WRN] recv cmd domain over length from %s:%d", ip, port );
        cJSON_Delete( json_root );
        return ;
    }
    snprintf( domain, sizeof(domain), "%s", json_domain->valuestring );
    

    
    strlower( domain );
    
    if( json_ip )
        ip = json_ip->valuestring;
    
    
    node = (dns_node*)rbtree_find( &domains, domain );
    if( !node )
    {
        node = malloc( sizeof( dns_node ) );
        if( !node )
        {
            write_log( "[ERR] malloc memory for dns_node failed! err:%s", ERR );
            strcpy( json, "{ \"retcode\": 1, \"desc\":\"mallof failed!\" }" );
            json_len = strlen( json );
            pack_update_pkg( auth_key, buf, json, json_len );
            udp_send( cmd_fd, ip, port, buf, json_len + sizeof(upd_pack_hdr) );
            cJSON_Delete( json_root );
            return;
        }
        strcpy( node->ip, ip );
        strcpy( node->domain, domain );
        
        rbtree_insert( &domains, (rbnode*)node );
        write_log( "set domain %-20s ip to %s", domain, ip );
    }
    else
    {
        if( strcmp( node->ip, ip ) )
        {
            write_log( "update domain %-20s ip to %s", domain, ip );
        }
        else
        {
            write_log( "client %-15s update domain %-20s", ip, domain );
            
        }
        
        strcpy( node->ip, ip );
        
    }
    
    cJSON_Delete( json_root );
    
    sprintf( json, "{ \"retcode\":0, \"desc\": \"ok\" }" );
    json_len = strlen( json );
    pack_update_pkg( auth_key, buf, json, json_len );
    udp_send( cmd_fd, ip, port, buf, json_len + sizeof(upd_pack_hdr) );
    return;
    
}








