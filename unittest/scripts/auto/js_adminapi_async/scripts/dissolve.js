//@ {VER(>=8.0.11)}

//@<> INCLUDE async_utils.inc

//@<> Initialization
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

//@<> Create replica set
shell.connect(__sandbox_uri1);
var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});

//@<> Validate arguments
EXPECT_THROWS(function(){ rset.dissolve(1); }, "Argument #1 is expected to be a map");
EXPECT_THROWS(function(){ rset.dissolve(1,2); }, "Invalid number of arguments, expected 0 to 1 but got 2");
EXPECT_THROWS(function(){ rset.dissolve(""); }, "Argument #1 is expected to be a map");
EXPECT_THROWS(function(){ rset.dissolve({foobar: true}); }, "Invalid options: foobar");
EXPECT_THROWS(function(){ rset.dissolve({force: "whatever"}); }, "Option 'force' Bool expected, but value is String");
EXPECT_THROWS(function(){ rset.dissolve({timeout: "foo"}); }, "Option 'timeout' Integer expected, but value is String");
EXPECT_THROWS(function(){ rset.dissolve({timeout: -2}); }, "Invalid value '-2' for option 'timeout'. It must be a positive integer representing the maximum number of seconds to wait");

//@<> Dissolve replica set in interactive mode (cancel)
shell.options.useWizards=1;

testutil.expectPrompt("Are you sure you want to dissolve the ReplicaSet?", "n");
EXPECT_THROWS(function() {
    rset.dissolve();
}, "Operation canceled by user.");

EXPECT_OUTPUT_CONTAINS("The ReplicaSet has the following registered instances:");
EXPECT_OUTPUT_CONTAINS_MULTILINE(`{
    "name": "rset",
    "topology": [
        {
            "address": "${hostname}:${__mysql_sandbox_port1}",
            "instanceRole": "PRIMARY",
            "label": "${hostname}:${__mysql_sandbox_port1}"
        }
    ]
}`);

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
WARNING: You are about to dissolve the whole ReplicaSet. This operation cannot be reverted. All members will be removed from the ReplicaSet and replication will be stopped, internal recovery user accounts and the ReplicaSet metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the ReplicaSet? [y/N]:`);

//@<> Dissolve replica set in interactive mode (success)

testutil.expectPrompt("Are you sure you want to dissolve the ReplicaSet?", "y");
EXPECT_NO_THROWS(function() { rset.dissolve(); });

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
WARNING: You are about to dissolve the whole ReplicaSet. This operation cannot be reverted. All members will be removed from the ReplicaSet and replication will be stopped, internal recovery user accounts and the ReplicaSet metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the ReplicaSet? [y/N]:
* Dissolving the ReplicaSet...

The ReplicaSet was successfully dissolved.
Replication was disabled but user data was left intact.
`);

// ensure everything was cleaned
check_dissolved_replica(session, session);

//@<> Dissolve replica set in interactive mode + force (success)
var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
WIPE_OUTPUT();

EXPECT_NO_THROWS(function() { rset.dissolve({force: true}); });

EXPECT_OUTPUT_CONTAINS("The ReplicaSet has the following registered instances:");

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
WARNING: You are about to dissolve the whole ReplicaSet. This operation cannot be reverted. All members will be removed from the ReplicaSet and replication will be stopped, internal recovery user accounts and the ReplicaSet metadata will be dropped. User data will be maintained intact in all instances.

* Dissolving the ReplicaSet...

The ReplicaSet was successfully dissolved.
Replication was disabled but user data was left intact.
`);

// ensure everything was cleaned
check_dissolved_replica(session, session);

//@<> Dissolve replica set in non-interactive mode
var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});

shell.options.useWizards=0;

WIPE_OUTPUT();
EXPECT_NO_THROWS(function() { rset.dissolve(); });

EXPECT_OUTPUT_NOT_CONTAINS("The ReplicaSet has the following registered instances:");

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
* Dissolving the ReplicaSet...

The ReplicaSet was successfully dissolved.
Replication was disabled but user data was left intact.
`);

// ensure everything was cleaned
check_dissolved_replica(session, session);

//@<> Dissolve replica set with instances
shell.options.useWizards=1;

var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2);
rset.addInstance(__sandbox_uri3);

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

WIPE_OUTPUT();

testutil.expectPrompt("Are you sure you want to dissolve the ReplicaSet?", "y");
EXPECT_NO_THROWS(function() { rset.dissolve(); });

