//@ {VER(>=8.0.11)}

function count_in_metadata_schema() {
    return session.runSql("SELECT COUNT(*) FROM mysql_innodb_cluster_metadata.instances").fetchOne()[0];
}

function validate_instance_status(status, instance, expected) {
    topology = status["replicaSet"]["topology"];
    if (expected == "N/A") {
        EXPECT_FALSE(`${hostname}:${instance}` in topology);
        return;
    }

    instance_status = topology[`${hostname}:${instance}`];
    if (expected == "OK") {
        EXPECT_EQ(instance_status["status"], "ONLINE");
        EXPECT_FALSE("instanceErrors" in instance_status);
    } else if (expected == "UNMANAGED") {
        EXPECT_EQ(instance_status["status"], "ERROR");
        EXPECT_TRUE("instanceErrors" in instance_status);
        EXPECT_CONTAINS("ERROR: Replication source channel is not configured, should be ", instance_status["instanceErrors"][0]);
    } else if (expected == "INVALIDATED") {
        EXPECT_EQ(instance_status["status"], "INVALIDATED");
        EXPECT_EQ(instance_status["instanceRole"], null);
    } else if (expected == "CONNECTING") {
        EXPECT_EQ(instance_status["status"], "CONNECTING");
        EXPECT_TRUE("instanceErrors" in instance_status);
        EXPECT_CONTAINS("NOTE: Replication I/O thread is reconnecting.", instance_status["instanceErrors"][0]);
    } else {
        testutil.fail("Unknown state: " + expected)
    }
}

function validate_status(status, instance_data) {
    instance_data.forEach(instance=>
        validate_instance_status(status, instance[0], instance[1]))
}

//@<> INCLUDE async_utils.inc

//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

//@<> Create replica set
shell.connect(__sandbox_uri1);
var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});

rset.addInstance(__sandbox_uri2);
rset.addInstance(__sandbox_uri3);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "OK"], [__mysql_sandbox_port3, "OK"]]);

//@<> Validate arguments
EXPECT_THROWS(function(){ rset.rescan(1); }, "Argument #1 is expected to be a map");
EXPECT_THROWS(function(){ rset.rescan(1,2); }, "Invalid number of arguments, expected 0 to 1 but got 2");
EXPECT_THROWS(function(){ rset.rescan(""); }, "Argument #1 is expected to be a map");
EXPECT_THROWS(function(){ rset.rescan({foobar: true}); }, "Invalid options: foobar");
EXPECT_THROWS(function(){ rset.rescan({addUnmanaged: "whatever"}); }, "Option 'addUnmanaged' Bool expected, but value is String");
EXPECT_THROWS(function(){ rset.rescan({removeObsolete: "foo"}); }, "Option 'removeObsolete' Bool expected, but value is String");

//@<> Check if rescan restores "server_id"

server_id_1 = session.runSql(`SELECT attributes->'$.server_id' FROM mysql_innodb_cluster_metadata.instances WHERE address = '${hostname}:${__mysql_sandbox_port1}'`).fetchOne()[0];
server_id_3 = session.runSql(`SELECT attributes->'$.server_id' FROM mysql_innodb_cluster_metadata.instances WHERE address = '${hostname}:${__mysql_sandbox_port3}'`).fetchOne()[0];
session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances SET attributes=JSON_REMOVE(attributes, '$.server_id') WHERE (address IN ('${hostname}:${__mysql_sandbox_port1}', '${hostname}:${__mysql_sandbox_port3}'))`);

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { rset.rescan(); });
EXPECT_OUTPUT_CONTAINS("Rescanning the ReplicaSet...");
EXPECT_OUTPUT_CONTAINS(`Updating server id for instance '${hostname}:${__mysql_sandbox_port1}'.`);
EXPECT_OUTPUT_CONTAINS(`Updating server id for instance '${hostname}:${__mysql_sandbox_port3}'.`);
EXPECT_OUTPUT_CONTAINS("Rescan finished.");

EXPECT_EQ(server_id_1, session.runSql(`SELECT attributes->'$.server_id' FROM mysql_innodb_cluster_metadata.instances WHERE address = '${hostname}:${__mysql_sandbox_port1}'`).fetchOne()[0]);
EXPECT_EQ(server_id_3, session.runSql(`SELECT attributes->'$.server_id' FROM mysql_innodb_cluster_metadata.instances WHERE address = '${hostname}:${__mysql_sandbox_port3}'`).fetchOne()[0]);

//@<> Check if rescan restores "server_uuid"

initial_uuid = session.runSql(`SELECT mysql_server_uuid FROM mysql_innodb_cluster_metadata.instances WHERE address = '${hostname}:${__mysql_sandbox_port2}'`).fetchOne()[0];
session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances SET mysql_server_uuid = '' WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`);

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { rset.rescan(); });
EXPECT_OUTPUT_CONTAINS(`Updating server id for instance '${hostname}:${__mysql_sandbox_port2}'.`);
EXPECT_OUTPUT_CONTAINS("Rescan finished.");

