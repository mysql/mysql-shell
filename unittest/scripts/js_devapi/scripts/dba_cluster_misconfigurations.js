// Assumptions: smart deployment rountines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});

// Change dynamic variables manually
session.runSql('SET GLOBAL binlog_checksum=CRC32');
session.runSql('SET GLOBAL binlog_format=MIXED');

//@ Dba.createCluster
if (__have_ssl)
  var cluster = dba.createCluster('dev', {memberSslMode:'REQUIRED'});
else
  var cluster = dba.createCluster('dev');

//@ Finalization
// Will delete the sandboxes ONLY if this test was executed standalone
if (deployed_here)
  cleanup_sandboxes(true);
