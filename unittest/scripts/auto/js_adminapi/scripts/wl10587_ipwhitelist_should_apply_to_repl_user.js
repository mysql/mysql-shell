// F2. It shall be possible to define an IP white-list to increase the cluster
// security, via the 'ipWhitelist' option on the "ipWhitelist commands", and
// guarantee that the following security requirement is honored on the
// internally created "replication-users":
//
// F2.1 The created user accounts can connect exclusively from the allowed
// hosts specified in the 'ipWhitelist' option
//
// F3. When a 'ipWhitelist' option is used on the "ipWhitelist" commands, the
// "replication-user" shall have for the host value:
//
// F3.1 Equivalent netmask values for all the 'ipWhitelist' values represented
// with the CIDR notation
//
// F3.2 The exact same IP values used on the
// 'ipWhitelist'

// NOTE: The tests for F3 cover F2.

//@<> SETUP
testutil.deploySandbox(__mysql_sandbox_port1, "root");
testutil.deploySandbox(__mysql_sandbox_port2, "root");
testutil.deploySandbox(__mysql_sandbox_port3, "root");
shell.connect(__sandbox_uri1);

var cluster = dba.createCluster("testCluster", {ipWhitelist:"192.168.1.1/8,127.0.0.1," + hostname_ip + "," + hostname});

// Store the internally generated replication user
var result = session.runSql("SELECT User FROM mysql.user WHERE User LIKE 'mysql_innodb_cluster_r%'");
var rows = result.fetchAll();

// There must be 3 users only, one for each host in the ipWhitelist
EXPECT_TRUE(rows.length == 3);

var __repl_user = rows[0][0];

//@<> F3 The replication-user matches the ipWhitelist filters: createCluster()
var result = session.runSql("SELECT Host FROM mysql.user WHERE User = '" + __repl_user +"'");
var rows = result.fetchAll();

EXPECT_TRUE(rows.length == 3);

rows
EXPECT_OUTPUT_CONTAINS("127.0.0.1");
EXPECT_OUTPUT_CONTAINS("192.168.1.1/255.0.0.0");
EXPECT_OUTPUT_CONTAINS(hostname_ip);

//@<> F3 The replication-user matches the ipWhitelist filters: addInstance()
cluster.addInstance(__sandbox_uri2, {ipWhitelist:"192.168.2.1/15,127.0.0.1," + hostname_ip + "," + hostname});
testutil.waitMemberState(__mysql_sandbox_port2, "ONLINE");
var result = session.runSql("SELECT Host FROM mysql.user WHERE User LIKE 'mysql_innodb_cluster_r%' AND User != '" + __repl_user + "'");
var rows = result.fetchAll();

EXPECT_TRUE(rows.length == 3);

rows
EXPECT_OUTPUT_CONTAINS("127.0.0.1");
EXPECT_OUTPUT_CONTAINS("192.168.2.1/255.254.0.0");
EXPECT_OUTPUT_CONTAINS(hostname_ip);

// F4. When a valid 'ipWhitelist' is used on the .rejoinInstance() command, the
// previously existing "replication-user" must be removed from all the cluster
// members and a new one created to match the 'ipWhitelist' defined filter.
cluster.addInstance(__sandbox_uri3);
testutil.waitMemberState(__mysql_sandbox_port3, "ONLINE");

//@<> Disable group_replication_start_on_boot if version >= 8.0.11 {VER(>=8.0.11)}
session.runSql("SET PERSIST group_replication_start_on_boot=OFF");

//@<> Kill the seed instance which we know which was the replication-user, to force the reelection of a new primary
testutil.killSandbox(__mysql_sandbox_port1);

session.close();
cluster.disconnect();
shell.connect(__sandbox_uri2);
testutil.waitMemberState(__mysql_sandbox_port1, "(MISSING)");
testutil.startSandbox(__mysql_sandbox_port1);
var cluster = dba.getCluster("testCluster");

// Rejoin the seed instance back again using an ipWhitelist
cluster.rejoinInstance(__sandbox_uri1, {ipWhitelist:"254.168.3.1/11,127.0.0.1," + hostname_ip + "," + hostname});

//@<> F4 The previous replication-user must be removed
var result = session.runSql("SELECT Host FROM mysql.user WHERE User = '" + __repl_user + "'");
var rows = result.fetchAll();
EXPECT_TRUE(rows.length == 0);

//@<> F4 A new replication user must be created to match ipWhitelist
var result = session.runSql("SELECT Host FROM mysql.user WHERE User LIKE 'mysql_innodb_cluster_r%'");
var rows = result.fetchAll();

EXPECT_TRUE(rows.length == 8);

rows
EXPECT_OUTPUT_CONTAINS("127.0.0.1"); // x2
EXPECT_OUTPUT_CONTAINS("%");
EXPECT_OUTPUT_CONTAINS("localhost");
EXPECT_OUTPUT_CONTAINS("192.168.2.1/255.254.0.0");
EXPECT_OUTPUT_CONTAINS("254.168.3.1/255.224.0.0");
EXPECT_OUTPUT_CONTAINS(hostname_ip); // x2

//@<> TEARDOWN
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
testutil.destroySandbox(__mysql_sandbox_port3);

// F5. If 'ipWhitelist' is not used on the "ipWhitelist commands", thus getting
// the default value of 'AUTOMATIC', then the replication-user is created using
// the wildcard '%' for the host name value.

//@<> F5 SETUP
testutil.deploySandbox(__mysql_sandbox_port1, "root");
shell.connect(__sandbox_uri1);

var cluster = dba.createCluster("testCluster");

//@<> F5 ipWhitelist not used, replication-user should be created on % and localhost
var result = session.runSql("SELECT Host FROM mysql.user WHERE User LIKE 'mysql_innodb_cluster_r%'");
var rows = result.fetchAll();

EXPECT_TRUE(rows.length == 2);

rows
EXPECT_OUTPUT_CONTAINS("localhost");
EXPECT_OUTPUT_CONTAINS("%");

//@<> F5 TEARDOWN
session.close();
cluster.disconnect();
testutil.destroySandbox(__mysql_sandbox_port1);
