// Deploy instances (with the same server UUID).
testutil.deploySandbox(__mysql_sandbox_port1, "root", {"server-uuid": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee", report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {"server-uuid": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee", report_host: hostname});

//@<> Create a cluster a cluster.
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("test_cluster", {gtidSetIsComplete: true});

//@<> Adding an instance with the same server UUID fails.
EXPECT_THROWS_TYPE(function() { cluster.addInstance(__sandbox_uri2); }, "Invalid server_uuid.", "MYSQLSH");

EXPECT_OUTPUT_CONTAINS(`The target instance '${__endpoint2}' has a 'server_uuid' already being used by instance '${__endpoint1}'.`);

//<> Clean-up deployed instances.
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
