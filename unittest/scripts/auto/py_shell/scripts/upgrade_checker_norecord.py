#@{VER(<8.0.0)}
#@<> Sandbox Deployment
import os
testutil.deploy_raw_sandbox(port=__mysql_sandbox_port1, rootpass="root")
shell.connect(__sandbox_uri1)


#@<> Check invalid names in deployed instance
util.check_for_server_upgrade(None, {"include":["invalid57Names"]})

EXPECT_STDOUT_CONTAINS_MULTILINE("""
1) Check for invalid table names and schema names used in 5.7 (invalid57Names)
   No issues found
Errors:   0
Warnings: 0
Notices:  0

No known compatibility errors or issues were found.
""")

#@<> Invalid Name Check
datadir = testutil.get_sandbox_datadir(__mysql_sandbox_port1)
session.run_sql("CREATE SCHEMA uctest1")
session.run_sql("CREATE TABLE uctest1.demo (ID INT)")
session.run_sql("CREATE SCHEMA uctest2")

testutil.stop_sandbox(__mysql_sandbox_port1, options={"wait":1})

old_table_name = os.path.join(datadir, "uctest1", "demo")
new_table_name = os.path.join(datadir, "uctest1", "lost+found")
old_schema_folder = os.path.join(datadir, "uctest2")
new_schema_folder = os.path.join(datadir, "lost+found")

os.rename(old_schema_folder, new_schema_folder)
os.rename(old_table_name + ".frm", new_table_name + ".frm")
os.rename(old_table_name + ".ibd", new_table_name + ".ibd")

testutil.start_sandbox(__mysql_sandbox_port1)
shell.connect(__sandbox_uri1)

util.check_for_server_upgrade(None, {"include":["invalid57Names"]})

EXPECT_STDOUT_CONTAINS_MULTILINE("""
1) Check for invalid table names and schema names used in 5.7 (invalid57Names)
   The following tables and/or schemas have invalid names.

   #mysql50#lost+found - Schema name
   uctest1.#mysql50#lost+found - Table name

   In order to fix them use the mysqlcheck utility as follows:

     $ mysqlcheck --check-upgrade --all-databases
     $ mysqlcheck --fix-db-names --fix-table-names --all-databases

   OR via mysql client, for eg:

     ALTER DATABASE `#mysql50#lost+found` UPGRADE DATA DIRECTORY NAME;

   More information:
     https://dev.mysql.com/doc/refman/5.7/en/identifier-mapping.html
     https://dev.mysql.com/doc/refman/5.7/en/alter-database.html
     https://dev.mysql.com/doc/refman/8.0/en/mysql-nutshell.html#mysql-nutshell-removals

Errors:   2
Warnings: 0
Notices:  0

ERROR: 2 errors were found. Please correct these issues before upgrading to avoid compatibility issues.
""")


#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)