EXPECT_EQ(initial_uuid, session.runSql(`SELECT mysql_server_uuid FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOne()[0]);

//@<> Rescan detects new instances (addUnmanaged: false + no-interactive)
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)", [`${hostname}:${__mysql_sandbox_port2}`]);

validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "N/A"], [__mysql_sandbox_port3, "OK"]]);

shell.options.useWizards=0;

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { rset.rescan(); });
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' is part of the replication topology but isn't present in the metadata. Use the "addUnmanaged" option to automatically add the instance to the ReplicaSet metadata.`);

session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)", [`${hostname}:${__mysql_sandbox_port3}`]);

validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "N/A"], [__mysql_sandbox_port3, "N/A"]]);

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { rset.rescan(); });
EXPECT_OUTPUT_CONTAINS(`The instances '${hostname}:${__mysql_sandbox_port2}' and '${hostname}:${__mysql_sandbox_port3}' are part of the ReplicaSet but aren't present in the metadata. Use the \"addUnmanaged\" option to automatically add them to the ReplicaSet metadata.`);

//@<> Rescan detects new instances (addUnmanaged: false + interactive + no adding)

shell.options.useWizards=1;

WIPE_OUTPUT();

testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port2}' is part of the replication topology but isn't present in the metadata. Do you wish to add it? [y/N]:`, "n");
testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port3}' is part of the replication topology but isn't present in the metadata. Do you wish to add it? [y/N]:`, "n");
EXPECT_NO_THROWS(function() { rset.rescan(); });

validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "N/A"], [__mysql_sandbox_port3, "N/A"]]);

//@<> Rescan detects new instances (addUnmanaged: false + interactive + adding)

WIPE_OUTPUT();

testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port2}' is part of the replication topology but isn't present in the metadata. Do you wish to add it? [y/N]:`, "y");
testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port3}' is part of the replication topology but isn't present in the metadata. Do you wish to add it? [y/N]:`, "n");
EXPECT_NO_THROWS(function() { rset.rescan(); });
EXPECT_OUTPUT_CONTAINS("Adding instance to the ReplicaSet metadata...");

validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "OK"], [__mysql_sandbox_port3, "N/A"]]);

//@<> Rescan detects new instances (addUnmanaged: false + interactive + adding last instance)

WIPE_OUTPUT();

testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port3}' is part of the replication topology but isn't present in the metadata. Do you wish to add it? [y/N]:`, "y");
EXPECT_NO_THROWS(function() { rset.rescan(); });
EXPECT_OUTPUT_CONTAINS("Adding instance to the ReplicaSet metadata...");
EXPECT_OUTPUT_CONTAINS("Rescan finished.");

validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "OK"], [__mysql_sandbox_port3, "OK"]]);

//@<> Rescan detects new instances (addUnmanaged: true)
session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE (address IN (?, ?))", [`${hostname}:${__mysql_sandbox_port2}`, `${hostname}:${__mysql_sandbox_port3}`]);

validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "N/A"], [__mysql_sandbox_port3, "N/A"]]);

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { rset.rescan({addUnmanaged: true}); });
EXPECT_OUTPUT_CONTAINS(`Adding instance '${hostname}:${__mysql_sandbox_port2}' to the ReplicaSet metadata...`);
EXPECT_OUTPUT_CONTAINS(`Adding instance '${hostname}:${__mysql_sandbox_port3}' to the ReplicaSet metadata...`);
EXPECT_OUTPUT_CONTAINS("Rescan finished.");

validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "OK"], [__mysql_sandbox_port3, "OK"]]);

//@<> Rescan detects obsolete instances (removeObsolete: false + no-interactive)
session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("STOP REPLICA FOR CHANNEL '';");
session2.runSql("RESET REPLICA ALL FOR CHANNEL ''");

EXPECT_EQ(3, count_in_metadata_schema());
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "UNMANAGED"], [__mysql_sandbox_port3, "OK"]]);

shell.options.useWizards=0;

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { rset.rescan(); });
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' is no longer part of the ReplicaSet but is still present in the metadata. Use the "removeObsolete" option to automatically remove the instance.`);

