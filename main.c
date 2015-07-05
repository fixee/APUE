#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<time.h>
#include<fcntl.h>
#include<dirent.h>
#include<pwd.h>
#include<sys/sysinfo.h>

//----------------  definitons ------------------------
#define False 0
#define True 1
#define bool int

struct proc_info
{
    long mem;
	long total_time;
	double uptime;
    double cpu;
    pid_t ppid;
    pid_t pid;
    uid_t uid;
    time_t starttime;
    char cmdline[4096];
};

struct proc_list
{
	struct proc_info *proc;
	struct proc_list *next;
};
// end definition 


//--------------- lib tools --------------------------
bool is_number( const char *str );
bool start_with( const char *str1, const char *str2 );
void s_trim( char *str );
void error( const char *errmsg );

void push_queue( struct proc_list **list_head, 
				 struct proc_list **list_tail, 
				 struct proc_info *proc );
void remove_queue( struct proc_list **list_head, 
				   struct proc_list **list_tail, 
				   struct proc_list *node );
void destroy_proc_list_node( struct proc_list *node );
void destroy_proc_list( struct proc_list **head, struct proc_list **tail );

double get_uptime();
// end lib tools


//-------------- useful funcs ------------------------
bool get_pid_starttime_uid( const char *pid, struct proc_info *proc );
bool get_pid_cmdline( const char *pid, struct proc_info *proc );
bool get_proc_mem_info( const char *pid, struct proc_info *proc );
bool get_proc_cpu_info( const char *pid, struct proc_info *proc );
bool get_pid_info( const char *pid, struct proc_info *proc );
bool print_proc_info( struct proc_info *proc );
const char *get_uername( uid_t uid );
// end useful funcs

//-------------- variables --------------------------
int userid[1024];
char username[1024][1024];
int user_len = 0;
// end variables


int main( int argc, char **argv )
{
	uid_t filter_uid = -1;
	const char *filter_username = NULL;
	if( argc > 2 && strcmp( "-u", argv[1] ) == 0 )
	{
		filter_username = argv[2];
		struct passwd *pwd = getpwnam( filter_username );
		if( pwd == NULL )
			exit(0);

		memset( username[user_len], 0, 1024 );
		userid[user_len] = pwd->pw_uid;
		strncpy( username[user_len], pwd->pw_name, 1024 - 1  );
		user_len++;

		filter_uid = pwd->pw_uid;
	}

	struct proc_list *list_head = NULL;
	struct proc_list *list_tail = NULL;
	struct proc_info *tmp_proc = NULL;

    //struct proc_info proc;

    DIR *dirp;
    struct dirent *dp;
    if( ( dirp = opendir( "/proc" ) ) == NULL )
        error( "read dir proc error" );
    
    while( ( dp = readdir( dirp ) ) != NULL )
    {   
        if( dp->d_type != DT_DIR )
            continue;

        if( !is_number( dp->d_name ) ) 
            continue;

		if( tmp_proc == NULL )
		{
			tmp_proc = (struct proc_info *)malloc( sizeof( struct proc_info ) );
			bzero( tmp_proc, sizeof( struct proc_info ) );
		}

		if( get_pid_info( dp->d_name, tmp_proc ) == False )
			continue;

		if( filter_username != NULL && tmp_proc->uid != filter_uid )
			continue;

		push_queue( &list_head, &list_tail, tmp_proc );
		tmp_proc = NULL;

		//tmp_proc->pid = atoi( dp->d_name );
		//get_pid_starttime_uid( dp->d_name, &tmp_proc );

        //print_proc_info( &proc );
    }   

    closedir( dirp );
	if( NULL != tmp_proc )
	{
		free( tmp_proc );
		tmp_proc = NULL;
	}

	sleep(1);

	////////////////////////////////////////////////////////////////////////////////
	struct proc_list *node = list_head;
	struct proc_list *next_node;
	struct proc_info *tmp = ( struct proc_info * )malloc( sizeof( struct proc_info ) );
	double uptime_diff;
	char str_proc_id[1024];
    long hterz = sysconf( _SC_CLK_TCK );
	while( node != NULL )
	{
		next_node = node->next;

		tmp_proc = node->proc;
		bzero( tmp, sizeof( struct proc_info ) );
		sprintf( str_proc_id, "%d\0", tmp_proc->pid );
		if( get_proc_cpu_info( str_proc_id, tmp ) == False )
		{
			remove_queue( &list_head, &list_tail, node );
		}
		else
		{
			uptime_diff = tmp->uptime - tmp_proc->uptime;
			if( 0 == uptime_diff )
				tmp_proc->cpu = 0.0f;
			else
			{
				tmp_proc->cpu = ( 100 * (double)(tmp->total_time - tmp_proc->total_time ) / hterz )/uptime_diff;
			}

			tmp_proc->uptime = tmp->uptime;
			tmp_proc->total_time = tmp->total_time;
		}

		node = next_node;
	}
	free( tmp );

	//print
	node = list_head;
    print_proc_info( NULL );
	while( node != NULL )
	{
		next_node = node->next;

		print_proc_info( node->proc );

		node = next_node;
	}

	//delete queue;
	destroy_proc_list( &list_head, &list_tail );

    exit(0);
}

