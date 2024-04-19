
#@<> prep
import re
import os
import sys

homedir = os.path.join(os.getenv("TMPDIR"), "myhome")
os.makedirs(homedir, exist_ok=True)
def make_cnf(text):
    if sys.platform == "win32":
        fn = os.path.join(homedir, "my.ini")
    else:
        fn = os.path.join(homedir, "my.cnf")
    with open(fn, "w+") as f:
        f.write(text)

myloginvar = "MYSQL_TEST_LOGIN_FILE"
myloginfile = os.path.join(__test_data_path, "config", "mysqlsh_mycnf_options", ".mylogin.cnf")

os.chmod(myloginfile, 0o600)


shell.connect(__mysqluripwd)
session.run_sql("drop user if exists mycnfusr@'%'")

with_statement = " with caching_sha2_password"
if __version_num <= 50744:
    with_statement = ""

session.run_sql(f"create user mycnfusr@'%' identified {with_statement} by 'testpwd'")

session.run_sql("drop user if exists lpathusr@'%'")
session.run_sql(f"create user lpathusr@'%' identified {with_statement} by 'lpathusrpwd'")

socket = get_socket_path(session)

#@<> check that options are read as expected from my.cnf 
# TSFR_1_1, TSFR_1_2, TSFR_2_1
make_cnf(f"""[mysqlsh]
user=mycnfusr
password=testpwd
host=localhost
socket={socket}
sql
""")

testutil.call_mysqlsh(["--print-defaults"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS("would have been started with the following arguments:")
EXPECT_STDOUT_MATCHES(re.compile(f".*\n--user=mycnfusr --password=\\*\\*\\*\\*\\* --host=localhost --socket={socket} --sql \n", re.DOTALL))

WIPE_OUTPUT()

# TSFR_6 - also verifies ordering of options
testutil.call_mysqlsh(["--print-defaults", "-uroot", "--cluster"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS("would have been started with the following arguments:")
EXPECT_STDOUT_MATCHES(re.compile(f".*\n--user=mycnfusr --password=\\*\\*\\*\\*\\* --host=localhost --socket={socket} --sql -uroot --cluster \n", re.DOTALL))

#@<> group mysql (or any other) should be ignored
make_cnf(f"""[mysql]
user=mycnfusr
password=testpwd
host=localhost
socket={socket}
""")
testutil.call_mysqlsh(["--print-defaults"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS("would have been started with the following arguments:")
EXPECT_STDOUT_MATCHES(re.compile(".*the following arguments:\n\n", re.DOTALL))

#@<> invalid option error
# TSFR_1_3
make_cnf(f"""[client]
binary_mode
""")
testutil.call_mysqlsh([], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS("While processing defaults options:")
EXPECT_STDOUT_CONTAINS("unknown option --binary-mode")

#@<> invalid option error from cmdline
make_cnf(f"""[mysql]
binary_mode
""")
testutil.call_mysqlsh(["--binary-mode"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_NOT_CONTAINS("While processing defaults options:")
EXPECT_STDOUT_CONTAINS("unknown option --binary-mode")

#@<> group client should work too
make_cnf(f"""[client]
user=mycnfusr
password=testpwd
host=localhost
socket={socket}
""")
testutil.call_mysqlsh(["--print-defaults"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS("would have been started with the following arguments:")
EXPECT_STDOUT_MATCHES(re.compile(f".*\n--user=mycnfusr --password=\\*\\*\\*\\*\\* --host=localhost --socket={socket} \n", re.DOTALL))

#@<> check --no-defaults
testutil.call_mysqlsh(["--no-defaults", "--sql", "-e", "select user(), @@port"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS("Not connected.")

#@<> check --no-defaults + uri
testutil.call_mysqlsh(["--no-defaults", __mysqluripwd, "--sql", "-e", "select user(), @@port"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS(f"root@localhost	{__mysql_port}")

#@<> explicit params override specific defaults
testutil.call_mysqlsh(["-ulpathusr", "--passwords-from-stdin"], "\n", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS("Access denied for user 'lpathusr'@'localhost' (using password: YES)")

#@<> check sessions opened with defaults use mysql protocol
testutil.call_mysqlsh(["--sql", "-e", "select user(), @@port"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS(f"mycnfusr@localhost	{__mysql_port}")

# check that cmdline password warning not shown if we only got pwd from my.cnf
EXPECT_STDOUT_NOT_CONTAINS(k_cmdline_password_insecure_msg)

#@<> check that cmdline password warning shown if we really passed a pwd
testutil.call_mysqlsh(["--sql", "-ptestpwd",  "-e", "select user(), @@port"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS(f"mycnfusr@localhost	{__mysql_port}")

EXPECT_STDOUT_CONTAINS(k_cmdline_password_insecure_msg)

#@<> check that login-path works
# TSFR_4_1
testutil.call_mysqlsh(["--login-path=lpathusr", "--sql", "-e", "select user(), @@port"], "", [myloginvar+"="+myloginfile])
EXPECT_STDOUT_CONTAINS(f"lpathusr@localhost	{__mysql_port}")

#@<> check that login-path + mycnf works (login-path wins)
testutil.call_mysqlsh(["--login-path=lpathusr", "--sql", "-e", "select user(), @@port"], "", [myloginvar+"="+myloginfile, "MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS(f"lpathusr@localhost	{__mysql_port}")

#@<> URI overrides all defaults
testutil.call_mysqlsh(["--print-defaults"], "", ["MYSQL_HOME="+homedir])
testutil.call_mysqlsh(["--sql", __mysqluripwd,  "-e", "select user(), @@port"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS(f"root@localhost	{__mysql_port}")

#@<> mysqlsh --help
# TSFR_5
testutil.call_mysqlsh(["--help"], "", ["MYSQL_HOME="+homedir])
EXPECT_STDOUT_CONTAINS("The following groups are read: mysqlsh client")
EXPECT_STDOUT_CONTAINS("Default options are read from the following files in the given order")
EXPECT_STDOUT_CONTAINS(homedir)

#@<> cleanup
session.run_sql("drop user if exists mycnfusr@'%'")
session.run_sql("drop user if exists lpathusr@'%'")
