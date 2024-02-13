// Assumptions: ensure_schema_does_not_exist is available
// Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);
var singleSession = session;

var devCluster = dba.createCluster('devCluster', {memberSslMode:'REQUIRED', gtidSetIsComplete: true});

devCluster.disconnect();

//@<> Cluster: validating members
var Cluster = dba.getCluster('devCluster');

validateMembers(Cluster, [
  'name',
  'getClusterSet',
  'getName',
  'addInstance',
  'removeInstance',
  'rejoinInstance',
  'describe',
  'status',
  'help',
  'dissolve',
  'disconnect',
  'rescan',
  'listRouters',
  'removeRouterMetadata',
  'resetRecoveryAccountsPassword',
  'forceQuorumUsingPartitionOf',
  'switchToSinglePrimaryMode',
  'switchToMultiPrimaryMode',
  'setPrimaryInstance',
  'options',
  'setOption',
  'setInstanceOption',
  'setupAdminAccount',
  'setupRouterAccount',
  'createClusterSet',
  'fenceAllTraffic',
  'fenceWrites',
  'unfenceWrites',
  'addReplicaInstance',
  'routingOptions',
  'setRoutingOption'
])

//@<> Cluster: addInstance errors
EXPECT_THROWS(function (){
    Cluster.addInstance();
}, "Invalid number of arguments, expected 1 to 2 but got 0");

EXPECT_THROWS(function (){
    Cluster.addInstance(5,6,7,1);
}, "Invalid number of arguments, expected 1 to 2 but got 4");

EXPECT_THROWS(function (){
    Cluster.addInstance(5, 5);
}, "Argument #2 is expected to be a map");

EXPECT_THROWS(function (){
    Cluster.addInstance('', 5);
}, "Argument #2 is expected to be a map");

EXPECT_THROWS(function (){
    Cluster.addInstance("");
}, "Argument #1: Invalid URI: empty");

EXPECT_THROWS(function (){
    Cluster.addInstance({user:"sample", weird:1},{});
}, "Invalid values in connection options: weird");

EXPECT_THROWS(function (){
    Cluster.addInstance({host: "localhost", schema: 'abs', user:"sample", "auth-method":56, memberSslMode: "foo", ipWhitelist: " "});
}, "Invalid values in connection options: ipWhitelist, memberSslMode");

EXPECT_THROWS(function (){
    Cluster.addInstance({user: "root", host: "localhost", port:__mysql_sandbox_port2}, "root");
}, "Argument #2 is expected to be a map");

EXPECT_THROWS(function (){
    Cluster.addInstance({HoSt:'localhost', port: __mysql_sandbox_port1, PassWord:'root', scheme:'mysql', user: 'root'}, {});
}, `The instance '${hostname}:${__mysql_sandbox_port1}' is already part of this InnoDB Cluster`);

EXPECT_THROWS(function (){
    Cluster.addInstance({user: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: ""});
}, "The label can not be empty.");

EXPECT_THROWS(function (){
    Cluster.addInstance({user: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "#invalid"});
}, "The label can only start with an alphanumeric or the '_' character.");

EXPECT_THROWS(function (){
    Cluster.addInstance({user: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "invalid#char"});
}, "The label can only contain alphanumerics or the '_', '.', '-', ':' characters. Invalid character '#' found.");

EXPECT_THROWS(function (){
Cluster.addInstance({user: "root", host: "localhost", port:__mysql_sandbox_port2}, {label: "over256chars_1234567890123456789012345678990123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123"});
}, "The label can not be greater than 256 characters.");

//@<> Cluster: addInstance 2
testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");
EXPECT_NO_THROWS(function (){ Cluster.addInstance(__sandbox_uri2, {'label': '2nd'}); });

// Third instance will be added while the second is still on RECOVERY
//@<> Cluster: addInstance 3
EXPECT_NO_THROWS(function (){ Cluster.addInstance(__sandbox_uri3); });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Cluster: describe cluster with instance
Cluster.describe();
EXPECT_OUTPUT_CONTAINS_MULTILINE(`{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "${hostname}:${__mysql_sandbox_port1}",
                "label": "${hostname}:${__mysql_sandbox_port1}",
                "role": "HA"
            },
            {
                "address": "${hostname}:${__mysql_sandbox_port2}",
                "label": "2nd",
                "role": "HA"
            },
            {
                "address": "${hostname}:${__mysql_sandbox_port3}",
                "label": "${hostname}:${__mysql_sandbox_port3}",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}`);

//@<> Cluster: status cluster with instance
var status = Cluster.status();
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["2nd"]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["2nd"]["address"], `${hostname}:${__mysql_sandbox_port2}`);