int get_pid_starttime_uid( const char *pid, struct proc_info *proc )
{
    if( NULL == pid || strlen( pid ) == 0 || strlen( pid ) > 256 )
        return False;

    char path[1024] = { 0 };
    sprintf( path, "/proc/%s", pid );

    if( access( path, R_OK ) < 0  )
        return False;

    struct stat st = { 0 };
    stat( path, &st );

    proc->starttime = st.st_ctime;
    proc->uid = st.st_uid;
    return True;
}

int get_pid_cmdline( const char *pid, struct proc_info *proc )
{
    if( NULL == pid || strlen( pid ) == 0 || strlen( pid ) > 256 )
        return False;

    char path[1024] = { 0 };
    sprintf( path, "/proc/%s/cmdline", pid );

    if( access( path, R_OK ) < 0  )
        return False;

    int fd = open( path, O_RDONLY );
    if( fd < 0 )
        return False;

    int i, size;
    size = read( fd, proc->cmdline, sizeof( proc->cmdline ) );
    for( i = 0; i < size && i < 1024; i++ )
    {
        if( proc->cmdline[i] == '\0' )
            proc->cmdline[i] = ' ';
    }

    if( size > 128 )
        proc->cmdline[128] = '\0';

    close( fd );
    return True;
}

int get_proc_mem_info( const char *pid, struct proc_info *proc )
{
    if( NULL == pid || strlen( pid ) == 0 || strlen( pid ) > 256 )
        return False;

    char path[1024] = {0};
    sprintf( path, "/proc/%s/status", pid );

    if( access( path, R_OK ) < 0  )
        return False;

    FILE *fp = fopen( path, "r" );
    if( NULL == fp )
        return False;

    char line[4096] = {0};
    char tmp[4096] = {0};
    while( !feof( fp ) )
    {
        fgets( line, 4096, fp );

        if( start_with( line, "Name:" ) )
        {
            if( strlen( proc->cmdline ) != 0 )
                continue;
			memset( tmp, 0, 4096 );
            strncpy( tmp, line + strlen( "PPid:" ) + 1, strlen( line ) - strlen( "PPid:" ) - 2 );
            s_trim( tmp );
            sprintf( proc->cmdline, "[%s]\0", tmp );
        }
        else if( start_with( line, "PPid:" ) )
        {
			memset( tmp, 0, 4096 );
            strncpy( tmp, line + strlen( "PPid:" ) + 1, strlen( line ) - strlen( "PPid:" ) - 2 );
            s_trim( tmp );
            proc->ppid = atoi( tmp );
        }
        else if( start_with( line, "VmSize:" ) )
        {
			memset( tmp, 0, 4096 );
            strncpy( tmp, line + strlen( "VmSize:" ) + 1, 
                    strlen( line ) - strlen( "VmSize:" ) - strlen( " kB" ) - 2 );
            s_trim( tmp );
            proc->mem = atol( tmp );
        }
    }

    fclose( fp );
    return True;
}

int get_proc_cpu_info( const char *pid, struct proc_info *proc )
{
    if( NULL == pid || strlen( pid ) == 0 || strlen( pid ) > 256 )
        return False;

    char path[1024] = {0};
    sprintf( path, "/proc/%s/stat", pid );

    if( access( path, R_OK ) < 0  )
        return False;

    FILE *fp = fopen( path, "r" );
    if( NULL == fp )
        return False;

    char line[8192] = {0};
    fgets( line, 8192, fp );
    fclose( fp );

    //extract cpu info
    long utime, stime, starttime; // $14, $15, $22
    int cnt = 0;
    char tmp[1024];
    char *p1, *p2;

    p1 = line;
    while( ( p2 = strchr( p1, ' ' ) ) != NULL )
    {
        cnt++;
        if( cnt == 14 )
        {
			memset( tmp, 0, 1024 );
            strncpy( tmp, p1, p2 - p1 );
            utime = atol( tmp );
            //pro->time = atol( tmp ) / hterz;
            //break;
        }
        else if( cnt == 15 )
        {
			memset( tmp, 0, 1024 );
            strncpy( tmp, p1, p2 - p1 );
            stime = atol( tmp );
        }
        else if( cnt == 22 )
        {
			memset( tmp, 0, 1024 );
            strncpy( tmp, p1, p2 - p1 );
            starttime = atol( tmp );
            break;
        }

        p1 = p2 + 1;
    }

    //long hterz = sysconf( _SC_CLK_TCK );
	proc->total_time = utime + stime;
	proc->uptime = get_uptime();
    //proc->cpu = 100 * ( (double)( utime + stime ) ) / (double)starttime;
    //proc->cpu = 100 * ( (double)( utime + stime ) ) / (double) hterz;
    return True;
}

