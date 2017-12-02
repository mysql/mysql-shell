
testutil.deploySandbox(__mysql_sandbox_port1, "root");

// --- Create Cluster Tests ---
shell.connect(__sandbox_uri1);
session.runSql("set global super_read_only=1");

//@# Dba_create_cluster.clear_read_only_invalid
dba.createCluster("dev", {clearReadOnly:"NotABool"});

//@# Dba_create_cluster.clear_read_only_unset
dba.createCluster("dev");

//@# Dba_create_cluster.clear_read_only_false
dba.createCluster("dev", {clearReadOnly:false});

// --- Configure Local Instance Tests ---

//@# Dba_configure_local_instance.clear_read_only_invalid
dba.configureLocalInstance(__sandbox_uri1, {clearReadOnly:"NotABool"});

//@# Dba_configure_local_instance.clear_read_only_unset
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: "/path/to/file.cnf", clusterAdmin: "root", clusterAdminPassword: "pwd"});

//@# Dba_configure_local_instance.clear_read_only_false
dba.configureLocalInstance(__sandbox_uri1, {mycnfPath: "/path/to/file.cnf", clusterAdmin: "root", clusterAdminPassword: "pwd", clearReadOnly: false});

// --- Drop Metadata Schema Tests ---
session.runSql("set global super_read_only=0");

var cluster = dba.createCluster("dev");
session.runSql("set global super_read_only=1");

//@# Dba_drop_metadata.clear_read_only_invalid
dba.dropMetadataSchema({clearReadOnly: "NotABool"});

//@# Dba_drop_metadata.clear_read_only_unset
dba.dropMetadataSchema({force:true});

//@# Dba_drop_metadata.clear_read_only_false
dba.dropMetadataSchema({force:true, clearReadOnly: false});

// --- Reboot Cluster From Complete Outage ---
session.runSql("stop group_replication");

//@# Dba_reboot_cluster.clear_read_only_invalid
dba.rebootClusterFromCompleteOutage("dev", {clearReadOnly: "NotABool"});

//@# Dba_reboot_cluster.clear_read_only_unset
dba.rebootClusterFromCompleteOutage("dev", {clearReadOnly: false});

testutil.destroySandbox(__mysql_sandbox_port1);
