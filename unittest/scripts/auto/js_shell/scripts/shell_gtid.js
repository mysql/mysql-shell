//@ Setup
testutil.deploySandbox(__mysql_sandbox_port1, 'root', {report_host: hostname});

shell.connect(__sandbox_uri1);

\sql

create schema testdb;
use testdb;
create table a (a int primary key auto_increment);

//@ No GTID
insert into testdb.a values (default);

//@ With GTID
set session_track_gtids=1;
insert into testdb.a values (default);

//@ With GTID rollback
start transaction;
insert into testdb.a values (default);
rollback;

//@ With GTID commit
start transaction;
insert into testdb.a values (default);
commit;

//@ Multiple GTIDs
delimiter $$
create procedure mytest()
begin
  insert into testdb.a values (default);
  insert into testdb.a values (default);
end$$
delimiter ;
call mytest();

//@<> disable GTID globally
set global session_track_gtids='off';
\js
EXPECT_OUTPUT_CONTAINS("Switching to JavaScript mode...");

//@<> cmdline - print GTID only if interactive (no GTID)
testutil.callMysqlsh([__sandbox_uri1, "--sqlc", "-e", "insert into testdb.a values (default)"]);
EXPECT_OUTPUT_NOT_CONTAINS("GTIDs:");
testutil.callMysqlsh([__sandbox_uri1, "--interactive", "--sqlc", "-e", "insert into testdb.a values (default);"]);
EXPECT_OUTPUT_NOT_CONTAINS("GTIDs:");

//@<> enable GTID globally
\sql
set global session_track_gtids='OWN_GTID';
\js

//@<> cmdline - print GTID only if interactive
testutil.callMysqlsh([__sandbox_uri1, "--sqlc", "-e", "show variables like 'session_track_gtids'"]);
testutil.callMysqlsh([__sandbox_uri1, "--sqlc", "-e", "insert into testdb.a values (default)"]);
EXPECT_OUTPUT_NOT_CONTAINS("GTIDs:");
testutil.callMysqlsh([__sandbox_uri1, "--interactive", "--sqlc", "-e", "insert into testdb.a values (default);"]);
EXPECT_OUTPUT_CONTAINS("GTIDs:");

//@ Cleanup
session.runSql("drop schema testdb");
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
