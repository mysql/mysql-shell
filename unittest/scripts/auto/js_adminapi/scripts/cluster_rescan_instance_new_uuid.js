//@<> Initialization
var scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2, __mysql_sandbox_port3]);
var cluster = scene.cluster;

//@<> Initial state validation
var uuid = get_sandbox_uuid(__mysql_sandbox_port2);
var new_uuid = ""
var status = cluster.status({extended:1});
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["status"], "ONLINE");
EXPECT_EQ(status["defaultReplicaSet"]["topology"][`${hostname}:${__mysql_sandbox_port2}`]["memberId"], `${uuid}`);


function restart_instance_with_new_uuid(target_port, new_uuid_suffix){
    testutil.stopSandbox(target_port);
    testutil.waitMemberState(target_port, "(MISSING)");

    new_uuid = uuid.substring(0, uuid.length - 4) + new_uuid_suffix;
    testutil.changeSandboxConf(target_port, 'server-uuid', new_uuid);
    testutil.startSandbox(target_port)
    testutil.waitMemberState(target_port, "ONLINE");
}

function validate_rescan() {
    EXPECT_STDOUT_CONTAINS_MULTILINE(`
Rescanning the cluster...
    
Result of the rescanning operation for the 'cluster' cluster:
{
    "name": "cluster", 
    "newTopologyMode": null, 
    "newlyDiscoveredInstances": [], 
    "unavailableInstances": [], 
    "updatedInstances": [
        {
            "host": "${hostname}:${__mysql_sandbox_port2}", 
            "label": "${hostname}:${__mysql_sandbox_port2}", 
            "member_id": "${new_uuid}",
            "old_member_id": "${uuid}"
        }
    ]
}

The instance '${hostname}:${__mysql_sandbox_port2}' is part of the cluster but its UUID has changed. Old UUID: ${uuid}. Current UUID: ${new_uuid}.
Updating instance metadata...
The instance metadata for '${hostname}:${__mysql_sandbox_port2}' was successfully updated.
`);
}

//@<> Update instance and do plain rescan
restart_instance_with_new_uuid(__mysql_sandbox_port2, "1111");
cluster.rescan();
validate_rescan();


//@<> Update instance and do interactive rescan
uuid = new_uuid;
restart_instance_with_new_uuid(__mysql_sandbox_port2, "2222");
cluster.rescan({interactive:true})
validate_rescan();

//@<> Finalize
scene.destroy();
