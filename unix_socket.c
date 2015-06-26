#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/un.h>

#define SOCKET_PATH "/tmp/my_socket"
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define QLEN 10

void error( const char *errmsg )
{
    perror( errmsg );
    exit(1);
}

int unix_socket_connect( const char *path );
int unix_socket_listen( const char *path );
int unix_socket_accept( int listenfd );

char send_buff[1024];
char recv_buff[1024];

int main( int argc, char ** argv )
{
	int serv_fd, cli_fd;

	if( argc == 1 )
	{
		serv_fd = unix_socket_listen( SOCKET_PATH );
		cli_fd = unix_socket_accept( serv_fd );

		while( 1 )
		{
			if( read( cli_fd, recv_buff, 1024 ) <= 0 )
				break;

			printf( "%s\n", recv_buff );
		}
	}
	else
	{
		serv_fd = unix_socket_connect( SOCKET_PATH );
		int i = 0;
		for( ; i < 10; i++ )
		{
			sprintf( send_buff, "hello world : %d", i );
			write( serv_fd, send_buff, strlen( send_buff ) + 1 );
		}

		close( serv_fd );
	}
    exit(0);
}

int unix_socket_listen( const char *path )
{
	unlink( path );

	int fd, size;
	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy( addr.sun_path, path );
	size = offsetof( struct sockaddr_un, sun_path ) + strlen( path );

	if( ( fd = socket( AF_UNIX, SOCK_STREAM, 0 ) ) < 0 )
		error( "建立SOCKET失败!" );

	if( bind( fd, ( struct sockaddr *)&addr, size ) < 0 )
		error( "绑定SOCKET失败!" );

	if( listen( fd, QLEN ) < 0 )
		error( "监听错误" );

	return fd;
}

int unix_socket_accept( int listenfd )
{
	int fd, len;
	struct sockaddr_un addr;

	len = sizeof( addr );
	if( ( fd = accept( listenfd, (struct sockaddr *)&addr, &len ) ) < 0 )
		return -1;

	return fd;
}

int unix_socket_connect( const char *path )
{
	int fd, len;
	struct sockaddr_un addr;

	fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	addr.sun_family = AF_UNIX;
	strcpy( addr.sun_path, path );
	len = offsetof( struct sockaddr_un, sun_path ) + strlen( path );
	//len = sizeof( addr );

	if( connect( fd, (struct sockaddr *)&addr, len ) < 0 )
		error( "连接SOCKET失败" );

	return fd;
}