//@<> Cluster: removeInstance errors
EXPECT_THROWS(function (){ Cluster.removeInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function (){ Cluster.removeInstance(1,2,3); }, "Invalid number of arguments, expected 1 to 2 but got 3");
EXPECT_THROWS(function (){ Cluster.removeInstance(1); }, "Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary");
EXPECT_THROWS(function (){ Cluster.removeInstance({host: "localhost", schema: 'abs', user:"sample", fakeOption:56}); }, "Argument #1: Invalid values in connection options: fakeOption");

// try to remove instance that is not in the cluster using the classic port
EXPECT_THROWS(function (){ Cluster.removeInstance({user: __user, host: __host, port: __mysql_port, password: shell.parseUri(__uripwd).password}); }, `Metadata for instance ${__host}:${__mysql_port} not found`);

//@<> Cluster: removeInstance read only
EXPECT_NO_THROWS(function (){ Cluster.removeInstance({host: "localhost", port:__mysql_sandbox_port2}); });

//@<> Cluster: describe removed read only member
Cluster.describe();
EXPECT_OUTPUT_CONTAINS_MULTILINE(`{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "${hostname}:${__mysql_sandbox_port1}",
                "label": "${hostname}:${__mysql_sandbox_port1}",
                "role": "HA"
            },
            {
                "address": "${hostname}:${__mysql_sandbox_port3}",
                "label": "${hostname}:${__mysql_sandbox_port3}",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}`);

//@<> Cluster: status removed read only member
var status = Cluster.status();
EXPECT_EQ(2, Object.keys(status["defaultReplicaSet"]["topology"]).length)
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"], "ONLINE");

//@<> Cluster: addInstance read only back
EXPECT_NO_THROWS(function (){ Cluster.addInstance(__sandbox_uri2); });

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> Cluster: describe after adding read only instance back
Cluster.describe();
EXPECT_OUTPUT_CONTAINS_MULTILINE(`{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "${hostname}:${__mysql_sandbox_port1}",
                "label": "${hostname}:${__mysql_sandbox_port1}",
                "role": "HA"
            },
            {
                "address": "${hostname}:${__mysql_sandbox_port3}",
                "label": "${hostname}:${__mysql_sandbox_port3}",
                "role": "HA"
            },
            {
                "address": "${hostname}:${__mysql_sandbox_port2}",
                "label": "${hostname}:${__mysql_sandbox_port2}",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}`);

//@<> Cluster: status after adding read only instance back
var status = Cluster.status();
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port1}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port3}`]["status"], "ONLINE");

// Make sure uri2 is selected as the new master
Cluster.removeInstance(__sandbox_uri3);

//@<> Cluster: remove_instance master
EXPECT_NO_THROWS(function (){ Cluster.removeInstance(__sandbox_uri1); });

//@<> Cluster: no operations can be done on a disconnected cluster
EXPECT_NO_THROWS(function (){ Cluster.disconnect(); });

EXPECT_THROWS(function (){ Cluster.addInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function (){ Cluster.describe(); }, "The cluster object is disconnected. Please use dba.getCluster() to obtain a fresh cluster handle");
EXPECT_THROWS(function (){ Cluster.dissolve(); }, "The cluster object is disconnected. Please use dba.getCluster() to obtain a fresh cluster handle");
EXPECT_THROWS(function (){ Cluster.forceQuorumUsingPartitionOf(); }, "Invalid number of arguments, expected 1 but got 0");
EXPECT_THROWS(function (){ Cluster.rejoinInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function (){ Cluster.removeInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function (){ Cluster.rescan(); }, "The cluster object is disconnected. Please use dba.getCluster() to obtain a fresh cluster handle");
EXPECT_THROWS(function (){ Cluster.status(); }, "The cluster object is disconnected. Please use dba.getCluster() to obtain a fresh cluster handle");

//@<> Connecting to new master
shell.connect(__sandbox_uri2);
var Cluster = dba.getCluster();

// Add back uri3
Cluster.addInstance(__sandbox_uri3, {'label': '3rd_sandbox'});
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Cluster: describe on new master
Cluster.describe();
EXPECT_OUTPUT_CONTAINS_MULTILINE(`{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "${hostname}:${__mysql_sandbox_port2}",
                "label": "${hostname}:${__mysql_sandbox_port2}",
                "role": "HA"
            },
            {
                "address": "${hostname}:${__mysql_sandbox_port3}",
                "label": "3rd_sandbox",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}`);

//@<> Cluster: status on new master
var status = Cluster.status();
EXPECT_EQ(2, Object.keys(status["defaultReplicaSet"]["topology"]).length)
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"], "R/W");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]['3rd_sandbox']["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["3rd_sandbox"]["address"], `${hostname}:${__mysql_sandbox_port3}`);

//@<> Cluster: addInstance adding old master as read only
EXPECT_NO_THROWS(function (){ Cluster.addInstance(__sandbox_uri1, {'label': '1st_sandbox'}); });

testutil.waitMemberState(__mysql_sandbox_port1, "ONLINE");

//@<> Cluster: describe on new master with slave
Cluster.describe();
EXPECT_OUTPUT_CONTAINS_MULTILINE(`{
    "clusterName": "devCluster",
    "defaultReplicaSet": {
        "name": "default",
        "topology": [
            {
                "address": "${hostname}:${__mysql_sandbox_port2}",
                "label": "${hostname}:${__mysql_sandbox_port2}",
                "role": "HA"
            },
            {
                "address": "${hostname}:${__mysql_sandbox_port3}",
                "label": "3rd_sandbox",
                "role": "HA"
            },
            {
                "address": "${hostname}:${__mysql_sandbox_port1}",
                "label": "1st_sandbox",
                "role": "HA"
            }
        ],
        "topologyMode": "Single-Primary"
    }
}`);

//@<> Cluster: status on new master with slave
var status = Cluster.status();
EXPECT_EQ(status["defaultReplicaSet"]["topology"]['1st_sandbox']["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["1st_sandbox"]["address"], `${hostname}:${__mysql_sandbox_port1}`);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"], "R/W");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]['3rd_sandbox']["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["3rd_sandbox"]["address"], `${hostname}:${__mysql_sandbox_port3}`);

// Rejoin tests

//@<> Dba: kill instance 3
testutil.killSandbox(__mysql_sandbox_port3);

// Since the cluster has quorum, the instance will be kicked off the
// Cluster going OFFLINE->UNREACHABLE->(MISSING)
testutil.waitMemberState(__mysql_sandbox_port3, "(MISSING)");

//@<> Dba: start instance 3
testutil.startSandbox(__mysql_sandbox_port3);

//@<> Cluster: rejoinInstance errors
EXPECT_THROWS(function (){ Cluster.rejoinInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function (){ Cluster.rejoinInstance(1,2,3); }, "Invalid number of arguments, expected 1 to 2 but got 3");
EXPECT_THROWS(function (){ Cluster.rejoinInstance(1); }, "Invalid connection options, expected either a URI or a Connection Options Dictionary");
EXPECT_THROWS(function (){ Cluster.rejoinInstance({host: "localhost", schema: 'abs', "auth-method":56, memberSslMode: "foo", ipWhitelist: " "}); }, "Invalid values in connection options: ipWhitelist, memberSslMode");
EXPECT_THROWS(function (){ Cluster.rejoinInstance({host: "localhost"}); }, "Could not open connection to 'localhost'");
EXPECT_THROWS(function (){ Cluster.rejoinInstance("localhost:3306"); }, "Could not open connection to 'localhost:3306'");
EXPECT_THROWS(function (){ Cluster.rejoinInstance("somehost:3306", "root"); }, "Argument #2 is expected to be a map");

//@<> Dba: rejoin instance 3 ok {VER(<8.0.11)}
EXPECT_NO_THROWS(function (){ Cluster.rejoinInstance({user: "root", host: "localhost", port:__mysql_sandbox_port3}, {memberSslMode: "AUTO", "password": "root"}); });

testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Dba: Wait instance 3 ONLINE {VER(>=8.0.11)}
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

// Verify if the cluster is OK

//@<> Cluster: status for rejoin: success
var status = Cluster.status();
EXPECT_EQ(status["defaultReplicaSet"]["topology"]['1st_sandbox']["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["1st_sandbox"]["address"], `${hostname}:${__mysql_sandbox_port1}`);
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["mode"], "R/W");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]['3rd_sandbox']["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"]["3rd_sandbox"]["address"], `${hostname}:${__mysql_sandbox_port3}`);

//@<> Cluster: dissolve errors
EXPECT_THROWS(function (){ Cluster.dissolve(1); }, "Argument #1 is expected to be a map");
EXPECT_THROWS(function (){ Cluster.dissolve(1,2); }, "Invalid number of arguments, expected 0 to 1 but got 2");
EXPECT_THROWS(function (){ Cluster.dissolve(""); }, "Argument #1 is expected to be a map");
EXPECT_THROWS(function (){ Cluster.dissolve({foobar: true}); }, "Invalid options: foobar");
EXPECT_THROWS(function (){ Cluster.dissolve({force: "whatever"}); }, "Option 'force' Bool expected, but value is String");

//@<> Cluster: final dissolve
shell.options["useWizards"] = false;
EXPECT_NO_THROWS(function (){ Cluster.dissolve({force: true}); });
shell.options["useWizards"] = true;

//@<> Cluster: no operations can be done on a dissolved cluster
EXPECT_EQ("devCluster", Cluster.getName());

EXPECT_THROWS(function (){ Cluster.addInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function (){ Cluster.describe(); }, "Can't call function 'describe' on an offline Cluster");
EXPECT_THROWS(function (){ Cluster.dissolve(); }, "Can't call function 'dissolve' on an offline Cluster");
EXPECT_THROWS(function (){ Cluster.forceQuorumUsingPartitionOf(); }, "Invalid number of arguments, expected 1 but got 0");
EXPECT_THROWS(function (){ Cluster.rejoinInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function (){ Cluster.removeInstance(); }, "Invalid number of arguments, expected 1 to 2 but got 0");
EXPECT_THROWS(function (){ Cluster.rescan(); }, "Can't call function 'rescan' on an offline Cluster");
EXPECT_THROWS(function (){ Cluster.status(); }, "Can't call function 'status' on an offline Cluster");

//@<> Cluster: disconnect should work, tho
EXPECT_NO_THROWS(function (){ Cluster.disconnect(); });

//@<> Cleanup
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);
