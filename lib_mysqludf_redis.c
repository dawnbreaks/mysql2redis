/*
@author lubinwang
*/


#include <jemalloc/jemalloc.h>
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
#include <mysql/plugin.h>


//apache portable runtime
#include <apr-1/apu.h>
#include <apr-1/apr.h>
#include <apr-1/apr_queue.h>
#include <apr-1/apr_thread_pool.h>
#include <apr-1/apr_time.h>
#include <apr-1/apr_general.h>
/*
 * self-defined head file
 */
#include "utils.h"


//fixed me!



#ifdef HAVE_DLOPEN
#ifdef  __cplusplus
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


struct crm_redis_command{
	char cmd[256];
	char key[256];
	char* value;
	size_t length[3];
};

enum connection_type {
    CONN_TCP,
    CONN_UNIX
};

struct config {
    enum connection_type type;

    struct {
        char host[256];
        int port;
    } tcp;

    struct {
        const char *path;
    } unix1;
   char password[256];
   int auth;
   char log_file[256];
   int bdebug;
};

static struct config cfg = {
        .tcp = {
            .host = "192.168.0.118",
            .port = 6379
        },
        .unix1 = {
            .path = "/tmp/redis.sock"
        },
       .password = "go2oovoo",
       .auth = 0,
       .log_file = "/tmp/redis_udf.log",
       .bdebug = 0,
       .type = CONN_TCP
	};

/* Redis Connection pool creation mutex */
static pthread_mutex_t sredisContext_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Redis Connection pool object */
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

long long _do_redis_command(const char** args, const size_t * argvlen);



void check_error(apr_status_t rv);
void deinitialize_apr(void);
void initialize_apr(void);
void free_command(struct crm_redis_command *cmd);
void * APR_THREAD_FUNC consumer(apr_thread_t *thd, void *data);
int start_consumer_worker(void);



#ifdef	__cplusplus
}
#endif


#include <stdlib.h>
#include <string.h>

void split(char *str, char **splitstr) {
  char *p;
  int i=0;

	p = strtok(str,"\n");
	while(p!= NULL) {
		// printf("%s", p);
		splitstr[i] = malloc(strlen(p) + 1);
		if (splitstr[i])
			strcpy(splitstr[i], p);
		i++;
		p = strtok (NULL, ",");
		}
	}



void check_error(apr_status_t rv) {
    if (rv != APR_SUCCESS  ) {
        char buf[512], ebuf[256];
        sprintf(buf, "(%d): %s\n", rv, apr_strerror(rv, ebuf, sizeof ebuf));
        if (pFile)
    	{
            fprintf(pFile,buf);
        }
     }
}
//apr initialization and destroy.
void deinitialize_apr(void)
{
	apr_status_t rv;

	if(queue)
	{
		rv =apr_queue_term(queue);
		check_error(rv);
	}
	if(thrp)
	{
		apr_thread_pool_destroy(thrp);
		check_error(rv);
	}
	if(mem_pool)
	{
		apr_pool_destroy(mem_pool);
		check_error(rv);
	}

	//when destroy mem pool, queue will be automatically destroy.
//	if(queue)
//		apr_queue_destroy(queue);
	apr_terminate();

	queue = NULL;
	thrp = NULL;
	mem_pool = NULL;
}
void initialize_apr(void) {

	apr_status_t rv;
    apr_initialize();
    atexit(deinitialize_apr);


    //create mem pool / thread pool /queue.
    rv = apr_pool_create(&mem_pool, NULL);
    check_error(rv);
    rv = apr_thread_pool_create(&thrp, 1, 1, mem_pool);
    check_error(rv);
    rv = apr_queue_create(&queue, QUEUE_SIZE, mem_pool);
    check_error(rv);
}



void free_command(struct crm_redis_command *cmd)
{
	if(cmd)
	{
		  free(cmd->value);
		  free(cmd);
	}
	  //tp = apr_pcalloc(mem_pool, sizeof(apr_thread_pool_t));
	  //cmd = malloc(totlen+1);
}


