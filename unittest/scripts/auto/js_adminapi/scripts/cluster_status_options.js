
// how many rows of data to insert in seed instance, so that recovery takes long
// enough to be observable in the test
const NUM_DATA_ROWS = 20;

// Setup
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port3, "root", {"slave_parallel_workers":2, "slave_preserve_commit_order":1, "slave_parallel_type": "LOGICAL_CLOCK", "slave_preserve_commit_order":1, report_host: hostname});

shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});

// Create a test dataset so that RECOVERY takes a while
session.runSql("create schema test");
session.runSql("create table test.data (a int primary key auto_increment, data longtext)");
for (i = 0; i < NUM_DATA_ROWS; i++) {
    session.runSql("insert into test.data values (default, repeat('x', 4*1024*1024))");
}

function json_check_fields(json, template) {
    function normtype(t) {
        t = type(t);
        if (t == "m.Map")
            return "Object";
        else if (t == "m.Array")
            return "Array";
        else if (t == "Integer")
            return "Number";
        return t;
    }
    var missing = [];
    var unexpected = [];
    for (var k in template) {
        if (k == "") {
            // any field name (used for topology -> localhost:#### fields)
            for (var jk in json) {
                if (json[k] != null)
                    EXPECT_EQ(normtype(template[k]), normtype(json[jk]), k);
                var r = json_check_fields(json[jk], template[k]);
                missing = missing.concat(r["missing"]);
                unexpected = unexpected.concat(r["unexpected"]);
            }
        } else if (k in json) {
            if (json[k] != null)
                EXPECT_EQ(normtype(template[k]), normtype(json[k]), k);
            if (normtype(template[k]) == 'Object') {
                var r = json_check_fields(json[k], template[k]);
                missing = missing.concat(r["missing"]);
                unexpected = unexpected.concat(r["unexpected"]);
            }
        } else {
            missing.push(k);
        }
    }
    for (var k in json) {
        if (k in template || "" in template) {
        } else {
            unexpected.push(k);
        }
    }
    return {missing:missing, unexpected:unexpected};
}

function json_check(json, template, allowed_missing, allowed_unexpected) {
    if (!allowed_missing) allowed_missing = [];
    if (!allowed_unexpected) allowed_unexpected = [];
    var r = json_check_fields(json, template);
    var nlist = [];
    for (var index in r["missing"]) {
        var missing = r["missing"][index];
        if (-1 == allowed_missing.indexOf(missing)) {
            nlist.push(missing);
        }
    }
    r["missing"] = nlist;
    nlist = [];
    for (var index in r["unexpected"]) {
        var unexpected = r["unexpected"][index];
        if (-1 == allowed_unexpected.indexOf(unexpected)) {
            nlist.push(unexpected);
        }
    }
    r["unexpected"] = nlist;

    EXPECT_EQ([], r["missing"], "Missing fields");
    EXPECT_EQ([], r["unexpected"], "Unexpected fields");
    if (r["missing"].length > 0 || r["unexpected"].length > 0) {
        println(r);
        println(json);
    }
}