EXPECT_OUTPUT_CONTAINS("The ReplicaSet has the following registered instances:");
EXPECT_OUTPUT_CONTAINS_MULTILINE(`{
    "name": "rset",
    "topology": [
        {
            "address": "${hostname}:${__mysql_sandbox_port1}",
            "instanceRole": "PRIMARY",
            "label": "${hostname}:${__mysql_sandbox_port1}"
        },
        {
            "address": "${hostname}:${__mysql_sandbox_port2}",
            "instanceRole": "REPLICA",
            "label": "${hostname}:${__mysql_sandbox_port2}"
        },
        {
            "address": "${hostname}:${__mysql_sandbox_port3}",
            "instanceRole": "REPLICA",
            "label": "${hostname}:${__mysql_sandbox_port3}"
        }
    ]
}`);

EXPECT_OUTPUT_CONTAINS_MULTILINE(`
WARNING: You are about to dissolve the whole ReplicaSet. This operation cannot be reverted. All members will be removed from the ReplicaSet and replication will be stopped, internal recovery user accounts and the ReplicaSet metadata will be dropped. User data will be maintained intact in all instances.

Are you sure you want to dissolve the ReplicaSet? [y/N]:
** Connecting to ${hostname}:${__mysql_sandbox_port2}
** Connecting to ${hostname}:${__mysql_sandbox_port3}
* Waiting for instance '${hostname}:${__mysql_sandbox_port2}' to apply received transactions...


* Waiting for instance '${hostname}:${__mysql_sandbox_port3}' to apply received transactions...


* Dissolving the ReplicaSet...
* Waiting for instance '${hostname}:${__mysql_sandbox_port2}' to apply received transactions...


* Waiting for instance '${hostname}:${__mysql_sandbox_port3}' to apply received transactions...



The ReplicaSet was successfully dissolved.
Replication was disabled but user data was left intact.
`);

// ensure everything was cleaned
var session2 = mysql.getSession(__sandbox_uri2);
var session3 = mysql.getSession(__sandbox_uri3);

check_dissolved_replica(session, session);
check_dissolved_replica(session, session2);
check_dissolved_replica(session, session3);

//@<> Check calling 'dissolve' again
EXPECT_THROWS(function() { rset.dissolve(); }, "The replicaset object is disconnected. Please use dba.getReplicaSet() to obtain a new object.");

//@<> Dissolve fail because one instance is not reachable.
var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2);
rset.addInstance(__sandbox_uri3);

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

disable_auto_rejoin(__mysql_sandbox_port3);

testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

WIPE_OUTPUT();

EXPECT_THROWS(function() {
    testutil.expectPrompt("Are you sure you want to dissolve the ReplicaSet?", "y");
    testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:", "n");
    rset.dissolve();
}, `ReplicaSet.dissolve: The instance '${hostname}:${__mysql_sandbox_port3}' is 'UNREACHABLE'`);

EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port3}' cannot be removed because it is on a 'UNREACHABLE' state. Please bring the instance back ONLINE and try to dissolve the ReplicaSet again. If the instance is permanently not reachable, then you can choose to proceed with the operation and only remove the instance from the ReplicaSet Metadata.`);

//@<> Dissolve fail because one instance is not reachable: continue

EXPECT_NO_THROWS(function() {
    testutil.expectPrompt("Are you sure you want to dissolve the ReplicaSet?", "y");
    testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:", "y");
    rset.dissolve();
});

EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port3}' cannot be removed because it is on a 'UNREACHABLE' state. Please bring the instance back ONLINE and try to dissolve the ReplicaSet again. If the instance is permanently not reachable, then you can choose to proceed with the operation and only remove the instance from the ReplicaSet Metadata.`);
EXPECT_OUTPUT_CONTAINS("* Dissolving the ReplicaSet...");
EXPECT_OUTPUT_CONTAINS(`WARNING: The ReplicaSet was successfully dissolved, but the following instance was skipped: '${hostname}:${__mysql_sandbox_port3}'. Please make sure this instance is permanently unavailable or take any necessary manual action to ensure the ReplicaSet is fully dissolved.`);

var session2 = mysql.getSession(__sandbox_uri2);

check_dissolved_replica(session, session);
check_dissolved_replica(session, session2);

testutil.startSandbox(__mysql_sandbox_port3);
var session3 = mysql.getSession(__sandbox_uri3);
reset_instance(session3);

//@<> Dissolve fail because one instance is not reachable + force.
var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2);
rset.addInstance(__sandbox_uri3);