EXPECT_EQ(3, count_in_metadata_schema());
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "UNMANAGED"], [__mysql_sandbox_port3, "OK"]]);

session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP REPLICA FOR CHANNEL '';");
session3.runSql("RESET REPLICA ALL FOR CHANNEL ''");

EXPECT_EQ(3, count_in_metadata_schema());
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "UNMANAGED"], [__mysql_sandbox_port3, "UNMANAGED"]]);

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { rset.rescan(); });
EXPECT_OUTPUT_CONTAINS(`The instances '${hostname}:${__mysql_sandbox_port2}' and '${hostname}:${__mysql_sandbox_port3}' are no longer part of the ReplicaSet but are still present in the metadata. Use the \"removeObsolete\" option to automatically remove them.`);

EXPECT_EQ(3, count_in_metadata_schema());
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "UNMANAGED"], [__mysql_sandbox_port3, "UNMANAGED"]]);

//@<> Rescan detects obsolete instances (removeObsolete: false + interactive + no remove)

shell.options.useWizards=1;

WIPE_OUTPUT();

testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port2}' is no longer part of the ReplicaSet but is still present in the metadata. Do you wish to remove it? [y/N]:`, "n");
testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port3}' is no longer part of the ReplicaSet but is still present in the metadata. Do you wish to remove it? [y/N]:`, "n");
EXPECT_NO_THROWS(function() { rset.rescan(); });

EXPECT_EQ(3, count_in_metadata_schema());
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "UNMANAGED"], [__mysql_sandbox_port3, "UNMANAGED"]]);

//@<> Rescan detects obsolete instances (removeObsolete: false + interactive + remove)
WIPE_OUTPUT();

testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port2}' is no longer part of the ReplicaSet but is still present in the metadata. Do you wish to remove it? [y/N]:`, "y");
testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port3}' is no longer part of the ReplicaSet but is still present in the metadata. Do you wish to remove it? [y/N]:`, "n");
EXPECT_NO_THROWS(function() { rset.rescan(); });
EXPECT_OUTPUT_CONTAINS("Removing instance from the ReplicaSet metadata...");

EXPECT_EQ(2, count_in_metadata_schema());
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "N/A"], [__mysql_sandbox_port3, "UNMANAGED"]]);

//@<> Rescan detects obsolete instances (removeObsolete: false + interactive + remove last)

WIPE_OUTPUT();

testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port3}' is no longer part of the ReplicaSet but is still present in the metadata. Do you wish to remove it? [y/N]:`, "y");
EXPECT_NO_THROWS(function() { rset.rescan(); });
EXPECT_OUTPUT_CONTAINS("Removing instance from the ReplicaSet metadata...");
EXPECT_OUTPUT_CONTAINS("Rescan finished.");

EXPECT_EQ(1, count_in_metadata_schema());
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "N/A"], [__mysql_sandbox_port3, "N/A"]]);

//@<> Rescan detects obsolete instances (removeObsolete: true)
rset.addInstance(__sandbox_uri2);
rset.addInstance(__sandbox_uri3);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

session2.runSql("STOP REPLICA FOR CHANNEL '';");
session2.runSql("RESET REPLICA ALL FOR CHANNEL ''");
session3.runSql("STOP REPLICA FOR CHANNEL '';");
session3.runSql("RESET REPLICA ALL FOR CHANNEL ''");

EXPECT_EQ(3, count_in_metadata_schema());
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "UNMANAGED"], [__mysql_sandbox_port3, "UNMANAGED"]]);

WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { rset.rescan({removeObsolete: true}); });
EXPECT_OUTPUT_CONTAINS(`Removing instance '${hostname}:${__mysql_sandbox_port2}' from the ReplicaSet metadata...`);
EXPECT_OUTPUT_CONTAINS(`Removing instance '${hostname}:${__mysql_sandbox_port3}' from the ReplicaSet metadata...`);
EXPECT_OUTPUT_CONTAINS("Rescan finished.");

EXPECT_EQ(1, count_in_metadata_schema());
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "N/A"], [__mysql_sandbox_port3, "N/A"]]);

session2.close();
session3.close();

