

//@<> version

testutil.callMysqlsh(["mysqlsh", "--version"]);

EXPECT_OUTPUT_CONTAINS(shell.version);


//@<> BUG#33945641 calling reconnect without a session
if (session) {
  session.close();
}

EXPECT_EQ(false, shell.reconnect());


//@<> BUG#34362541	Allow duplicating active shell session without re-inputting password
shell.connect(__mysql_uri)

session.runSql("create user if not exists dropme@'%' identified by '12345'")
shell.connect(`dropme:12345@localhost:${__mysql_port}`)

session2 = shell.openSession()
EXPECT_NE(session2.runSql("select connection_id()").fetchOne()[0], session.runSql("select connection_id()").fetchOne()[0])
EXPECT_EQ(session2.uri, session.uri)

shell.connect(__mysql_uri)
session.runSql("drop user dropme@'%'")
