
// Various negative test cases for createCluster()

//@ Initialize
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);

//@# create_cluster_with_cluster_admin_type
dba.createCluster("dev", {"clusterAdminType": "local"});

//@ Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
