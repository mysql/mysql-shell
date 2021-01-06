#@ utils CLI calls
rc = testutil.call_mysqlsh(["--", "util", "check-for-server-upgrade"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
rc = testutil.call_mysqlsh(["--", "util", "check-for-server-upgrade", "--outputFormat=whatever"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
rc = testutil.call_mysqlsh(["--", "util", "check-for-server-upgrade", "--dummyOption=whatever"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

#@ shell CLI calls
rc = testutil.call_mysqlsh(["--", "shell", "status"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_EQ(0, rc)
rc = testutil.call_mysqlsh(["--", "shell", "connect"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

#@ cluster CLI calls
rc = testutil.call_mysqlsh(["--", "cluster", "status"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

#@ dba CLI calls
rc = testutil.call_mysqlsh(["--", "dba", "drop-metadata-schema", "--force"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
rc = testutil.call_mysqlsh(["--", "dba", "deploy-sandbox-instance", "invalid-port"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