void * APR_THREAD_FUNC consumer(apr_thread_t *thd, void *data)
{
    apr_status_t rv;
    struct crm_redis_command *command = NULL;

	if (pFile)
	{
		fprintf(pFile,"enter: consumer thread ");
		fflush(pFile);
	}
    while (1)
    {
        rv = apr_queue_pop(queue, (void **)&command);

        if (rv == APR_EINTR)
            continue;

        if (rv == APR_EOF)
        {
    		if (pFile)
    		{
    			fprintf(pFile,"queue has terminated, consumer thread exit");
    			fflush(pFile);
    		}
        	break;
        }

        if (rv != APR_SUCCESS  ) {
        	apr_sleep(1000* 1000); //sleep 1 second.
        	continue;
        }

        if(command)
        {
        	char *argv[3]; // = { "rpush %s %s", "TEST", "yippie" }
        	argv[0] = command->cmd;//cmd
        	argv[1] = command->key;//para1
            argv[2] = command->value;//para2

            int res = _do_redis_command((const char **)argv,(const size_t *)command->length);

            //cfg.bdebug
        	if (cfg.bdebug && pFile)
    		{
    			fprintf(pFile,"consumer, _do_redis_command:res= %d\n",res);
    			fflush(pFile);
    		}

            free_command(command);
            command = NULL;
        }
//        apr_sleep(500* 1000); /* sleep random seconds */
    }
	if (pFile)
	{
		fprintf(pFile,"exit:consumer thread");
		fflush(pFile);
	}
    return NULL;
}

int start_consumer_worker(void) {
	apr_status_t rv = -1;
	if(!mem_pool && !thrp)
	{
		initialize_apr();

		rv = apr_thread_pool_push(thrp, consumer, NULL, 0, NULL);
		check_error(rv);

		if (pFile)
		{
			fprintf(pFile,"start_consumer_worker: initialize_apr,apr_thread_pool_push|rv= %d\n",rv);
			fflush(pFile);
		}
	}
}





/* Internal Helper Functions */
redisContext *_redis_context_init()
{
  //  pthread_mutex_lock(&sredisContext_mutex);

    if (!sredisContext)
    {
		sredisContext = _myredisConnect(cfg);
		if (pFile)
		{
			fprintf(pFile,"try to connect to redis db ");
			fflush(pFile);
		}	
	}
    if (!sredisContext)
    {
   //    pthread_mutex_unlock(&sredisContext_mutex);
       return NULL;
    }
   // pthread_mutex_unlock(&sredisContext_mutex);
    return sredisContext;
}

/* Internal Helper Functions */
redisContext *_redis_context_reinit()
{
   // pthread_mutex_lock(&sredisContext_mutex);

    if (!sredisContext)
    {
    	sredisContext = _myredisConnect(cfg);
    }
    else
    {
	    if (pFile)
		{
			fprintf(pFile,"try to reconnect to redis db ");
			fflush(pFile);
		}
    	redisFree(sredisContext);
    	sredisContext = NULL;
    	sredisContext = _myredisConnect(cfg);
    }

    if (!sredisContext)
    {
 //      pthread_mutex_unlock(&sredisContext_mutex);
       return NULL;
    }
   // pthread_mutex_unlock(&sredisContext_mutex);
    return sredisContext;
}
	
void _redis_context_deinit()
{
   // pthread_mutex_lock(&sredisContext_mutex);
    if (sredisContext)
      _myredisDconnect(sredisContext);
    sredisContext = NULL;
 //  pthread_mutex_unlock(&sredisContext_mutex);
    return;
}
	
void _myredisDconnect(redisContext *c)
{
    /* Free the context */
    redisFree(c);
}

redisContext *_myredisConnect(struct config config)
{
    redisContext *c = NULL;
    redisReply *reply;
    char cmd[256];
    int len;

   if (config.type == CONN_TCP) {
        c = redisConnect(config.tcp.host, config.tcp.port);
    } else if (config.type == CONN_UNIX) {
        c = redisConnectUnix(config.unix1.path);
    } else {
        assert(NULL);
    }

    if (c->err && pFile) 
	{
        fprintf(pFile,"Connection error: %s\n", c->errstr);
        fflush(pFile);
		return c;
    }


    /* Authenticate */
    if (config.auth)
    {
       reply = redisCommand(c,"AUTH %s",config.password);
	   if(reply)
       {	
			freeReplyObject(reply);
	   }
    }
    return c;
//    return select_database(c);
}





