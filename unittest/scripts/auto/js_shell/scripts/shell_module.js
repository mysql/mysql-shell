

//@<> version

testutil.callMysqlsh(["mysqlsh", "--version"]);

EXPECT_OUTPUT_CONTAINS(shell.version);

//@<> BUG#33945641 calling reconnect without a session
if (session) {
  session.close();
}

EXPECT_EQ(false, shell.reconnect());
