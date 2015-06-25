#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/un.h>

#define socket_path "/tmp/my_socket"

void error( const char *errmsg )
{
    perror( errmsg );
    exit(1);
}

int main( int argc, char ** argv )
{
    int fd, size;

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy( addr.sun_path, socket_path );

    if( ( fd = socket( AF_UNIX, SOCK_STREAM ) ) < 0 )
        error( "建立 SOCKET 失败" );

    size = offsetof( struct sockaddr_un, sun_path ) + strlen( addr.sun_path );
    if( bind( fd, (struct sockaddr *)&addr, size ) < 0 )
        error( "绑定 SOCKET 失败" );

    printf("绑定SOCKET成功\n");
    exit(0);
}
