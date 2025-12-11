#@{VER(>8.0.0) and __mysh_version_num < __version_num}


#@<> Setup
# This test only executes when source server is on a specific version range
should_execute = ((__version_num >= 80003) and (__version_num <= 80040) or 
                  (__version_num >= 80100 and __version_num < 80403) or 
                  (__version_num >= 90000 and __version_num < 90100))

testutil.deploy_raw_sandbox(port=__mysql_sandbox_port1, rootpass="root")

shell.connect(__sandbox_uri1)


session.run_sql("CREATE DATABASE IF NOT EXISTS spidxdb CHARACTER SET utf8mb3")
session.run_sql("USE spidxdb")
session.run_sql("CREATE TABLE simple_auto_clustered_correct(col INT NOT NULL)")
session.run_sql("CREATE TABLE simple_clustered_correct (id INT PRIMARY KEY,col INT NOT NULL)")
session.run_sql("CREATE TABLE spatial_correct (geo GEOMETRY NOT NULL SRID 4326)")
session.run_sql("CREATE TABLE spatial_clustered_correct (id INT PRIMARY KEY,geo2 GEOMETRY NOT NULL SRID 4326)")

#@<> execute the check with no issues
WIPE_OUTPUT()
rc = testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include", "spatialIndex"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQL_HOME=."])
EXPECT_EQ(0, rc)

if (should_execute):
   EXPECT_OUTPUT_CONTAINS("1) Checks for Spatial Indexes (spatialIndex)")
else:
   EXPECT_OUTPUT_NOT_CONTAINS("1) Checks for Spatial Indexes (spatialIndex)")


#@<> execute the check with issues {should_execute}
session.run_sql("CREATE TABLE spatial_warning (geo3 GEOMETRY NOT NULL SRID 4326,SPATIAL KEY  `index_geo_warn` (geo3))")
session.run_sql("CREATE TABLE spatial_clustered_warning (id INT PRIMARY KEY,geo4 GEOMETRY NOT NULL SRID 4326,SPATIAL KEY  `index_geo2_warn` (geo4))")

WIPE_OUTPUT()
rc = testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include", "spatialIndex"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQL_HOME=."])
EXPECT_EQ(0, rc)

if should_execute:
   EXPECT_OUTPUT_CONTAINS_MULTILINE(f'''1) Checks for Spatial Indexes (spatialIndex)
   MySQL {__mysh_version} includes a fix for an issue with InnoDB spatial indexes.
   In order to avoid corruption, these indexes must be dropped before the
   upgrade and re-created afterwards. The following InnoDB tables currently
   have spatial indexes that must be dropped before the upgrade proceeds:

   Warning: 
   - spidxdb.spatial_clustered_warning: index_geo2_warn
   - spidxdb.spatial_warning: index_geo_warn


Errors:   0
Warnings: 2
Notices:  0

NOTE: No fatal errors were found that would prevent an upgrade, but some potential issues were detected. Please ensure that the reported issues are not significant before upgrading.
''')


#@<> TearDown
testutil.destroy_sandbox(port=__mysql_sandbox_port1)
