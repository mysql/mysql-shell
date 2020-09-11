

//@<> version

testutil.callMysqlsh(["mysqlsh", "--version"]);

EXPECT_OUTPUT_CONTAINS(shell.version);

