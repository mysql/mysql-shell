# @<>

import time


def EXPECT_LOG_CONTAINS(text, path=None):
    if not path:
        path = testutil.get_shell_log_path()
    logs = open(path, "r").read()
    if text not in logs:
        testutil.fail("Log file does not contain expected text: "+text)
        print(logs)


def EXPECT_LOG_NOT_CONTAINS(text, path=None):
    if not path:
        path = testutil.get_shell_log_path()
    logs = open(path, "r").read()
    if text in logs:
        testutil.fail("Log file contains text that it shouldn't:"+text)
        print(logs)

# @<> Check --log-file option


token = f"test1-{time.time()}"
token1 = token
# default location
testutil.call_mysqlsh(["--py", "-e", f"shell.log('info', '{token}')"])
EXPECT_LOG_CONTAINS(token)

# empty (disables log file)
token = f"test2-{time.time()}"
testutil.call_mysqlsh(["--py", "--log-file=", "-e",
                       f"shell.log('info', '{token}')"])
EXPECT_LOG_NOT_CONTAINS(token)

# /dev/null
token = f"test3-{time.time()}"
testutil.call_mysqlsh(["--py", "--log-file=/dev/null", "-e",
                       f"shell.log('info', '{token}')"])
EXPECT_LOG_NOT_CONTAINS(token)

# some file
token = f"test4-{time.time()}"
testutil.call_mysqlsh(["--py", "--log-file=here.txt", "-e",
                       f"shell.log('info', '{token}')"])
EXPECT_LOG_CONTAINS(token, "here.txt")
EXPECT_LOG_NOT_CONTAINS(token1, "here.txt")

# invalid directory
token = f"test4-{time.time()}"
r = testutil.call_mysqlsh(["--py", "--log-file=/invalid/dir/here.txt", "-e",
                           f"shell.log('info', '{token}')"])
EXPECT_EQ(1, r)
EXPECT_STDOUT_CONTAINS(
    "Error opening log file '/invalid/dir/here.txt' for writing: No such file or directory")
