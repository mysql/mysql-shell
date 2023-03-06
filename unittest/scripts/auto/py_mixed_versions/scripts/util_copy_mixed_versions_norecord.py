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

#@<> WL15298_TSFR_4_5_20
EXPECT_FAIL("Error: Shell Error (53011)", "MySQL version mismatch", __sandbox_uri2, { "ignoreVersion": False })
EXPECT_STDOUT_CONTAINS(f"Target is MySQL {__version_full}. Dump was produced from MySQL {MYSQLD_SECONDARY_SERVER_A['version']}")
EXPECT_STDOUT_CONTAINS("Destination MySQL version is newer than the one where the dump was created. Loading dumps from different major MySQL versions is not fully supported and may not work. Enable the 'ignoreVersion' option to load anyway.")

#@<> WL15298_TSFR_4_5_21
EXPECT_SUCCESS(__sandbox_uri2, { "ignoreVersion": True })
EXPECT_STDOUT_CONTAINS(f"Target is MySQL {__version_full}. Dump was produced from MySQL {MYSQLD_SECONDARY_SERVER_A['version']}")
EXPECT_STDOUT_CONTAINS("Destination MySQL version is newer than the one where the dump was created. Loading dumps from different major MySQL versions is not fully supported and may not work. The 'ignoreVersion' option is enabled, so loading anyway.")

#@<> Cleanup
cleanup_copy_tests()
