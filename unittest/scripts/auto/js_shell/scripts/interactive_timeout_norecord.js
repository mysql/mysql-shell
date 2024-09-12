//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {interactive_timeout: 1});
if (__version_num >= 80000) {
    error = "The client was disconnected by the server because of inactivity. See wait_timeout and interactive_timeout for configuring this behavior.";
} else {
    error = "Lost connection to MySQL server during query";
}


//@<> shell.connect (interactive by default)
shell.connect(__sandbox_uri1);
os.sleep(2);
EXPECT_THROWS(function() {session.runSql("SELECT 1")}, error);

//@<> shell.reconnect (continues interactive)
shell.reconnect();
os.sleep(2);
EXPECT_THROWS(function() {session.runSql("SELECT 1")}, error);

//@<> shell.openSession (non interactive by default)
var mySession = shell.openSession(__sandbox_uri1);
os.sleep(2);
shell.dumpRows(mySession.runSql("SELECT 1"));
EXPECT_OUTPUT_CONTAINS_MULTILINE(`+---+
| 1 |
+---+
| 1 |
+---+`);

//@<> shell.openSession (explicitly interactive)
var mySession = shell.openSession(__sandbox_uri1 + "?client-interactive=1");
os.sleep(2);
EXPECT_THROWS(function() {mySession.runSql("SELECT 1")}, error);

//@<> mysql.getSession (non interactive by default)
var mySession = mysql.getSession(__sandbox_uri1);
os.sleep(2);
shell.dumpRows(mySession.runSql("SELECT 1"));
EXPECT_OUTPUT_CONTAINS_MULTILINE(`+---+
| 1 |
+---+
| 1 |
+---+`);

//@<> mysql.getSession (explicitly interactive)
var mySession = mysql.getSession(__sandbox_uri1 + "?client-interactive=true");
os.sleep(2);
EXPECT_THROWS(function() {mySession.runSql("SELECT 1")}, error);


//@<> shell.connect (explicitly not interactive)
WIPE_OUTPUT();
shell.connect(__sandbox_uri1 + "?client-interactive=false")
os.sleep(2);
shell.dumpRows(session.runSql("SELECT 1"));
EXPECT_OUTPUT_CONTAINS_MULTILINE(`+---+
| 1 |
+---+
| 1 |
+---+`);

//@<> shell.reconnect (continues non interactive)
WIPE_OUTPUT();
shell.reconnect()
os.sleep(2);
shell.dumpRows(session.runSql("SELECT 1"));
EXPECT_OUTPUT_CONTAINS_MULTILINE(`+---+
| 1 |
+---+
| 1 |
+---+`);



//@<> Command line connection (non interactive by default)
WIPE_OUTPUT();
testutil.callMysqlsh([__sandbox_uri1, "--js", "-e", "os.sleep(2);shell.dumpRows(session.runSql('select 1'))"]);
EXPECT_OUTPUT_CONTAINS_MULTILINE(`+---+
| 1 |
+---+
| 1 |
+---+`);


//@<> Command line connection (explicitly interactive)
WIPE_OUTPUT();
testutil.callMysqlsh([__sandbox_uri1, "--interactive", "--js", "-e", "os.sleep(2);shell.dumpRows(session.runSql('select 1'))"]);
EXPECT_OUTPUT_CONTAINS(error)

//@<> Drops the sandbox
testutil.destroySandbox(__mysql_sandbox_port1);
