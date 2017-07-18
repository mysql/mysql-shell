// Assumptions: smart deployment rountines available
//@ Initialization
var deployed_here = reset_or_deploy_sandboxes();

shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
var clusterSession = session;
//@<OUT> cluster status
if (__have_ssl)
    var cluster = dba.createCluster('dev', {memberSslMode: 'REQUIRED'});
else
    var cluster = dba.createCluster('dev', {memberSslMode: 'DISABLED'});

// session is stored on the cluster object so changing the global session should not affect cluster operations
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port2, user: 'root', password: 'root'});
session.close();

cluster.status();

//@ cluster session closed: error
shell.connect({scheme: 'mysql', host: localhost, port: __mysql_sandbox_port1, user: 'root', password: 'root'});
cluster = dba.getCluster();
session.close();
cluster.status();

//@ Finalization
//  Will close opened sessions and delete the sandboxes ONLY if this test was executed standalone
clusterSession.close();
if (deployed_here)
    cleanup_sandboxes(true);