//@<> Rescan stores the correct recovery accounts in the MD
rset.addInstance(__sandbox_uri2);
rset.addInstance(__sandbox_uri3);

EXPECT_EQ(3, count_in_metadata_schema());
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "OK"], [__mysql_sandbox_port3, "OK"]]);

account_old_2 = session.runSql(`SELECT (attributes->>'$.replicationAccountUser') as recovery_user, (attributes->>'$.replicationAccountHost') as recovery_host FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOneObject();
account_old_3 = session.runSql(`SELECT (attributes->>'$.replicationAccountUser') as recovery_user, (attributes->>'$.replicationAccountHost') as recovery_host FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port3}')`).fetchOneObject();
session.runSql(`UPDATE mysql_innodb_cluster_metadata.instances SET attributes = JSON_REMOVE(JSON_REMOVE(attributes, '$.replicationAccountUser'), '$.replicationAccountHost') WHERE (address IN ('${hostname}:${__mysql_sandbox_port2}', '${hostname}:${__mysql_sandbox_port3}'))`);

// check status error message
status = rset.status();
EXPECT_TRUE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"].length, 1);
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0], "ERROR: the instance is missing its replication account info from the metadata. Please call rescan() to fix this.");
EXPECT_TRUE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]);
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"].length, 1);
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["instanceErrors"][0], "ERROR: the instance is missing its replication account info from the metadata. Please call rescan() to fix this.");

WIPE_OUTPUT();

EXPECT_NO_THROWS(function(){ rset.rescan(); });
EXPECT_OUTPUT_CONTAINS(`Updating replication account for instance '${hostname}:${__mysql_sandbox_port2}'.`);
EXPECT_OUTPUT_CONTAINS(`Updating replication account for instance '${hostname}:${__mysql_sandbox_port3}'.`);
EXPECT_OUTPUT_CONTAINS("Rescan finished.");

account_new_2 = session.runSql(`SELECT (attributes->>'$.replicationAccountUser') as recovery_user, (attributes->>'$.replicationAccountHost') as recovery_host FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port2}')`).fetchOneObject();
account_new_3 = session.runSql(`SELECT (attributes->>'$.replicationAccountUser') as recovery_user, (attributes->>'$.replicationAccountHost') as recovery_host FROM mysql_innodb_cluster_metadata.instances WHERE (address = '${hostname}:${__mysql_sandbox_port3}')`).fetchOneObject();

EXPECT_EQ(account_old_2.recovery_user, account_new_2.recovery_user);
EXPECT_EQ(account_old_2.recovery_host, account_new_2.recovery_host);
EXPECT_EQ(account_old_3.recovery_user, account_new_3.recovery_user);
EXPECT_EQ(account_old_3.recovery_host, account_new_3.recovery_host);

status = rset.status();
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
EXPECT_FALSE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]);

//@<> Rescan stores the correct recovery accounts in the MD with the correct host
var cluster_id = session.runSql("SELECT cluster_id FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)", [`${hostname}:${__mysql_sandbox_port2}`]).fetchOne()[0];
session.runSql("UPDATE mysql_innodb_cluster_metadata.clusters SET attributes = json_set(COALESCE(attributes, '{}'), \"$.opt_replicationAllowedHost\", ?) WHERE (cluster_id = ?)", ["foobar", cluster_id]);

EXPECT_NO_THROWS(function(){ rset.rescan(); });
EXPECT_OUTPUT_NOT_CONTAINS(`Updating replication account for instance '${hostname}:${__mysql_sandbox_port1}'.`);
EXPECT_OUTPUT_CONTAINS(`Updating replication account for instance '${hostname}:${__mysql_sandbox_port2}'.`);
EXPECT_OUTPUT_CONTAINS(`Updating replication account for instance '${hostname}:${__mysql_sandbox_port3}'.`);

host2 = session.runSql("SELECT attributes->>'$.replicationAccountHost' FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)", [`${hostname}:${__mysql_sandbox_port2}`]).fetchOne()[0];
host3 = session.runSql("SELECT attributes->>'$.replicationAccountHost' FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)", [`${hostname}:${__mysql_sandbox_port3}`]).fetchOne()[0];
EXPECT_EQ(host2, "foobar");
EXPECT_EQ(host3, "foobar");

//@<> Running rescan again should produce no changes

EXPECT_NO_THROWS(function(){ rset.rescan(); });