const base_status_templ_80 = {
    "clusterName": "",
    "defaultReplicaSet": {
    "name": "",
        "primary": "",
        "ssl": "",
        "status": "",
        "statusText": "",
        "topology": {
            "": {
                "address": "",
                "mode": "",
                "readReplicas": {},
                "replicationLag": "",
                "role": "",
                "status": "",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": ""
};

const base_status_templ_57 = {
    "clusterName": "",
    "defaultReplicaSet": {
        "name": "",
        "primary": "",
        "ssl": "",
        "status": "",
        "statusText": "",
        "topology": {
            "": {
                "address": "",
                "mode": "",
                "readReplicas": {},
                "role": "",
                "status": ""
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": ""
};

const extended_1_status_templ_80 = {
    "clusterName": "",
    "defaultReplicaSet": {
        "GRProtocolVersion": "",
        "name": "",
        "groupName": "",
        "primary": "",
        "ssl": "",
        "status": "",
        "statusText": "",
        "topology": {
            "": {
                "address": "",
                "fenceSysVars": [],
                "memberId": "",
                "memberRole": "",
                "memberState": "",
                "mode": "",
                "readReplicas": {},
                "replicationLag": "",
                "role": "",
                "status": "",
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": ""
};

const extended_1_status_templ_57 = {
    "clusterName": "",
    "defaultReplicaSet": {
        "GRProtocolVersion": "",
        "name": "",
        "groupName": "",
        "primary": "",
        "ssl": "",
        "status": "",
        "statusText": "",
        "topology": {
            "": {
                "address": "",
                "fenceSysVars": [],
                "memberId": "",
                "memberRole": "",
                "memberState": "",
                "mode": "",
                "readReplicas": {},
                "role": "",
                "status": "",
            }
        },
        "topologyMode": "Single-Primary"
    },
    "groupInformationSourceMember": ""
};

const extended_2_status_templ_80 = {
    "clusterName": "",
    "defaultReplicaSet": {
        "GRProtocolVersion": "",
        "name": "",
        "groupName": "",
        "primary": "",
        "ssl": "",
        "status": "",
        "statusText": "",
        "topology": {
            "": {
                "address": "",
                "fenceSysVars": [],
                "memberId": "",
                "memberRole": "",
                "memberState": "",
                "mode": "",
                "readReplicas": {},
                "role": "",
                "status": "",
                "transactions": {
                    "appliedCount": 0,
                    "checkedCount": 0, 
                    "committedAllMembers": "", 
                    "conflictsDetectedCount": 0, 
                    "inApplierQueueCount": 0, 
                    "inQueueCount": 0, 
                    "lastConflictFree": "", 
                    "proposedCount": 0, 
                    "rollbackCount": 0
                },
                "version": "<<<__version>>>"
            }
        },
        "topologyMode": "Single-Primary"
    }, 
    "groupInformationSourceMember": ""
};

const extended_2_status_templ_57 = {
    "clusterName": "",
    "defaultReplicaSet": {
        "GRProtocolVersion": "",
        "name": "", 
        "groupName": "",
        "primary": "",
        "ssl": "", 
        "status": "", 
        "statusText": "", 
        "topology": {
            "": {
                "address": "",
                "fenceSysVars": [],
                "memberId": "",
                "memberRole": "",
                "memberState": "",
                "mode": "", 
                "readReplicas": {},
                "role": "", 
                "status": "", 
                "transactions": {
                    "checkedCount": 0, 
                    "committedAllMembers": "", 
                    "conflictsDetectedCount": 0, 
                    "inQueueCount": 0,
                    "lastConflictFree" : ""
                }
            }
        },
        "topologyMode": "Single-Primary"
    }, 
    "groupInformationSourceMember": ""
};

const full_status_templ_80 = {
    "clusterName": "",
    "defaultReplicaSet": {
        "GRProtocolVersion": "",
        "name": "", 
        "groupName": "",
        "primary": "",
        "ssl": "", 
        "status": "", 
        "statusText": "", 
        "topology": {
            "": {
                "address": "",
                "fenceSysVars": [],
                "memberId": "",
                "memberRole": "",
                "memberState": "",
                "mode": "", 
                "readReplicas": {},
                "role": "", 
                "status": "", 
                "version": "<<<__version>>>", 
                "transactions": {
                    "appliedCount": 0, 
                    "checkedCount": 0, 
                    "committedAllMembers": "", 
                    "conflictsDetectedCount": 0, 
                    "connection": {
                        "lastHeartbeatTimestamp": "", 
                        "lastQueued": {
                            "endTimestamp": "", 
                            "immediateCommitTimestamp": "", 
                            "immediateCommitToEndTime": 0.0, 
                            "originalCommitTimestamp": "", 
                            "originalCommitToEndTime": 0.0, 
                            "queueTime": 0.1, 
                            "startTimestamp": "", 
                            "transaction": ""
                        }, 
                        "receivedHeartbeats": 0, 
                        "receivedTransactionSet": "",
                        "threadId": 0
                    }, 
                    "inApplierQueueCount": 0, 
                    "inQueueCount": 0, 
                    "lastConflictFree": "", 
                    "proposedCount": 0, 
                    "rollbackCount": 0, 
                    "workers": [
                        {
                            "lastApplied": {
                                "applyTime": 0.0, 
                                "endTimestamp": "", 
                                "immediateCommitTimestamp": "", 
                                "immediateCommitToEndTime": 0.0, 
                                "originalCommitTimestamp": "", 
                                "originalCommitToEndTime": 0.0, 
                                "startTimestamp": "", 
                                "transaction": ""
                            }, 
                            "threadId": 0
                        }
                    ]
                }
            }
        },
        "topologyMode": "Single-Primary"
    }, 
    "groupInformationSourceMember": ""
};

const full_status_templ_57 = {
    "clusterName": "",
    "defaultReplicaSet": {
        "GRProtocolVersion": "",
        "name": "", 
        "groupName": "",
        "primary": "",
        "ssl": "", 
        "status": "", 
        "statusText": "", 
        "topology": {
            "": {
                "address": "",
                "fenceSysVars": [],
                "memberId": "",
                "memberRole": "",
                "memberState": "",
                "mode": "", 
                "readReplicas": {},
                "role": "", 
                "status": "", 
                "version": "<<<__version>>>", 
                "transactions": {
                    "checkedCount": 0, 
                    "committedAllMembers": "", 
                    "conflictsDetectedCount": 0, 
                    "connection": {
                        "lastHeartbeatTimestamp": "", 
                        "receivedHeartbeats": 0, 
                        "receivedTransactionSet": "",
                        "threadId": 0
                    },
                    "inQueueCount": 0, 
                    "lastConflictFree": "", 
                    "workers": [
                        {
                            "lastSeen": "",
                            "threadId": 0
                        }
                    ]
                }
            }
        },
        "topologyMode": "Single-Primary"
    }, 
    "groupInformationSourceMember": ""
};

const transaction_status_templ = {
    "appliedCount": 0, 
    "checkedCount": 0, 
    "committedAllMembers": "", 
    "conflictsDetectedCount": 0, 
    "connection": {
        "currentlyQueueing": {
            "immediateCommitTimestamp": "", 
            "immediateCommitToNowTime": 0.0, 
            "originalCommitTimestamp": "", 
            "originalCommitToNowTime": 0.0, 
            "startTimestamp": "", 
            "transaction": ""
        }, 
        "lastHeartbeatTimestamp": "", 
        "lastQueued": {
            "endTimestamp": "", 
            "immediateCommitTimestamp": "", 
            "immediateCommitToEndTime": 0.0, 
            "originalCommitTimestamp": "", 
            "originalCommitToEndTime": 0.0, 
            "queueTime": 0.1, 
            "startTimestamp": "", 
            "transaction": ""
        }, 
        "receivedHeartbeats": 0, 
        "receivedTransactionSet": "",
        "threadId": 0
    }, 
    "inApplierQueueCount": 0, 
    "inQueueCount": 0, 
    "lastConflictFree": "", 
    "proposedCount": 0, 
    "rollbackCount": 0, 
    "workers": [
        {
            "lastApplied": {
                "applyTime": 0.0, 
                "endTimestamp": "", 
                "immediateCommitTimestamp": "", 
                "immediateCommitToEndTime": 0.0, 
                "originalCommitTimestamp": "", 
                "originalCommitToEndTime": 0.0, 
                "startTimestamp": "", 
                "transaction": ""
            }, 
            "threadId": 0
        }
    ]
};

const coordinator_status_templ_80 = {
    "lastProcessed": {
        "bufferTime": 0.0, 
        "endTimestamp": "", 
        "immediateCommitTimestamp": "", 
        "immediateCommitToEndTime": null, 
        "originalCommitTimestamp": "", 
        "originalCommitToEndTime": 0, 
        "startTimestamp": "", 
        "transaction": ""
    }, 
    "threadId": 0
};

const coordinator_status_templ_57 = {
    "threadId": 0
};

const distributed_recovery_status_templ = {
    "state": ""
};

const clone_recovery_status_templ = {
    "cloneStartTime": "",
    "cloneState": "",
    "currentStage": "",
    "currentStageProgress": 0,
    "currentStageState": ""
};

//---

//@<> Errors using invalid options and values.
// Invalid option.
EXPECT_THROWS(function(){cluster.status({wrong_option: true})}, "Cluster.status: Invalid options: wrong_option");
// Invalid value type string for extended.
EXPECT_THROWS(function(){cluster.status({extended: ""})}, "Cluster.status: Option 'extended' UInteger expected, but value is String");
// Invalid value type (negative) integer for extended.
EXPECT_THROWS(function(){cluster.status({extended: -1})}, "Cluster.status: Option 'extended' UInteger expected, but Integer value is out of range");
// Invalid value (out of range) for extended.
EXPECT_THROWS(function(){cluster.status({extended: 4})}, "Cluster.status: Invalid value '4' for option 'extended'. It must be an integer in the range [0, 3].");
// Invalid value type float for extended.
EXPECT_THROWS(function(){cluster.status({extended: 1.5})}, "Option 'extended' UInteger expected, but Float value is out of range");
// Invalid value type string for queryMembers.
EXPECT_THROWS(function(){cluster.status({queryMembers: ""})}, "Cluster.status: Option 'queryMembers' Bool expected, but value is String");


//@<> WL#13084 - TSF4_2: extended: 1 provides the following information:
// - Group Protocol Version;
// - Group name (UUID);
// - Each cluster member UUID;
// - Role of the cluster member as reported by GR;
// - Status of the cluster member as reported by Group Replication;
// - A list with the enabled fencing sysvars (read_only, super_read_only, offline_mode);
var stat = cluster.status({extended: 1});
var gr_protocol_version = json_find_key(stat, "GRProtocolVersion");
EXPECT_NE(undefined, gr_protocol_version);
var group_name = json_find_key(stat, "groupName");
EXPECT_NE(undefined, group_name);
var member_id = json_find_key(stat, "memberId");
EXPECT_NE(undefined, member_id);
var member_role = json_find_key(stat, "memberRole");
EXPECT_NE(undefined, member_role);
var member_state = json_find_key(stat, "memberState");
EXPECT_NE(undefined, member_state);
var fence_sys_vars = json_find_key(stat, "fenceSysVars");
EXPECT_NE(undefined, fence_sys_vars);

//@<> WL#13084 - TSF4_2: verify status result with extended:1 for 8.0 {VER(>=8.0)}
json_check(stat, extended_1_status_templ_80);

//@<> WL#13084 - TSF4_2: verify status result with extended:1 for 5.7 {VER(<8.0)}
json_check(stat, extended_1_status_templ_57);

//@<> WL#13084 - TSF4_2: extended: 1 is the same as extended: true
var stat_ext_true = cluster.status({extended: true});
EXPECT_EQ(stat, stat_ext_true);

//@<> Basics

// TS1_3	Verify that information about additional transactions stats is printed when using cluster.status() and the option extended is set to true.
// TS1_6	Validate that the information about number of transactions processed by each member of the group is printed under a sub-object named transactions.

var stat = cluster.status({extended:2});

//@<> WL#13084 - TSF4_3: verify status result with extended:2 for 8.0 {VER(>=8.0)}
json_check(stat, extended_2_status_templ_80, [], ["replicationLag"]);

//@<> WL#13084 - TSF4_3: verify status result with extended:2 for 5.7 {VER(<8.0)}
json_check(stat, extended_2_status_templ_57);

//@<> WL#13084 - TSF4_3: extended: 2 also includes the transaction information.
var gr_protocol_version = json_find_key(stat, "GRProtocolVersion");
EXPECT_NE(undefined, gr_protocol_version);
var group_name = json_find_key(stat, "groupName");
EXPECT_NE(undefined, group_name);
var member_id = json_find_key(stat, "memberId");
EXPECT_NE(undefined, member_id);
var member_role = json_find_key(stat, "memberRole");
EXPECT_NE(undefined, member_role);
var member_state = json_find_key(stat, "memberState");
EXPECT_NE(undefined, member_state);
var fence_sys_vars = json_find_key(stat, "fenceSysVars");
EXPECT_NE(undefined, fence_sys_vars);
var transactions = json_find_key(stat, "transactions");
EXPECT_NE(undefined, transactions);

// TS_E4	Validate that when calling cluster.status() with extended and queryMembers options set to false, information about additional transactions stats is not printed.
var stat = cluster.status({extended:2, queryMembers:false});

//@<> 8.0 execution 2 {VER(>=8.0)}
json_check(stat, extended_2_status_templ_80, [], ["replicationLag"]);

//@<> 5.7 execution 2 {VER(<8.0)}
json_check(stat, extended_2_status_templ_57);

// TS_E2	Verify that giving no boolean values to the new dictionary options throws an exception when calling cluster.status().
EXPECT_THROWS(function(){cluster.status({extended:"yes", queryMembers:"maybe"})}, "Option 'extended' UInteger expected, but value is String");

// TS1_2 - Verify that information about additional transactions stats is not printed when using cluster.status() and the option extended is not given or is set to false.
//@<> F2- default 8.0 {VER(>=8.0)}
var stat = cluster.status();
json_check(stat, base_status_templ_80);

//@<> F2- default 5.7 {VER(<8.0)}
var stat = cluster.status();
json_check(stat, base_status_templ_57);

// TS1_5    Verify that information about additional transactions stats is printed by each member of the group when using cluster.status() and the option queryMembers is set to true.
// TS4_1    Verify that the I/O thread and applied workers information is added to the status of a member when calling cluster.status().
// TS7_1    Verify that information about the regular transaction processed is added to the status of a member that is Online when calling cluster.status().
// TS10_1   Validate that information about member_id/server_uuid and GR group name is added to the output when calling cluster.status() if brief is set to false.
// WL#13084 - TSF4_4: extended: 3 includes transactions information (same as queryMembers: true).

//@<> F3- queryMembers (deprecated and replaced by extended: 3) for 8.0 {VER(>=8.0)}
var stat = cluster.status({extended: 3});
json_check(stat, full_status_templ_80);

//@<> F3- queryMembers (deprecated and replaced by extended: 3) for 5.7 {VER(<8.0)}
var stat = cluster.status({extended: 3});
json_check(stat, full_status_templ_57);

// WL#13084 - TSF4_4: extended: 3 includes additional replication information.
var gr_protocol_version = json_find_key(stat, "GRProtocolVersion");
EXPECT_NE(undefined, gr_protocol_version);
var group_name = json_find_key(stat, "groupName");
EXPECT_NE(undefined, group_name);
var member_id = json_find_key(stat, "memberId");
EXPECT_NE(undefined, member_id);
var member_role = json_find_key(stat, "memberRole");
EXPECT_NE(undefined, member_role);
var member_state = json_find_key(stat, "memberState");
EXPECT_NE(undefined, member_state);
var fence_sys_vars = json_find_key(stat, "fenceSysVars");
EXPECT_NE(undefined, fence_sys_vars);
var transactions = json_find_key(stat, "transactions");
EXPECT_NE(undefined, transactions);
var connection = json_find_key(stat, "connection");
EXPECT_NE(undefined, connection);
var workers = json_find_key(stat, "workers");
EXPECT_NE(undefined, workers);

//@<> TS_E3	Validate that when calling cluster.status() if the option queryMembers is set to true, it takes precedence over extended option set to false. for 8.0 {VER(>=8.0)}
var stat_qm = cluster.status({queryMembers: true, extended:false});
json_check(stat_qm, full_status_templ_80);

//@<> TS_E3 Validate that when calling cluster.status() if the option queryMembers is set to true, it takes precedence over extended option set to false. for 8.0 {VER(<8.0)}
var stat_qm = cluster.status({queryMembers: true, extended:false});
json_check(stat_qm, full_status_templ_57);

//@<> WL#13084 - TSF5_2: extended: 3 is the same as queryMembers: true.
var stat_ext_3 = cluster.status({queryMembers: true});
EXPECT_EQ(stat, stat_ext_3);

// If the option queryMembers is set to true, it takes precedence over extended option overriding the used value to 3.
var stat_qm_true_ext_2 = cluster.status({queryMembers: true, extended: 2});
EXPECT_EQ(stat, stat_qm_true_ext_2);

// With 2 members
cluster.addInstance(__sandbox_uri2, {waitRecovery: 0});

// TS6_1	Verify that information about the recovery process is added to the status of a member that is on Recovery status when calling cluster.status().

//@<> F7- Check that recovery stats are there 5.7 {VER(<8.0)}
var stat = cluster.status({extended:2});
var allowed_missing = ["transactions"];
var allowed_unexpected = ["recovery", "recoveryStatusText", ["replicationLag"]];
json_check(stat, extended_2_status_templ_57, allowed_missing, allowed_unexpected);

//@<> F7- Check that recovery stats are there 8.0 {VER(>=8.0)}
var stat = cluster.status({extended:2});
var allowed_unexpected = ["recovery", "recoveryStatusText", "replicationLag"];
json_check(stat, extended_2_status_templ_80, [], allowed_unexpected);

var stat = cluster.status({extended:3});
println(stat);
EXPECT_EQ("RECOVERING", json_find_key(stat, hostname+":"+__mysql_sandbox_port2)["status"]);
var allowed_missing = ["appliedCount", "checkedCount", "committedAllMembers", "conflictsDetectedCount", "inApplierQueueCount", "inQueueCount", "lastConflictFree", "proposedCount", "rollbackCount"];
// currentlyQueueing only appears if the worker is currently processing a tx
allowed_missing.push("currentlyQueueing");
var transactions = json_find_key(stat, "transactions");
json_check(transactions, transaction_status_templ, allowed_missing);

// WL13208-TS_FR8_3 - Distributed Recovery displays state
var recovery = json_find_key(stat, "recovery");
json_check(recovery, distributed_recovery_status_templ);

// Wait recovery to finish
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

var stat = cluster.status({extended:3});
println(stat);
var recovery = json_find_key(stat, "recovery");
EXPECT_FALSE(recovery);

//@<> WL13208-TS_FR8_2 - Clone Recovery status displays state information {VER(>=8.0)}
cluster.removeInstance(hostname+":"+__mysql_sandbox_port2);

// Insert more rows in primary so that a recovery is necessary
for (i = 0; i < NUM_DATA_ROWS; i++) {
    session.runSql("insert into test.data values (default, repeat('x', 4*1024*1024))");
}

// the recovery should fail because it can't connect to the primary
cluster.addInstance(__sandbox_uri2, {waitRecovery: 0, recoveryMethod:'clone'});

var stat = cluster.status({ extended: 3 })
var recovery = json_find_key(stat, "recovery");
var allowed_missing = ["currentStageProgress"];
json_check(recovery, clone_recovery_status_templ, allowed_missing);

testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");

//@<> F4 - shutdown member2 and try again to see if connect error is included
// TS_3_1	Verify that a shellConnectError is added to the status of a member if the shell can't connect to it when calling cluster.status().
// WL13208-TS_FR8_4
testutil.stopSandbox(__mysql_sandbox_port2);

var stat = cluster.status({extended:3});
var loc = json_find_key(stat, hostname+":" + __mysql_sandbox_port2);
EXPECT_NE(undefined, loc["shellConnectError"]);

//@<> F6- With parallel appliers for 8.0 {VER(>=8.0)}
// TS5_1	Verify that information about the coordinator thread is added to the status of a member if the member has multithreaded slave enabled when calling cluster.status().
cluster.addInstance(__sandbox_uri3, {waitRecovery: 0});
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

var stat = cluster.status({extended:3});
var tx = stat["defaultReplicaSet"]["topology"][hostname+":"+__mysql_sandbox_port3]["transactions"];
EXPECT_NE(undefined, tx["connection"]);
EXPECT_NE(undefined, tx["coordinator"]);
EXPECT_NE(undefined, tx["workers"]);
EXPECT_EQ(2, tx["workers"].length);
json_check(tx["coordinator"], coordinator_status_templ_80);

//@<> F6- With parallel appliers for 5.7 {VER(<8.0)}
// TS5_1    Verify that information about the coordinator thread is added to the status of a member if the member has multithreaded slave enabled when calling cluster.status().
cluster.addInstance(__sandbox_uri3, {waitRecovery: 0});
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

var stat = cluster.status({extended:3});
var tx = stat["defaultReplicaSet"]["topology"][hostname+":"+__mysql_sandbox_port3]["transactions"];
EXPECT_NE(undefined, tx["connection"]);
EXPECT_NE(undefined, tx["coordinator"]);
EXPECT_NE(undefined, tx["workers"]);
EXPECT_EQ(2, tx["workers"].length);
json_check(tx["coordinator"], coordinator_status_templ_57);

//@<> F9- Rejoin member2 but trigger a failed recovery so we get a recovery error

// Insert more rows in primary so that a recovery is necessary
for (i = 0; i < NUM_DATA_ROWS; i++) {
    session.runSql("insert into test.data values (default, repeat('x', 4*1024*1024))");
}

// Drop replication accounts
var accounts = session.runSql("select concat(user, '@', quote(host)) from mysql.user where user like 'mysql_innodb_cluster%'").fetchAll();
for (var i in accounts) {
    var user = accounts[i][0];
    session.runSql("drop user "+user);
}

testutil.startSandbox(__mysql_sandbox_port2);
// the recovery should fail because it can't connect to the primary
cluster.rejoinInstance(__sandbox_uri2);

testutil.waitForConnectionErrorInRecovery(__mysql_sandbox_port2, 1045);

var stat = cluster.status({extended:3});
println(stat);
// TS8_1	Verify that any error present in PFS is added to the status of the members when calling cluster.status().
var recovery = json_find_key(json_find_key(stat, hostname+":" + __mysql_sandbox_port2), "recovery");
EXPECT_EQ(1045, recovery["receiverErrorNumber"]);
EXPECT_EQ("CONNECTION_ERROR", recovery["state"]);
EXPECT_NE(undefined, recovery["receiverErrorNumber"]);
EXPECT_NE(undefined, recovery["state"]);

// Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);