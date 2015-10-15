## This file should be placed in the root directory of the build tree.

set(CTEST_CUSTOM_COVERAGE_EXCLUDE
	".*ext/rapidjson.*"
	".*mysqlxtest/myasio.*"
	".*mysqlxtest/.*\.pb\..*"
	".*common/myjson.*")