long long _do_redis_command( const char ** args,const size_t * argvlen)
{


	pthread_mutex_lock(&sredisContext_mutex);

    redisContext *c = NULL;
    redisReply *reply;

   // *is_null = 0;
    //*error = 0;

    c = (redisContext*)_redis_context_init();
    if (!c)
    {
		if (pFile)
		{	
			fprintf(pFile,"_redis_context_init return null, connect failed\n");
			fflush(pFile);
		}
		pthread_mutex_unlock(&sredisContext_mutex);
		return -1;
    }
    if (cfg.bdebug && pFile)
    {
		fprintf(pFile,"%s %s %s\n",args[0] ,args[1],args[2]);
    }
//    reply = redisCommand(c,"%s %s %s",cmd , args->args[0] ,args->args[1]);

 /*   char * commandArgs[3];
    commandArgs[0] =  args[0];
    commandArgs[1] =  args[1];
    commandArgs[2] =  args[2];*/
    reply =  redisCommandArgv(c,3,args,argvlen);
    if(!reply)
    {

    	c = (redisContext*)_redis_context_reinit();
        if (!c)
        {
		  if (pFile)
          {
			  fprintf(pFile,"_do_redis_command, Cannot reconnect to redis\n ");
			  fflush(pFile);
          }
		  pthread_mutex_unlock(&sredisContext_mutex);
          return -1;
        }
    	reply = redisCommandArgv(c,3,args,argvlen);
        if (!reply)
        {
          if (pFile)
          {
			fprintf(pFile,"_do_redis_command, reconnect to redis and re-execute redisCommandArgv failed\n ");
			fflush(pFile);
          }
          pthread_mutex_unlock(&sredisContext_mutex);
          return -1;
        }
    }
//    test_cond(reply->type == REDIS_REPLY_INTEGER && reply->integer == 1)
	if(reply)
    {
		freeReplyObject(reply);
	}
	pthread_mutex_unlock(&sredisContext_mutex);
    return 0;
}



/*	
set server info.
*/
my_bool redis_servers_set_v2_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{

	pthread_mutex_lock(&sredisContext_mutex);
	
    redisContext *c = NULL;
   if (args->arg_count < 2 || args->arg_type[0] != STRING_RESULT || args->arg_type[1] != INT_RESULT)
   {
     strncpy(message,"Wrong arguments to Redis function.  Usage: 'tcp.host' (string) 'tcp.port' (string)", MYSQL_ERRMSG_SIZE);
	 pthread_mutex_unlock(&sredisContext_mutex);
     return -1;
   }

   if (args->arg_count == 3 && args->arg_type[2] != STRING_RESULT)
   {
     strncpy(message,"Wrong arguments to Redis function1.  Usage: 'password' (string)", MYSQL_ERRMSG_SIZE);
	 pthread_mutex_unlock(&sredisContext_mutex);
     return -2;
   }

   strncpy(cfg.tcp.host, (char*)args->args[0], 256);
   cfg.tcp.port = *((longlong*)args->args[1]);
   if (args->arg_count == 3)
   {
      cfg.auth = 1;
      strncpy(cfg.password, (char*)args->args[2], 256);
   }
    _redis_context_deinit();
    c = (redisContext*)_redis_context_init();;
    if (!c)
    {
      strncpy(message, "Failed to connect to Redis", MYSQL_ERRMSG_SIZE);
	  pthread_mutex_unlock(&sredisContext_mutex);
      return 2;
    }
	
	//open log file
	pFile = fopen(cfg.log_file,"a");
	if(pFile)
	{
		fprintf(pFile,"redis_servers_set_v2_init done.\n ");
	}

	//start consumer thread.
	start_consumer_worker();
	pthread_mutex_unlock(&sredisContext_mutex);
   return 0;
}

