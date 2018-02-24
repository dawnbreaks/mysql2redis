
#include <pthread.h>
#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(WIN32)
#define DLLEXP __declspec(dllexport) 
#else
#define DLLEXP
#endif

#ifdef STANDARD
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef __WIN__
typedef unsigned __int64 ulonglong;
typedef __int64 longlong;
#else
typedef unsigned long long ulonglong;
typedef long long longlong;
#endif /*__WIN__*/
#else
#include <my_global.h>
#include <my_sys.h>
#endif
#include <mysql.h>
#include <m_ctype.h>
#include <m_string.h>
#include <stdlib.h>

#include <ctype.h>
#include <stdarg.h>
#include <unistd.h>
/**
 * hiredis head file
 */
#include <hiredis/hiredis.h>

/**
 * mysql head file
 */
#define MYSQL_ABI_CHECK
#include <mysql/plugin.h>


//apache portable runtime
#include <apr-1/apu.h>
#include <apr-1/apr.h>
#include <apr-1/apr_queue.h>
#include <apr-1/apr_thread_pool.h>
#include <apr-1/apr_time.h>
#include <apr-1/apr_general.h>

//fix me!
#ifdef	__cplusplus
extern "C" {
#endif

#define LIBVERSION "lib_mysqludf_redis version 0.2.0"
#define MAX_LEN 8192 // max allowed length of input string, including 0 terminator
#define MAX_STR 16 // max allowed number of substrings in input string

#ifdef __WIN__
#define SETENV(name,value)		SetEnvironmentVariable(name,value);
#else
#define SETENV(name,value)		setenv(name,value,1);		
#endif


struct redis_command{
	size_t arg_count;		/* Number of arguments */
	char **argv;				/* Pointer to argument */
	size_t *argvlen;			/* lengths of arguments */
};

enum conn_type {
    CONN_TCP,
    CONN_UNIX_SOCK
};

struct config {
    enum conn_type type;

    struct {
        char host[256];
        int port;
    } tcp;

    struct {
        const char *path;
    } unix_sock;
   char password[256];
   int auth;
   char log_file[256];
   int debug;
};


/* Redis connection mutex */
static pthread_mutex_t sredisContext_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Redis Connection context */
static redisContext *sredisContext = NULL;


static FILE * pFile = NULL;


#define QUEUE_SIZE  100000

static apr_queue_t *queue = NULL;

static apr_pool_t *mem_pool = NULL;

static apr_thread_pool_t *thrp = NULL;


/**
set server info command.
 */
DLLEXP 
my_bool redis_servers_set_v2_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
);

DLLEXP 
void redis_servers_set_v2_deinit(
	UDF_INIT *initid
);

DLLEXP 
my_ulonglong redis_servers_set_v2(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *is_null
,	char *error
);


/**
 * redis_command
 * 
 * execute multiple redis command (ex: select 1\n set x 1\n)
 */
DLLEXP 
my_bool redis_command_v2_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
);

DLLEXP 
void redis_command_v2_deinit(
	UDF_INIT *initid
);

DLLEXP 
my_ulonglong redis_command_v2(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *is_null
,	char *error
);



/**
free resources.
 */
DLLEXP
my_bool free_resources_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
);

DLLEXP
void free_resources_deinit(
	UDF_INIT *initid
);

DLLEXP
my_ulonglong free_resources(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *is_null
,	char *error
);


/* Helpers */
redisContext *_myredisConnect(struct config config);
void _myredisDconnect(redisContext *c);

redisContext *_redis_context_init();
redisContext *_redis_context_reinit();
void _redis_context_deinit();

long long _do_redis_command(const char** args, const size_t * argvlen, size_t arg_count);



void check_error(apr_status_t rv);
void deinitialize_apr(void);
void initialize_apr(void);
void free_command(struct redis_command *cmd);
void * APR_THREAD_FUNC consumer(apr_thread_t *thd, void *data);
int start_consumer_worker(void);





#define debug_print(...) \
   do { \
	   if (cfg.debug && pFile) \
	   	   fprintf(pFile,  __VA_ARGS__); \
   } while (0)


#define info_print(...) \
   do { \
	   if (pFile) \
	   	   fprintf(pFile,  __VA_ARGS__); \
   } while (0)



#ifdef	__cplusplus
}
#endif

