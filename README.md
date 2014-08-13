mysql2redis
===========

    Redis udf to sync newly modified data from mysql to redis cache.



   When you use redis to cache data only(not persistent), you will face the issue that how to sync the newly modified data from mysql to the redis cache? This project aims to provide a high performance and stable redis udf to solve this issue.
   
## Dependencies
   please download the dependencies below and compile/install it properly :
 
  * apr-1.4.6(http://apr.apache.org/download.cgi)
  * apr-util-1.5.2(http://apr.apache.org/download.cgi)
  * hiredis(https://github.com/redis/hiredis)
  * lib_mysqludf_json(https://github.com/mysqludf/lib_mysqludf_json)
   
   
## Compile  
   run  make from the src dir directly.
  ```
  make
  ```
   
## Install redis udf  
  please make sure that  the lib_mysqludf_redis_v2.so has been put into the mysql plugin dir. By the way, you can examine where is the mysql plugin dir by run '''
  mysql_config  --plugindir
  '''. and then connect to your mysql server, run the following command to install the the redis udf.
  ```sql
DROP FUNCTION IF EXISTS redis_servers_set_v2;
DROP FUNCTION IF EXISTS redis_command_v2;
DROP FUNCTION IF EXISTS free_resources;

CREATE FUNCTION redis_servers_set_v2 RETURNS int SONAME "lib_mysqludf_redis_v2.so";
CREATE FUNCTION redis_command_v2 RETURNS int SONAME "lib_mysqludf_redis_v2.so";
CREATE FUNCTION free_resources RETURNS int SONAME "lib_mysqludf_redis_v2.so";
  ```
  
## Test redis udf  
   connect to your mysql server, run the following command to test the the redis udf.
```sql
select redis_command_v2("lpush","crmInboxEvents11",json_object(json_members("op","insert","value","valuettt")));

select redis_servers_set_v2("192.168.0.118",6379);
select redis_command_v2("lpush","crmInboxEvents11",json_object(json_members("op","insert","value","valuettt")));

select redis_command_v2("hset","hkey","hfield",json_object(json_members("op","insert","value","valuettt")));


select free_resources();
select redis_servers_set_v2("192.168.0.118",6379);
```

## What's more
   you should create a trigger which will lpush the newly modified data to redis list juste as the following  example:
   ```sql
DELIMITER $$
CREATE TRIGGER insert_trigger AFTER INSERT ON email_folder
  FOR EACH ROW BEGIN
    SET @ret=
     	 redis_command_v2("lpush","crmInboxEvents",
							  json_object
                              (
							    	json_members
									(
											"op",
											"insert",
											"value",
											json_object
											(
												NEW.Id as "id",NEW.type as "type",
												NEW.mailserver_id as "mailserverId",NEW.sender as "sender",
												NEW.sender_name as "senderName",NEW.recevier as "recevier",
												NEW.replyto as "replyto",NEW.bbemails as "bbemails",
												NEW.ccemails as "ccemails",NEW.subject as "subject"
                                            )
										)
						    	)
					      );
  END$$
DELIMITER ;
   ```
   
## Finally
   Now all update events for that mysql table has been transfer to a redis queue.  You need to start a thread(or process) to handle these update events and update the redis cache properly. 
  For example, you can  start a java process to poll the redis queue , and pop the events,  then handle it and update the redis cache properly.

   
## Performance test
   In my poor labtop, 10,000 row data were synced into redis in 0.4 second.
   
   
========
Please feel free to contact me(2005dawnbreaks@gmail.com) if you have any questions.
