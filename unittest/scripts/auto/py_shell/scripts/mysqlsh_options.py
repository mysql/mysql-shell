
#@<> Check that --disable-builtin-plugins disables loading shell plugins
import sys

testutil.call_mysqlsh(["--py", "-e", "assert 'plugins' in dir(), 'plugins plugin is missing when expected'"])

testutil.call_mysqlsh(["--py", "--disable-builtin-plugins", "-e", "assert 'plugins' not in dir(), 'plugins dit not get disabled'"])

EXPECT_STDOUT_NOT_CONTAINS("AssertionError")
