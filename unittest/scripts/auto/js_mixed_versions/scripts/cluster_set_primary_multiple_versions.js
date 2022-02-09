//@ {DEF(MYSQLD_SECONDARY_SERVER_A) && VER(>=8.0.13) && testutil.versionCheck(MYSQLD_SECONDARY_SERVER_A.version, ">=", "8.0.13")}

// Deploy sandbox with current version
testutil.deploySandbox(__mysql_sandbox_port2, "root", { report_host: localhost });

// Deploy sandboxes with secondary version
testutil.deploySandbox(__mysql_sandbox_port1, "root", { report_host: localhost }, { mysqldPath: MYSQLD_SECONDARY_SERVER_A.path });

shell.connect(__sandbox_uri1);

var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});
cluster.addInstance(__sandbox_uri2);

//@<> Check if the primary version corresponds to an instance with the secondary (server) version
cluster.status();
EXPECT_OUTPUT_CONTAINS("\"primary\": \"" + localhost + ":" + __mysql_sandbox_port1 + "\"");

//@<> Check if the primary version is the minimum of all the instances
EXPECT_THROWS(function() {
    cluster.setPrimaryInstance(localhost + ":" + __mysql_sandbox_port2);
}, "Instance cannot be set as primary");
EXPECT_OUTPUT_CONTAINS("Failed to set '" + localhost + ":" + __mysql_sandbox_port2 + "' as primary instance: The function 'group_replication_set_as_primary' failed. Error processing configuration start message: The appointed primary member has a version that is greater than the one of some of the members in the group.");

cluster.disconnect();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);