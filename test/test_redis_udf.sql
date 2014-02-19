select redis_command_v2("127.0.0.1",6379,"lpush","crmInboxEvents11",json_object(json_members("op","insert","value","valuettt")));
select redis_servers_set_v2("127.0.0.1",6379);
select redis_command_v2("127.0.0.1",6379,"lpush","crmInboxEvents11",json_object(json_members("op","insert","value","valuettt")));
select free_resources();
select redis_servers_set_v2("127.0.0.1",6379);
