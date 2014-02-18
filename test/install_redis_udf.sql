DROP FUNCTION IF EXISTS redis_servers_set_v2;
DROP FUNCTION IF EXISTS redis_command_v2;
DROP FUNCTION IF EXISTS free_resources;

CREATE FUNCTION redis_servers_set_v2 RETURNS int SONAME "lib_mysqludf_redis_v2.so";
CREATE FUNCTION redis_command_v2 RETURNS int SONAME "lib_mysqludf_redis_v2.so";
CREATE FUNCTION free_resources RETURNS int SONAME "lib_mysqludf_redis_v2.so";