EXPECT_OUTPUT_CONTAINS("Rescanning the ReplicaSet...");
EXPECT_OUTPUT_CONTAINS("Rescan finished.");
EXPECT_OUTPUT_NOT_CONTAINS("Updating server id for instance ");
EXPECT_OUTPUT_NOT_CONTAINS("Adding instance ");
EXPECT_OUTPUT_NOT_CONTAINS("Removing instance ");
EXPECT_OUTPUT_NOT_CONTAINS("Updating replication account for instance ");

//@<> Rescan must ignore invalidated former primaries
rset.status();

testutil.killSandbox(__mysql_sandbox_port1);
shell.connect(__sandbox_uri2);

rset = dba.getReplicaSet();
rset.forcePrimaryInstance(__sandbox_uri2);

validate_status(rset.status(), [[__mysql_sandbox_port1, "INVALIDATED"], [__mysql_sandbox_port2, "OK"], [__mysql_sandbox_port3, "OK"]]);

testutil.startSandbox(__mysql_sandbox_port1);

EXPECT_NO_THROWS(function(){ rset.rescan(); });
EXPECT_OUTPUT_CONTAINS(`Ignoring instance '${hostname}:${__mysql_sandbox_port1}' because it's INVALIDATED. Please rejoin or remove it from the ReplicaSet.`);

//@<> Rescan after instance changed server_id on the primary
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2);
testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);

testutil.stopSandbox(__mysql_sandbox_port2);
testutil.changeSandboxConf(__mysql_sandbox_port2, "server_id", (10000000 + parseInt(__mysql_sandbox_port2)).toString());
testutil.startSandbox(__mysql_sandbox_port2);

EXPECT_NO_THROWS(function(){ rset.setPrimaryInstance(__sandbox_uri2); });
EXPECT_OUTPUT_CONTAINS(`${hostname}:${__mysql_sandbox_port2} was promoted to PRIMARY.`);
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "OK"]]);

EXPECT_NO_THROWS(function(){ rset.rescan(); });
EXPECT_OUTPUT_CONTAINS(`Updating server id for instance '${hostname}:${__mysql_sandbox_port2}'.`);
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "OK"]]);

EXPECT_NO_THROWS(function(){ rset.setPrimaryInstance(__sandbox_uri1); });
EXPECT_OUTPUT_CONTAINS(`${hostname}:${__mysql_sandbox_port1} was promoted to PRIMARY.`);
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "OK"]]);

//@<> Rescan after an unreachable instance was forcefully removed
WIPE_SHELL_LOG();

testutil.stopSandbox(__mysql_sandbox_port2);
rset.removeInstance(__endpoint2, {force: true});

EXPECT_NO_THROWS(function(){ rset.rescan(); });
EXPECT_SHELL_LOG_CONTAINS_COUNT("No updates required.", 3);

testutil.startSandbox(__mysql_sandbox_port2);

//@<> If rescan removes the current replicaset object instances, it should leave the object invalidated
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri3);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2);
rset.setPrimaryInstance(__sandbox_uri2);

session.runSql("STOP REPLICA");
session.runSql("RESET REPLICA ALL");

rset.status();

WIPE_SHELL_LOG();

EXPECT_NO_THROWS(function(){ rset.rescan({removeObsolete: true}); });

EXPECT_OUTPUT_CONTAINS(`Removing instance '${hostname}:${__mysql_sandbox_port1}' from the ReplicaSet metadata...`);
EXPECT_SHELL_LOG_CONTAINS("Invalidating ReplicaSet object.");

EXPECT_THROWS(function(){
    rset.status();
}, "The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object.");

//@<> Check rescan warning message according to the state of the channel
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2);

shell.connect(__sandbox_uri2);

port = session.runSql("SHOW REPLICA STATUS FOR CHANNEL ''").fetchOneObject().Source_Port;
port = port * 100 + port;
session.runSql("STOP REPLICA");
session.runSql(`CHANGE REPLICATION SOURCE TO SOURCE_PORT = ${port} FOR CHANNEL ''`);

