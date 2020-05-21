/// BUG#30885157 UNCLEAR ERROR MESSAGE WHEN CLUSTER REACHED MAXIMUM NUMBER OF
///              MEMBERS
///
/// Group replication maximum number of cluster members is 9.
///
/// This test checks if Admin API correctly blocks attempt to add 10th member
/// to the cluster.
///
/// Admin API must also prevent adding 10th cluster member if one or more
/// cluster members are (MISSING), i.e. total number of members cannot exceed 9.

const port_start = 3001;
const user = "root";
const passwd = "root";
const hostname = "127.0.0.1";
const connection_uri = "mysql://" + user + ":" + passwd + "@" + hostname;

function range(from, to) {
    return [...Array(to - from).keys()].map(x => from + x);
};

//@<> Create 10 sandboxes
for (let port of range(port_start, port_start + 10)) {
    println("Deploying port " + port);
    testutil.deploySandbox(port, "root");
}

//@<> Connect to first, and create cluster named "Europe"
shell.connect(connection_uri + ":" + port_start);
var c = dba.createCluster("Europe", { gtidSetIsComplete: true });

//@<> Add 8 members more.
for (let port of range(port_start + 1, port_start + 9)) {
    c.addInstance(connection_uri + ":" + port);
    println(c.status());
}

//@<> Total members in cluster is now 9. Adding one more must return an error to the user
try {
    c.addInstance(connection_uri + ":" + (port_start + 9));
} catch (e) {
    if (e.type == "RuntimeError") {
        // Group already has maximum number of 9 members.
        println(e.type + ": " + e.message);
    } else {
        throw e;
    }
}
println(c.status());

//@<> Kill 4th member.
testutil.killSandbox(port_start + 4);
testutil.waitMemberState(port_start + 4, "(MISSING)");

//@<> Total members in cluster is still 9. Return an error.
try {
    c.addInstance(connection_uri + ":" + (port_start + 9));
} catch (e) {
    if (e.type == "RuntimeError") {
        // Group already has maximum number of 9 members.
        println(e.type + ": " + e.message);
    } else {
        throw e;
    }
}
println(c.status());

//@<> Remove 4th member from cluster.
c.removeInstance(connection_uri + ":" + (port_start + 4), { force: true });

//@<> Total number of members is 8. Add new one should end with success.
c.addInstance(connection_uri + ":" + (port_start + 9));
println(c.status());

//@<> Cleanup, destroy 10 sandboxes
session.close();
for (let port of range(port_start, port_start + 10)) {
    println("destroySandbox on port " + port);
    testutil.destroySandbox(port);
}
