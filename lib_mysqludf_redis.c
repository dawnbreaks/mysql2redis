/*
@author lubinwang
*/

#include "lib_mysqludf_redis.h"

static struct config cfg = {
    .tcp = {
	.host = "192.168.0.118",
	.port = 6379
    },
    .unix_sock = {
    	.path = "/tmp/redis.sock"
     },
    .password = "go2oovoo",
    .auth = 0,
    .log_file = "/tmp/redis_udf.log",
    .debug = 0,
    .type = CONN_TCP
};



void split(char *str, char **splitstr) {

    char *p;
    int i = 0;

    p = strtok(str, "\n");
    while (p != NULL) {
		// printf("%s", p);
	splitstr[i] = malloc(strlen(p) + 1);
	if (splitstr[i])
	    strcpy(splitstr[i], p);
	i++;
	p = strtok(NULL, ",");
    }
}



void check_error(apr_status_t rv) {
    if (rv != APR_SUCCESS) {
	char buf[512], ebuf[256];
	sprintf(buf, "(%d): %s\n", rv, apr_strerror(rv, ebuf, sizeof ebuf));
	debug_print(buf);
    }
}
//apr initialization and destroy.
void deinitialize_apr(void) {
    apr_status_t rv;
    if(queue) {
	rv =apr_queue_term(queue);
	check_error(rv);
    }
    if(thrp) {
    	apr_thread_pool_destroy(thrp);
	check_error(rv);
    }
    if(mem_pool) {
	apr_pool_destroy(mem_pool);
	check_error(rv);
    }


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



void free_command(struct redis_command *cmd) {
    if(cmd) {
    	int i=0;
    	for(;i<cmd->arg_count;i++){
    		free(cmd->argv[i]);
    	}
    	free(cmd->argv);
    	free(cmd->argvlen);
		free(cmd);
    }
}


void * APR_THREAD_FUNC consumer(apr_thread_t *thd, void *data) {
    apr_status_t rv;
    struct redis_command *command = NULL;

    info_print("enter: consumer thread\n");

    while (1) {
        rv = apr_queue_pop(queue, (void **)&command);

        if (rv == APR_EINTR)
            continue;

        if (rv == APR_EOF) {
    	    info_print("queue has terminated, consumer thread exit\n");
            break;
        }

        if (rv != APR_SUCCESS  ) {
            apr_sleep(1000* 1000); //sleep 1 second.
            continue;
        }

        if(command) {
            int res = _do_redis_command((const char **)command->argv,(const size_t *)command->argvlen, command->arg_count);
            free_command(command);
            command = NULL;
        }
    }

    info_print("exit:consumer thread\n");
    return NULL;
}

int start_consumer_worker(void) {
    apr_status_t rv = -1;
    if(!mem_pool && !thrp) {
    	initialize_apr();

    	rv = apr_thread_pool_push(thrp, consumer, NULL, 0, NULL);
    	check_error(rv);

    	info_print("start_consumer_worker: initialize_apr,apr_thread_pool_push|rv= %d\n",rv);
    }
}





/* Internal Helper Functions */
redisContext *_redis_context_init() {
    if (!sredisContext) {
	sredisContext = _myredisConnect(cfg);
	info_print("try to connect to redis db \n");
    }
    if (!sredisContext) {
       return NULL;
    }
    return sredisContext;
}

/* Internal Helper Functions */
redisContext *_redis_context_reinit() {
    if (!sredisContext) {
    	sredisContext = _myredisConnect(cfg);
    }
    else {
    	info_print("try to reconnect to redis db \n");

    	redisFree(sredisContext);
    	sredisContext = NULL;
    	sredisContext = _myredisConnect(cfg);
    }

    return sredisContext;
}
	
void _redis_context_deinit() {
    if (sredisContext)
      _myredisDconnect(sredisContext);
    sredisContext = NULL;
    return;
}
	
void _myredisDconnect(redisContext *c) {
    redisFree(c);
}

redisContext *_myredisConnect(struct config config) {
    redisContext *c = NULL;
    redisReply *reply;
    char cmd[256];
    int len;

   if (config.type == CONN_TCP) {
        c = redisConnect(config.tcp.host, config.tcp.port);
    } else if (config.type == CONN_UNIX_SOCK) {
        c = redisConnectUnix(config.unix_sock.path);
    } else {
        assert(NULL);
    }

    if (c->err && pFile) {
    	info_print("Connection error: %s\n", c->errstr);
	return c;
    }


    /* Authenticate */
    if (config.auth) {
        reply = redisCommand(c,"AUTH %s",config.password);
        if(reply) {
            freeReplyObject(reply);
	}
    }
    return c;
}





long long _do_redis_command( const char ** args,const size_t * argvlen, size_t arg_count) {

    pthread_mutex_lock(&sredisContext_mutex);

    redisContext *c = NULL;
    redisReply *reply;


    c = (redisContext*)_redis_context_init();
    if (!c) {
	info_print("_redis_context_init return null, connect failed\n");
	pthread_mutex_unlock(&sredisContext_mutex);
	return -1;
    }

    debug_print("%s %s %s\n",args[0] ,args[1],args[2]);

    reply =  redisCommandArgv(c,arg_count,args,argvlen);
    if(!reply) {

    	c = (redisContext*)_redis_context_reinit();
        if (!c) {
            info_print("_do_redis_command, Cannot reconnect to redis\n ");
            pthread_mutex_unlock(&sredisContext_mutex);
            return -1;
        }
    	reply = redisCommandArgv(c,arg_count,args,argvlen);
        if (!reply) {
            info_print("_do_redis_command, reconnect to redis and re-execute redisCommandArgv failed\n ");
            pthread_mutex_unlock(&sredisContext_mutex);
            return -1;
        }
    }

	if(reply) {
	    freeReplyObject(reply);
	}
	pthread_mutex_unlock(&sredisContext_mutex);
    return 0;
}



/*	
set server info.
*/
my_bool redis_servers_set_v2_init(UDF_INIT *initid, UDF_ARGS *args, char *message) {

    pthread_mutex_lock(&sredisContext_mutex);
	
   
    redisContext *c = NULL;
    if(args->arg_count < 2 || args->arg_type[0] != STRING_RESULT || args->arg_type[1] != INT_RESULT) {
         strncpy(message,"Wrong arguments to Redis function.  Usage: 'tcp.host' (string) 'tcp.port' (string)", MYSQL_ERRMSG_SIZE);
	 pthread_mutex_unlock(&sredisContext_mutex);
         return -1;
    }

    if(args->arg_count == 3 && args->arg_type[2] != STRING_RESULT) {
        strncpy(message,"Wrong arguments to Redis function1.  Usage: 'password' (string)", MYSQL_ERRMSG_SIZE);
        pthread_mutex_unlock(&sredisContext_mutex);
        return -2;
    }

    strncpy(cfg.tcp.host, (char*)args->args[0], 256);
    cfg.tcp.port = *((longlong*)args->args[1]);
    if (args->arg_count == 3) {
       cfg.auth = 1;
       strncpy(cfg.password, (char*)args->args[2], 256);
    }
     _redis_context_deinit();
     c = (redisContext*)_redis_context_init();;
    if (!c) {
          strncpy(message, "Failed to connect to Redis", MYSQL_ERRMSG_SIZE);
	  pthread_mutex_unlock(&sredisContext_mutex);
      return 2;
    }
	
	//open log file
    pFile = fopen(cfg.log_file,"a");

	//start consumer thread.
    start_consumer_worker();
    pthread_mutex_unlock(&sredisContext_mutex);

    info_print("redis_servers_set_v2_init done.\n ");
    return 0;
}

void redis_servers_set_v2_deinit(UDF_INIT *initid) {
	debug_print("redis_servers_set_v2_deinit\n ");
}

my_ulonglong redis_servers_set_v2(UDF_INIT *initid, UDF_ARGS *args, char *is_null, char *error) {
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
    args->arg_count>=3 &&
	args->arg_type[0]==STRING_RESULT &&
	args->arg_type[1]==STRING_RESULT &&
	args->arg_type[2]==STRING_RESULT
    )
    {

    	args->maybe_null = 0; // each parameter could not be NULL

    	// everthing looks OK.
    	return 0;
    } else {
    	snprintf(message,MYSQL_ERRMSG_SIZE,	"redis_command(cmd,arg1,arg2,..) Expected exactly 3+ string parameteres" );
    	return 1;
    }

}
void redis_command_v2_deinit(UDF_INIT *initid __attribute__((__unused__))){
    debug_print("redis_command_v2_deinit\n ");
}


my_ulonglong redis_command_v2(
    UDF_INIT *initid __attribute__((__unused__)),
    UDF_ARGS *args,
    char *is_null __attribute__((__unused__)),
    char *error __attribute__((__unused__)))
{
    apr_status_t status;
    
	struct redis_command* command = malloc(sizeof(struct redis_command));
	command->argv = malloc(args->arg_count*sizeof(void*));//allocate argv mem.
	command->argvlen = malloc(args->arg_count*sizeof(size_t));//allocate argvlen mem.
	command->arg_count = args->arg_count;

//	info_print("NUMBER OF ARGS %d\n",args->arg_count);

	int i=0;
	for(;i<args->arg_count;i++){
		command->argv[i] = malloc(args->lengths[i]);
		memcpy(command->argv[i],args->args[i],args->lengths[i]);
		command->argvlen[i] = args->lengths[i];
	}

    if(queue) {
    	apr_status_t status = apr_queue_trypush(queue, (void*)command);
    	check_error(status);
    }

    if (!queue || status != APR_SUCCESS  ) {   //send event directly.

    	long long res = _do_redis_command((const char **)command->argv,(const size_t *)command->argvlen, command->arg_count);
    	free_command(command);
    	return res;
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