status = rset.status();
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"], "ERROR");
EXPECT_TRUE("instanceErrors" in status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]);
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"].length, 2);
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][0], "ERROR: Replication is stopped.");
EXPECT_EQ(status["replicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["instanceErrors"][1], `ERROR: Replication source misconfigured. Expected ${hostname}:${__mysql_sandbox_port1} but is ${hostname}:${port}.`);

WIPE_OUTPUT();

shell.options.useWizards=1;

testutil.expectPrompt(`The instance '${hostname}:${__mysql_sandbox_port2}' is no longer part of the ReplicaSet but is still present in the metadata. Do you wish to remove it? [y/N]:`, "n");
EXPECT_NO_THROWS(function(){ rset.rescan(); });
EXPECT_OUTPUT_CONTAINS(`Replication is not active in instance '${hostname}:${__mysql_sandbox_port2}', however, it is configured.`);

shell.options.useWizards=0;

//@<> Rescan when replicas are connecting
shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2, {recoveryMethod: "clone"});
rset.addInstance(__sandbox_uri3, {recoveryMethod: "clone"});
validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "OK"], [__mysql_sandbox_port3, "OK"]]);

user2 = session.runSql(`SELECT concat('\\'', attributes->>'$.replicationAccountUser', '\\'@\\'', attributes->>'$.replicationAccountHost', '\\'') FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)`, [`${hostname}:${__mysql_sandbox_port2}`]).fetchOne()[0];
session.runSql(`SET PASSWORD FOR ${user2} = 'foo_bar'`);
user3 = session.runSql(`SELECT concat('\\'', attributes->>'$.replicationAccountUser', '\\'@\\'', attributes->>'$.replicationAccountHost', '\\'') FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)`, [`${hostname}:${__mysql_sandbox_port3}`]).fetchOne()[0];
session.runSql(`SET PASSWORD FOR ${user3} = 'foo_bar'`);

session2 = mysql.getSession(__sandbox_uri2);
session2.runSql("STOP REPLICA");
session2.runSql("START REPLICA");
session3 = mysql.getSession(__sandbox_uri3);
session3.runSql("STOP REPLICA");
session3.runSql("START REPLICA");

validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "CONNECTING"], [__mysql_sandbox_port3, "CONNECTING"]]);

shell.connect(__sandbox_uri2);
rset = dba.getReplicaSet();

WIPE_SHELL_LOG();
EXPECT_NO_THROWS(function(){ rset.rescan(); });
EXPECT_SHELL_LOG_CONTAINS_COUNT("No updates required.", 3);

//@<> Rescan cannot add new instances if the ReplicaSet auth type uses certificates
ca_path = testutil.sslCreateCa("myca", "/CN=Test_CA");

cert1_path = testutil.sslCreateCert("server", "myca", `/CN=${hostname}`, __mysql_sandbox_port1);
cert2_path = testutil.sslCreateCert("server", "myca", `/CN=${hostname}`, __mysql_sandbox_port2);

testutil.changeSandboxConf(__mysql_sandbox_port1, "ssl_ca", ca_path);
testutil.changeSandboxConf(__mysql_sandbox_port1, "ssl_cert", cert1_path);
testutil.changeSandboxConf(__mysql_sandbox_port1, "ssl_key", cert1_path.replace("-cert.pem", "-key.pem"));
testutil.restartSandbox(__mysql_sandbox_port1);

testutil.changeSandboxConf(__mysql_sandbox_port2, "ssl_ca", ca_path);
testutil.changeSandboxConf(__mysql_sandbox_port2, "ssl_cert", cert2_path);
testutil.changeSandboxConf(__mysql_sandbox_port2, "ssl_key", cert2_path.replace("-cert.pem", "-key.pem"));
testutil.restartSandbox(__mysql_sandbox_port2);

shell.connect(__sandbox_uri1);
reset_instance(session);
session2 = mysql.getSession(__sandbox_uri2);
reset_instance(session2);

var rset = dba.createReplicaSet("rset", {gtidSetIsComplete: true, memberAuthType: "CERT_ISSUER", certIssuer: "/CN=Test_CA"});
rset.addInstance(__sandbox_uri2);

session.runSql("DELETE FROM mysql_innodb_cluster_metadata.instances WHERE (address = ?)", [`${hostname}:${__mysql_sandbox_port2}`]);

validate_status(rset.status(), [[__mysql_sandbox_port1, "OK"], [__mysql_sandbox_port2, "N/A"]]);

WIPE_OUTPUT();

EXPECT_NO_THROWS(function(){ rset.rescan(); });
EXPECT_OUTPUT_CONTAINS("New instances were discovered in the ReplicaSet but ignored because the ReplicaSet requires SSL certificate authentication.\nPlease stop the asynchronous channel on those instances and then add them to the ReplicaSet using addInstance() with the appropriate authentication options.");

session2.close();

//@<> Finalization
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
