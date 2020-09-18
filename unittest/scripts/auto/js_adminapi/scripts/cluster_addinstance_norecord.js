//@ {!real_host_is_loopback}

//@<> BUG#29255212 adding instance with lower_case_tablenames different from the cluster is not allowed.
var __sandbox_dir = testutil.getSandboxPath();
//NOTE: Can not use testutil.deploySandbox here as it will reuse a pre-generated data file that is not
//      compatible with the lower_case_table_names used on this test, so we need clean sandboxes generated
//      with the required lowe_case_table_names value
var lower_case_value = (__os_type !="macos" && __os_type != "windows") ? "0" : "2";
dba.deploySandboxInstance(__mysql_sandbox_port1, {allowRootFrom:"%", mysqldOptions: ["lower_case_table_names=1", "report_host="+hostname], password: 'root', sandboxDir:__sandbox_dir});
dba.deploySandboxInstance(__mysql_sandbox_port2, {allowRootFrom:"%", mysqldOptions: ["lower_case_table_names=" + lower_case_value, "report_host="+hostname], password: 'root', sandboxDir:__sandbox_dir});
EXPECT_STDERR_EMPTY();
shell.connect(__sandbox_uri1);
var cluster = dba.createCluster("cluster", {gtidSetIsComplete: true, ipAllowlist:"127.0.0.1," + hostname_ip});

//@<> addInstance fails with nice error if lower_case_table_names of instance different from the value off the cluster
EXPECT_THROWS_TYPE(function(){cluster.addInstance(__sandbox_uri2, {ipAllowlist:"127.0.0.1," + hostname_ip});}, "Cluster.addInstance: The 'lower_case_table_names' value '" + lower_case_value + "' of the instance 'localhost:" + __mysql_sandbox_port2 + "' is different from the value of the cluster '1'.", "RuntimeError");
EXPECT_OUTPUT_CONTAINS(`ERROR: Cannot join instance 'localhost:${__mysql_sandbox_port2}' to cluster: incompatible 'lower_case_table_names' value.`);

//@<> BUG#29255212 cleanup
cluster.disconnect();
session.close();
cleanup_sandbox(__mysql_sandbox_port1);
cleanup_sandbox(__mysql_sandbox_port2);
