//@ {VER(>=8.0.11)}

//@<> WL#13788 Deploy replicaset instances and create ReplicaSet
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
var rs = dba.createReplicaSet("myrs", {gtidSetIsComplete:true});
rs.addInstance(__sandbox_uri2);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

//@<> STOP secondary before adding tags to test that information is retrived from the primary
shell.connect(__sandbox_uri2);
session.runSql("STOP SLAVE");

//@<> WL#13788 Set some tags and later confirm the output of rs.options
rs.setInstanceOption(__sandbox_uri2, "tag:_hidden", 0);
rs.setInstanceOption(__sandbox_uri2, "tag:_disconnect_existing_sessions_when_hidden", true);
rs.setInstanceOption(__sandbox_uri2, "tag:int_tag", 222);
rs.setInstanceOption(__sandbox_uri2, "tag:string_tag", "instance2");
rs.setInstanceOption(__sandbox_uri2, "tag:bool_tag", false);
rs.setInstanceOption(__sandbox_uri1, "tag:int_tag", 111);
rs.setInstanceOption(__sandbox_uri1, "tag:string_tag", "instance1");
rs.setInstanceOption(__sandbox_uri1, "tag:bool_tag", true);
rs.setOption("tag:global_custom", "global_tag");


//@ WL#13788 Check the output of rs.options is as expected and that the function gets its information through the primary
rs = dba.getReplicaSet();
rs.options();

//@<> Cleanup
session.close();
rs.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);