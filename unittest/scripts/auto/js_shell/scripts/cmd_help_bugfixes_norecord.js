// BUG#28508724 MYSQLSH GIVES INCONSISTENT RESPONSES FROM \? HELP LOOK UPS

//@<> help mysql
\help mysql

out = testutil.fetchCapturedStdout()
err = testutil.fetchCapturedStderr()

//@<> help with trailing space [USE:help mysql]
\help mysql

out2 = testutil.fetchCapturedStdout()
err2 = testutil.fetchCapturedStderr()

EXPECT_EQ(out2, out)
EXPECT_EQ(err2, err)

//@<> help with leading space [USE:help mysql]
\help  mysql

out2 = testutil.fetchCapturedStdout()
err2 = testutil.fetchCapturedStderr()

EXPECT_EQ(out2, out)
EXPECT_EQ(err2, err)

//@<> help with space before command [USE:help mysql]
\help mysql

out2 = testutil.fetchCapturedStdout()
err2 = testutil.fetchCapturedStderr()

EXPECT_EQ(out2, out)
EXPECT_EQ(err2, err)