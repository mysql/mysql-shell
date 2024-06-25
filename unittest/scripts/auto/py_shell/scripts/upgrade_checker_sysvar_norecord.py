#@{VER(<8.0.0)}

#@<> WL16262-TSFR_1_1
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "list": "true"})
EXPECT_STDOUT_NOT_CONTAINS("- sysVarsNewDefaults")
EXPECT_STDOUT_NOT_CONTAINS("- removedSysVars")
EXPECT_STDOUT_NOT_CONTAINS("- removedSysLogVars")
EXPECT_STDOUT_NOT_CONTAINS("- sysvarAllowedValues")
EXPECT_STDOUT_CONTAINS("- sysVars")
WIPE_OUTPUT()


#@<> WL16262-TSFR3_1
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.4.0", "include":["sysVarsNewDefaults", "removedSysVars", "removedSysLogVars", "sysvarAllowedValues"]})
EXPECT_STDERR_CONTAINS("ValueError: Option include contains unknown values 'removedSysLogVars', 'removedSysVars', 'sysVarsNewDefaults', 'sysvarAllowedValues'")
WIPE_OUTPUT()

util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.4.0", "exclude":["sysVarsNewDefaults", "removedSysVars", "removedSysLogVars", "sysvarAllowedValues"]})
EXPECT_STDERR_CONTAINS("ValueError: Option exclude contains unknown values 'removedSysLogVars', 'removedSysVars', 'sysVarsNewDefaults', 'sysvarAllowedValues'")
WIPE_OUTPUT()
