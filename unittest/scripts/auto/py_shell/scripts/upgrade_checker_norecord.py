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

   Solution:
   - In order to fix them use the mysqlcheck utility as follows:

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


#@<> Invalid FK References Check - Setup
session.run_sql("CREATE SCHEMA uctest3")
session.run_sql("USE uctest3")

session.run_sql("""
  CREATE TABLE parent (
    a int,
    b int,
    c int NOT NULL,
    d int NOT NULL,
    e int,
    f int,
    PRIMARY KEY (a,b))""")

session.run_sql("CREATE UNIQUE INDEX my_unique_index ON parent (c, d)")

session.run_sql("CREATE INDEX my_non_unique_index ON parent (e, f)")

session.run_sql("""
  CREATE TABLE child_aaa_partial_fk_to_primary (
   a int PRIMARY KEY,
   b int,
   FOREIGN KEY (a) REFERENCES parent(a))""")

session.run_sql("""
  CREATE TABLE child_bbb_partial_fk_to_unique_index (
   a int PRIMARY KEY,
   b int,
   FOREIGN KEY (a) REFERENCES parent(c))""")

session.run_sql("""
  CREATE TABLE child_ccc_full_fk_to_non_unique_index (
   a int PRIMARY KEY,
   b int,
   FOREIGN KEY (a, b) REFERENCES parent(e, f))""")

session.run_sql("""
  CREATE TABLE child_ddd_full_fk_to_parent_pk (
   a int PRIMARY KEY,
   b int,
   FOREIGN KEY (a, b) REFERENCES parent(a, b))""")

session.run_sql("""
  CREATE TABLE child_eee_full_fk_to_parent_unique_index (
   a int PRIMARY KEY,
   b int,
   FOREIGN KEY (a, b) REFERENCES parent(c, d))""")


#@<> Invalid FK References Check - Verification
testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include=foreignKeyReferences"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

EXPECT_STDOUT_CONTAINS_MULTILINE("""
1) Checks for foreign keys not referencing a full unique index
(foreignKeyReferences)
   Foreign keys to partial indexes may be forbidden as of 8.4.0, this check
   identifies such cases to warn the user.

   uctest3.child_ccc_full_fk_to_non_unique_index_ibfk_1 - invalid foreign key
      defined as 'child_ccc_full_fk_to_non_unique_index(a,b)' references a non
      unique key at table 'parent'.
   uctest3.child_aaa_partial_fk_to_primary_ibfk_1 - invalid foreign key defined
      as 'uctest3.child_aaa_partial_fk_to_primary(a)' references a partial key at
      table 'parent'.
   uctest3.child_bbb_partial_fk_to_unique_index_ibfk_1 - invalid foreign key
      defined as 'uctest3.child_bbb_partial_fk_to_unique_index(a)' references a
      partial key at table 'parent'.

   Solutions:
   - Convert non unique key to unique key if values do not have any duplicates.
      In case of foreign keys involving partial columns of key, create composite
      unique key containing all the referencing columns if values do not have any
      duplicates.

   - Remove foreign keys referring to non unique key/partial columns of key.

   - In case of multi level references which involves more than two tables
      change foreign key reference.

                                 
Errors:   0
Warnings: 3
Notices:  0
""")

#@<> Invalid FK References Check - Verification JSON
testutil.call_mysqlsh([__sandbox_uri1, "--", "util", "check-for-server-upgrade", "--include=foreignKeyReferences", "--outputFormat=JSON"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"])

EXPECT_STDOUT_CONTAINS_MULTILINE("""
{
    "serverAddress": "localhost:[[*]]",
    "serverVersion": "[[*]]",
    "targetVersion": "[[*]]",
    "errorCount": 0,
    "warningCount": 3,
    "noticeCount": 0,
    "summary": "No fatal errors were found that would prevent an upgrade, but some potential issues were detected. Please ensure that the reported issues are not significant before upgrading.",
    "checksPerformed": [
        {
            "id": "foreignKeyReferences",
            "title": "Checks for foreign keys not referencing a full unique index",
            "status": "OK",
            "description": "Foreign keys to partial indexes may be forbidden as of 8.4.0, this check identifies such cases to warn the user.",
            "detectedProblems": [
                {
                    "level": "Warning",
                    "dbObject": "uctest3.child_ccc_full_fk_to_non_unique_index_ibfk_1",
                    "description": "invalid foreign key defined as 'child_ccc_full_fk_to_non_unique_index(a,b)' references a non unique key at table 'parent'.",
                    "dbObjectType": "ForeignKey"
                },
                {
                    "level": "Warning",
                    "dbObject": "uctest3.child_aaa_partial_fk_to_primary_ibfk_1",
                    "description": "invalid foreign key defined as 'uctest3.child_aaa_partial_fk_to_primary(a)' references a partial key at table 'parent'.",
                    "dbObjectType": "ForeignKey"
                },
                {
                    "level": "Warning",
                    "dbObject": "uctest3.child_bbb_partial_fk_to_unique_index_ibfk_1",
                    "description": "invalid foreign key defined as 'uctest3.child_bbb_partial_fk_to_unique_index(a)' references a partial key at table 'parent'.",
                    "dbObjectType": "ForeignKey"
                }
            ],
            "solutions": [
                "Convert non unique key to unique key if values do not have any duplicates. In case of foreign keys involving partial columns of key, create composite  unique key containing all the referencing columns if values do not have any  duplicates.",
                "Remove foreign keys referring to non unique key/partial columns of key.",
                "In case of multi level references which involves more than two tables change foreign key reference."
            ]
        }
    ],
    "manualChecks": []
}
""")

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)