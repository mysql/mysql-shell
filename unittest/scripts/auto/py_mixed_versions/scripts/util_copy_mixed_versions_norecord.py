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

#@<> BUG#35359364 - loading from 5.7 into 8.0 should not require the `ignoreVersion` option {VER(<8.1.0)}
EXPECT_SUCCESS(__sandbox_uri2)
EXPECT_STDOUT_CONTAINS(f"Target is MySQL {__version_full}. Dump was produced from MySQL {MYSQLD_SECONDARY_SERVER_A['version']}")
EXPECT_STDOUT_CONTAINS("Destination MySQL version is newer than the one where the dump was created.")
EXPECT_STDOUT_NOT_CONTAINS("ignoreVersion")

#@<> loading from 5.7 into 8.1.0+ fails without the `ignoreVersion` option {VER(>=8.1.0)}
EXPECT_FAIL("Shell Error (53011)", "MySQL version mismatch", __sandbox_uri2)
EXPECT_STDOUT_CONTAINS(f"Target is MySQL {__version_full}. Dump was produced from MySQL {MYSQLD_SECONDARY_SERVER_A['version']}")
EXPECT_STDOUT_CONTAINS("Destination MySQL version is newer than the one where the dump was created. Loading dumps from non-consecutive major MySQL versions is not fully supported and may not work. Enable the 'ignoreVersion' option to load anyway.")

#@<> loading from 5.7 into 8.1.0+ succeeds with the `ignoreVersion` option {VER(>=8.1.0)}
EXPECT_SUCCESS(__sandbox_uri2, {"ignoreVersion": True})
EXPECT_STDOUT_CONTAINS(f"Target is MySQL {__version_full}. Dump was produced from MySQL {MYSQLD_SECONDARY_SERVER_A['version']}")
EXPECT_STDOUT_CONTAINS("Destination MySQL version is newer than the one where the dump was created. Source and destination have non-consecutive major MySQL versions. The 'ignoreVersion' option is enabled, so loading anyway.")

#@<> Cleanup
cleanup_copy_tests()
