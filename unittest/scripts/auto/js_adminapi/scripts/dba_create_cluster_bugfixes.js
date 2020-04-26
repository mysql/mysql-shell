//@<> dba.createCluster does not error if using a single clusterAdmin account with netmask BUG#31018091
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
shell.connect(__sandbox_uri1);
var ip_mask = hostname_ip.split(".").splice(0,3).join(".") + ".0";
dba.configureInstance(__sandbox_uri1, {clusterAdmin: "'admin'@'"+ ip_mask + "/255.255.255.0'", clusterAdminPassword: "pwd"});
var cluster_admin_uri= "mysql://admin:pwd@" + hostname_ip + ":" + __mysql_sandbox_port1;
shell.connect(cluster_admin_uri);
c = dba.createCluster("sample");
EXPECT_STDERR_EMPTY();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
