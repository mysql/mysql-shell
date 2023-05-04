#@ {DEF(MYSQLD57_PATH)}

#@<> INCLUDE dump_utils.inc
#@<> INCLUDE copy_utils.inc

#@<> entry point

# helpers
def EXPECT_SUCCESS(connectionData, options = {}, src = __sandbox_uri1, setup = None):
    EXPECT_SUCCESS_IMPL(util.copy_instance, connectionData, options, src, setup)

def EXPECT_FAIL(error, msg, connectionData, options = {}, src = __sandbox_uri1, setup = None):
    EXPECT_FAIL_IMPL(util.copy_instance, error, msg, connectionData, options, src, setup)

#@<> Setup
setup_copy_tests(2, { "mysqldPath": MYSQLD57_PATH })

#@<> BUG#35359364 - loading from 5.7 into 8.0 should not require the `ignoreVersion` option
EXPECT_SUCCESS(__sandbox_uri2)
EXPECT_STDOUT_CONTAINS(f"Target is MySQL {__version_full}. Dump was produced from MySQL {MYSQLD_SECONDARY_SERVER_A['version']}")
EXPECT_STDOUT_CONTAINS("Destination MySQL version is newer than the one where the dump was created.")
EXPECT_STDOUT_NOT_CONTAINS("ignoreVersion")

#@<> Cleanup
cleanup_copy_tests()
