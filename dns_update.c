#include "util.h"
#include "md5.h"
#include "cjson.h"

int fd;
void usage()
{
    printf( "usage:dns_update server:port auth_key domain [ip]\n" );
}

int main( int argc, char* argv[] )
{
    int rc;
    char json[1024];
    char buf[1024];
    char ip[16];
    int port;
    int json_len;
    struct pollfd pfd;
    char* p;
    cJSON *json_root, *json_retcode;
    
    if( argc < 4 )
    {
        usage();
        return 1;
    }
    
    
    strcpy( log_file, "dns_update.log" );
    
    snprintf( server, sizeof(server), "%s", argv[1] );
    p = strchr( server, ':' );
    if( !p )
    {
        usage();
        return 1;
    }
    if( strlen( p+1 ) == 0 )
    {
        usage();
        return 1;
    }
    
    p[0] = 0;
    
    cmd_port = atoi( p+1 );
    snprintf( auth_key, sizeof(auth_key), "%s", argv[2] );
    
    
    fd = udp_create();
    if( fd == -1 )
    {
        printf( "create udp socket failed! err:%s\n", ERR );
        write_log( "[ERR] create udp socket failed! err:%s", ERR );
        return 1;
    }
    
    if( resolve_host_ip( server, ip ) )
    {
        printf( "resolve server ip failed!\n" );
        write_log( "[ERR] resolve server ip failed!" );
        return 1;
    }
    
    if( argc > 4 )
    {
        sprintf( buf, ", \"ip\":\"%s\" ", argv[4] );
    }
    
    sprintf( json, "{ \"cmd\":\"update_domain\", \"domain\":\"%s\" %s }", argv[3], argc > 4 ? buf : " " );
    
    json_len = strlen( json );
    pack_update_pkg( auth_key, buf, json, json_len );
    
    rc = udp_send( fd, ip, cmd_port, buf, json_len + sizeof(upd_pack_hdr) );
    if( rc != json_len + sizeof(upd_pack_hdr) )
    {
        printf( "send udp pack failed! err:%s\n", ERR );
        write_log( "[ERR] send udp pack failed! err:%s", ERR );
        return 1;
    }
    
    pfd.fd = fd;
    pfd.events = POLLIN;
    
    rc = poll( &pfd, 1, 1000 );
    if( rc <= 0 )
    {
        printf( "recv udp response timeout!\n" );
        write_log( "[ERR] recv udp response timeout." );
        return 1;
    }
    
    rc = udp_recv( fd, buf, sizeof(buf), ip, &port );
    if( rc <= 0 )
    {
        printf( "recv udp response failed! err:%s\n", ERR );
        write_log( "[ERR] recv udp response failed! err:%s", ERR );
        return 1;
    }
    buf[rc] = 0;
    
    if( unpack_update_pkg( auth_key, TIME_DIFF, buf, rc ) )
    {
        printf( "recv invalid response!\n" );
        write_log( "[ERR] recv invalid response!" );
        return 1;
    }
    
        
    json_root = cJSON_Parse( buf + sizeof(upd_pack_hdr) );
    if( !json_root )
    {
        printf( "parse response json failed!\n" );
        write_log( "[ERR] parse response json failed! resp:[%s]", buf );
        return 1;
    }
    
    json_retcode = cJSON_GetObjectItem( json_root, "retcode" );
    if( !json_retcode )
    {
        printf( "invlaid json response.\n" );
        write_log( "[ERR] invalid json response, resp:[%s]", buf );
        return 1;
    }
    
    if( json_retcode->valueint )
    {
        printf( "update domain failed, retcode:%d\n", (int)json_retcode->valueint );
        write_log( "[ERR] update domain fialed, resp:[%s]", buf );
        return 1;
    }
    
    printf( "update domain %s ok!\n", argv[3] );
    write_log( "update domain %s ok!", argv[3] );
    close( fd );
    cJSON_Delete( json_root );
    
    
    
    
    return 0;
}