void redis_servers_set_v2_deinit(UDF_INIT *initid)
{
    if (cfg.bdebug&&pFile)
    {
		fprintf(pFile,"redis_servers_set_v2_deinit\n ");
		fflush(pFile);
    }
}

my_ulonglong redis_servers_set_v2(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error)
{
   return 0;
}


/**
 * redis_command implementation
 *
 * redis_command(host,port,cmd,para1,para1)
 *
 */
my_bool redis_command_v2_init(
	UDF_INIT *initid __attribute__((__unused__)),
	UDF_ARGS *args,
	char *message
){
	if(
		args->arg_type[0]==STRING_RESULT &&
		args->arg_type[1]==INT_RESULT &&
		args->arg_type[2]==STRING_RESULT
	  )
	{

		args->maybe_null = 0; // each parameter could not be NULL

		char *host = args->args[0];
		unsigned long host_len = args->lengths[0];
		long long port = *((long long*)args->args[1]);

		char *cmd = args->args[2];
		if(!check_host(host) || !check_ip(host))
		{
			snprintf(message,MYSQL_ERRMSG_SIZE,	"The first parameter is not a valid host or ip address");
			return 2;
		}

		if(port < 0)
		{
			snprintf(message,MYSQL_ERRMSG_SIZE,
				"The second parameter must be an integer bigger than zero");
			return 2;
		}

		if(strlen(cmd) <=0 || NULL == cmd)
		{
			snprintf(message,MYSQL_ERRMSG_SIZE,"The third parameter error,[%s]\n",cmd);
			return 2;
		}
		// everthing looks OK.
		return 0;
		
	} else
	{
		snprintf(message,MYSQL_ERRMSG_SIZE,	"redis_command(host,port,command1,command2,..) Expected exactly 3+ parameteres, a string, an integer and a string(s)" );
		return 1;
	}

}
void redis_command_v2_deinit(UDF_INIT *initid __attribute__((__unused__))){
    if (cfg.bdebug&&pFile)
    {
		fprintf(pFile,"redis_command_v2_deinit\n ");
		fflush(pFile);
    }
}


my_ulonglong redis_command_v2(
		UDF_INIT *initid __attribute__((__unused__)),
		UDF_ARGS *args,
		char *is_null __attribute__((__unused__)),
		char *error __attribute__((__unused__)))
{

	//__unused__   paras
	char *hostorfile = args->args[0];
	long long port = *((long long*)args->args[1]);

	
	apr_status_t status;

	if(queue)
	{
//		apr_status_t status;
		struct crm_redis_command* command = malloc(sizeof(struct crm_redis_command));
		command->value = malloc(args->lengths[4]);//allocate value mem.

		memcpy(command->cmd,args->args[2],args->lengths[2]);//cmd
		memcpy(command->key,args->args[3],args->lengths[3]);//key
		memcpy(command->value,args->args[4],args->lengths[4]);//value
		command->length[0] = args->lengths[2];
		command->length[1] = args->lengths[3];
		command->length[2] = args->lengths[4];

		apr_status_t status = apr_queue_trypush(queue, (void*)command);
		check_error(status);
	}

    if (!queue || status != APR_SUCCESS  ) {   //send event directly.

    	char *argv[3]; // = { "rpush %s %s", "TEST", "yippie" }
    	argv[0] =  args->args[2];//cmd
    	argv[1] =  args->args[3];//key
        argv[2] =  args->args[4];//value

    	size_t argvlen[3];
    	argvlen[0] = args->lengths[2];
    	argvlen[1] = args->lengths[3];
    	argvlen[2] = args->lengths[4];

    	return _do_redis_command((const char **)argv,(const size_t *)argvlen);

     }
	return 0;
}




DLLEXP
my_bool free_resources_init(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *message
)
{
	return 0;
}

DLLEXP
void free_resources_deinit(
	UDF_INIT *initid
)
{

}

DLLEXP
my_ulonglong free_resources(
	UDF_INIT *initid
,	UDF_ARGS *args
,	char *is_null
,	char *error
)
{
	deinitialize_apr();
	return 0;
}

#endif /* HAVE_DLOPEN */
