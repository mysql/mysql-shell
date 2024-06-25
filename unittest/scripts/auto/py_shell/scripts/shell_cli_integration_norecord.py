#@<> utils CLI calls
rc = testutil.call_mysqlsh(["--", "util", "check-for-server-upgrade"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("ERROR: ArgumentError: Please connect the shell to the MySQL server to be checked or specify the server URI as a parameter.")
WIPE_OUTPUT()

rc = testutil.call_mysqlsh(["--", "util", "check-for-server-upgrade", "--outputFormat=whatever"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("ERROR: ArgumentError: Please connect the shell to the MySQL server to be checked or specify the server URI as a parameter.")
WIPE_OUTPUT()

rc = testutil.call_mysqlsh(["--", "util", "check-for-server-upgrade", "--dummyOption=whatever"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("The following option is invalid: --dummyOption")
WIPE_OUTPUT()

#@<> Utils CLI calls with an options file {VER(<8.0.0)}
testutil.deploy_raw_sandbox(__mysql_sandbox_port1, 'root')

testutil.call_mysqlsh(['--sql', '--vertical', '-e', 'select * from scti_test.t;'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

options_file = f"""
[client]
user=root
port={__mysql_sandbox_port1}
password=root
"""
testutil.create_file("my.cnf", options_file)

#@<> Testing upgrade checker with no parameters{VER(<8.0.0)}
# NOTE: This test would simply use the global session as no explicit session parameter was set
rc = testutil.call_mysqlsh(["--", "util", "check-for-server-upgrade"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQL_HOME=."])
EXPECT_EQ(0, rc)
WIPE_OUTPUT()

#@<> Testing upgrade checker passing option but skipping session {VER(<8.0.0)}
# NOTE: This test would simply use the global session because an explicit default parameter (null) was passed from CLI mapper
rc = testutil.call_mysqlsh(["--", "util", "check-for-server-upgrade", "--outputFormat=whatever"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQL_HOME=."])
EXPECT_STDOUT_CONTAINS("ERROR: ArgumentError: Allowed values for outputFormat parameter are TEXT or JSON")
EXPECT_NE(0, rc)
WIPE_OUTPUT()

rc = testutil.call_mysqlsh(["--", "util", "check-for-server-upgrade", "--outputFormat=JSON"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQL_HOME=."])
EXPECT_EQ(0, rc)
WIPE_OUTPUT()

rc = testutil.call_mysqlsh(["--", "util", "check-for-server-upgrade", "--dummyOption=whatever"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQL_HOME=."])
EXPECT_STDOUT_CONTAINS("ERROR: The following option is invalid: --dummyOption")
EXPECT_NE(0, rc)
WIPE_OUTPUT()

testutil.rmfile("my.cnf")
testutil.destroy_sandbox(__mysql_sandbox_port1)

#@<> shell CLI calls
rc = testutil.call_mysqlsh(["--", "shell", "status"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_EQ(0, rc)
rc = testutil.call_mysqlsh(["--", "shell", "connect"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("ERROR: Invalid operation for shell object: connect")
EXPECT_NE(0, rc)
WIPE_OUTPUT()

#@<> cluster CLI calls
rc = testutil.call_mysqlsh(["--", "cluster", "status"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("ERROR: RuntimeError: An open session is required to perform this operation.")
EXPECT_NE(0, rc)
WIPE_OUTPUT()

#@<> dba CLI calls
rc = testutil.call_mysqlsh(["--", "dba", "drop-metadata-schema", "--force"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("ERROR: RuntimeError: An open session is required to perform this operation.")
EXPECT_NE(0, rc)
WIPE_OUTPUT()

rc = testutil.call_mysqlsh(["--", "dba", "deploy-sandbox-instance", "invalid-port"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])
EXPECT_STDOUT_CONTAINS("ERROR: Argument error at 'invalid-port': Integer expected, but value is String")
EXPECT_NE(0, rc)
WIPE_OUTPUT()
