// Prepares the environment
// BUG# 29451154 Describes a problem where a stored procedure produces multiple
// results and the shell does not print them as expected.
shell.connect(__uripwd);

session.createSchema("bug29451154");
session.runSql("use bug29451154;");

// This stored procedure will loop in a cycle of a second for 5 times, sending
// a couple of results on each loop.
session.runSql("CREATE PROCEDURE mysp() BEGIN DECLARE i INT DEFAULT 0; WHILE i < 5 DO SELECT i; SELECT sleep(1); SET i = i + 1; END WHILE; END;");
session.runSql("CREATE PROCEDURE multiupdates() BEGIN create table sp_table (name varchar(30)); insert into sp_table values ('one'), ('two'), ('three'); select * from sp_table; delete from sp_table; drop table sp_table; END;");

//@ Test using X Protocol
session.runSql("call mysp();");

//@ Test multi-updates using X Protocol
session.runSql("call multiupdates();");
session.close();

//@ Test using Classic Protocol [USE:Test using X Protocol]
shell.connect(__mysqluripwd + "/bug29451154");
session.runSql("call mysp();");

//@ Test multi-updates using Classic [USE:Test multi-updates using X Protocol]
session.runSql("call multiupdates();");

session.runSql("drop schema bug29451154");

session.close();