testutil.waitMemberTransactions(__mysql_sandbox_port2, __mysql_sandbox_port1);
testutil.waitMemberTransactions(__mysql_sandbox_port3, __mysql_sandbox_port1);

disable_auto_rejoin(__mysql_sandbox_port3);

testutil.stopSandbox(__mysql_sandbox_port3);
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

WIPE_OUTPUT();

EXPECT_NO_THROWS(function(){ rset.dissolve({force: true}); });

EXPECT_OUTPUT_CONTAINS("The ReplicaSet has the following registered instances:");
EXPECT_OUTPUT_CONTAINS(`NOTE: The instance '${hostname}:${__mysql_sandbox_port3}' is 'UNREACHABLE' and it will only be removed from the metadata. Please take any necessary actions to make sure that the instance will not start/join the ReplicaSet if brought back online.`);
EXPECT_OUTPUT_CONTAINS("* Dissolving the ReplicaSet...");
EXPECT_OUTPUT_CONTAINS(`WARNING: The ReplicaSet was successfully dissolved, but the following instance was skipped: '${hostname}:${__mysql_sandbox_port3}'. Please make sure this instance is permanently unavailable or take any necessary manual action to ensure the ReplicaSet is fully dissolved.`);

testutil.startSandbox(__mysql_sandbox_port3);
var session3 = mysql.getSession(__sandbox_uri3);
reset_instance(session3);

//@<> Dissolve where an instance has the async channel OFFLINE
shell.connect(__sandbox_uri1);

var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2);

shell.connect(__sandbox_uri2);
session.runSql("STOP REPLICA FOR CHANNEL ''");

WIPE_OUTPUT();

shell.options.useWizards=0;

EXPECT_THROWS(function() {
    rset.dissolve();
}, `ReplicaSet.dissolve: The instance '${hostname}:${__mysql_sandbox_port2}' is 'OFFLINE'`);
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port2}' cannot be removed because it is on a 'OFFLINE' state. Please bring the instance back ONLINE and try to dissolve the ReplicaSet again. If the instance is permanently not reachable, then please use <ReplicaSet>.dissolve() with the force option set to true to proceed with the operation and only remove the instance from the ReplicaSet Metadata.`);

shell.options.useWizards=1;

EXPECT_NO_THROWS(function() {
    testutil.expectPrompt("Are you sure you want to dissolve the ReplicaSet?", "y");
    testutil.expectPrompt("Do you want to continue anyway (only the instance metadata will be removed)? [y/N]:", "y");
    rset.dissolve();
});
EXPECT_OUTPUT_CONTAINS(`ERROR: The instance '${hostname}:${__mysql_sandbox_port2}' cannot be removed because it is on a 'OFFLINE' state. Please bring the instance back ONLINE and try to dissolve the ReplicaSet again. If the instance is permanently not reachable, then you can choose to proceed with the operation and only remove the instance from the ReplicaSet Metadata.`);
EXPECT_OUTPUT_NOT_CONTAINS(`* Waiting for instance '${hostname}:${__mysql_sandbox_port2}' to apply received transactions...`);

//check if msg from createReplicaSet mentions dba.dropMetadataSchema()
shell.connect(__sandbox_uri2);
try {
    dba.createReplicaSet('rset', {gtidSetIsComplete: true});
} catch (err) {
    msg = err.message.replaceAll('<','').replaceAll('>','');
    EXPECT_EQ(`Dba.createReplicaSet: Unable to create replicaset. The instance '${hostname}:${__mysql_sandbox_port2}' already belongs to a replicaset. Use dba.getReplicaSet() to access it or dba.dropMetadataSchema() to drop the metadata if the replicaset was dissolved.`, msg);
}

shell.connect(__sandbox_uri2);
reset_instance(session);
shell.connect(__sandbox_uri1);
reset_instance(session);

shell.options.useWizards=0;

//@<> Dissolve fail due to timeout {!__dbug_off}
var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2);
rset.addInstance(__sandbox_uri3);

WIPE_OUTPUT();

testutil.dbugSet("+d,dba_sync_transactions_timeout");

EXPECT_THROWS(function() {
    rset.dissolve();
}, `Timeout reached waiting for all received transactions to be applied on instance '${hostname}:${__mysql_sandbox_port2}'`);

