//@ {!real_host_is_loopback}

// -------------------------------------------------------------------------------------
// BUG#27329079 - CREATE CLUSTER NOT POSSIBLE IF SERVER STARTED WITH INNODB_PAGE_SIZE=4K
// -------------------------------------------------------------------------------------

//@<> Deploy instances (with specific innodb_page_size).
var __sandbox_dir = testutil.getSandboxPath();
//NOTE: Can not use testutil.deploySandbox here as it will reuse a pre-generated data file that is not
//      compatible with the innodb_page_size used on this test, so we need clean sandboxes generated
//      with the required page size
dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom:"%", mysqldOptions: ["innodb_page_size=4k", "report_host="+hostname], password: 'root', sandboxDir:__sandbox_dir});
dba.deploySandboxInstance(__mysql_sandbox_port2, {allowRootFrom:"%", mysqldOptions: ["innodb_page_size=8k", "report_host="+hostname], password: 'root', sandboxDir:__sandbox_dir});
EXPECT_STDERR_EMPTY();

var mycnf1 = testutil.getSandboxConfPath(__mysql_sandbox_port1);

//@<> checkInstanceConfiguration error with innodb_page_size=4k.
EXPECT_THROWS(function(){
  dba.checkInstanceConfiguration(__sandbox_uri1, {mycnfPath: mycnf1});
}, "Dba.checkInstanceConfiguration: Unsupported innodb_page_size value: 4096");
EXPECT_OUTPUT_CONTAINS(`ERROR: Instance '${hostname}:${__mysql_sandbox_port1}' is using a non-supported InnoDB page size (innodb_page_size=4096). Only instances with innodb_page_size greater than 4k (4096) can be used with InnoDB Cluster.`);

//@<> configureLocalInstance error with innodb_page_size=4k.
EXPECT_THROWS(function(){
  dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: mycnf1});
}, "Dba.configureLocalInstance: Unsupported innodb_page_size value: 4096");
EXPECT_OUTPUT_CONTAINS(`ERROR: Instance '${hostname}:${__mysql_sandbox_port1}' is using a non-supported InnoDB page size (innodb_page_size=4096). Only instances with innodb_page_size greater than 4k (4096) can be used with InnoDB Cluster.`);


//@<> create cluster fails with nice error if innodb_page_size=4k
shell.connect(__sandbox_uri1);
EXPECT_THROWS(function(){
  var cluster = dba.createCluster("test_cluster", {gtidSetIsComplete: true, ipAllowlist:"127.0.0.1," + hostname_ip});
}, "Dba.createCluster: Unsupported innodb_page_size value: 4096");
EXPECT_OUTPUT_CONTAINS(`ERROR: Instance '${hostname}:${__mysql_sandbox_port1}' is using a non-supported InnoDB page size (innodb_page_size=4096). Only instances with innodb_page_size greater than 4k (4096) can be used with InnoDB Cluster.`);

//@<> create cluster works with innodb_page_size=8k (> 4k)
shell.connect(__sandbox_uri2);
var cluster = dba.createCluster("test_cluster", {gtidSetIsComplete: true, ipAllowlist:"127.0.0.1," + hostname_ip});
EXPECT_STDERR_EMPTY();

//@<> Clean-up deployed instances.
cleanup_sandbox(__mysql_sandbox_port1);
cleanup_sandbox(__mysql_sandbox_port2);

// -----------------------------------------------------------------------------------------
// BUG#28531271 - CREATECLUSTER FAILS WHEN INNODB_DEFAULT_ROW_FORMAT IS COMPACT OR REDUNDANT
// -----------------------------------------------------------------------------------------
//@<> dba.createCluster using innodb_default_row__format=COMPACT
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, innodb_default_row_format: 'COMPACT'});
shell.connect(__sandbox_uri1);
var c = dba.createCluster('sample', {ipAllowlist:"127.0.0.1," + hostname_ip});
EXPECT_STDERR_EMPTY();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);

//@<> dba.createCluster using innodb_default_row__format=REDUNDANT
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, innodb_default_row_format: 'REDUNDANT'});
shell.connect(__sandbox_uri1);
var c = dba.createCluster('sample', {ipAllowlist:"127.0.0.1," + hostname_ip});
EXPECT_STDERR_EMPTY();
session.close();
testutil.destroySandbox(__mysql_sandbox_port1);
