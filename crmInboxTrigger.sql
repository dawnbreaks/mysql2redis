DELIMITER $$
CREATE TRIGGER inbox_insert_trigger AFTER INSERT ON crm.sys_email_folder
  FOR EACH ROW BEGIN
    SET @ret=
 		 redis_commands("192.168.0.118",6379,"rpush","crmInboxEvents",
												json_object(
																			json_members
																			(
																				"op",
																				"update",
																				"value",
																				json_object
																				(
																					NEW.Id as "Id",NEW.type as "type",
																					NEW.mailserver_id as "mailserver_id",NEW.sender as "sender",
																					NEW.sender_name as "sender_name",NEW.recevier as "recevier",
																					NEW.replyto as "replyto",NEW.bbemails as "bbemails",
																					NEW.ccemails as "ccemails",NEW.subject as "subject",
																					NEW.content as "content",NEW.email_size as "email_size",
																					NEW.important_flag as "important_flag",NEW.attach_flag as "attach_flag",
																					NEW.read_flag as "read_flag",NEW.process_flag as "process_flag",
																					NEW.recevi_time as "recevi_time",NEW.email_flag as "email_flag",
																					NEW.urgent_flag as "urgent_flag",NEW.receipt_flag as "receipt_flag",
																					NEW.refer_linkmanid as "refer_linkmanid",NEW.refer_cusid as "refer_cusid",
																					NEW.company_id as "company_id",NEW.tags as "tags",
																					NEW.create_time as "create_time",NEW.create_user as "create_user",
																					NEW.lastupdate_time as "lastupdate_time",NEW.lastupdate_user as "lastupdate_user",
																					NEW.refer_attachIds as "refer_attachIds",NEW.copyrecevier as "copyrecevier",
																					NEW.fullrecevier as "fullrecevier",NEW.reply_time as "reply_time" 
																				)
																			)
																		)
										,"");
  END$$
DELIMITER ;

DELIMITER $$
CREATE TRIGGER inbox_update_trigger AFTER UPDATE ON crm.sys_email_folder
  FOR EACH ROW BEGIN
    SET @ret=
 		 redis_commands("192.168.0.118",6379,"rpush","crmInboxEvents",
												json_object(
																			json_members
																			(
																				"op",
																				"update",
																				"value",
																				json_object
																				(
																					NEW.Id as "Id",NEW.type as "type",
																					NEW.mailserver_id as "mailserver_id",NEW.sender as "sender",
																					NEW.sender_name as "sender_name",NEW.recevier as "recevier",
																					NEW.replyto as "replyto",NEW.bbemails as "bbemails",
																					NEW.ccemails as "ccemails",NEW.subject as "subject",
																					NEW.content as "content",NEW.email_size as "email_size",
																					NEW.important_flag as "important_flag",NEW.attach_flag as "attach_flag",
																					NEW.read_flag as "read_flag",NEW.process_flag as "process_flag",
																					NEW.recevi_time as "recevi_time",NEW.email_flag as "email_flag",
																					NEW.urgent_flag as "urgent_flag",NEW.receipt_flag as "receipt_flag",
																					NEW.refer_linkmanid as "refer_linkmanid",NEW.refer_cusid as "refer_cusid",
																					NEW.company_id as "company_id",NEW.tags as "tags",
																					NEW.create_time as "create_time",NEW.create_user as "create_user",
																					NEW.lastupdate_time as "lastupdate_time",NEW.lastupdate_user as "lastupdate_user",
																					NEW.refer_attachIds as "refer_attachIds",NEW.copyrecevier as "copyrecevier",
																					NEW.fullrecevier as "fullrecevier",NEW.reply_time as "reply_time" 
																				)
																			)
																		)
										,"");
     END$$
DELIMITER ;

DELIMITER $$
CREATE TRIGGER inbox_delete_trigger AFTER DELETE ON crm.sys_email_folder
  FOR EACH ROW BEGIN
    SET @ret=
 		 redis_commands("192.168.0.118",6379,"rpush","crmInboxEvents",
												json_object(
																			json_members
																			(
																				"op",
																				"delete",
																				"value",
																				json_object
																				(
																					OLD.Id as "Id",OLD.type as "type",
																					OLD.mailserver_id as "mailserver_id",OLD.sender as "sender",
																					OLD.sender_name as "sender_name",OLD.recevier as "recevier",
																					OLD.replyto as "replyto",OLD.bbemails as "bbemails",
																					OLD.ccemails as "ccemails",OLD.subject as "subject",
																					OLD.content as "content",OLD.email_size as "email_size",
																					OLD.important_flag as "important_flag",OLD.attach_flag as "attach_flag",
																					OLD.read_flag as "read_flag",OLD.process_flag as "process_flag",
																					OLD.recevi_time as "recevi_time",OLD.email_flag as "email_flag",
																					OLD.urgent_flag as "urgent_flag",OLD.receipt_flag as "receipt_flag",
																					OLD.refer_linkmanid as "refer_linkmanid",OLD.refer_cusid as "refer_cusid",
																					OLD.company_id as "company_id",OLD.tags as "tags",
																					OLD.create_time as "create_time",OLD.create_user as "create_user",
																					OLD.lastupdate_time as "lastupdate_time",OLD.lastupdate_user as "lastupdate_user",
																					OLD.refer_attachIds as "refer_attachIds",OLD.copyrecevier as "copyrecevier",
																					OLD.fullrecevier as "fullrecevier",OLD.reply_time as "reply_time" 
																				)
																			)
																		)
										,"");
  END$$
DELIMITER ;