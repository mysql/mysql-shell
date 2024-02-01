//@ {!real_host_is_loopback}

// -------------------------------------------------------------------------------------
// BUG#27329079 - CREATE CLUSTER NOT POSSIBLE IF SERVER STARTED WITH INNODB_PAGE_SIZE=4K
// -------------------------------------------------------------------------------------

//@<> Deploy instances (with specific innodb_page_size).
var __sandbox_dir = testutil.getSandboxPath();
//NOTE: Can not use testutil.deploySandbox here as it will reuse a pre-generated data file that is not
//      compatible with the innodb_page_size used on this test, so we need clean sandboxes generated
//      with the required page size
dba.deploySandboxInstance(__mysql_sandbox_port1, {mysqldOptions: ["innodb_page_size=4k", "report_host="+hostname], password: 'root', sandboxDir:__sandbox_dir});
dba.deploySandboxInstance(__mysql_sandbox_port2, {mysqldOptions: ["innodb_page_size=8k", "report_host="+hostname], password: 'root', sandboxDir:__sandbox_dir});
EXPECT_STDERR_EMPTY();

var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);

//@<> checkInstanceConfiguration error with innodb_page_size=4k.
EXPECT_THROWS(function(){
  dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});
}, "Unsupported innodb_page_size value: 4096");
EXPECT_OUTPUT_CONTAINS(`ERROR: Instance '${hostname}:${__mysql_sandbox_port1}' is using a non-supported InnoDB page size (innodb_page_size=4096). Only instances with innodb_page_size greater than 4k (4096) can be used with InnoDB Cluster.`);

//@<> configureInstance error with innodb_page_size=4k.
EXPECT_THROWS(function(){
  dba.configureInstance(__sandbox_uri1, {mycnfPath: mycnf1});
}, "Unsupported innodb_page_size value: 4096");
EXPECT_OUTPUT_CONTAINS(`ERROR: Instance '${hostname}:${__mysql_sandbox_port1}' is using a non-supported InnoDB page size (innodb_page_size=4096). Only instances with innodb_page_size greater than 4k (4096) can be used with InnoDB Cluster.`);


//@<> create cluster fails with nice error if innodb_page_size=4k
shell.connect(__sandbox_uri1);
EXPECT_THROWS(function(){
  if (__version_num < 80027) {
        dba.createCluster("test_cluster", {gtidSetIsComplete: true, ipAllowlist:"127.0.0.1," + hostname_ip});
  } else {
    dba.createCluster("test_cluster", {gtidSetIsComplete: true});
  }
}, "Unsupported innodb_page_size value: 4096");
EXPECT_OUTPUT_CONTAINS(`ERROR: Instance '${hostname}:${__mysql_sandbox_port1}' is using a non-supported InnoDB page size (innodb_page_size=4096). Only instances with innodb_page_size greater than 4k (4096) can be used with InnoDB Cluster.`);

//@<> create cluster works with innodb_page_size=8k (> 4k)
shell.connect(__sandbox_uri2);
var cluster;
if (__version_num < 80027) {
        cluster = dba.createCluster("test_cluster", {gtidSetIsComplete: true, ipAllowlist:"127.0.0.1," + hostname_ip});
} else {
    cluster = dba.createCluster("test_cluster", {gtidSetIsComplete: true});
}
EXPECT_STDERR_EMPTY();

//@<> Clean-up deployed instances.
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);

// -----------------------------------------------------------------------------------------
// BUG#28531271 - CREATECLUSTER FAILS WHEN INNODB_DEFAULT_ROW_FORMAT IS COMPACT OR REDUNDANT
// -----------------------------------------------------------------------------------------
//@<> dba.createCluster using innodb_default_row__format=COMPACT
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, innodb_default_row_format: 'COMPACT'});
shell.connect(__sandbox_uri1);
var c;
if (__version_num < 80027) {
        c = dba.createCluster('sample', {ipAllowlist:"127.0.0.1," + hostname_ip});
} else {
    c = dba.createCluster('sample');
}
EXPECT_STDERR_EMPTY();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> dba.createCluster using innodb_default_row__format=REDUNDANT
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, innodb_default_row_format: 'REDUNDANT'});
shell.connect(__sandbox_uri1);
var c;
if (__version_num < 80027) {
        c = dba.createCluster('sample', {ipAllowlist:"127.0.0.1," + hostname_ip});
} else {
    c = dba.createCluster('sample');
}
EXPECT_STDERR_EMPTY();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> Check if invalid localAddress is checked against ipAllowlist (if not explicitly specified) BUG#31357039
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname});
testutil.deploySandbox(__mysql_sandbox_port2, "root", {report_host: hostname});

shell.connect(__sandbox_uri1);

if (__version_num >= 80027) {
    EXPECT_THROWS(function(){
        dba.createCluster("test", {communicationStack: "XCOM", localAddress:"0.0.0.0:1"});
    }, "The 'localAddress' isn't compatible with the Group Replication automatically generated list of allowed IPs.");
}
else {
    EXPECT_THROWS(function(){
        dba.createCluster("test", {localAddress:"0.0.0.0:1"});
    }, "The 'localAddress' isn't compatible with the Group Replication automatically generated list of allowed IPs.");
}

EXPECT_OUTPUT_CONTAINS(`The 'localAddress' "0.0.0.0" isn't compatible with the Group Replication automatically generated list of allowed IPs.`);
EXPECT_OUTPUT_CONTAINS("In this scenario, it's necessary to explicitly use the 'ipAllowlist' option to manually specify the list of allowed IPs.");
EXPECT_OUTPUT_CONTAINS("See https://dev.mysql.com/doc/refman/en/group-replication-ip-address-permissions.html for more details.");

shell.options["dba.connectivityChecks"] = false;

if (__version_num >= 80027) {
    WIPE_OUTPUT();
    EXPECT_THROWS(function(){
        cluster = dba.createCluster("test", {communicationStack: "MYSQL", localAddress:`0.0.0.0:${__mysql_sandbox_port1}`});
    }, "The START GROUP_REPLICATION command failed as there was an error when initializing the group communication layer.");

    EXPECT_OUTPUT_CONTAINS(`Unable to start Group Replication for instance '${hostname}:${__mysql_sandbox_port1}'.`);
}

if (__version_num >= 80027) {

    WIPE_OUTPUT();
    EXPECT_THROWS(function(){
        cluster = dba.createCluster("test", {communicationStack: "XCOM", localAddress:"0.0.0.0:1", ipAllowlist: "1.1.1.1"});
    }, "The START GROUP_REPLICATION command failed as there was an error when initializing the group communication layer.");

    EXPECT_OUTPUT_CONTAINS(`Unable to start Group Replication for instance '${hostname}:${__mysql_sandbox_port1}'.`);
}
else {

    WIPE_OUTPUT();
    EXPECT_THROWS(function(){
        cluster = dba.createCluster("test", {localAddress:"0.0.0.0:1", ipAllowlist: "1.1.1.1"});
    }, "The START GROUP_REPLICATION command failed as there was an error when initializing the group communication layer.");

    EXPECT_OUTPUT_CONTAINS(`Unable to start Group Replication for instance '${hostname}:${__mysql_sandbox_port1}'.`);
}

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
