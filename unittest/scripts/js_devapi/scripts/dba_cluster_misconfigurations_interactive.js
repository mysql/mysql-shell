// Assumptions: smart deployment routines available
//@ Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root");

//@ connect and change vars
shell.connect({scheme:'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
session.runSql('SET GLOBAL binlog_checksum=CRC32');
// session.runSql('SET GLOBAL binlog_format=MIXED');

//@<OUT> Dba.createCluster: cancel
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED'});
else
  var cluster = dba.createCluster('dev', {memberSslMode:'DISABLED'});

//@<OUT> Dba.createCluster: ok
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED'});
else
  var cluster = dba.createCluster('dev', {memberSslMode:'DISABLED'});

session.close();

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
testutil.destroySandbox(__mysql_sandbox_port1);
