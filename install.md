mysql2redis
==========

## 
## INSTALL DEPS
* sudo apt-get install libjemalloc1 libjemalloc-dev
  or source download install
* install apr: 1. download 2. ./configure 3. make 4. make test 5. make
  install
* install apr-util: 1. download 2.  ./configure
  --with-apr=/usr/local/apr 3. make 4. make install
* install lib_mysqludf_json
  1. download: wget https://github.com/mysqludf/lib_mysqludf_json
  2. copy lib_mysqludf_json.so to /usr/lib/mysql/plugin
* install hiredis
  1. download: git clone https://github.com/redis/hiredis.git
  2. cd hiredis;
  3. make
  4. sudo make install #or copy libhiredis.so in mysql plugin dir
* configure Makefile
  1. modify its format.
  2. modify mysqlplugindir i.e. PLUGINDIR
* install mysql2redis:
  1. make (note QA)


## QA
  
1.  /usr/local/apr/include/apr-1/apr.h:358:1: error: unknown type name
  ‘off64_t’
  * sudo vi /usr/local/apr/include/apr-1/apr.h in line 358
  * add line       typedef long    off64_t;
  before         typedef  off64_t           apr_off_t;
  
2. lib_mysqludf_redis.c:271:13: error: format not a string literal and
   no format arguments [-Werror=format-security]
   * vi lib_mysqludf_redis.c  in line 271
   * /*fprintf(pFile,buf)*/
   fputs(buf, pFile);

3. install hiredis
   * make: Nothing to be done for `all'.
   * sudo make install

## APPEND
1. vagrant@precise32 ~/mysql2redis
 % make
gcc -Werror -O2 -g `/usr/bin/mysql_config --include` -I/usr/local/include  -I/usr/local/apr/include  -I. -fPIC -shared -rdynamic lib_mysqludf_redis.c utils.c\
		-lhiredis -L"/usr/lib64/mysql/plugin"  -L/usr/local/apr/lib  -lapr-1  -laprutil-1 -ljemalloc -o "/usr/lib64/mysql/plugin"/lib_mysqludf_redis_v2.so
In file included from lib_mysqludf_redis.c:50:0:
/usr/local/apr/include/apr-1/apr.h:358:1: error: unknown type name ‘off64_t’
lib_mysqludf_redis.c: In function ‘check_error’:
lib_mysqludf_redis.c:271:13: error: format not a string literal and no format arguments [-Werror=format-security]
cc1: all warnings being treated as errors
make: *** [linux] Error 1

2.  % make
gcc -Werror -O2 -g `/usr/bin/mysql_config --include` -I/usr/local/include  -I/usr/local/apr/include  -I. -fPIC -shared -rdynamic lib_mysqludf_redis.c utils.c\
		-lhiredis -L"/usr/lib64/mysql/plugin"  -L/usr/local/apr/lib  -lapr-1  -laprutil-1 -ljemalloc -o "/usr/lib64/mysql/plugin"/lib_mysqludf_redis_v2.so
lib_mysqludf_redis.c: In function ‘check_error’:
lib_mysqludf_redis.c:271:13: error: format not a string literal and no format arguments [-Werror=format-security]
cc1: all warnings being treated as errors
make: *** [linux] Error 1

3. vagrant@precise32 ~/sw/hiredis
 % sudo make install
mkdir -p /usr/local/include/hiredis /usr/local/lib
cp -a hiredis.h async.h adapters /usr/local/include/hiredis
cp -a libhiredis.so /usr/local/lib/libhiredis.so.0.10
cd /usr/local/lib && ln -sf libhiredis.so.0.10 libhiredis.so.0
cd /usr/local/lib && ln -sf libhiredis.so.0 libhiredis.so
cp -a libhiredis.a /usr/local/lib

4. mysql> CREATE FUNCTION redis_servers_set_v2 RETURNS int SONAME "lib_mysqludf_redis_v2.so";
ERROR 1126 (HY000): Can't open shared library
'lib_mysqludf_redis_v2.so' (errno: 0 libhiredis.so.0.10: cannot open
shared object file: No such file or directory)
 * copy libhiredis.so.0.10 to /lib and libexpat.so to /lib
 * or check  echo $LD_LIBRARY_PATH
 * ref http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html
 * follow error is the same:
  1.  vagrant@precise32 ~ % mysql -u root -p < mysql_cmd.sql
  2.  Enter password: 
  3.  ERROR 1126 (HY000) at line 5: Can't open shared library 'lib_mysqludf_redis_v2.so' (errno: 0 libexpat.so.0: cannot open shared object file: No such file or directory)

 
5. % sudo make
gcc -Werror -O2 -g `/usr/bin/mysql_config --include` -I/usr/local/include  -I/usr/local/apr/include  -I. -fPIC -shared -rdynamic lib_mysqludf_redis.c utils.c\
:		-lhiredis -L"/usr/lib/mysql/plugin"  -L/usr/local/apr/lib
        -lapr-1  -laprutil-1 -ljemalloc -o
        "/usr/lib/mysql/plugin"/lib_mysqludf_redis_v2.so
 /usr/local/apr/lib
 /usr/lib
 ldd /usr/lib/mysql/plugin/lib_mysqludf_redis_v2.so
