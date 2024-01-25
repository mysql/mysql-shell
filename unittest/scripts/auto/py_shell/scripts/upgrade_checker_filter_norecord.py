#@{VER(<8.0.0)}

#@<> WL15974-TSFR_1_1_1
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "include":["checkTableCommand"]})
EXPECT_STDOUT_NOT_CONTAINS("(reservedKeywords)")
EXPECT_STDOUT_NOT_CONTAINS("(dollarSignName)")
EXPECT_STDOUT_CONTAINS("(checkTableCommand)")
WIPE_OUTPUT()

util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "include":["checkTableCommand", "dollarSignName"]})
EXPECT_STDOUT_NOT_CONTAINS("(reservedKeywords)")
EXPECT_STDOUT_CONTAINS("(dollarSignName)")
EXPECT_STDOUT_CONTAINS("(checkTableCommand)")
WIPE_OUTPUT()

#@<> WL15974-TSFR_1_1_2
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "include":[]})
EXPECT_STDOUT_CONTAINS("(reservedKeywords)")
EXPECT_STDOUT_CONTAINS("(checkTableCommand)")
EXPECT_STDOUT_CONTAINS("(changedFunctionsInGeneratedColumns)")
EXPECT_STDOUT_CONTAINS("(dollarSignName)")
EXPECT_STDOUT_CONTAINS("(authMethodUsage)")
EXPECT_STDOUT_CONTAINS("(deprecatedDefaultAuth)")
EXPECT_STDOUT_CONTAINS("(deprecatedRouterAuthMethod)")
EXPECT_STDOUT_CONTAINS("(deprecatedTemporalDelimiter)")
WIPE_OUTPUT()

#@<> WL15974-TSFR_1_1_3
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "exclude":["dollarSignName"]})
EXPECT_STDOUT_CONTAINS("(reservedKeywords)")
EXPECT_STDOUT_CONTAINS("(checkTableCommand)")
EXPECT_STDOUT_CONTAINS("(changedFunctionsInGeneratedColumns)")
EXPECT_STDOUT_NOT_CONTAINS("(dollarSignName)")
EXPECT_STDOUT_CONTAINS("(authMethodUsage)")
EXPECT_STDOUT_CONTAINS("(deprecatedDefaultAuth)")
EXPECT_STDOUT_CONTAINS("(deprecatedRouterAuthMethod)")
EXPECT_STDOUT_CONTAINS("(deprecatedTemporalDelimiter)")
WIPE_OUTPUT()

util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "exclude":["dollarSignName","deprecatedDefaultAuth"]})
EXPECT_STDOUT_CONTAINS("(reservedKeywords)")
EXPECT_STDOUT_CONTAINS("(checkTableCommand)")
EXPECT_STDOUT_CONTAINS("(changedFunctionsInGeneratedColumns)")
EXPECT_STDOUT_NOT_CONTAINS("(dollarSignName)")
EXPECT_STDOUT_CONTAINS("(authMethodUsage)")
EXPECT_STDOUT_NOT_CONTAINS("(deprecatedDefaultAuth)")
EXPECT_STDOUT_CONTAINS("(deprecatedRouterAuthMethod)")
EXPECT_STDOUT_CONTAINS("(deprecatedTemporalDelimiter)")
WIPE_OUTPUT()

#@<> WL15974-TSFR_1_1_4
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "exclude":[]})
EXPECT_STDOUT_CONTAINS("(reservedKeywords)")
EXPECT_STDOUT_CONTAINS("(checkTableCommand)")
EXPECT_STDOUT_CONTAINS("(changedFunctionsInGeneratedColumns)")
EXPECT_STDOUT_CONTAINS("(dollarSignName)")
EXPECT_STDOUT_CONTAINS("(authMethodUsage)")
EXPECT_STDOUT_CONTAINS("(deprecatedDefaultAuth)")
EXPECT_STDOUT_CONTAINS("(deprecatedRouterAuthMethod)")
EXPECT_STDOUT_CONTAINS("(deprecatedTemporalDelimiter)")
WIPE_OUTPUT()

#@<> WL15974-TSFR_1_1_5
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "include":{}})
EXPECT_STDERR_CONTAINS("Argument #2: Option 'include' is expected to be of type Array, but is Map")
WIPE_OUTPUT()

util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "include":1})
EXPECT_STDERR_CONTAINS("Argument #2: Option 'include' is expected to be of type Array, but is Integer")
WIPE_OUTPUT()

util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "include":False})
EXPECT_STDERR_CONTAINS("Argument #2: Option 'include' is expected to be of type Array, but is Bool")
WIPE_OUTPUT()

util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "exclude":{}})
EXPECT_STDERR_CONTAINS("Argument #2: Option 'exclude' is expected to be of type Array, but is Map")
WIPE_OUTPUT()

util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "exclude":5.23})
EXPECT_STDERR_CONTAINS("Argument #2: Option 'exclude' is expected to be of type Array, but is Float")
WIPE_OUTPUT()

util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "include":True})
EXPECT_STDERR_CONTAINS("Argument #2: Option 'include' is expected to be of type Array, but is Bool")
WIPE_OUTPUT()

