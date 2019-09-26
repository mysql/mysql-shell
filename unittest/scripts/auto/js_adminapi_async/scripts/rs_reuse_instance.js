//@ {VER(>=8.0.11)}

// Tests rejoinInstance() function for async replicasets.

//@ INCLUDE async_utils.inc

//@<> Initialization.
var uuid1 = "5ef81566-9395-11e9-87e9-111111111111";
var uuid2 = "5ef81566-9395-11e9-87e9-222222222222";
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"report_host": hostname_ip, server_uuid: uuid1, server_id: 11});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"report_host": hostname_ip, server_uuid: uuid2, server_id: 22});
var session1 = mysql.getSession(__sandbox_uri1);
var session2 = mysql.getSession(__sandbox_uri2);

//@<> Create replicaset A.
shell.connect(__sandbox_uri1);
var rsa = dba.createReplicaSet("A", {gtidSetIsComplete:true});
rsa.addInstance(__sandbox_uri2);

//@<> Change the primary to remove th old one.
rsa.setPrimaryInstance(__sandbox2);
//shell.connect(__sandbox_uri2);
//var rsa = dba.getReplicaSet();

//@<> Remove the old primary from the replicaset.
rsa.removeInstance(__sandbox1);

//@<> Create a replicaset B with the instance from the previous replicaset A.
shell.connect(__sandbox_uri1);
dba.createReplicaSet("B", {gtidSetIsComplete:true});

//@<> Get replicaset B.
var rsb = dba.getReplicaSet();

//@ Check the status of replicaset B.
rsb.status();

//@ Check the status of replicaset A.
shell.connect(__sandbox_uri2);
var rsa = dba.getReplicaSet();
rsa.status();

//@<> Cleanup.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
