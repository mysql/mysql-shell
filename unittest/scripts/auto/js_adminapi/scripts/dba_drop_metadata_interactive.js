//@<> Initialization
var scene = new ClusterScenario([__mysql_sandbox_port1]);
var cluster = scene.cluster
shell.connect(__sandbox_uri1);
EXPECT_STDERR_EMPTY()

//@<> Invalid dropMetadataSchema call
EXPECT_THROWS(function () { dba.dropMetadataSchema(1,2,3,4,5) }, "Invalid number of arguments, expected 0 to 1 but got 5")
EXPECT_THROWS(function () { dba.dropMetadataSchema("Whatever") }, "Argument #1 is expected to be a map")
EXPECT_THROWS(function () { dba.dropMetadataSchema({not_valid:true}) }, "Invalid options: not_valid")
EXPECT_THROWS(function () { dba.dropMetadataSchema({force:"NotABool"}) }, "Option 'force' Bool expected, but value is String")
EXPECT_THROWS(function () { dba.dropMetadataSchema({clearReadOnly:"NotABool"}) }, "Option 'clearReadOnly' Bool expected, but value is String")


//@<> drop metadata: no user response
testutil.expectPrompt("Are you sure you want to remove the Metadata?", "");
dba.dropMetadataSchema()
EXPECT_STDOUT_CONTAINS("No changes made to the Metadata Schema.")

//@<> drop metadata: user response no
testutil.expectPrompt("Are you sure you want to remove the Metadata?", "n");
dba.dropMetadataSchema()
EXPECT_STDOUT_CONTAINS("No changes made to the Metadata Schema.")

//@<> drop metadata: force option false
dba.dropMetadataSchema({force:false});
EXPECT_STDOUT_CONTAINS("No changes made to the Metadata Schema.")

//@<> drop metadata: force option true, no confirmation prompt expected
dba.dropMetadataSchema({force:true});
EXPECT_STDOUT_CONTAINS("Metadata Schema successfully removed.")

session.close();
scene.destroy();

//@<> InnoDB Cluster: drop metadata on read only master, rejecting to clear it
scene = new ClusterScenario([__mysql_sandbox_port1, __mysql_sandbox_port2]);
shell.connect(__sandbox_uri1);
session.runSql("SET GLOBAL super_read_only=1");

testutil.expectPrompt("Are you sure you want to remove the Metadata?", "y");
testutil.expectPrompt("Do you want to disable super_read_only and continue? [y/N]: ", "n");
EXPECT_THROWS(function () { dba.dropMetadataSchema() }, "Server in SUPER_READ_ONLY");

//@<> InnoDB Cluster: drop metadata on slave with read only master, rejecting to clear it
shell.connect(__sandbox_uri2);
testutil.expectPrompt("Are you sure you want to remove the Metadata?", "y");
testutil.expectPrompt("Do you want to disable super_read_only and continue? [y/N]: ", "n");
EXPECT_THROWS(function () { dba.dropMetadataSchema() }, "Server in SUPER_READ_ONLY");


//@<> InnoDB Cluster: drop metadata on read only master, accepting to clear it
testutil.expectPrompt("Are you sure you want to remove the Metadata?", "y");
testutil.expectPrompt("Do you want to disable super_read_only and continue? [y/N]: ", "y");
dba.dropMetadataSchema()
EXPECT_STDOUT_CONTAINS("Metadata Schema successfully removed.")

session.close();
scene.destroy();


//@<> InnoDB Cluster: drop metadata on read only master, no prompts
scene = new ClusterScenario([__mysql_sandbox_port1]);
shell.connect(__sandbox_uri1);
session.runSql("SET GLOBAL super_read_only=1");
dba.dropMetadataSchema({force:true, clearReadOnly:true});
EXPECT_STDOUT_CONTAINS("Metadata Schema successfully removed.")
session.close();
scene.destroy();


//@<> Setup for Replica Set Tests {VER(>8.0.0)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
shell.connect(__sandbox_uri1);
var rset = dba.createReplicaSet("myrs");

//@<> Add instance to RS {VER(>=8.0.17)}
testutil.expectPrompt("Please select a recovery method [C]lone/[I]ncremental recovery/[A]bort (default Clone)", "i")
rset.addInstance(__sandbox_uri2);
EXPECT_STDERR_EMPTY()

//@<> Add instance to RS {VER(>= 8.0.0) && VER(< 8.0.17)}
testutil.expectPrompt("Please select a recovery method [I]ncremental recovery/[A]bort (default Incremental recovery):", "i")
rset.addInstance(__sandbox_uri2);
EXPECT_STDERR_EMPTY()

//@<> Replica Set: drop metadata on read only master, rejecting to clear it {VER(>8.0.0)}
shell.connect(__sandbox_uri1);
session.runSql("SET GLOBAL super_read_only=1");
testutil.expectPrompt("Are you sure you want to remove the Metadata?", "y");
testutil.expectPrompt("Do you want to disable super_read_only and continue? [y/N]: ", "n");
EXPECT_THROWS(function () { dba.dropMetadataSchema() }, "Server in SUPER_READ_ONLY");

//@<> Replica Set: drop metadata on a slave with a read only master, rejecting to clear it {VER(>8.0.0)}
shell.connect(__sandbox_uri2);
testutil.expectPrompt("Are you sure you want to remove the Metadata?", "y");
testutil.expectPrompt("Do you want to disable super_read_only and continue? [y/N]: ", "n");
EXPECT_THROWS(function () { dba.dropMetadataSchema() },
    "Server in SUPER_READ_ONLY mode");

//@<> Replica Set: drop metadata on read only master, accepting to clear it {VER(>8.0.0)}
testutil.expectPrompt("Are you sure you want to remove the Metadata?", "y");
testutil.expectPrompt("Do you want to disable super_read_only and continue? [y/N]: ", "y");
dba.dropMetadataSchema()
EXPECT_STDOUT_CONTAINS("Metadata Schema successfully removed.")

testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);


//@<> Replica Set: drop metadata on read only master, no prompts {VER(>8.0.0)}
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);
var rset = dba.createReplicaSet("myrs");
session.runSql("SET GLOBAL super_read_only=1");
dba.dropMetadataSchema({force:true, clearReadOnly:true});
EXPECT_STDOUT_CONTAINS("Metadata Schema successfully removed.")
testutil.destroySandbox(__mysql_sandbox_port1);