EXPECT_OUTPUT_CONTAINS(`* Waiting for instance '${hostname}:${__mysql_sandbox_port2}' to apply received transactions...`);
EXPECT_OUTPUT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' was unable to catch up with cluster transactions. There might be too many transactions to apply or some replication error. In the former case, you can retry the operation (using a higher timeout value). In the later case, analyze and fix any replication error. You can also choose to skip this error using the 'force: true' option, but it might leave the instance in an inconsistent state and lead to errors if you want to reuse it.`);

WIPE_OUTPUT();

EXPECT_NO_THROWS(function(){ rset.dissolve({force: true}); });

EXPECT_OUTPUT_CONTAINS(`* Waiting for instance '${hostname}:${__mysql_sandbox_port2}' to apply received transactions...`);
EXPECT_OUTPUT_NOT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port2}' was unable to catch up with cluster transactions. There might be too many transactions to apply or some replication error. In the former case, you can retry the operation (using a higher timeout value). In the later case, analyze and fix any replication error. You can also choose to skip this error using the 'force: true' option, but it might leave the instance in an inconsistent state and lead to errors if you want to reuse it.`);
EXPECT_OUTPUT_CONTAINS(`* Waiting for instance '${hostname}:${__mysql_sandbox_port3}' to apply received transactions...`);
EXPECT_OUTPUT_NOT_CONTAINS(`The instance '${hostname}:${__mysql_sandbox_port3}' was unable to catch up with cluster transactions. There might be too many transactions to apply or some replication error. In the former case, you can retry the operation (using a higher timeout value). In the later case, analyze and fix any replication error. You can also choose to skip this error using the 'force: true' option, but it might leave the instance in an inconsistent state and lead to errors if you want to reuse it.`);
EXPECT_OUTPUT_CONTAINS("* Dissolving the ReplicaSet...");
EXPECT_OUTPUT_CONTAINS(`WARNING: An error occurred when trying to catch up with the ReplicaSet transactions and instance '${hostname}:${__mysql_sandbox_port2}' might have been left in an inconsistent state that will lead to errors if it is reused.`);
EXPECT_OUTPUT_CONTAINS(`WARNING: An error occurred when trying to catch up with the ReplicaSet transactions and instance '${hostname}:${__mysql_sandbox_port3}' might have been left in an inconsistent state that will lead to errors if it is reused.`);
EXPECT_OUTPUT_NOT_CONTAINS("The ReplicaSet was successfully dissolved.");
EXPECT_OUTPUT_CONTAINS(`The ReplicaSet was successfully dissolved, but the following instances weren't properly dissolved: '${hostname}:${__mysql_sandbox_port2}', '${hostname}:${__mysql_sandbox_port3}'. Please make sure the ReplicaSet metadata and replication channel were removed from these instances in order to be able to be reused.`);

testutil.dbugSet("");

//@<> Dissolve doesn't stop due to error in replication cleanup {!__dbug_off}
var rset = dba.createReplicaSet('rset', {gtidSetIsComplete: true});
rset.addInstance(__sandbox_uri2);
rset.addInstance(__sandbox_uri3);

WIPE_OUTPUT();

testutil.dbugSet("+d,dba_dissolve_replication_error");

EXPECT_NO_THROWS(function(){ rset.dissolve(); });

EXPECT_OUTPUT_CONTAINS(`* Waiting for instance '${hostname}:${__mysql_sandbox_port2}' to apply received transactions...`);
EXPECT_OUTPUT_CONTAINS(`An error occurred when trying to remove instance '${hostname}:${__mysql_sandbox_port2}' from the ReplicaSet. The instance might have been left active and in an inconsistent state, requiring manual action to fully dissolve the ReplicaSet.`);
EXPECT_OUTPUT_CONTAINS(`* Waiting for instance '${hostname}:${__mysql_sandbox_port3}' to apply received transactions...`);
EXPECT_OUTPUT_CONTAINS(`An error occurred when trying to remove instance '${hostname}:${__mysql_sandbox_port3}' from the ReplicaSet. The instance might have been left active and in an inconsistent state, requiring manual action to fully dissolve the ReplicaSet.`);
EXPECT_OUTPUT_CONTAINS(`The ReplicaSet was successfully dissolved, but the following instances weren't properly dissolved: '${hostname}:${__mysql_sandbox_port2}', '${hostname}:${__mysql_sandbox_port3}'. Please make sure the ReplicaSet metadata and replication channel were removed from these instances in order to be able to be reused.`);

testutil.dbugSet("");

//@<> Finalization
session2.close();
session3.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
