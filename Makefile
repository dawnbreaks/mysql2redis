PLUGINDIR = "/usr/lib64/mysql/plugin"
INCLUDE=`/usr/bin/mysql_config --include` -I/usr/local/include  -I/usr/local/apr/include
LIBS=-lhiredis -L$(PLUGINDIR)  -L/usr/local/apr/lib  -lapr-1  -laprutil-1 -ljemalloc

linux:
	gcc -Werror -O2 -g $(INCLUDE)  -I. -fPIC -shared -rdynamic lib_mysqludf_redis.c utils.c\
		$(LIBS) -o ${PLUGINDIR}/lib_mysqludf_redis_v2.so
		
uninstall:
	rm -f $(PLUGINDIR)/lib_mysqludf_redis_v2.so

clean:
	rm -f *~
	rm -f *.so
	rm -f *.out