int get_pid_info( const char *pid, struct proc_info *proc )
{
    if( NULL == pid )
        return False;

	int ret = True;

    proc->pid = atoi( pid );
    ret &= get_pid_starttime_uid( pid, proc );
    ret &= get_pid_cmdline( pid, proc );
    ret &= get_proc_mem_info( pid, proc );
    ret &= get_proc_cpu_info( pid, proc );

    return ret;
}

int print_proc_info( struct proc_info *proc )
{
    if( NULL == proc ) //print head
    {
        printf( "user\tpid\tppid\tmem\tcpu\tstime\tcommand\n" );
        return False;
    }

    const char *username = get_uername( proc->uid );
    if( NULL == username )
        username = "    ";

    printf( "%s\t", username );
    printf( "% 5d\t", proc->pid );
    printf( "% 5d\t", proc->ppid );
    printf( "% 9ldKB\t", proc->mem );
    printf( "%.2lf\t", proc->cpu );

    struct tm *t = localtime( &proc->starttime );
    printf( "%02d/%02d %02d:%02d\t", t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min );

    printf( "%s\n", proc->cmdline );
    return True;
}

bool is_number( const char *str )
{
    if( NULL == str || !isdigit( *str ) || *str == '0' )
        return False;

    const char *p = str;
    while( *(++p) != '\0' )
    {
        if( !isdigit( *p ) )
            return False;
    }

    return True;
}

bool start_with( const char *str1, const char *str2 )
{
    if( NULL == str1 || NULL == str2 )
        return False;

    while( *(str1) != '\0' && *(str2) != '\0' 
            && *(str1++) == *(str2++) );

    if( *str2 == '\0' )
        return True;

    return False;
}

void s_trim( char *str )
{
    if( NULL == str )
        return;

    char *p1 = str;

    while( *p1 != '\0' )
    {
        if( isspace( *p1 ) )
            p1++;
        else
            break;
    }

    while( *p1 != ' ' && !isspace( *p1 ) )
    {
        *str++ = *p1++;
    }

    *str = '\0';
}

void error( const char *errmsg )
{
    perror( errmsg );
    exit( 1 );
}

const char *get_uername( uid_t uid )
{
    int i = 0;
    for( ; i < user_len; i++ )
    {
        if( userid[i] == uid )
            return username[i];
    }

    struct passwd *pwd = getpwuid( uid );
    if( NULL == pwd )
        return NULL;

    memset( username[user_len], 0, 1024 );
    strncpy( username[user_len], pwd->pw_name, 1024 - 1  );
    user_len++;

    return username[user_len-1];
}

void push_queue( struct proc_list **list_head, 
				 struct proc_list **list_tail, 
				 struct proc_info *proc )
{
	if( NULL == proc )
		return;

	struct proc_list *node = (struct proc_list *)malloc( sizeof( struct proc_list ) );
	node->proc = proc;
	node->next = NULL;

	if( NULL == *list_head )
		*list_head = node;

	if( NULL != *list_tail )
		(*list_tail)->next = node;

	*list_tail = node;
	return;
}

void remove_queue( struct proc_list **list_head, 
				   struct proc_list **list_tail, 
				   struct proc_list *node )
{
	if( NULL == node )
		return;

	if( node == *list_head )
	{
		*list_head = node->next;
		destroy_proc_list_node( node );
		return;
	}

	struct proc_list *tmp = *list_head;
	while( tmp->next != NULL )
	{
		if( tmp->next == node )
		{
			tmp->next = node->next;
			destroy_proc_list_node( node );
			if( *list_tail == node )
				*list_tail = tmp;
		}
	}
	return;
}

void destroy_proc_list_node( struct proc_list *node )
{
	if( NULL != node )
	{
		free( node->proc );
		free( node );
	}
}

void destroy_proc_list( struct proc_list **head, struct proc_list **tail )
{
	struct proc_list *node = *head;
	struct proc_list *node_next;
	while( node != NULL )
	{
		node_next = node->next;
		destroy_proc_list_node( node );
		node = node_next;
	}

	*head = *tail = NULL;
}


double get_uptime()
{
	static const char *path = "/proc/uptime";
	double uptime;

	FILE *fp = fopen( path, "r" );
	if( NULL == fp )
		return 0;

	fscanf( fp, "%lf %*lf", &uptime );
	fclose( fp );
	return uptime;

}
