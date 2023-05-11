//@<> Check if invalid localAddress is checked against ipAllowlist (if not explicitly specified) BUG#31357039
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");

shell.connect(__sandbox_uri1);

var cluster;
if (__version_num >= 80027)
    cluster = dba.createCluster("cluster", {gtidSetIsComplete: true, communicationStack: "XCOM"});
else
    cluster = dba.createCluster("cluster", {gtidSetIsComplete: true});

EXPECT_THROWS(function(){
    cluster.addInstance(__sandbox_uri2, {localAddress:"0.0.0.0:1"});
}, "The 'localAddress' isn't compatible with the Group Replication automatically generated list of allowed IPs.");

EXPECT_OUTPUT_CONTAINS(`The 'localAddress' "0.0.0.0" isn't compatible with the Group Replication automatically generated list of allowed IPs.`);
if (__os_type != 'windows') {
    EXPECT_OUTPUT_CONTAINS("In this scenario, it's necessary to explicitly use the 'ipAllowlist' option to manually specify the list of allowed IPs.");
}
EXPECT_OUTPUT_CONTAINS("See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.");

WIPE_OUTPUT();

if (__os_type != 'windows') {
    EXPECT_THROWS(function(){
        cluster.addInstance(__sandbox_uri2, {localAddress:"0.0.0.0:1", ipAllowlist: "1.1.1.1"});
    }, "The START GROUP_REPLICATION command failed as there was an error when initializing the group communication layer.");

    EXPECT_OUTPUT_MATCHES(new RegExp(`Unable to start Group Replication for instance '[\\d\\.]+:${__mysql_sandbox_port2}'`));
}
else {
    EXPECT_THROWS(function(){
        cluster.addInstance(__sandbox_uri2, {localAddress:"0.0.0.0:1", ipAllowlist: "1.1.1.1"});
    }, "Server address configuration error");

    EXPECT_OUTPUT_MATCHES(new RegExp(`MySQL server at '.*:${__mysql_sandbox_port2}' can't connect to '0\\.0\\.0\\.0:${__mysql_sandbox_port2}'\\. Verify configured localAddress and that the address is valid at that host\\.`));
}

EXPECT_NO_THROWS(function(){
    cluster.addInstance(__sandbox_uri2);
});

//@<> Check for invalid localAddress is checked against ipAllowlist (if not explicitly specified) is ignored with MYSQL comm stack BUG#31357039 {VER(>=8.0.27)}
cluster.dissolve();

cluster = dba.createCluster("cluster", {gtidSetIsComplete: true, communicationStack: "MYSQL"});

WIPE_OUTPUT();

EXPECT_THROWS(function(){
    cluster.addInstance(__sandbox_uri2, {localAddress: `1.2.3.4:${__mysql_sandbox_port2}`});
}, "Server address configuration error");

EXPECT_OUTPUT_MATCHES(new RegExp(`MySQL server at '.*:${__mysql_sandbox_port2}' can't connect to '1\\.2\\.3\\.4:${__mysql_sandbox_port2}'\\. Verify configured localAddress and that the address is valid at that host\\.`));

//@<> Destroy sandboxes
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