#@<> WL15974-TSFR_1_4_1
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "include":[], "exclude":[]})
EXPECT_STDOUT_CONTAINS("(reservedKeywords)")
EXPECT_STDOUT_CONTAINS("(checkTableCommand)")
EXPECT_STDOUT_CONTAINS("(changedFunctionsInGeneratedColumns)")
EXPECT_STDOUT_CONTAINS("(dollarSignName)")
EXPECT_STDOUT_CONTAINS("(authMethodUsage)")
EXPECT_STDOUT_CONTAINS("(deprecatedDefaultAuth)")
EXPECT_STDOUT_CONTAINS("(deprecatedRouterAuthMethod)")
EXPECT_STDOUT_CONTAINS("(deprecatedTemporalDelimiter)")
WIPE_OUTPUT()

#@<> WL15974-TSFR_1_6_1
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "include":["reservedKeywords", "dollarSignName"], "exclude":["dollarSignName", "deprecatedRouterAuthMethod"]})
EXPECT_STDOUT_CONTAINS("ERROR: Both include and exclude options contain check `dollarSignName`.")
WIPE_OUTPUT()

#@<> WL15974-TSFR_2_1
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0"})
EXPECT_STDOUT_CONTAINS("1) Usage of old temporal type (oldTemporal)")
EXPECT_STDOUT_CONTAINS("2) MySQL syntax check for routine-like objects (routineSyntax)")
EXPECT_STDOUT_CONTAINS("3) Usage of db objects with names conflicting with new reserved keywords")
EXPECT_STDOUT_CONTAINS("(reservedKeywords)")
EXPECT_STDOUT_CONTAINS("4) Usage of utf8mb3 charset (utf8mb3)")
EXPECT_STDOUT_CONTAINS("5) Table names in the mysql schema conflicting with new tables in the latest")
EXPECT_STDOUT_CONTAINS("MySQL. (mysqlSchema)")
EXPECT_STDOUT_CONTAINS("6) Partitioned tables using engines with non native partitioning")
EXPECT_STDOUT_CONTAINS("(nonNativePartitioning)")
EXPECT_STDOUT_CONTAINS("7) Foreign key constraint names longer than 64 characters (foreignKeyLength)")
EXPECT_STDOUT_CONTAINS("8) Usage of obsolete MAXDB sql_mode flag (maxdbSqlModeFlags)")
EXPECT_STDOUT_CONTAINS("9) Usage of obsolete sql_mode flags (obsoleteSqlModeFlags)")
EXPECT_STDOUT_CONTAINS("10) ENUM/SET column definitions containing elements longer than 255 characters")
EXPECT_STDOUT_CONTAINS("(enumSetElementLength)")
WIPE_OUTPUT()


#@<> WL15974-TSFR_3_2
util.check_for_server_upgrade(__mysql_uri, {"list": ["123","asd"]})
EXPECT_STDERR_CONTAINS("Argument #2: Option 'list' is expected to be of type Bool, but is Array")
WIPE_OUTPUT()

#@ WL15974-TSFR_3_3_1
util.check_for_server_upgrade(options={"list": True, "include": ["checkTableCommand"]})

#@ WL15974-TSFR_3_3_2
shell.disconnect()
WIPE_OUTPUT()
util.check_for_server_upgrade(options={"list": True})

#@<> invalid check ids in include
util.check_for_server_upgrade(options={"list": True, "include": ["checkTableCommand", "invalidCheck"]})
EXPECT_STDERR_CONTAINS("Option include contains unknown values 'invalidCheck'")
WIPE_OUTPUT()

util.check_for_server_upgrade(options={"list": True, "include": ["tableCommand"]})
EXPECT_STDERR_CONTAINS("Option include contains unknown values 'tableCommand'")
WIPE_OUTPUT()

util.check_for_server_upgrade(options={"list": True, "include": ["invalid1", "invalid2", "invalid3"]})
EXPECT_STDERR_CONTAINS("Option include contains unknown values 'invalid1', 'invalid2', 'invalid3'")
WIPE_OUTPUT()

#@<> invalid check ids in exclude
util.check_for_server_upgrade(options={"list": True, "exclude": ["checkTableCommand", "invalidCheck"]})
EXPECT_STDERR_CONTAINS("Option exclude contains unknown values 'invalidCheck'")
WIPE_OUTPUT()

util.check_for_server_upgrade(options={"list": True, "exclude": ["tableCommand"]})
EXPECT_STDERR_CONTAINS("Option exclude contains unknown values 'tableCommand'")
WIPE_OUTPUT()

util.check_for_server_upgrade(options={"list": True, "exclude": ["invalid1", "invalid2", "invalid3"]})
EXPECT_STDERR_CONTAINS("Option exclude contains unknown values 'invalid1', 'invalid2', 'invalid3'")
WIPE_OUTPUT()

#@ warning about excluded checks
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "exclude":["checkTableCommand", "dollarSignName"]})

#@ warning about excluded checks JSON
util.check_for_server_upgrade(__mysql_uri, {"targetVersion": "8.1.0", "exclude":["checkTableCommand", "dollarSignName"], "outputFormat": "JSON"})
