//@ GlobalSetUp
testutil.deploySandbox(__mysql_sandbox_port1, "root", {report_host: hostname, loose_mysql_native_password: "ON"});
var rootsess = mysql.getClassicSession(__sandbox_uri1);

const new_user = "random_user";
rootsess.runSql("CREATE USER " + new_user + "@localhost IDENTIFIED WITH mysql_native_password BY ''");
const uri_ = 'mysql://' + new_user + '@localhost:' + __mysql_sandbox_port1;
const uri_pass = 'mysql://' + new_user + ':xyz@localhost:' + __mysql_sandbox_port1;
const uri_empty_pass = 'mysql://' + new_user + ':@localhost:' + __mysql_sandbox_port1;

//@ password on command line
var rc = testutil.callMysqlsh([uri_, '--password=', '--js', '-e', 'println(session)'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
EXPECT_EQ(0, rc);

//@ no password
var rc = testutil.callMysqlsh([uri_, '--no-password','--js', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ no password and password prompt
var rc = testutil.callMysqlsh([uri_, '--no-password','--js', '--password', '-e', 'println(session)']);
EXPECT_EQ(1, rc);

//@ no password and empty password
var rc = testutil.callMysqlsh([uri_, '--no-password','--js', '--password=', '-e', 'println(session)']);
EXPECT_EQ(0, rc);

//@ no password and password
var rc = testutil.callMysqlsh([uri_, '--no-password','--js', '--password=xyz', '-e', 'println(session)']);
EXPECT_EQ(1, rc);

//@ no password and URI empty password
// --no-password should suppress the WARNING
var rc = testutil.callMysqlsh([uri_empty_pass, '--no-password','--js', '-e', 'println(session)'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
EXPECT_EQ(0, rc);

//@ no password and URI password
var rc = testutil.callMysqlsh([uri_pass, '--no-password','--js', '-e', 'println(session)']);
EXPECT_EQ(1, rc);

rootsess.close();
testutil.destroySandbox(__mysql_sandbox_port1);
