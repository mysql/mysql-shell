# Special case integration tests of dump and load

#@<> INCLUDE dump_utils.inc

#@<> Setup
from contextlib import ExitStack
import json
import os
import os.path
import random
import shutil
import threading
import time
from typing import NamedTuple
import urllib.parse

outdir = os.path.join(__tmp_dir, "ldtest")
wipe_dir(outdir)
testutil.mkdir(outdir)

mysql_tmpdir = os.path.join(__tmp_dir, "mysql")
wipe_dir(mysql_tmpdir)
testutil.mkdir(mysql_tmpdir)

def prepare(sbport, options={}):
    # load test schemas
    uri = "mysql://root:root@localhost:%s" % sbport
    datadir = os.path.join(outdir, f"datadirs{sbport}")
    testutil.mkdir(datadir)
    # 8.0 seems to create dirs automatically but not 5.7
    if __version_num < 80000:
        testutil.mkdir(os.path.join(datadir, "testindexdir"))
        testutil.mkdir(os.path.join(datadir, "test datadir"))
    options.update({
        "loose_innodb_directories": datadir,
        "early_plugin_load": "keyring_file."+("dll" if __os_type == "windows" else "so"),
        "keyring_file_data": os.path.join(datadir, "keyring"),
        "local_infile": "1",
        "tmpdir": mysql_tmpdir,
        # small sort buffer to force stray filesorts to be triggered even if we don't have much data
        "sort_buffer_size": 32768,
        "loose_mysql_native_password": "ON"
    })
    if __os_type == "windows":
        options.update({
            "named_pipe": "1",
            "socket": f"mysql-dl-{sbport}"
        })
    testutil.deploy_sandbox(sbport, "root", options)


prepare(__mysql_sandbox_port1)
session1 = mysql.get_session(__sandbox_uri1)
session1.run_sql("SET @@GLOBAL.time_zone = '+9:00'")
session1.run_sql("SET @@GLOBAL.character_set_server = 'euckr'")
session1.run_sql("SET @@GLOBAL.collation_server = 'euckr_korean_ci'")
session1.run_sql("set names utf8mb4")
session1.run_sql("create schema world")
testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "world.sql"), "world")
testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "misc_features.sql"))
testutil.import_data(__sandbox_uri1, os.path.join(__data_path, "sql", "fieldtypes_all.sql"), "", "utf8mb4")
session1.run_sql("create schema tests")
# BUG#33079172 "SQL SECURITY INVOKER" INSERTED AT WRONG LOCATION
session1.run_sql("create procedure tests.tmp() IF @v > 0 THEN SELECT 1; ELSE SELECT 2; END IF;;")

shell.connect(__sandbox_uri1)
util.dump_instance(os.path.join(outdir, "dump_instance"), {"showProgress": False})

# BUG#34566034 - load failed when performance_schema was disabled
prepare(__mysql_sandbox_port2, options={"performance_schema": "OFF"})
session2 = mysql.get_session(__sandbox_uri2)
session2.run_sql("set names utf8mb4")

# BUG#36159820 - accounts which are excluded when dumping with ocimds=true and when loading into MHS
# BUG#37457569 - added mysql_option_tracker_persister
# BUG#37686519 - added:
# * mysql_rest_service_admin,
# * mysql_rest_service_data_provider,
# * mysql_rest_service_dev,
# * mysql_rest_service_meta_provider,
# * mysql_rest_service_schema_admin,
# * mysql_rest_service_user,
# * mysql_task_admin,
# * mysql_task_user,
# BUG#37792183 - added ocirest - MRS admin in MHS
mhs_excluded_users = [
    "administrator",
    "mysql_option_tracker_persister",
    "mysql_rest_service_admin",
    "mysql_rest_service_data_provider",
    "mysql_rest_service_dev",
    "mysql_rest_service_meta_provider",
    "mysql_rest_service_schema_admin",
    "mysql_rest_service_user",
    "mysql_task_admin",
    "mysql_task_user",
    "ociadmin",
    "ocidbm",
    "ocimonitor",
    "ocirest",
    "ocirpl",
    "oracle-cloud-agent",
    "rrhhuser",
]

## Tests to ensure restricted users dumped with strip_restricted_grants can be loaded with a restricted user and not just with root

#@<> ensure accounts dumped in compat mode can be loaded {VER(>=8.0.16) and not __dbug_off}
testutil.dbug_set("+d,dump_loader_force_mds")
session2.run_sql("SET global partial_revokes=1")

mds_administrator_role_grants = [
"CREATE ROLE IF NOT EXISTS administrator",
"GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO `administrator`@`%` WITH GRANT OPTION",
"GRANT APPLICATION_PASSWORD_ADMIN,CONNECTION_ADMIN,REPLICATION_APPLIER,RESOURCE_GROUP_ADMIN,RESOURCE_GROUP_USER,XA_RECOVER_ADMIN ON *.* TO `administrator`@`%` WITH GRANT OPTION",
"REVOKE INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `mysql`.* FROM `administrator`@`%`",
"REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `administrator`@`%`"
]

mds_user_grants = [
    "GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO {} WITH GRANT OPTION",
    "GRANT APPLICATION_PASSWORD_ADMIN,CONNECTION_ADMIN,REPLICATION_APPLIER,RESOURCE_GROUP_ADMIN,RESOURCE_GROUP_USER,XA_RECOVER_ADMIN ON *.* TO {} WITH GRANT OPTION",
    "REVOKE INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `mysql`.* FROM {}",
    "REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM {}",
    "GRANT PROXY ON ''@'' TO {} WITH GRANT OPTION",
    "GRANT `administrator`@`%` TO {} WITH ADMIN OPTION"
]

def reset_server(session):
    wipeout_server(session)
    for grant in mds_administrator_role_grants:
        session.run_sql(grant)
    session.run_sql("create user if not exists admin@'%' identified by 'pass'")
    for grant in mds_user_grants:
        session.run_sql(grant.format("admin@'%'"))

reset_server(session2)

session2.run_sql("create user testuser@'%' IDENTIFIED BY 'password'")
session2.run_sql("grant select, delete on *.* to testuser@'%'")
session2.run_sql("create user myuser@'%' IDENTIFIED BY 'password'")
session2.run_sql("create user myuser2@'%' IDENTIFIED BY 'password'")
session2.run_sql("create user myuser3@'%' IDENTIFIED BY 'password'")
session2.run_sql("CREATE SCHEMA test_schema")

#Bug #32526567 - strip_restricted_grants should allow GRANT ALL ON user schemas
# should work as is in MDS
session2.run_sql("grant all on test_schema.* to myuser@'%'")
# should expand and filter out privs
session2.run_sql("grant all on mysql.* to myuser@'%'")

# should expand and filter out privs
session2.run_sql("grant all on *.* to myuser2@'%'")

# single user with all grants
session2.run_sql("grant all on test_schema.* to myuser3@'%' with grant option")
session2.run_sql("grant all on mysql.* to myuser3@'%' with grant option")
session2.run_sql("grant all on *.* to myuser3@'%' with grant option")

# test role
test_role = "sample-test-role"
session2.run_sql(f"CREATE ROLE IF NOT EXISTS '{test_role}'")

# BUG#36159820 - accounts which are excluded when dumping with ocimds=true
for u in mhs_excluded_users:
    session2.run_sql(f"CREATE USER IF NOT EXISTS '{u}'@'localhost' IDENTIFIED BY 'pwd';")

def format_rows(rows):
    return "\n".join([r[0] for r in rows])

shell.connect(__sandbox_uri2)

# dump with root
util.dump_instance(os.path.join(outdir, "dump_root"), {"compatibility":["strip_restricted_grants", "strip_definers"], "ocimds":True})
users_file = os.path.join(outdir, "dump_root", "@.users.sql")

# CREATE USER for role is converted to CREATE ROLE
EXPECT_FILE_CONTAINS(f"""CREATE ROLE IF NOT EXISTS {get_user_account_for_output(f"'{test_role}'@'%'")}""", users_file)

# BUG#36159820 - accounts were not dumped
for u in mhs_excluded_users:
    EXPECT_FILE_NOT_CONTAINS(f"""CREATE USER IF NOT EXISTS {get_user_account_for_output(f"'{u}'@'localhost'")}""", users_file)

# BUG#36159820 - administrator role was excluded, there should be a warning that a user has excluded role
EXPECT_STDOUT_CONTAINS(f"User 'admin'@'%' has a grant statement on a role `administrator`@`%` which is not included in the dump (")

# dump with admin user
shell.connect(f"mysql://admin:pass@localhost:{__mysql_sandbox_port2}")
# disable consistency b/c we won't be able to acquire a backup lock
util.dump_instance(os.path.join(outdir, "dump_admin"), {"compatibility":["strip_restricted_grants", "strip_definers"], "ocimds":True, "consistent":False})

# load with admin user
reset_server(session2)
util.load_dump(os.path.join(outdir, "dump_admin"), {"loadUsers":1, "loadDdl":0, "loadData":0, "excludeUsers":["root@%","root@localhost"]})

EXPECT_EQ("""GRANT USAGE ON *.* TO `myuser`@`%`
GRANT SELECT, SHOW VIEW ON `mysql`.* TO `myuser`@`%`
GRANT ALL PRIVILEGES ON `test_schema`.* TO `myuser`@`%`""",
        format_rows(session2.run_sql("show grants for myuser@'%'").fetch_all()))

EXPECT_EQ("""GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO `myuser2`@`%`
GRANT APPLICATION_PASSWORD_ADMIN,CONNECTION_ADMIN,REPLICATION_APPLIER,XA_RECOVER_ADMIN ON *.* TO `myuser2`@`%`
REVOKE INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `mysql`.* FROM `myuser2`@`%`
REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `myuser2`@`%`""",
          format_rows(session2.run_sql("show grants for myuser2@'%'").fetch_all()))

EXPECT_EQ("""GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT APPLICATION_PASSWORD_ADMIN,CONNECTION_ADMIN,REPLICATION_APPLIER,XA_RECOVER_ADMIN ON *.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT SELECT, SHOW VIEW ON `mysql`.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT ALL PRIVILEGES ON `test_schema`.* TO `myuser3`@`%` WITH GRANT OPTION
REVOKE INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `mysql`.* FROM `myuser3`@`%`
REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `myuser3`@`%`""",
          format_rows(session2.run_sql("show grants for myuser3@'%'").fetch_all()))

reset_server(session2)
util.load_dump(os.path.join(outdir, "dump_root"), {"loadUsers":1, "loadDdl":0, "loadData":0, "excludeUsers":["root@%","root@localhost"]})

EXPECT_EQ("""GRANT USAGE ON *.* TO `myuser`@`%`
GRANT SELECT, SHOW VIEW ON `mysql`.* TO `myuser`@`%`
GRANT ALL PRIVILEGES ON `test_schema`.* TO `myuser`@`%`""",
          format_rows(session2.run_sql("show grants for myuser@'%'").fetch_all()))

EXPECT_EQ("""GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO `myuser2`@`%`
GRANT APPLICATION_PASSWORD_ADMIN,CONNECTION_ADMIN,REPLICATION_APPLIER,XA_RECOVER_ADMIN ON *.* TO `myuser2`@`%`
REVOKE INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `mysql`.* FROM `myuser2`@`%`
REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `myuser2`@`%`""",
          format_rows(session2.run_sql("show grants for myuser2@'%'").fetch_all()))

EXPECT_EQ("""GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT APPLICATION_PASSWORD_ADMIN,CONNECTION_ADMIN,REPLICATION_APPLIER,XA_RECOVER_ADMIN ON *.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT SELECT, SHOW VIEW ON `mysql`.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT ALL PRIVILEGES ON `test_schema`.* TO `myuser3`@`%` WITH GRANT OPTION
REVOKE INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `mysql`.* FROM `myuser3`@`%`
REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `myuser3`@`%`""",
          format_rows(session2.run_sql("show grants for myuser3@'%'").fetch_all()))

testutil.dbug_set("")

wipeout_server(session2)
session2.run_sql("SET global partial_revokes=0")

#@<> Dump ddlOnly
shell.connect(__sandbox_uri1)
util.dump_instance(outdir+"/ddlonly", {"ddlOnly": True})

shell.connect(__sandbox_uri2)

#@<> load data which is not in the dump (fail)
EXPECT_THROWS(lambda: util.load_dump(outdir+"/ddlonly",
                                     {"loadData": True, "loadDdl": False, "excludeSchemas":["all_features","all_features2","xtest"]}), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_MATCHES(re.compile(r"ERROR: \[Worker00\d\]: While executing DDL script for `.+`\.`.+`: Unknown database 'world'"))

testutil.rmfile(outdir+"/ddlonly/load-progress*.json")

#@<> load ddl normally
util.load_dump(outdir+"/ddlonly")

compare_servers(session1, session2, check_rows=False, check_users=False)
#@<> Dump dataOnly
shell.connect(__sandbox_uri1)
util.dump_instance(outdir+"/dataonly", {"dataOnly": True})

shell.connect(__sandbox_uri2)

#@<> load data assuming tables already exist

# WL14841-TSFR_2_1
# will fail because tables already exist
EXPECT_THROWS(lambda: util.load_dump(outdir+"/dataonly",
                                     {"dryRun": True}), "Error: Shell Error (53021): Util.load_dump: While 'Checking for pre-existing objects': Duplicate objects found in destination database")

# will pass because we're explicitly only loading data
util.load_dump(outdir+"/dataonly", {"dryRun": True, "loadDdl": False})

util.load_dump(outdir+"/dataonly", {"ignoreExistingObjects": True})

compare_servers(session1, session2, check_users=False)

wipeout_server(session2)


#@<> load ddl which is not in the dump (fail/no-op)
util.load_dump(outdir+"/dataonly", {"loadData": False, "loadDdl": True})

#@<> BUG#33502098 - dump which does not include any schemas but includes users should succeed
dump_dir = os.path.join(outdir, "bug_33502098")

# dump
shell.connect(__sandbox_uri1)
EXPECT_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [ "non_existing_schema" ], "users": False }), "Filters for schemas result in an empty set.")
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [ "non_existing_schema" ], "users": True }), "Dump")

# load
shell.connect(__sandbox_uri2)
wipeout_server(session2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers":True }), "Load")

# validation
compare_users(session1, session2)

#@<> Bug #32526496 - ensure dumps with users that have grants on specific objects (tables, SPs etc) can be loaded

shell.connect(__sandbox_uri1)
# first create all objects that can reference a user, since the load has to create accounts last
# this also ensures that our assumption that the server allows objects to reference users that don't exist still holds
session.run_sql("CREATE SCHEMA schema1")
session.run_sql("CREATE SCHEMA schema2")
session.run_sql("USE schema1")
session.run_sql("CREATE definer=uuuser@localhost PROCEDURE mysp1() BEGIN END")
session.run_sql("CREATE definer=uuuuser@localhost FUNCTION myfun1() RETURNS INT NO SQL RETURN 1")
session.run_sql("CREATE definer=uuuuuser@localhost VIEW view1 AS select 1")
session.run_sql("CREATE TABLE table1 (pk INT PRIMARY KEY)")
session.run_sql("CREATE definer=uuuuuuser@localhost TRIGGER trigger1 BEFORE UPDATE ON table1 FOR EACH ROW BEGIN END")
session.run_sql("CREATE definer=uuuuuuuser@localhost EVENT event1 ON SCHEDULE EVERY 1 year DISABLE DO BEGIN END")

session.run_sql("CREATE USER uuuser@localhost IDENTIFIED BY 'pwd'")
session.run_sql("CREATE USER uuuuser@localhost IDENTIFIED BY 'pwd'")
session.run_sql("CREATE USER uuuuuser@localhost IDENTIFIED BY 'pwd'")
session.run_sql("CREATE USER uuuuuuser@localhost IDENTIFIED BY 'pwd'")
session.run_sql("CREATE USER uuuuuuuser@localhost IDENTIFIED BY 'pwd'")

dump_dir = os.path.join(outdir, "user_load_order")
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir), "Dump")

shell.connect(__sandbox_uri2)
wipeout_server(session2)

EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, {"loadUsers":True, "excludeUsers":["root@%"]}), "Load")

compare_servers(session1, session2)

# cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP USER uuuser@localhost")
session.run_sql("DROP USER uuuuser@localhost")
session.run_sql("DROP USER uuuuuser@localhost")
session.run_sql("DROP USER uuuuuuser@localhost")
session.run_sql("DROP USER uuuuuuuser@localhost")

#@<> BUG#34952027 - privileges are no longer filtered based on object filters
# NOTE: this removes filtering introduced by fix for BUG#33406711
# setup
dump_dir = os.path.join(outdir, "bug_34952027")

tested_user = "'user_34952027'@'localhost'"
tested_user_key = tested_user.replace("'", "")
tested_user_for_output = get_user_account_for_output(tested_user)

session1.run_sql(f"CREATE USER {tested_user} IDENTIFIED BY 'pwd'")
grant_on_table = f"GRANT SELECT ON `schema1`.`table1` TO {tested_user_for_output}"
session1.run_sql(grant_on_table)
grant_on_function = f"GRANT EXECUTE ON FUNCTION `schema1`.`myfun1` TO {tested_user_for_output}"
session1.run_sql(grant_on_function)
grant_on_procedure = f"GRANT ALTER ROUTINE ON PROCEDURE `schema1`.`mysp1` TO {tested_user_for_output}"
session1.run_sql(grant_on_procedure)
grant_on_excluded_schema = f"GRANT ALTER ON `schema2`.* TO {tested_user_for_output}"
session1.run_sql(grant_on_excluded_schema)
# schema-level privileges with wild-cards
schema_level_grant_with_underscore = f"GRANT SELECT ON `some_schema`.* TO {tested_user_for_output}"
session1.run_sql(schema_level_grant_with_underscore)
schema_level_grant_with_escaped_underscore = f"GRANT SELECT ON `another\\_schema`.* TO {tested_user_for_output}"
session1.run_sql(schema_level_grant_with_escaped_underscore)
schema_level_grant_with_percent = f"GRANT SELECT ON `myschema%`.* TO {tested_user_for_output}"
session1.run_sql(schema_level_grant_with_percent)
schema_level_grant_with_escaped_percent = f"GRANT SELECT ON `all\\%`.* TO {tested_user_for_output}"
session1.run_sql(schema_level_grant_with_escaped_percent)

expected_accounts = snapshot_accounts(session1)
del expected_accounts["root@%"]

#@<> BUG#34952027 - warnings for grants with excluded objects
shell.connect(__sandbox_uri1)

EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [ "schema2" ], "showProgress": False }), "Dump")

EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, grant_on_table).warning())
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, grant_on_function).warning())
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, grant_on_procedure).warning())
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, schema_level_grant_with_escaped_underscore).warning())
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, schema_level_grant_with_escaped_percent).warning())
# wildcard grants are not reported
EXPECT_STDOUT_NOT_CONTAINS(grant_on_excluded_object(tested_user, schema_level_grant_with_underscore).warning())
EXPECT_STDOUT_NOT_CONTAINS(grant_on_excluded_object(tested_user, schema_level_grant_with_percent).warning())

wipe_dir(dump_dir)

#@<> BUG#34952027 - warnings for grants with excluded objects with partial_revokes ON {VER(>=8.0.16)}
shell.connect(__sandbox_uri1)
session.run_sql("SET @@GLOBAL.partial_revokes = ON")

EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [ "schema2" ], "showProgress": False }), "Dump")

EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, grant_on_table).warning())
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, grant_on_function).warning())
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, grant_on_procedure).warning())
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, schema_level_grant_with_escaped_underscore).warning())
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, schema_level_grant_with_escaped_percent).warning())
# partial_revokes is ON, wildcard grants are reported
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, schema_level_grant_with_underscore).warning())
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, schema_level_grant_with_percent).warning())

wipe_dir(dump_dir)
session.run_sql("SET @@GLOBAL.partial_revokes = OFF")

#@<> BUG#34952027 - dump excluding one of the schemas, load with no filters, schema-related privilege is present
wipeout_server(session2)

shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "excludeSchemas": [ "schema2" ], "showProgress": False }), "Dump")
EXPECT_STDOUT_CONTAINS(grant_on_excluded_object(tested_user, grant_on_excluded_schema).warning())

shell.connect(__sandbox_uri2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "excludeUsers": [ "root@%" ], "showProgress": False }), "Load")

# BUG#36197620 - summary should contain more details regarding all executed stages
EXPECT_STDOUT_CONTAINS("59 DDL files were executed in ")
EXPECT_STDOUT_CONTAINS("1 accounts were loaded")
EXPECT_STDOUT_CONTAINS("Data load duration: ")
EXPECT_STDOUT_CONTAINS("Total duration: ")

# root is not re-created
actual_accounts = snapshot_accounts(session2)
del actual_accounts["root@%"]
# validation, privileges for schema2 are included
EXPECT_EQ(expected_accounts, actual_accounts)

#@<> BUG#34952027 - use the same dump, exclude a table when loading, throws because there's a grant which refers to an excluded table
wipeout_server(session2)
testutil.rmfile(os.path.join(dump_dir, "load-progress*.json"))

shell.connect(__sandbox_uri2)

EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeTables": [ "schema1.table1" ], "loadUsers": True, "excludeUsers": [ "root@%" ], "showProgress": False }), "Table 'schema1.table1' doesn't exist")
# 'abort' is the default value for handleGrantErrors
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "handleGrantErrors": "abort", "excludeTables": [ "schema1.table1" ], "loadUsers": True, "excludeUsers": [ "root@%" ], "showProgress": False }), "Table 'schema1.table1' doesn't exist")
# invalid value for handleGrantErrors
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "handleGrantErrors": "invalid", "showProgress": False }), "ValueError: Util.load_dump: Argument #2: The value of the 'handleGrantErrors' option must be set to one of: 'abort', 'drop_account', 'ignore'.")

#@<> BUG#34952027 - same as above, ignore the error
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "handleGrantErrors": "ignore", "excludeTables": [ "schema1.table1" ], "loadUsers": True, "excludeUsers": [ "root@%" ], "showProgress": False }), "Load")
EXPECT_STDOUT_CONTAINS(f"""
ERROR: While executing user accounts SQL: MySQL Error 1146 (42S02): Table 'schema1.table1' doesn't exist: {grant_on_table};
NOTE: The above error was ignored, the load operation will continue.
""")
EXPECT_STDOUT_CONTAINS("1 accounts were loaded, 1 GRANT statement errors were ignored")

# privileges for schema1.table1 are not included
expected_accounts[tested_user_key]["grants"] = [grant for grant in expected_accounts[tested_user_key]["grants"] if "table1" not in grant]
# root is not re-created
actual_accounts = snapshot_accounts(session2)
del actual_accounts["root@%"]
# validation
EXPECT_EQ(expected_accounts, actual_accounts)

#@<> BUG#34952027 - same as above, drop the account
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "handleGrantErrors": "drop_account", "excludeTables": [ "schema1.table1" ], "loadUsers": True, "excludeUsers": [ "root@%" ], "showProgress": False }), "Load")
EXPECT_STDOUT_CONTAINS(f"""
ERROR: While executing user accounts SQL: MySQL Error 1146 (42S02): Table 'schema1.table1' doesn't exist: {grant_on_table};
NOTE: Due to the above error the account 'user_34952027'@'localhost' was dropped, the load operation will continue.
""")
EXPECT_STDOUT_CONTAINS("0 accounts were loaded, 1 accounts were dropped due to GRANT statement errors")
# ensure that no more grant statements are executed
EXPECT_STDOUT_NOT_CONTAINS("You are not allowed to create a user with GRANT")

# privileges for the user are not included
del expected_accounts[tested_user_key]
# root is not re-created
actual_accounts = snapshot_accounts(session2)
del actual_accounts["root@%"]
# validation
EXPECT_EQ(expected_accounts, actual_accounts)

#@<> BUG#34952027 - a warning if the value of partial_revoke differs between source and target {VER(>=8.0.16)}
session.run_sql("SET GLOBAL partial_revokes=1")
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "handleGrantErrors": "drop_account", "excludeTables": [ "schema1.table1" ], "loadUsers": True, "excludeUsers": [ "root@%" ], "showProgress": False }), "Load")
EXPECT_STDOUT_CONTAINS("WARNING: The dump was created on an instance where the 'partial_revokes' system variable was disabled, however the target instance has it enabled. GRANT statements on object names with wildcard characters (% or _) will behave differently.")
session.run_sql("SET GLOBAL partial_revokes=0")

#@<> BUG#34952027 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql(f"DROP USER {tested_user}")

#@ Source data is utf8mb4, but double-encoded in latin1 (preparation)
session1.run_sql("create schema dblenc")
testutil.import_data(__sandbox_uri1, __data_path+"/sql/double_encoded_utf8mb4_with_latin1.sql", "dblenc")

# query as latin1 = OK
session1.run_sql("SET NAMES latin1")
shell.dump_rows(session1.run_sql("SELECT * FROM dblenc.client_latin1_table_utf8mb4"), "tabbed")

# query as utf8mb4 = bakemoji
session1.run_sql("SET NAMES utf8mb4")
shell.dump_rows(session1.run_sql("SELECT * FROM dblenc.client_latin1_table_utf8mb4"), "tabbed")

#@ Preserve double-encoding as latin1
# Dump and load with defaults should leave the data double-encoded in the same way, so select output should be identical as long as client charsets is latin1 in both
shell.connect(__sandbox_uri1)
util.dump_schemas(["dblenc"], outdir+"/dblenc-defaults")
shell.connect(__sandbox_uri2)
util.load_dump(outdir+"/dblenc-defaults")

# query as latin1 should be OK
session2.run_sql("SET NAMES latin1")
shell.dump_rows(session2.run_sql("SELECT * FROM dblenc.client_latin1_table_utf8mb4"), "tabbed")

session2.run_sql("drop schema dblenc")

#@ Fix double-encoding so it can be queried as utf8mb4
# Dump as latin1 and load as utf8mb4 should fix the double-encoding, so select output will be correct in the loaded copy, even when client charset is utf8mb4
shell.connect(__sandbox_uri1)
util.dump_schemas(["dblenc"], outdir+"/dblenc-latin1", {"defaultCharacterSet": "latin1"})
shell.connect(__sandbox_uri2)
# override the source character set
util.load_dump(outdir+"/dblenc-latin1", {"characterSet": "utf8mb4"})

# query as utf8mb4 should be OK
session2.run_sql("SET NAMES utf8mb4")
shell.dump_rows(session2.run_sql("SELECT * FROM dblenc.client_latin1_table_utf8mb4"), "tabbed")

#@<> setup tests with include/exclude users
shell.connect(__sandbox_uri1)
session.run_sql("CREATE USER IF NOT EXISTS 'first'@'localhost' IDENTIFIED BY 'pwd';")
session.run_sql("CREATE USER IF NOT EXISTS 'first'@'10.11.12.13' IDENTIFIED BY 'pwd';")
session.run_sql("CREATE USER IF NOT EXISTS 'firstfirst'@'localhost' IDENTIFIED BY 'pwd';")
session.run_sql("CREATE USER IF NOT EXISTS 'second'@'localhost' IDENTIFIED BY 'pwd';")
session.run_sql("CREATE USER IF NOT EXISTS 'second'@'10.11.12.14' IDENTIFIED BY 'pwd';")
# account which is excluded always, dumper also excludes it, so we need to hack the sql file
session.run_sql("CREATE USER IF NOT EXISTS 'mysql.sys-ex'@'localhost' IDENTIFIED BY 'pwd';")

# BUG#36159820 - accounts which are excluded when loading into MHS - dump is without ocimds, so these accounts are dumped normally
for u in mhs_excluded_users:
    session.run_sql(f"CREATE USER IF NOT EXISTS '{u}'@'localhost' IDENTIFIED BY 'pwd';")

users_outdir = os.path.join(outdir, "users")
util.dump_instance(users_outdir, { "users": True, "ddlOnly": True, "showProgress": False })

# replace 'mysql.sys-ex'@'localhost' with 'mysql.sys'@'localhost' in the SQL
with open(os.path.join(users_outdir, "@.users.sql"), "r+", encoding="utf-8", newline="") as f:
    new_contents = f.read().replace("mysql.sys-ex", "mysql.sys")
    # overwrite the file
    f.seek(0)
    f.write(new_contents)
    f.truncate()

# helper function
def EXPECT_INCLUDE_EXCLUDE(options, included, excluded, expected_exception=None, is_mds=False):
    opts = { "loadUsers": True, "showProgress": False, "resetProgress": True }
    opts.update(options)
    shell.connect(__sandbox_uri2)
    if is_mds:
        reset_server(session)
    else:
        wipeout_server(session)
    WIPE_OUTPUT()
    if expected_exception:
        EXPECT_THROWS(lambda: util.load_dump(users_outdir, opts), expected_exception)
    else:
        EXPECT_NO_THROWS(lambda: util.load_dump(users_outdir, opts), "Loading the dump should not fail")
    all_users = {c[0] for c in session.run_sql("SELECT GRANTEE FROM information_schema.user_privileges;").fetch_all()}
    for i in included:
        EXPECT_TRUE(i in all_users, "User {0} should exist".format(i))
        EXPECT_STDOUT_NOT_CONTAINS("NOTE: Skipping CREATE/ALTER USER statements for user {0}".format(i))
        EXPECT_STDOUT_NOT_CONTAINS("NOTE: Skipping GRANT/REVOKE statements for user {0}".format(i))
    for e in excluded:
        EXPECT_TRUE(e not in all_users, "User {0} should not exist".format(e))
        EXPECT_STDOUT_CONTAINS("NOTE: Skipping CREATE/ALTER USER statements for user {0}".format(e))
        EXPECT_STDOUT_CONTAINS("NOTE: Skipping GRANT/REVOKE statements for user {0}".format(e))
    for u in included + excluded:
        if u.find('mysql.') != -1:
            session.run_sql("DROP USER IF EXISTS {0};".format(u))

#@<> the `includeUsers` and `excludeUsers` options cannot be used when `loadUsers` is false
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": False, "includeUsers": ["third"] }), "ValueError: Util.load_dump: Argument #2: The 'includeUsers' option cannot be used if the 'loadUsers' option is set to false.")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": False, "excludeUsers": ["third"] }), "ValueError: Util.load_dump: Argument #2: The 'excludeUsers' option cannot be used if the 'loadUsers' option is set to false.")

#@<> test invalid user names
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": [""] }), "ValueError: Util.load_dump: Argument #2: User name must not be empty.")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "excludeUsers": [""] }), "ValueError: Util.load_dump: Argument #2: User name must not be empty.")

EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["@"] }), "ValueError: Util.load_dump: Argument #2: User name must not be empty.")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "excludeUsers": ["@"] }), "ValueError: Util.load_dump: Argument #2: User name must not be empty.")

EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["@@"] }), "ValueError: Util.load_dump: Argument #2: Invalid user name: @")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "excludeUsers": ["@@"] }), "ValueError: Util.load_dump: Argument #2: Invalid user name: @")

EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["foo@''nope"] }), "ValueError: Util.load_dump: Argument #2: Malformed hostname. Cannot use \"'\" or '\"' characters on the hostname without quotes")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["foo@''nope"] }), "ValueError: Util.load_dump: Argument #2: Malformed hostname. Cannot use \"'\" or '\"' characters on the hostname without quotes")

#@<> don't include or exclude any users, all accounts are loaded (error from duplicates)
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": [], "excludeUsers": [] }, [], [], "Duplicate objects found in destination database")

EXPECT_STDOUT_CONTAINS("ERROR: Account 'root'@'%' already exists")

#@<> don't include or exclude any users, all accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "ignoreExistingObjects":True, "includeUsers": [], "excludeUsers": [] }, ["'first'@'localhost'", "'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"], [])

EXPECT_STDOUT_CONTAINS("NOTE: Account 'root'@'%' already exists")

#@<> include non-existent user, no accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["third"] }, [], ["'first'@'localhost'", "'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> exclude non-existent user (and root@%), all accounts are loaded, mysql.sys is always excluded (and it already exists)
EXPECT_INCLUDE_EXCLUDE({ "excludeUsers": ["third", "'root'@'%'"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"], [])
EXPECT_STDOUT_CONTAINS("NOTE: Skipping CREATE/ALTER USER statements for user 'mysql.sys'@'localhost'")
EXPECT_STDOUT_CONTAINS("NOTE: Skipping GRANT/REVOKE statements for user 'mysql.sys'@'localhost'")

#@<> include an existing user, one account is loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first@localhost"] }, ["'first'@'localhost'"], ["'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include an existing user, one account is loaded - single quotes
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["'first'@'localhost'"] }, ["'first'@'localhost'"], ["'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include an existing user, one account is loaded - double quotes
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ['"first"@"localhost"'] }, ["'first'@'localhost'"], ["'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include an existing user, one account is loaded - backticks
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["`first`@`localhost`"] }, ["'first'@'localhost'"], ["'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using just the username, two accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'"], ["'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using just the same username twice, two accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "first"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'"], ["'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using just the username, exclude different accounts using just the username, two accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first"], "excludeUsers": ["second"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'"], ["'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include and exclude the same username, conflicting options -> exception
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first"], "excludeUsers": ["first"] }, [], [], "Util.load_dump: Argument #2: Conflicting filtering options")

#@<> include using just the username, exclude one of the accounts, one account is loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first"], "excludeUsers": ["first@10.11.12.13"] }, ["'first'@'localhost'"], ["'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using just the username, exclude one of the accounts, one account is loaded - single quotes
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first"], "excludeUsers": ["'first'@'10.11.12.13'"] }, ["'first'@'localhost'"], ["'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using just the username, exclude one of the accounts, one account is loaded - double quotes
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first"], "excludeUsers": ['"first"@"10.11.12.13"'] }, ["'first'@'localhost'"], ["'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using just the username, exclude one of the accounts, one account is loaded - backticks
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first"], "excludeUsers": ["`first`@`10.11.12.13`"] }, ["'first'@'localhost'"], ["'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using two usernames, four accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'", "'second'@'localhost'", "'second'@'10.11.12.14'"], ["'firstfirst'@'localhost'"])

#@<> include using two usernames, exclude one of them, conflicting options ->exception
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second"], "excludeUsers": ["second"] }, [], [], "Util.load_dump: Argument #2: Conflicting filtering options")

#@<> include using two usernames, exclude one of the accounts, three accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second"], "excludeUsers": ["second@localhost"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'", "'second'@'10.11.12.14'"], ["'firstfirst'@'localhost'", "'second'@'localhost'"])

#@<> include using an username and an account, three accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second@localhost"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'", "'second'@'localhost'"], ["'firstfirst'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using an username and an account, exclude using username, conflicting options -> exception
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second@localhost"], "excludeUsers": ["second"]  }, [], [], "Util.load_dump: Argument #2: Conflicting filtering options")

#@<> include using an username and an account, exclude using an account, conflicting options -> exception
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second@localhost"], "excludeUsers": ["second@localhost"]  }, [], [], "Util.load_dump: Argument #2: Conflicting filtering options")

#@<> include using an username and non-existing username, exclude using a non-existing username, two accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "third"], "excludeUsers": ["fourth"]  }, ["'first'@'localhost'", "'first'@'10.11.12.13'"], ["'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> don't include or exclude anything, mysql.sys is always excluded (and it already exists)
EXPECT_INCLUDE_EXCLUDE({ "ignoreExistingObjects": True }, [], [])
EXPECT_STDOUT_CONTAINS("NOTE: Skipping CREATE/ALTER USER statements for user 'mysql.sys'@'localhost'")
EXPECT_STDOUT_CONTAINS("NOTE: Skipping GRANT/REVOKE statements for user 'mysql.sys'@'localhost'")

#@<> BUG#36159820 - don't include or exclude anything when loading into MHS, some accounts are always excluded {VER(>=8.0.16) and not __dbug_off}
testutil.dbug_set("+d,dump_loader_force_mds")
session2.run_sql("SET global partial_revokes=1")

EXPECT_INCLUDE_EXCLUDE({ "ignoreExistingObjects": True, 'ignoreVersion': True }, [], [ f"'{u}'@'localhost'" for u in mhs_excluded_users ], is_mds = True)
EXPECT_STDOUT_CONTAINS("NOTE: Skipping CREATE/ALTER USER statements for user 'mysql.sys'@'localhost'")
EXPECT_STDOUT_CONTAINS("NOTE: Skipping GRANT/REVOKE statements for user 'mysql.sys'@'localhost'")

wipeout_server(session2)
session2.run_sql("SET global partial_revokes=0")
testutil.dbug_set("")

#@<> cleanup tests with include/exclude users
shell.connect(__sandbox_uri1)
session.run_sql("DROP USER 'first'@'localhost';")
session.run_sql("DROP USER 'first'@'10.11.12.13';")
session.run_sql("DROP USER 'firstfirst'@'localhost';")
session.run_sql("DROP USER 'second'@'localhost';")
session.run_sql("DROP USER 'second'@'10.11.12.14';")
session.run_sql("DROP USER 'mysql.sys-ex'@'localhost';")

for u in mhs_excluded_users:
    session.run_sql(f"DROP USER '{u}'@'localhost'")

#@<> Bug#33128803 default roles {VER(>= 8.0.11)}
shell.connect(__sandbox_uri1)
session.run_sql("CREATE ROLE IF NOT EXISTS 'aaabra';")
session.run_sql("CREATE ROLE IF NOT EXISTS aaabby;")
session.run_sql("CREATE ROLE IF NOT EXISTS 'local'@'localhost';")
session.run_sql("create user IF NOT EXISTS wolodia@localhost default role 'aaabra', local@localhost, aaabby;")
session.run_sql("create user IF NOT EXISTS zenon@localhost default role aaabby;")

default_roles_dir = os.path.join(outdir, "default_roles_dir")
util.dump_instance(default_roles_dir, { "users": True, "showProgress": False })

shell.connect(__sandbox_uri2)
wipeout_server(session2)

EXPECT_NO_THROWS(lambda: util.load_dump(default_roles_dir, {"loadUsers":True, "excludeUsers":["root@%"]}), "Load")

compare_servers(session1, session2)

shell.connect(__sandbox_uri1)
session.run_sql("DROP ROLE 'aaabra';")
session.run_sql("DROP ROLE aaabby;")
session.run_sql("DROP ROLE 'local'@'localhost';")
session.run_sql("DROP user wolodia@localhost;")
session.run_sql("DROP user zenon@localhost;")

#@<> BUG#31748786 {not __dbug_off}
# create a MDS-compatible dump
shell.connect(__sandbox_uri1)

dump_dir = os.path.join(outdir, "skip_binlog")
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "ocimds": True, "compatibility": ["ignore_missing_pks", "strip_restricted_grants", "strip_definers", "skip_invalid_accounts"], "ddlOnly": True, "showProgress": False }), "Dumping the instance should not fail")

# when loading into non-MDS instance, if skipBinlog is set, but user doesn't have required privileges, exception should be thrown
wipeout_server(session2)

session2.run_sql("CREATE USER IF NOT EXISTS admin@'%' IDENTIFIED BY 'pass'")
session2.run_sql("GRANT ALL ON *.* TO 'admin'@'%'")

sql_log_bin_privileges = [ 'SUPER', 'SYSTEM_VARIABLES_ADMIN', 'SESSION_VARIABLES_ADMIN' ]
all_privileges = [x[0].upper() for x in session2.run_sql('SHOW PRIVILEGES').fetch_all()]
sql_log_bin_privileges = [x for x in sql_log_bin_privileges if x in all_privileges]

session2.run_sql("REVOKE {0} ON *.* FROM 'admin'@'%'".format(", ".join(sql_log_bin_privileges)))

shell.connect("mysql://admin:pass@{0}:{1}".format(__host, __mysql_sandbox_port2))

# join all but the last privilege with ', ', append ' or ' if there is more than one privilege, append the last privilege
missing_privileges = ', '.join(sql_log_bin_privileges[:-1]) + (' or ' if len(sql_log_bin_privileges) > 1 else '') + sql_log_bin_privileges[-1]
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "skipBinlog": True, "showProgress": False, "resetProgress": True  }), "Error: Shell Error (53004): Util.load_dump: 'SET sql_log_bin=0' failed with error: MySQL Error 1227 (42000): Access denied; you need (at least one of) the {0} privilege(s) for this operation".format(missing_privileges))

# when loading into MDS instance, if skipBinlog is set, but user doesn't have required privileges, exception should be thrown
testutil.dbug_set("+d,dump_loader_force_mds")

EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "skipBinlog": True, "showProgress": False, "resetProgress": True  }), "Error: Shell Error (53004): Util.load_dump: 'SET sql_log_bin=0' failed with error: MySQL Error 1227 (42000): Access denied; you need (at least one of) the {0} privilege(s) for this operation".format(missing_privileges))

testutil.dbug_set("")

#@<> BUG#32140970 {not __dbug_off}
# create a MDS-compatible dump but without 'ocimds' option
shell.connect(__sandbox_uri1)

dump_dir = os.path.join(outdir, "ignore_version")
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "compatibility": ["strip_restricted_grants", "strip_definers"], "ddlOnly": True, "showProgress": False }), "Dumping the instance should not fail")

shell.connect(__sandbox_uri2)
wipeout_server(session2)

testutil.dbug_set("+d,dump_loader_force_mds")

# loading non-MDS dump into MDS should fail

EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "Error: Shell Error (53010): Util.load_dump: Dump is not compatible with MySQL HeatWave Service")
EXPECT_STDOUT_CONTAINS("ERROR: Destination is a MySQL HeatWave Service DB System instance but the dump was produced without the compatibility option. Please enable the 'ocimds' option when dumping your database. Alternatively, enable the 'ignoreVersion' option to load anyway.")

# loading non-MDS dump into MDS with the 'ignoreVersion' option enabled should succeed
wipeout_server(session2)
WIPE_OUTPUT()

EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "ignoreVersion": True, "showProgress": False }), "Loading with 'ignoreVersion' should succeed")
EXPECT_STDOUT_CONTAINS("WARNING: Destination is a MySQL HeatWave Service DB System instance but the dump was produced without the compatibility option. The 'ignoreVersion' option is enabled, so loading anyway. If this operation fails, create the dump once again with the 'ocimds' option enabled.")

testutil.dbug_set("")

#@<> WL14506: create tables and dumps with and without 'create_invisible_pks' compatibility option
dump_pks_dir = os.path.join(outdir, "invisible_pks")
dump_no_pks_dir = os.path.join(outdir, "no_invisible_pks")
dump_just_pk_dir = os.path.join(outdir, "just_pk")
schema_name = "wl14506"
pk_table_name = "pk"
no_pk_table_name = "no_pk"

shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [schema_name])
session.run_sql("CREATE TABLE !.! (id BIGINT AUTO_INCREMENT PRIMARY KEY, data BIGINT)", [ schema_name, pk_table_name ])

for i in range(20):
    session.run_sql("INSERT INTO !.! (data) VALUES " + ",".join(["("+ str(random.randint(0, 2 ** 63 - 1)) + ")" for _ in range(1000)]), [ schema_name, pk_table_name ])

# two statements to avoid 'Statement violates GTID consistency'
session.run_sql("CREATE TABLE !.! (data BIGINT)", [ schema_name, no_pk_table_name ])
session.run_sql("INSERT INTO !.! SELECT data FROM !.! GROUP BY data", [ schema_name, no_pk_table_name, schema_name, pk_table_name ])
# add unique index to make sure this table is chunked
session.run_sql("ALTER TABLE !.! ADD UNIQUE INDEX idx (data);", [ schema_name, no_pk_table_name ])

session.run_sql("ANALYZE TABLE !.!;", [ schema_name, pk_table_name ])
session.run_sql("ANALYZE TABLE !.!;", [ schema_name, no_pk_table_name ])

# small 'bytesPerChunk' value to force chunking
util.dump_schemas([schema_name], dump_pks_dir, { "ocimds": True, "compatibility": ["create_invisible_pks"], "bytesPerChunk" : "128k", "showProgress": False })
util.dump_schemas([schema_name], dump_no_pks_dir, { "ocimds": True, "compatibility": ["ignore_missing_pks"], "bytesPerChunk" : "128k", "showProgress": False })
# dump which has only table with a primary key
util.dump_tables(schema_name, [ pk_table_name ], dump_just_pk_dir, { "bytesPerChunk" : "128k", "showProgress": False })

# helper function
def EXPECT_PK(dump_dir, options, pk_created, expected_exception=None, wipeout=True):
    opts = { "showProgress": False, "resetProgress": True }
    opts.update(options)
    shell.connect(__sandbox_uri2)
    if wipeout:
        wipeout_server(session)
    WIPE_OUTPUT()
    if expected_exception:
        EXPECT_THROWS(lambda: util.load_dump(dump_dir, opts), expected_exception)
    else:
        EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, opts), "Loading the dump should not fail")
    pk = session.run_sql("SELECT COLUMN_NAME, COLUMN_TYPE, EXTRA FROM information_schema.columns WHERE COLUMN_KEY = 'PRI' AND TABLE_SCHEMA = ? AND TABLE_NAME = ?", [schema_name, no_pk_table_name]).fetch_all()
    if pk_created:
        # WL14506-FR4.6 - If the createInvisiblePKs option is set to true and the target instance supports WL#13784, the primary keys must be created automatically by setting the session server variable SQL_GENERATE_INVISIBLE_PRIMARY_KEY to ON.
        # WL14506-FR4.7 - If the createInvisiblePKs option is set to true and the target instance does not support WL#13784, the primary key must be created for each table which does not have one by modifying the CREATE TABLE statement and adding the my_row_id BIGINT UNSIGNED AUTO_INCREMENT INVISIBLE PRIMARY KEY column.
        # if server supports WL#13784 loader will use that feature, if something is wrong tests will fail
        # the format of the primary key is the same in case of both FR4.6 and FR4.7
        # WL14506-TSFR_4.7_1
        EXPECT_EQ(1, len(pk), "Primary key should have been created using a single column")
        EXPECT_EQ("my_row_id", pk[0][0], "Primary key should be named `my_row_id`")
        EXPECT_EQ("BIGINT UNSIGNED", pk[0][1].upper(), "Primary key should be a BIGINT UNSIGNED")
        EXPECT_NE(-1, pk[0][2].upper().find("AUTO_INCREMENT"), "Primary key should have the AUTO_INCREMENT attribute")
        EXPECT_NE(-1, pk[0][2].upper().find("INVISIBLE"), "Primary key should have the INVISIBLE attribute")
    else:
        EXPECT_EQ(0, len(pk), "Primary key should have NOT been created")

# WL14506-FR4 - The util.loadDump() function must support a new boolean option, createInvisiblePKs, which allows for automatic creation of invisible primary keys.
# WL14506-FR4.1 - The default value of this option must depend on the dump which is being loaded: if it was created using the create_invisible_pks value given to the compatibility option, it must be set to true. Otherwise it must be set to false.

#@<> dump created without 'create_invisible_pks', 'createInvisiblePKs' not given, primary key should not be created {VER(>= 8.0.24)}
EXPECT_PK(dump_no_pks_dir, {}, False)

#@<> dump created without 'create_invisible_pks', 'createInvisiblePKs' is false, primary key should not be created {VER(>= 8.0.24)}
# WL14506-TSFR_4_2
EXPECT_PK(dump_no_pks_dir, { "createInvisiblePKs": False }, False)

#@<> dump created without 'create_invisible_pks', 'createInvisiblePKs' is true, primary key should be created {VER(>= 8.0.24)}
# WL14506-TSFR_4_3
# WL14506-TSFR_4_8
EXPECT_PK(dump_no_pks_dir, { "createInvisiblePKs": True }, True)

#@<> dump created with 'create_invisible_pks', 'createInvisiblePKs' not given, primary key should be created {VER(>= 8.0.24)}
# WL14506-TSFR_4_1
EXPECT_PK(dump_pks_dir, {}, True)

#@<> dump created with 'create_invisible_pks', 'createInvisiblePKs' is false, primary key should not be created {VER(>= 8.0.24)}
# WL14506-TSFR_4_6
EXPECT_PK(dump_pks_dir, { "createInvisiblePKs": False }, False)

#@<> dump created with 'create_invisible_pks', 'createInvisiblePKs' is true, primary key should be created {VER(>= 8.0.24)}
# WL14506-TSFR_4_1
EXPECT_PK(dump_pks_dir, { "createInvisiblePKs": True }, True)

#@<> WL14506-FR4.2 - If the createInvisiblePKs option is set to true and the loadDdl option is set to false, a warning must be printed, the value of createInvisiblePks must be ignored and the load process must continue. {VER(>= 8.0.24)}
# WL14506-TSFR_4_4
# first load just DDL
EXPECT_PK(dump_pks_dir, { "createInvisiblePKs": False, "loadData": False }, False)
# then load data with 'createInvisiblePKs' set to true
EXPECT_PK(dump_pks_dir, { "loadDdl": False }, False, wipeout=False)
EXPECT_STDOUT_CONTAINS("WARNING: The 'createInvisiblePKs' option is set to true, but the 'loadDdl' option is false, Primary Keys are not going to be created.")

#@<> WL14506-TSFR_4_4 {VER(>= 8.0.24)}
# first load just DDL
EXPECT_PK(dump_pks_dir, { "createInvisiblePKs": False, "loadData": False }, False)
# then load data with 'createInvisiblePKs' set to true
EXPECT_PK(dump_pks_dir, { "createInvisiblePKs": True, "loadDdl": False }, False, wipeout=False)
EXPECT_STDOUT_CONTAINS("WARNING: The 'createInvisiblePKs' option is set to true, but the 'loadDdl' option is false, Primary Keys are not going to be created.")

#@<> WL14506-FR4.3 - If the createInvisiblePKs option is set to true and the target instance has version lower than 8.0.24, an error must be reported and the load process must be aborted. {VER(< 8.0.24)}
# 'createInvisiblePKs' is true implicitly
# WL14506-TSFR_4.3_1
EXPECT_PK(dump_pks_dir, {}, False, "The 'createInvisiblePKs' option requires server 8.0.24 or newer.")

# 'createInvisiblePKs' is true explicitly
EXPECT_PK(dump_no_pks_dir, { "createInvisiblePKs": True }, False, "The 'createInvisiblePKs' option requires server 8.0.24 or newer.")

#@<> WL14506-FR4.4 - If the createInvisiblePKs option is set to false and the dump which contains tables without primary keys is loaded into MDS, a warning must be reported stating that MDS HA cannot be used with this dump and the load process must continue. {VER(>= 8.0.24) and not __dbug_off}
# WL14506-TSFR_4_5
testutil.dbug_set("+d,dump_loader_force_mds")

EXPECT_PK(dump_no_pks_dir, { "createInvisiblePKs": False, "ignoreVersion": True }, False)
EXPECT_STDOUT_CONTAINS("WARNING: The dump contains tables without Primary Keys and it is loaded with the 'createInvisiblePKs' option set to false, this dump cannot be loaded into an MySQL HeatWave Service DB System instance with High Availability.")

# all tables have primary keys, no warning
EXPECT_PK(dump_just_pk_dir, { "createInvisiblePKs": False, "ignoreVersion": True }, False)
EXPECT_STDOUT_NOT_CONTAINS("createInvisiblePKs")

testutil.dbug_set("")

#@<> WL14506-FR4.5 - If the createInvisiblePKs option is set to true and the dump which contains tables without primary keys is loaded into MDS, a warning must be reported stating that Inbound Replication into an MDS HA instance cannot be used with this dump and the load process must continue. {VER(>= 8.0.24) and not __dbug_off}
# WL14506-TSFR_4_7
testutil.dbug_set("+d,dump_loader_force_mds")

EXPECT_PK(dump_no_pks_dir, { "createInvisiblePKs": True, "ignoreVersion": True }, True)
if __version_num < 80032:
    EXPECT_STDOUT_CONTAINS("WARNING: The dump contains tables without Primary Keys and it is loaded with the 'createInvisiblePKs' option set to true, Inbound Replication into an MySQL HeatWave Service DB System instance with High Availability cannot be used with this dump.")
else:
    EXPECT_STDOUT_CONTAINS("NOTE: The dump contains tables without Primary Keys and it is loaded with the 'createInvisiblePKs' option set to true, Inbound Replication into an MySQL HeatWave Service DB System instance with High Availability can be used with this dump.")

# all tables have primary keys, no warning
EXPECT_PK(dump_just_pk_dir, { "createInvisiblePKs": True, "ignoreVersion": True }, False)
EXPECT_STDOUT_NOT_CONTAINS("createInvisiblePKs")

testutil.dbug_set("")

#@<> if SQL_GENERATE_INVISIBLE_PRIMARY_KEY is globally ON and createInvisiblePKs is false, primary key should not be created {VER(>= 8.0.24)}
try:
    session.run_sql("SET @@GLOBAL.SQL_GENERATE_INVISIBLE_PRIMARY_KEY = ON")
    EXPECT_PK(dump_pks_dir, { "createInvisiblePKs": False }, False)
    session.run_sql("SET @@GLOBAL.SQL_GENERATE_INVISIBLE_PRIMARY_KEY = OFF")
except Exception as e:
    # server does not support SQL_GENERATE_INVISIBLE_PRIMARY_KEY
    EXPECT_EQ(1193, e.code)

#@<> WL14506: cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])

#@<> BUG#32734880 progress file is not removed when resetProgress is used
# create a dump
shell.connect(__sandbox_uri1)

dump_dir = os.path.join(outdir, "dump_all")
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "showProgress": False }), "Dumping the instance should not fail")

# wipe the destination server
wipeout_server(session2)

# load the full dump
shell.connect(__sandbox_uri2)
WIPE_OUTPUT()
WIPE_SHELL_LOG()
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "Loading the dump should not fail")
EXPECT_SHELL_LOG_CONTAINS(".tsv.zst: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")

# wipe the destination server again
wipeout_server(session2)

# reset the progress, load the dump again, this time just the DDL
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadData": False, "resetProgress": True, "showProgress": False }), "Loading the dump should not fail")
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected for the instance but 'resetProgress' option was enabled. Load progress will be discarded and the whole dump will be reloaded.")

# load the data without resetting the progress
WIPE_OUTPUT()
WIPE_SHELL_LOG()
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadDdl": False, "showProgress": False }), "Loading the dump should not fail")
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
# ensure data was loaded
EXPECT_SHELL_LOG_CONTAINS(".tsv.zst: Records: 4079  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> WL14632: create tables with partitions
dump_dir = os.path.join(outdir, "part")
metadata_file = os.path.join(dump_dir, "@.json")
schema_name = "wl14632"
no_partitions_table_name = "no_partitions"
partitions_table_name = "partitions"
subpartitions_table_name = "subpartitions"
all_tables = [ no_partitions_table_name, partitions_table_name, subpartitions_table_name ]
subpartition_prefix = "@o" if __os_type == "windows" else "@ó"

shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [schema_name])

session.run_sql("CREATE TABLE !.! (`id` int NOT NULL AUTO_INCREMENT PRIMARY KEY, `data` blob)", [ schema_name, no_partitions_table_name ])

session.run_sql("""CREATE TABLE !.!
(`id` int NOT NULL AUTO_INCREMENT PRIMARY KEY, `data` blob)
PARTITION BY RANGE (`id`)
(PARTITION x0 VALUES LESS THAN (10000),
 PARTITION x1 VALUES LESS THAN (20000),
 PARTITION x2 VALUES LESS THAN (30000),
 PARTITION x3 VALUES LESS THAN MAXVALUE)""", [ schema_name, partitions_table_name ])

session.run_sql(f"""CREATE TABLE !.!
(`id` int, `data` blob)
PARTITION BY RANGE (`id`)
SUBPARTITION BY KEY (id)
SUBPARTITIONS 2
(PARTITION `{subpartition_prefix}0` VALUES LESS THAN (10000),
 PARTITION `{subpartition_prefix}1` VALUES LESS THAN (20000),
 PARTITION `{subpartition_prefix}2` VALUES LESS THAN (30000),
 PARTITION `{subpartition_prefix}3` VALUES LESS THAN MAXVALUE)""", [ schema_name, subpartitions_table_name ])

for x in range(4):
    session.run_sql(f"""INSERT INTO !.! (`data`) VALUES {",".join([f"('{random_string(100,200)}')" for i in range(10000)])}""", [ schema_name, partitions_table_name ])
session.run_sql("INSERT INTO !.! SELECT * FROM !.!", [ schema_name, subpartitions_table_name, schema_name, partitions_table_name ])
session.run_sql("INSERT INTO !.! SELECT * FROM !.!", [ schema_name, no_partitions_table_name, schema_name, partitions_table_name ])

for table in all_tables:
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, table ])

partition_names = {}
for table in all_tables:
    partition_names[table] = []
    for row in session.run_sql("SELECT IFNULL(SUBPARTITION_NAME, PARTITION_NAME) FROM information_schema.partitions WHERE TABLE_SCHEMA = ? AND TABLE_NAME = ? AND PARTITION_NAME IS NOT NULL;", [ schema_name, table ]).fetch_all():
        partition_names[table].append(row[0])

# helper functions
def compute_checksum(schema, table):
    return session.run_sql("CHECKSUM TABLE !.!", [ schema, table ]).fetch_one()[1]

def EXPECT_DUMP_AND_LOAD_PARTITIONED(dump, tables = all_tables):
    wipe_dir(dump_dir)
    shell.connect(__sandbox_uri1)
    WIPE_OUTPUT()
    WIPE_SHELL_LOG()
    EXPECT_NO_THROWS(dump, "Dumping the data should not fail")
    files = os.listdir(dump_dir)
    for table in tables:
        # there should be a metadata file for the table
        EXPECT_TRUE(f"{schema_name}@{table}.json" in files)
        # there should be an SQL file for the table
        EXPECT_TRUE(f"{schema_name}@{table}.sql" in files)
        for partition in partition_names[table]:
            # there should be at least one partition-based data file
            EXPECT_TRUE([f for f in files if f.startswith(encode_partition_basename(schema_name, table, partition)) and f.endswith(".tsv.zst")])
            # dumper should mention the partition as a separate entity
            EXPECT_SHELL_LOG_CONTAINS(f"Data dump for table `{schema_name}`.`{table}` partition `{partition}` will be written to ")
    shell.connect(__sandbox_uri2)
    wipeout_server(session)
    WIPE_OUTPUT()
    WIPE_SHELL_LOG()
    # WL14632-TSFR_2_1
    EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "Loading the dump should not fail")
    for table in tables:
        for partition in partition_names[table]:
            # loader should mention the partition as a separate entity
            EXPECT_SHELL_LOG_MATCHES(re.compile(f"\\[Worker00\\d\\]: {encode_partition_basename(schema_name, table, partition)}.*tsv.zst: Records: \\d+  Deleted: 0  Skipped: 0  Warnings: 0"))
    # verify consistency
    for table in tables:
        EXPECT_EQ(checksums[table], compute_checksum(schema_name, table))

checksums = {}
for table in all_tables:
    checksums[table] = compute_checksum(schema_name, table)

#@<> WL14632-TSFR_1_1
EXPECT_DUMP_AND_LOAD_PARTITIONED(lambda: util.dump_instance(dump_dir, { "showProgress": False }))
EXPECT_DUMP_AND_LOAD_PARTITIONED(lambda: util.dump_instance(dump_dir, { "bytesPerChunk": "128k", "showProgress": False }))

#@<> WL14632-TSFR_1_2
EXPECT_DUMP_AND_LOAD_PARTITIONED(lambda: util.dump_schemas([ schema_name ], dump_dir, { "showProgress": False }))
EXPECT_DUMP_AND_LOAD_PARTITIONED(lambda: util.dump_schemas([ schema_name ], dump_dir, { "bytesPerChunk": "128k", "showProgress": False }))

#@<> WL14632-TSFR_1_3
# test table with partitions
EXPECT_DUMP_AND_LOAD_PARTITIONED(lambda: util.dump_tables(schema_name, [ partitions_table_name ], dump_dir, { "showProgress": False }), [ partitions_table_name ])
EXPECT_DUMP_AND_LOAD_PARTITIONED(lambda: util.dump_tables(schema_name, [ partitions_table_name ], dump_dir, { "bytesPerChunk": "128k", "showProgress": False }), [ partitions_table_name ])

# BUG#33063035 check if the metadata file has the partition awareness capability
EXPECT_CAPABILITIES(metadata_file, [ partition_awareness_capability ])

# test table with sub-partitions
EXPECT_DUMP_AND_LOAD_PARTITIONED(lambda: util.dump_tables(schema_name, [ subpartitions_table_name ], dump_dir, { "showProgress": False }), [ subpartitions_table_name ])
EXPECT_DUMP_AND_LOAD_PARTITIONED(lambda: util.dump_tables(schema_name, [ subpartitions_table_name ], dump_dir, { "bytesPerChunk": "128k", "showProgress": False }), [ subpartitions_table_name ])

# BUG#33063035 check if the metadata file has the partition awareness capability
EXPECT_CAPABILITIES(metadata_file, [ partition_awareness_capability ])

#@<> BUG#33063035 dump table without partitions, metadata should not have the partition awareness capability
EXPECT_DUMP_AND_LOAD_PARTITIONED(lambda: util.dump_tables(schema_name, [ no_partitions_table_name ], dump_dir, { "showProgress": False }), [ ])
EXPECT_NO_CAPABILITIES(metadata_file, [ partition_awareness_capability ])

#@<> BUG#33063035 reuse previous dump, hack metadata with some fake capability
metadata = read_json(metadata_file)
metadata["capabilities"].append({
    "id": "make_toast",
    "description": "Makes toasts, yummy.",
    "versionRequired": "8.0.28",
})
write_json(metadata_file, metadata)

EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "Unsupported dump capabilities")
EXPECT_STDOUT_CONTAINS("""
ERROR: Dump is using capabilities which are not supported by this version of MySQL Shell:

* Makes toasts, yummy.

The minimum required version of MySQL Shell to load this dump is: 8.0.28.
""")

#@<> BUG#33063035 reuse previous dump, hack metadata with another fake capability, check if the correct version is suggested
metadata = read_json(metadata_file)
metadata["capabilities"].append({
    "id": "make_another_toast",
    "description": "Makes more toasts, great!",
    "versionRequired": "8.0.29",
})
write_json(metadata_file, metadata)

EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "Unsupported dump capabilities")
EXPECT_STDOUT_CONTAINS("""
ERROR: Dump is using capabilities which are not supported by this version of MySQL Shell:

* Makes toasts, yummy.

* Makes more toasts, great!

The minimum required version of MySQL Shell to load this dump is: 8.0.29.
""")

#@<> WL14632-TSFR_3_1
exported_file = os.path.join(outdir, "part.tsv")

for table in all_tables:
    try:
        os.remove(exported_file)
    except:
        pass
    shell.connect(__sandbox_uri1)
    WIPE_OUTPUT()
    WIPE_SHELL_LOG()
    EXPECT_NO_THROWS(lambda: util.export_table(f"{schema_name}.{table}", exported_file, { "showProgress": False }), "Exporting the table should not fail")
    # dumper should not mention the partitions
    EXPECT_SHELL_LOG_CONTAINS(f"Data dump for table `{schema_name}`.`{table}` will be written to ")
    create_statement = session.run_sql("SHOW CREATE TABLE !.!", [ schema_name, table ]).fetch_one()[1]
    shell.connect(__sandbox_uri2)
    wipeout_server(session)
    session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [schema_name])
    session.run_sql("USE !", [schema_name])
    session.run_sql(create_statement)
    WIPE_OUTPUT()
    util.import_table(exported_file, { "schema": schema_name, "table": table, "columns": [ "id", "data" ], "decodeColumns": { "data": "FROM_BASE64" } })
    EXPECT_EQ(checksums[table], compute_checksum(schema_name, table))

#@<> WL14632: cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])

#@<> BUG#33144419: setup
dumper_user = "dumper"
loader_user = "loader"
password = "pass"
host_with_netmask = "127.0.0.0/255.0.0.0"
dump_dir = os.path.join(outdir, "host_with_netmask")

def create_account_with_netmask(name):
    session.run_sql(f"DROP USER IF EXISTS '{name}'@'{host_with_netmask}'")
    session.run_sql(f"CREATE USER '{name}'@'{host_with_netmask}' IDENTIFIED BY '{password}'")
    session.run_sql(f"GRANT ALL ON *.* TO '{name}'@'{host_with_netmask}' WITH GRANT OPTION")

# source server has two accounts with IPv4 and a netmask, one of them is used to do the dump
shell.connect(__sandbox_uri1)
create_account_with_netmask(dumper_user)
create_account_with_netmask(loader_user)

# wipe the destination server
wipeout_server(session2)

# destination server has one of the accounts created above, it is going to load the dump; the other account is going to be recreated
shell.connect(__sandbox_uri2)
create_account_with_netmask(loader_user)

#@<> BUG#33144419: dump
shell.connect(f"{dumper_user}:{password}@127.0.0.1:{__mysql_sandbox_port1}")
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "users": True, "ddlOnly": True, "showProgress": False }), "dump should succeed")

#@<> BUG#33144419: load
# dumper user does not exist
EXPECT_THROWS(lambda: shell.connect(f"{dumper_user}:{password}@127.0.0.1:{__mysql_sandbox_port2}"), f"Access denied for user '{dumper_user}'@'localhost'")

# load the dump, users are created
shell.connect(f"{loader_user}:{password}@127.0.0.1:{__mysql_sandbox_port2}")
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "excludeUsers": [ "root@%", "root@localhost" ], "showProgress": False }), "load should succeed")
EXPECT_STDOUT_CONTAINS(f"NOTE: Skipping CREATE/ALTER USER statements for user '{loader_user}'@'{host_with_netmask}'")
EXPECT_STDOUT_CONTAINS(f"NOTE: Skipping GRANT/REVOKE statements for user '{loader_user}'@'{host_with_netmask}'")

# dumper user was created
EXPECT_NO_THROWS(lambda: shell.connect(f"{dumper_user}:{password}@127.0.0.1:{__mysql_sandbox_port2}"), "user should exist")

#@<> BUG#33144419: cleanup
shell.connect(__sandbox_uri1)
session.run_sql(f"DROP USER IF EXISTS '{dumper_user}'@'{host_with_netmask}'")
session.run_sql(f"DROP USER IF EXISTS '{loader_user}'@'{host_with_netmask}'")

#@<> BUG#32561035: setup
# reuse dump dir from one of the tests above
dump_dir = os.path.join(outdir, "host_with_netmask")

shell.connect(__sandbox_uri2)
saved_uuid = session.run_sql("SELECT @@server_uuid").fetch_one()[0]

shell.connect(__sandbox_uri1)
new_uuid = session.run_sql("SELECT @@server_uuid").fetch_one()[0]

#@<> BUG#32561035: test
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "progressFile": os.path.join(dump_dir, f"load-progress.{saved_uuid}.json"),"showProgress": False }), f"Progress file was created for a server with UUID {saved_uuid}, while the target server has UUID: {new_uuid}")
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")

#@<> WL14244 - help entries
util.help('load_dump')

# WL14244-TSFR_3_3
EXPECT_STDOUT_CONTAINS("""
      - includeRoutines: array of strings (default not set) - Loads only the
        specified routines from the dump. Strings are in format schema.routine,
        quoted using backtick characters when required. By default, all
        routines are included.
""")

# WL14244-TSFR_4_3
EXPECT_STDOUT_CONTAINS("""
      - excludeRoutines: array of strings (default not set) - Skip loading
        specified routines from the dump. Strings are in format schema.routine,
        quoted using backtick characters when required.
""")

# WL14244-TSFR_5_3
EXPECT_STDOUT_CONTAINS("""
      - includeEvents: array of strings (default not set) - Loads only the
        specified events from the dump. Strings are in format schema.event,
        quoted using backtick characters when required. By default, all events
        are included.
""")

# WL14244-TSFR_6_3
EXPECT_STDOUT_CONTAINS("""
      - excludeEvents: array of strings (default not set) - Skip loading
        specified events from the dump. Strings are in format schema.event,
        quoted using backtick characters when required.
""")

# WL14244-TSFR_7_4
EXPECT_STDOUT_CONTAINS("""
      - includeTriggers: array of strings (default not set) - Loads only the
        specified triggers from the dump. Strings are in format schema.table
        (all triggers from the specified table) or schema.table.trigger (the
        individual trigger), quoted using backtick characters when required. By
        default, all triggers are included.
""")

# WL14244-TSFR_8_4
EXPECT_STDOUT_CONTAINS("""
      - excludeTriggers: array of strings (default not set) - Skip loading
        specified triggers from the dump. Strings are in format schema.table
        (all triggers from the specified table) or schema.table.trigger (the
        individual trigger), quoted using backtick characters when required.
""")

#@<> WL14244 - helpers
shell.connect(__sandbox_uri2)
dump_dir = os.path.join(outdir, "wl14244")

def dump_and_load(options):
    WIPE_STDOUT()
    # remove everything from the server
    wipeout_server(session)
    # create sample DB structure
    session.run_sql("CREATE SCHEMA existing_schema_1")
    session.run_sql("CREATE TABLE existing_schema_1.existing_table (id INT)")
    session.run_sql("CREATE VIEW existing_schema_1.existing_view AS SELECT * FROM existing_schema_1.existing_table")
    session.run_sql("CREATE EVENT existing_schema_1.existing_event ON SCHEDULE EVERY 1 YEAR DO BEGIN END")
    session.run_sql("CREATE FUNCTION existing_schema_1.existing_routine() RETURNS INT DETERMINISTIC RETURN 1")
    session.run_sql("CREATE TRIGGER existing_schema_1.existing_trigger AFTER DELETE ON existing_schema_1.existing_table FOR EACH ROW BEGIN END")
    session.run_sql("CREATE SCHEMA existing_schema_2")
    session.run_sql("CREATE TABLE existing_schema_2.existing_table (id INT)")
    session.run_sql("CREATE VIEW existing_schema_2.existing_view AS SELECT * FROM existing_schema_2.existing_table")
    session.run_sql("CREATE EVENT existing_schema_2.existing_event ON SCHEDULE EVERY 1 YEAR DO BEGIN END")
    session.run_sql("CREATE PROCEDURE existing_schema_2.existing_routine() DETERMINISTIC BEGIN END")
    session.run_sql("CREATE TRIGGER existing_schema_2.existing_trigger AFTER DELETE ON existing_schema_2.existing_table FOR EACH ROW BEGIN END")
    # do the dump
    shutil.rmtree(dump_dir, True)
    util.dump_instance(dump_dir, { "showProgress": False })
    # remove everything from the server once again, load the dump
    wipeout_server(session)
    # we're only interested in DDL, progress is not important
    options["loadData"] = False
    options["showProgress"] = False
    util.load_dump(dump_dir, options)
    # fetch data about the current DB structure
    return snapshot_instance(session)

def entries(snapshot, keys = []):
    entry = snapshot["schemas"]
    for key in keys:
        entry = entry[key]
    return sorted(list(entry.keys()))

#@<> WL14244 - includeRoutines - invalid values
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeRoutines": [ "routine" ] }), "ValueError: Util.load_dump: Argument #2: The routine to be included must be in the following form: schema.routine, with optional backtick quotes, wrong value: 'routine'.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeRoutines": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Failed to parse routine to be included 'schema.@': Invalid character in identifier")

#@<> WL14244-TSFR_3_6
snapshot = dump_and_load({})
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_2", "procedures"]))

snapshot = dump_and_load({ "includeRoutines": [] })
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_2", "procedures"]))

#@<> WL14244-TSFR_3_9
snapshot = dump_and_load({ "includeRoutines": ['existing_schema_1.existing_routine', 'existing_schema_1.non_existing_routine', 'non_existing_schema.routine'] })
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "procedures"]))

#@<> WL14244 - excludeRoutines - invalid values
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeRoutines": [ "routine" ] }), "ValueError: Util.load_dump: Argument #2: The routine to be excluded must be in the following form: schema.routine, with optional backtick quotes, wrong value: 'routine'.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeRoutines": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Failed to parse routine to be excluded 'schema.@': Invalid character in identifier")

#@<> WL14244-TSFR_4_6
snapshot = dump_and_load({})
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_2", "procedures"]))

snapshot = dump_and_load({ "excludeRoutines": [] })
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_2", "procedures"]))

#@<> WL14244-TSFR_4_9
snapshot = dump_and_load({ "excludeRoutines": ['existing_schema_1.existing_routine', 'existing_schema_1.non_existing_routine', 'non_existing_schema.routine'] })
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "functions"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "procedures"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "functions"]))
EXPECT_EQ(["existing_routine"], entries(snapshot, ["existing_schema_2", "procedures"]))

#@<> WL14244 - includeEvents - invalid values
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeEvents": [ "event" ] }), "ValueError: Util.load_dump: Argument #2: The event to be included must be in the following form: schema.event, with optional backtick quotes, wrong value: 'event'.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeEvents": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Failed to parse event to be included 'schema.@': Invalid character in identifier")

#@<> WL14244-TSFR_5_6
snapshot = dump_and_load({})
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_2", "events"]))

snapshot = dump_and_load({ "includeEvents": [] })
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_2", "events"]))

#@<> WL14244-TSFR_5_9
snapshot = dump_and_load({ "includeEvents": ['existing_schema_1.existing_event', 'existing_schema_1.non_existing_event', 'non_existing_schema.event'] })
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "events"]))

#@<> WL14244 - excludeEvents - invalid values
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeEvents": [ "event" ] }), "ValueError: Util.load_dump: Argument #2: The event to be excluded must be in the following form: schema.event, with optional backtick quotes, wrong value: 'event'.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeEvents": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Failed to parse event to be excluded 'schema.@': Invalid character in identifier")

#@<> WL14244-TSFR_6_6
snapshot = dump_and_load({})
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_2", "events"]))

snapshot = dump_and_load({ "excludeEvents": [] })
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_2", "events"]))

#@<> WL14244-TSFR_6_9
snapshot = dump_and_load({ "excludeEvents": ['existing_schema_1.existing_event', 'existing_schema_1.non_existing_event', 'non_existing_schema.event'] })
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "events"]))
EXPECT_EQ(["existing_event"], entries(snapshot, ["existing_schema_2", "events"]))

#@<> WL14244 - includeTriggers - invalid values
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeTriggers": [ "trigger" ] }), "ValueError: Util.load_dump: Argument #2: The trigger to be included must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes, wrong value: 'trigger'.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeTriggers": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Failed to parse trigger to be included 'schema.@': Invalid character in identifier")

#@<> WL14244-TSFR_7_8
snapshot = dump_and_load({})
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

snapshot = dump_and_load({ "includeTriggers": [] })
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

#@<> WL14244-TSFR_7_12
snapshot = dump_and_load({ "includeTriggers": ['existing_schema_1.existing_table', 'existing_schema_1.non_existing_table', 'non_existing_schema.table', 'existing_schema_2.existing_table.existing_trigger', 'existing_schema_1.existing_table.non_existing_trigger'] })
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

#@<> WL14244 - excludeTriggers - invalid values
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeTriggers": [ "trigger" ] }), "ValueError: Util.load_dump: Argument #2: The trigger to be excluded must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes, wrong value: 'trigger'.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeTriggers": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Failed to parse trigger to be excluded 'schema.@': Invalid character in identifier")

#@<> WL14244-TSFR_8_8
snapshot = dump_and_load({})
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

snapshot = dump_and_load({ "excludeTriggers": [] })
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ(["existing_trigger"], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

#@<> WL14244-TSFR_8_12
snapshot = dump_and_load({ "excludeTriggers": ['existing_schema_1.existing_table', 'existing_schema_1.non_existing_table', 'non_existing_schema.table', 'existing_schema_2.existing_table.existing_trigger', 'existing_schema_1.existing_table.non_existing_trigger'] })
EXPECT_EQ([], entries(snapshot, ["existing_schema_1", "tables", "existing_table", "triggers"]))
EXPECT_EQ([], entries(snapshot, ["existing_schema_2", "tables", "existing_table", "triggers"]))

#@<> BUG#35102738 - additional fixes: existing duplicate triggers were not reported, excluded objects were reported as duplicates
# prepare the instance
dump_and_load({})

# another load throws due to existing objects
WIPE_STDOUT()
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "resetProgress": True, "loadData": False, "showProgress": False }), "Duplicate objects found in destination database")
EXPECT_STDOUT_CONTAINS("""
ERROR: Schema `existing_schema_1` already contains a table named `existing_table`
ERROR: Schema `existing_schema_1` already contains a view named `existing_view`
ERROR: Schema `existing_schema_1` already contains a trigger named `existing_trigger`
ERROR: Schema `existing_schema_1` already contains a function named `existing_routine`
ERROR: Schema `existing_schema_1` already contains an event named `existing_event`
""")
EXPECT_STDOUT_CONTAINS("""
ERROR: Schema `existing_schema_2` already contains a table named `existing_table`
ERROR: Schema `existing_schema_2` already contains a view named `existing_view`
ERROR: Schema `existing_schema_2` already contains a trigger named `existing_trigger`
ERROR: Schema `existing_schema_2` already contains a procedure named `existing_routine`
ERROR: Schema `existing_schema_2` already contains an event named `existing_event`
""")

# excluded trigger is not reported as a duplicate
WIPE_STDOUT()
EXPECT_THROWS(lambda: util.load_dump(dump_dir, {
    "excludeTables": [ "existing_schema_1.existing_table", "existing_schema_1.existing_view", "existing_schema_2.existing_view" ],
    "excludeTriggers": [ "existing_schema_2.existing_table.existing_trigger" ],
    "excludeRoutines": [ "existing_schema_1.existing_routine", "existing_schema_2.existing_routine" ],
    "excludeEvents": [ "existing_schema_1.existing_event", "existing_schema_2.existing_event" ],
    "resetProgress": True,
    "loadData": False,
    "showProgress": False }), "Duplicate objects found in destination database")
EXPECT_STDOUT_CONTAINS("ERROR: Schema `existing_schema_2` already contains a table named `existing_table`")
EXPECT_STDOUT_NOT_CONTAINS("existing_trigger")

# exclude all existing objects to load the dump (triggers are excluded implicitly because tables are excluded)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, {
    "excludeTables": [ "existing_schema_1.existing_table", "existing_schema_1.existing_view", "existing_schema_2.existing_table", "existing_schema_2.existing_view" ],
    "excludeRoutines": [ "existing_schema_1.existing_routine", "existing_schema_2.existing_routine" ],
    "excludeEvents": [ "existing_schema_1.existing_event", "existing_schema_2.existing_event" ],
    "resetProgress": True,
    "loadData": False,
    "showProgress": False }), "load works")

#@<> includeX + excludeX conflicts - helpers
def load_with_conflicts(options, throws = True):
    WIPE_STDOUT()
    # reuse dump from the previous test
    # we're only interested in errors
    options["dryRun"] = True
    options["showProgress"] = False
    if throws:
        EXPECT_THROWS(lambda: util.load_dump(dump_dir, options), "Util.load_dump: Argument #2: Conflicting filtering options")
    else:
        # there could be some other exceptions, we ignore them
        try:
            util.load_dump(dump_dir, options)
        except Exception as e:
            print("Exception:", e)

#@<> includeSchemas + excludeSchemas conflicts
# no conflicts
load_with_conflicts({ "includeSchemas": [], "excludeSchemas": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeSchemas")
EXPECT_STDOUT_NOT_CONTAINS("excludeSchemas")

load_with_conflicts({ "includeSchemas": [ "a" ], "excludeSchemas": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeSchemas")
EXPECT_STDOUT_NOT_CONTAINS("excludeSchemas")

load_with_conflicts({ "includeSchemas": [], "excludeSchemas": [ "a" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeSchemas")
EXPECT_STDOUT_NOT_CONTAINS("excludeSchemas")

load_with_conflicts({ "includeSchemas": [ "a" ], "excludeSchemas": [ "b" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeSchemas")
EXPECT_STDOUT_NOT_CONTAINS("excludeSchemas")

# conflicts
load_with_conflicts({ "includeSchemas": [ "a" ], "excludeSchemas": [ "a" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeSchemas and excludeSchemas options contain a schema `a`.")

load_with_conflicts({ "includeSchemas": [ "a", "b" ], "excludeSchemas": [ "a" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeSchemas and excludeSchemas options contain a schema `a`.")

load_with_conflicts({ "includeSchemas": [ "a" ], "excludeSchemas": [ "a", "b" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeSchemas and excludeSchemas options contain a schema `a`.")

#@<> includeTables + excludeTables conflicts
# no conflicts
load_with_conflicts({ "includeTables": [], "excludeTables": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "includeTables": [], "excludeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [ "b.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [ "a.t1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "excludeSchemas": [ "b" ], "includeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "excludeSchemas": [], "includeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "includeSchemas": [], "includeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "includeSchemas": [ "a" ], "includeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "excludeSchemas": [ "a" ], "excludeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "excludeSchemas": [ "b" ], "excludeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "excludeSchemas": [], "excludeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "includeSchemas": [], "excludeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

load_with_conflicts({ "includeSchemas": [ "a" ], "excludeTables": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTables")
EXPECT_STDOUT_NOT_CONTAINS("excludeTables")

# conflicts
load_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTables and excludeTables options contain a table `a`.`t`.")

load_with_conflicts({ "includeTables": [ "a.t", "b.t" ], "excludeTables": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTables and excludeTables options contain a table `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`t`")

load_with_conflicts({ "includeTables": [ "a.t", "a.t1" ], "excludeTables": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTables and excludeTables options contain a table `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

load_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [ "a.t", "b.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTables and excludeTables options contain a table `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`t`")

load_with_conflicts({ "includeTables": [ "a.t" ], "excludeTables": [ "a.t", "a.t1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTables and excludeTables options contain a table `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

load_with_conflicts({ "excludeSchemas": [ "a" ], "includeTables": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTables option contains a table `a`.`t` which refers to an excluded schema.")

load_with_conflicts({ "includeSchemas": [ "b" ], "includeTables": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTables option contains a table `a`.`t` which refers to a schema which was not included in the dump.")

load_with_conflicts({ "includeSchemas": [ "b" ], "excludeTables": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTables option contains a table `a`.`t` which refers to a schema which was not included in the dump.")

#@<> includeEvents + excludeEvents conflicts
# no conflicts
load_with_conflicts({ "includeEvents": [], "excludeEvents": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "includeEvents": [], "excludeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [ "b.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [ "a.e1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "excludeSchemas": [ "b" ], "includeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "excludeSchemas": [], "includeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "includeSchemas": [], "includeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "includeSchemas": [ "a" ], "includeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "excludeSchemas": [ "a" ], "excludeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "excludeSchemas": [ "b" ], "excludeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "excludeSchemas": [], "excludeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "includeSchemas": [], "excludeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

load_with_conflicts({ "includeSchemas": [ "a" ], "excludeEvents": [ "a.e" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeEvents")
EXPECT_STDOUT_NOT_CONTAINS("excludeEvents")

# conflicts
load_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [ "a.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeEvents and excludeEvents options contain an event `a`.`e`.")

load_with_conflicts({ "includeEvents": [ "a.e", "b.e" ], "excludeEvents": [ "a.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeEvents and excludeEvents options contain an event `a`.`e`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`e`")

load_with_conflicts({ "includeEvents": [ "a.e", "a.e1" ], "excludeEvents": [ "a.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeEvents and excludeEvents options contain an event `a`.`e`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`e1`")

load_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [ "a.e", "b.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeEvents and excludeEvents options contain an event `a`.`e`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`e`")

load_with_conflicts({ "includeEvents": [ "a.e" ], "excludeEvents": [ "a.e", "a.e1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeEvents and excludeEvents options contain an event `a`.`e`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`e1`")

load_with_conflicts({ "excludeSchemas": [ "a" ], "includeEvents": [ "a.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeEvents option contains an event `a`.`e` which refers to an excluded schema.")

load_with_conflicts({ "includeSchemas": [ "b" ], "includeEvents": [ "a.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeEvents option contains an event `a`.`e` which refers to a schema which was not included in the dump.")

load_with_conflicts({ "includeSchemas": [ "b" ], "excludeEvents": [ "a.e" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeEvents option contains an event `a`.`e` which refers to a schema which was not included in the dump.")

#@<> includeRoutines + excludeRoutines conflicts
# no conflicts
load_with_conflicts({ "includeRoutines": [], "excludeRoutines": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "includeRoutines": [], "excludeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [ "b.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [ "a.r1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "excludeSchemas": [ "b" ], "includeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "excludeSchemas": [], "includeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "includeSchemas": [], "includeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "includeSchemas": [ "a" ], "includeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "excludeSchemas": [ "a" ], "excludeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "excludeSchemas": [ "b" ], "excludeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "excludeSchemas": [], "excludeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "includeSchemas": [], "excludeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

load_with_conflicts({ "includeSchemas": [ "a" ], "excludeRoutines": [ "a.r" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeRoutines")
EXPECT_STDOUT_NOT_CONTAINS("excludeRoutines")

# conflicts
load_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [ "a.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeRoutines and excludeRoutines options contain a routine `a`.`r`.")

load_with_conflicts({ "includeRoutines": [ "a.r", "b.r" ], "excludeRoutines": [ "a.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeRoutines and excludeRoutines options contain a routine `a`.`r`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`r`")

load_with_conflicts({ "includeRoutines": [ "a.r", "a.r1" ], "excludeRoutines": [ "a.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeRoutines and excludeRoutines options contain a routine `a`.`r`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`r1`")

load_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [ "a.r", "b.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeRoutines and excludeRoutines options contain a routine `a`.`r`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`r`")

load_with_conflicts({ "includeRoutines": [ "a.r" ], "excludeRoutines": [ "a.r", "a.r1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeRoutines and excludeRoutines options contain a routine `a`.`r`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`r1`")

load_with_conflicts({ "excludeSchemas": [ "a" ], "includeRoutines": [ "a.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeRoutines option contains a routine `a`.`r` which refers to an excluded schema.")

load_with_conflicts({ "includeSchemas": [ "b" ], "includeRoutines": [ "a.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeRoutines option contains a routine `a`.`r` which refers to a schema which was not included in the dump.")

load_with_conflicts({ "includeSchemas": [ "b" ], "excludeRoutines": [ "a.r" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeRoutines option contains a routine `a`.`r` which refers to a schema which was not included in the dump.")

#@<> includeTriggers + excludeTriggers conflicts
# no conflicts
load_with_conflicts({ "includeTriggers": [], "excludeTriggers": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [], "excludeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [], "excludeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "b.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "b.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "b.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "b.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t1.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t1.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "excludeSchemas": [ "b" ], "includeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "excludeSchemas": [], "includeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeSchemas": [], "includeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeSchemas": [ "a" ], "includeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "excludeSchemas": [ "a" ], "excludeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "excludeSchemas": [ "b" ], "excludeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "excludeSchemas": [], "excludeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeSchemas": [], "excludeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeSchemas": [ "a" ], "excludeTriggers": [ "a.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "excludeTables": [ "b.t" ], "includeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "excludeTables": [], "includeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTables": [], "includeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTables": [ "a.t" ], "includeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "excludeTables": [ "a.t" ], "excludeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "excludeTables": [ "b.t" ], "excludeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "excludeTables": [], "excludeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTables": [], "excludeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

load_with_conflicts({ "includeTables": [ "a.t" ], "excludeTriggers": [ "a.t.t" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeTriggers")
EXPECT_STDOUT_NOT_CONTAINS("excludeTriggers")

# conflicts
load_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")

load_with_conflicts({ "includeTriggers": [ "a.t", "b.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`t`")

load_with_conflicts({ "includeTriggers": [ "a.t", "a.t1" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

load_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t", "b.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`t`")

load_with_conflicts({ "includeTriggers": [ "a.t" ], "excludeTriggers": [ "a.t", "a.t1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a filter `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

load_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")

load_with_conflicts({ "includeTriggers": [ "a.t.t", "b.t.t" ], "excludeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`t`")

load_with_conflicts({ "includeTriggers": [ "a.t.t", "a.t1.t" ], "excludeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

load_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t.t", "b.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`t`")

load_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t.t", "a.t1.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeTriggers and excludeTriggers options contain a trigger `a`.`t`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

load_with_conflicts({ "includeTriggers": [ "a.t.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which is excluded by the value of the excludeTriggers option: `a`.`t`.")

load_with_conflicts({ "includeTriggers": [ "a.t.t", "b.t.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which is excluded by the value of the excludeTriggers option: `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`b`.`t`")

load_with_conflicts({ "includeTriggers": [ "a.t.t", "a.t1.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which is excluded by the value of the excludeTriggers option: `a`.`t`.")
EXPECT_STDOUT_NOT_CONTAINS("`a`.`t1`")

load_with_conflicts({ "excludeSchemas": [ "a" ], "includeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a filter `a`.`t` which refers to an excluded schema.")

load_with_conflicts({ "excludeSchemas": [ "a" ], "includeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which refers to an excluded schema.")

load_with_conflicts({ "includeSchemas": [ "b" ], "includeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a filter `a`.`t` which refers to a schema which was not included in the dump.")

load_with_conflicts({ "includeSchemas": [ "b" ], "includeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which refers to a schema which was not included in the dump.")

load_with_conflicts({ "includeSchemas": [ "b" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a filter `a`.`t` which refers to a schema which was not included in the dump.")

load_with_conflicts({ "includeSchemas": [ "b" ], "excludeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a trigger `a`.`t`.`t` which refers to a schema which was not included in the dump.")

load_with_conflicts({ "excludeTables": [ "a.t" ], "includeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a filter `a`.`t` which refers to an excluded table.")

load_with_conflicts({ "excludeTables": [ "a.t" ], "includeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which refers to an excluded table.")

load_with_conflicts({ "includeTables": [ "b.t" ], "includeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a filter `a`.`t` which refers to a table which was not included in the dump.")

load_with_conflicts({ "includeTables": [ "b.t" ], "includeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeTriggers option contains a trigger `a`.`t`.`t` which refers to a table which was not included in the dump.")

load_with_conflicts({ "includeTables": [ "b.t" ], "excludeTriggers": [ "a.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a filter `a`.`t` which refers to a table which was not included in the dump.")

load_with_conflicts({ "includeTables": [ "b.t" ], "excludeTriggers": [ "a.t.t" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The excludeTriggers option contains a trigger `a`.`t`.`t` which refers to a table which was not included in the dump.")

#@<> includeUsers + excludeUsers conflicts
# no conflicts
load_with_conflicts({ "loadUsers": True, "includeUsers": [], "excludeUsers": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeUsers")
EXPECT_STDOUT_NOT_CONTAINS("excludeUsers")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u" ], "excludeUsers": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeUsers")
EXPECT_STDOUT_NOT_CONTAINS("excludeUsers")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h" ], "excludeUsers": [] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeUsers")
EXPECT_STDOUT_NOT_CONTAINS("excludeUsers")

load_with_conflicts({ "loadUsers": True, "includeUsers": [], "excludeUsers": [ "u" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeUsers")
EXPECT_STDOUT_NOT_CONTAINS("excludeUsers")

load_with_conflicts({ "loadUsers": True, "includeUsers": [], "excludeUsers": [ "u@h" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeUsers")
EXPECT_STDOUT_NOT_CONTAINS("excludeUsers")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u" ], "excludeUsers": [ "u1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeUsers")
EXPECT_STDOUT_NOT_CONTAINS("excludeUsers")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h" ], "excludeUsers": [ "u1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeUsers")
EXPECT_STDOUT_NOT_CONTAINS("excludeUsers")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u" ], "excludeUsers": [ "u1@h" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeUsers")
EXPECT_STDOUT_NOT_CONTAINS("excludeUsers")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h" ], "excludeUsers": [ "u@h1" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeUsers")
EXPECT_STDOUT_NOT_CONTAINS("excludeUsers")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u" ], "excludeUsers": [ "u@h" ] }, False)
EXPECT_STDOUT_NOT_CONTAINS("includeUsers")
EXPECT_STDOUT_NOT_CONTAINS("excludeUsers")

# conflicts
load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u" ], "excludeUsers": [ "u" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeUsers and excludeUsers options contain a user 'u'@''.")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u", "u1" ], "excludeUsers": [ "u" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeUsers and excludeUsers options contain a user 'u'@''.")
EXPECT_STDOUT_NOT_CONTAINS("u1")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u" ], "excludeUsers": [ "u", "u1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeUsers and excludeUsers options contain a user 'u'@''.")
EXPECT_STDOUT_NOT_CONTAINS("u1")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h" ], "excludeUsers": [ "u@h" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeUsers and excludeUsers options contain a user 'u'@'h'.")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h", "u1" ], "excludeUsers": [ "u@h" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeUsers and excludeUsers options contain a user 'u'@'h'.")
EXPECT_STDOUT_NOT_CONTAINS("u1")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h", "u1@h" ], "excludeUsers": [ "u@h" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeUsers and excludeUsers options contain a user 'u'@'h'.")
EXPECT_STDOUT_NOT_CONTAINS("u1")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h" ], "excludeUsers": [ "u@h", "u1" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeUsers and excludeUsers options contain a user 'u'@'h'.")
EXPECT_STDOUT_NOT_CONTAINS("u1")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h" ], "excludeUsers": [ "u@h", "u1@h" ] })
EXPECT_STDOUT_CONTAINS("ERROR: Both includeUsers and excludeUsers options contain a user 'u'@'h'.")
EXPECT_STDOUT_NOT_CONTAINS("u1")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h" ], "excludeUsers": [ "u" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeUsers option contains a user 'u'@'h' which is excluded by the value of the excludeUsers option: 'u'@''.")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h", "u1" ], "excludeUsers": [ "u" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeUsers option contains a user 'u'@'h' which is excluded by the value of the excludeUsers option: 'u'@''.")
EXPECT_STDOUT_NOT_CONTAINS("u1")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h", "u1@h" ], "excludeUsers": [ "u" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeUsers option contains a user 'u'@'h' which is excluded by the value of the excludeUsers option: 'u'@''.")
EXPECT_STDOUT_NOT_CONTAINS("u1")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h", "u" ], "excludeUsers": [ "u" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeUsers option contains a user 'u'@'h' which is excluded by the value of the excludeUsers option: 'u'@''.")
EXPECT_STDOUT_CONTAINS("ERROR: Both includeUsers and excludeUsers options contain a user 'u'@''.")

load_with_conflicts({ "loadUsers": True, "includeUsers": [ "u@h" ], "excludeUsers": [ "u", "u@h" ] })
EXPECT_STDOUT_CONTAINS("ERROR: The includeUsers option contains a user 'u'@'h' which is excluded by the value of the excludeUsers option: 'u'@''.")
EXPECT_STDOUT_CONTAINS("ERROR: Both includeUsers and excludeUsers options contain a user 'u'@'h'.")

#@<> BUG#33414321 - table with a secondary engine {VER(>=8.0.21)}
# setup
tested_schema = "test_schema"
tested_table = "test_table"
dump_dir = os.path.join(outdir, "bug_33414321")

shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [tested_schema])
session.run_sql("CREATE TABLE !.! (id BIGINT PRIMARY KEY)", [ tested_schema, tested_table + "1" ])
session.run_sql("""CREATE TABLE !.! (
    id BIGINT AUTO_INCREMENT,
    data BIGINT,
    description TEXT,
    fk_id BIGINT,
    PRIMARY KEY (id),
    UNIQUE KEY (data),
    FULLTEXT KEY (description),
    FOREIGN KEY (fk_id) REFERENCES !.! (id)
) SECONDARY_ENGINE=tmp SECONDARY_ENGINE_ATTRIBUTE='{"name": "value"}'""", [ tested_schema, tested_table, tested_schema, tested_table + "1" ])
session.run_sql("""CREATE TABLE !.! (
    id BIGINT AUTO_INCREMENT,
    data BIGINT,
    location GEOMETRY NOT NULL,
    PRIMARY KEY (id),
    UNIQUE KEY (data),
    SPATIAL KEY (location)
) SECONDARY_ENGINE=tmp SECONDARY_ENGINE_ATTRIBUTE='{"name": "value"}'""", [ tested_schema, tested_table + "2" ])

# dump data
util.dump_instance(dump_dir, { "showProgress": False })
# connect to the destination server
shell.connect(__sandbox_uri2)

# BUG#36197620 - summary should contain more details regarding all executed stages
indexes_summary = "indexes were built in "

# load with various values of deferTableIndexes
for deferred in [ ("off", 0), ("fulltext", 1), ("all", 13) ]:
    # wipe the destination server
    wipeout_server(session2)
    WIPE_OUTPUT()
    # load
    util.load_dump(dump_dir, { "deferTableIndexes": deferred[0], "loadUsers": False, "resetProgress": True, "showProgress": False })
    if deferred[1]:
        EXPECT_STDOUT_CONTAINS(f"{deferred[1]} {indexes_summary}")
    else:
        EXPECT_STDOUT_NOT_CONTAINS(indexes_summary)
    # verify correctness
    compare_servers(session1, session2, check_users=False)

#@<> BUG#33414321 - table with a secondary engine with resume {VER(>=8.0.21) and (not __dbug_off)}
# connect to the destination server
shell.connect(__sandbox_uri2)
wipeout_server(session2)

# fail after some of the ALTER TABLE statements which restore indexes were successfully executed
# BUG#33976718: This also tests that SPATIAL key is added in a separate query
testutil.set_trap("mysql", ["sql == ALTER TABLE `test_schema`.`test_table2` ADD SPATIAL KEY `location` (`location`)"], { "code": 1045, "msg": "Access denied for user `root`@`%` (using password: YES)", "state": "28000" })

EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "deferTableIndexes": "all", "loadUsers": False, "resetProgress": True, "showProgress": False }), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_MATCHES(re.compile(r"ERROR: \[Worker00\d\]: While building indexes for table `test_schema`.`test_table2`: Access denied for user `root`@`%` \(using password: YES\)"))

testutil.clear_traps("mysql")

# resume the load operation, without a trap it should succeed
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "deferTableIndexes": "all", "loadUsers": False, "showProgress": False }), "resume should succeed")
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")

# verify correctness
compare_servers(session1, session2, check_users=False)

#@<> BUG#33976718 - retry in case of full innodb_tmpdir {(not __dbug_off)}
# setup
tested_schema = "test_schema"
tested_table = "test_table"
dump_dir = os.path.join(outdir, "bug_33976718")

shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [tested_schema])
session.run_sql("""CREATE TABLE !.! (
    id BIGINT AUTO_INCREMENT,
    data BIGINT,
    description TEXT,
    fk_id BIGINT,
    PRIMARY KEY (id),
    UNIQUE KEY (data),
    FULLTEXT KEY (description)
)""", [ tested_schema, tested_table ])

# dump data
util.dump_instance(dump_dir, { "showProgress": False })

# connect to the destination server
shell.connect(__sandbox_uri2)

# wipe the destination server
wipeout_server(session2)
# trap on all ALTERs
testutil.set_trap("mysql", ["sql regex ALTER TABLE `test_schema`.`test_table` .*"], { "code": 1878, "msg": "Temporary file write failure.", "state": "HY000" })

# try to load, ALTER TABLE fails each time, load fails as well
WIPE_OUTPUT()
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "deferTableIndexes": "all", "loadUsers": False, "showProgress": False }), "Error loading dump")
# first error is silent
EXPECT_STDOUT_NOT_CONTAINS("ERROR: While building indexes for table `test_schema`.`test_table`: MySQL Error 1878 (HY000): Temporary file write failure.: ALTER TABLE `test_schema`.`test_table` ADD FULLTEXT KEY `description` (`description`),ADD UNIQUE KEY `data` (`data`)")
# second error is fatal and reported
EXPECT_STDOUT_CONTAINS("ERROR: While building indexes for table `test_schema`.`test_table`: MySQL Error 1878 (HY000): Temporary file write failure.: ALTER TABLE `test_schema`.`test_table` ADD FULLTEXT KEY `description` (`description`)")

testutil.clear_traps("mysql")

# wipe the destination server once again
wipeout_server(session2)
# trap on the first ALTER
testutil.set_trap("mysql", ["sql regex ALTER TABLE `test_schema`.`test_table` .*"], { "code": 1878, "msg": "Temporary file write failure.", "state": "HY000", "onetime": True })

# try to load, ALTER TABLE fails first time, but then succeeds
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "deferTableIndexes": "all", "loadUsers": False, "resetProgress": True, "showProgress": False }), "load should succeed")
# no errors are reported
EXPECT_STDOUT_NOT_CONTAINS("ERROR: While building indexes for table `test_schema`.`test_table`: MySQL Error 1878 (HY000): Temporary file write failure.")

testutil.clear_traps("mysql")

# verify correctness
compare_servers(session1, session2, check_users=False)

# cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])

#@<> BUG#33976718 - test scheduling {(not __dbug_off)}
# setup
tested_schema = "test_schema"
tested_table = "test_table"
dump_dir = os.path.join(outdir, "bug_33976718_scheduling")

shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [tested_schema])
session.run_sql("CREATE TABLE !.! (id BIGINT PRIMARY KEY)", [ tested_schema, tested_table + "1" ])
session.run_sql("""CREATE TABLE !.! (
    id BIGINT AUTO_INCREMENT,
    data BIGINT,
    description TEXT,
    fk_id BIGINT,
    PRIMARY KEY (id),
    UNIQUE KEY (data),
    FULLTEXT KEY (description),
    FOREIGN KEY (fk_id) REFERENCES !.! (id)
)""", [ tested_schema, tested_table, tested_schema, tested_table + "1" ])
session.run_sql("""CREATE TABLE !.! (
    id BIGINT AUTO_INCREMENT,
    data BIGINT,
    location GEOMETRY NOT NULL,
    PRIMARY KEY (id),
    UNIQUE KEY (data),
    SPATIAL KEY (location)
)""", [ tested_schema, tested_table + "2" ])

# dump data
util.dump_instance(dump_dir, { "showProgress": False })
# connect to the destination server
shell.connect(__sandbox_uri2)

testutil.dbug_set("+d,dump_loader_force_index_weight")

# load with various number of threads, loader has arfiticial number of ALTER TABLE .. ADD INDEX threads set to four
for threads in [2, 4, 8]:
    # load with various values of deferTableIndexes
    for deferred in [ "off", "fulltext", "all" ]:
        # wipe the destination server
        wipeout_server(session2)
        # load
        util.load_dump(dump_dir, { "deferTableIndexes": deferred, "threads": threads, "loadUsers": False, "resetProgress": True, "showProgress": False })
        # verify correctness
        compare_servers(session1, session2, check_users=False)

testutil.dbug_set("")

#@<> BUG#33592520 dump when --skip-grant-tables is active
# prepare the server
shell.connect(__sandbox_uri2)
wipeout_server(session)

session.run_sql("create database test;")
session.run_sql("use test;")
session.run_sql("create table t(a int primary key, b int, c int, index(b), index(c)) engine=innodb;")
session.run_sql("set @N=0;")
session.run_sql("insert into t select @N:=@N+1, if(@N<50 or @N=999950, 1, 2 + rand()*1000), if(@N<50 or @N=999950, 50, 49 + rand()*5) from mysql.help_topic a, mysql.help_topic b, mysql.help_topic c limit 100000;")
session.run_sql("analyze table t;")

# get the socket URI, use invalid credentials
socket_uri = shell.unparse_uri({ **shell.parse_uri(get_socket_uri(session)), "user": "invalid", "password": "invalid" })

if __os_type == "windows":
    socket_uri += "?get-server-public-key=true"

# enable --skip-grant-tables, restart the server
testutil.change_sandbox_conf(__mysql_sandbox_port2, "skip-grant-tables", "ON")
testutil.restart_sandbox(__mysql_sandbox_port2)
testutil.wait_sandbox_alive(socket_uri)

# dump the instance, include users, this should be implicitly disabled, since --skip-grant-tables is in effect
dump_dir = os.path.join(outdir, "bug_33592520")

shell.connect(socket_uri)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "users": True, "showProgress": False }), "Dumping should not throw")
EXPECT_STDOUT_CONTAINS("WARNING: Server is running with the --skip-grant-tables option active, dumping users, roles and grants has been disabled.")

# disable --skip-grant-tables, restart the server
testutil.change_sandbox_conf(__mysql_sandbox_port2, "skip-grant-tables", "OFF")

# testutil.restart_sandbox() will not work here, as ports are not active (--skip-grant-tables enables --skip-networking)
try:
    session.run_sql("SHUTDOWN")
except Exception as e:
    # we may get an exception while shutting down, i.e. lost connection
    print("Exception while shutting down:", e)

testutil.wait_sandbox_dead(__mysql_sandbox_port2)
testutil.start_sandbox(__mysql_sandbox_port2)
testutil.wait_sandbox_alive(__mysql_sandbox_port2)
# server was restarted, need to recreate the session
session2 = mysql.get_session(__sandbox_uri2)

# load the dump
shell.connect(__sandbox_uri2)
wipeout_server(session)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "Loading should not throw")

#@<> BUG#33640887 - routine created while ANSI_QUOTES was in effect
# setup
tested_schema = '"test\'schema"'
tested_procedure = '"test\'procedure"'
tested_function = '"test\'function"'
tested_event = '"test\'event"'
tested_table = '"test\'table"'
tested_view = '"test\'view"'
tested_trigger = '"test\'trigger"'
dump_dir = os.path.join(outdir, "bug_33640887")

shell.connect(__sandbox_uri1)
session.run_sql("SET @saved_sql_mode = @@sql_mode")
session.run_sql("SET sql_mode = ANSI_QUOTES")
session.run_sql(f"DROP SCHEMA IF EXISTS {tested_schema}")
session.run_sql(f"CREATE SCHEMA IF NOT EXISTS {tested_schema}")
session.run_sql(f"CREATE PROCEDURE {tested_schema}.{tested_procedure}() SELECT 1")
session.run_sql(f"CREATE FUNCTION {tested_schema}.{tested_function}() RETURNS INT NO SQL RETURN 1")
session.run_sql(f"CREATE EVENT {tested_schema}.{tested_event} ON SCHEDULE EVERY 1 year DISABLE DO BEGIN END")
session.run_sql(f"CREATE TABLE {tested_schema}.{tested_table} (id INT PRIMARY KEY)")
session.run_sql(f"CREATE TRIGGER {tested_schema}.{tested_table} BEFORE UPDATE ON {tested_schema}.{tested_table} FOR EACH ROW BEGIN END")
session.run_sql(f"CREATE VIEW {tested_schema}.{tested_view} AS SELECT 1")
session.run_sql("SET sql_mode = @saved_sql_mode")

# dump data
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "showProgress": False }), "Dump should not fail")
# connect to the destination server
shell.connect(__sandbox_uri2)
# wipe the destination server
wipeout_server(session2)
# load
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": False, "resetProgress": True, "showProgress": False }), "Load should not fail")
# verify correctness
compare_servers(session1, session2, check_users=False)

# cleanup
session.run_sql("SET @saved_sql_mode = @@sql_mode")
session.run_sql("SET sql_mode = ANSI_QUOTES")
session.run_sql(f"DROP SCHEMA IF EXISTS {tested_schema}")
session.run_sql("SET sql_mode = @saved_sql_mode")

#@<> BUG#33497745 - load a dump created by shell 8.0.21
# prepare the server
shell.connect(__sandbox_uri2)
wipeout_server(session)

# disable log_bin_trust_function_creators, some functions have none of DETERMINISTIC, NO SQL, or READS SQL DATA
session.run_sql("SET @old_log_bin_trust_function_creators = @@global.log_bin_trust_function_creators")
session.run_sql("SET @@global.log_bin_trust_function_creators = ON")

# extract the dump
source_archive = os.path.join(__data_path, "load", "dump_instance_8.0.21.tar.gz")
dump_dir = os.path.join(outdir, "dump_instance_8.0.21")

try:
    # extraction filters were added recently, using a default value if they are available may result in a warning, which causes this test to fail
    shutil.unpack_archive(source_archive, outdir, filter="data")
except:
    shutil.unpack_archive(source_archive, outdir)

# run the test
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "excludeUsers": [ "'root'@'%'" ], "ignoreVersion": True, "showProgress": False }), "Loading should not throw")
EXPECT_STDOUT_CONTAINS(f"Loading DDL, Data and Users from '{dump_dir}' using 4 threads.")
EXPECT_STDOUT_CONTAINS("NOTE: Dump format has version 1.0.0 and was created by an older version of MySQL Shell. If you experience problems loading it, please recreate the dump using the current version of MySQL Shell and try again.")
EXPECT_STDOUT_CONTAINS("62 chunks (5.45K rows, 199.62 KB) for 62 tables in 8 schemas were loaded")
EXPECT_STDOUT_CONTAINS("0 warnings were reported during the load.")

# restore log_bin_trust_function_creators
session.run_sql("SET @@global.log_bin_trust_function_creators = @old_log_bin_trust_function_creators")

#@<> BUG#33743612 - issues when dumping/loading data using an account with user name containing '@' character
# constants
dump_dir = os.path.join(outdir, "bug_33743612")
first_user = "'admin@domain.com'"
second_user = "'user@domain.com'"

# setup source server
shell.connect(__sandbox_uri1)

session.run_sql(f"DROP USER IF EXISTS {first_user}")
session.run_sql(f"CREATE USER {first_user} IDENTIFIED BY 'pwd'")
session.run_sql(f"GRANT ALL ON *.* TO {first_user}")

session.run_sql(f"DROP USER IF EXISTS {second_user}")
session.run_sql(f"CREATE USER {second_user} IDENTIFIED BY 'pwd'")
session.run_sql(f"GRANT SELECT ON *.* TO {second_user}")

# setup destination server
shell.connect(__sandbox_uri2)
wipeout_server(session)

session.run_sql(f"CREATE USER {first_user} IDENTIFIED BY 'pwd'")
session.run_sql(f"GRANT ALL ON *.* TO {first_user} WITH GRANT OPTION")

# dump data
shell.connect(f"{urllib.parse.quote(first_user[1:-1])}:pwd@{__host}:{__mysql_sandbox_port1}")
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "users": True, "showProgress": False }), "Dump should not fail")

# load data
shell.connect(f"{urllib.parse.quote(first_user[1:-1])}:pwd@{__host}:{__mysql_sandbox_port2}")
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "excludeUsers": [ "root" ], "showProgress": False }), "Load should not fail")

# cleanup
shell.connect(__sandbox_uri1)
session.run_sql(f"DROP USER IF EXISTS {first_user}")
session.run_sql(f"DROP USER IF EXISTS {second_user}")

#@<> TEST_STRING_OPTION + setup
shell.connect(__sandbox_uri2)
dump_dir = os.path.join(outdir, "ddlonly")

def TEST_STRING_OPTION(option):
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { option: None }), f"TypeError: Util.load_dump: Argument #2: Option '{option}' is expected to be of type String, but is Null")
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { option: 5 }), f"TypeError: Util.load_dump: Argument #2: Option '{option}' is expected to be of type String, but is Integer")
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { option: -5 }), f"TypeError: Util.load_dump: Argument #2: Option '{option}' is expected to be of type String, but is Integer")
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { option: [] }), f"TypeError: Util.load_dump: Argument #2: Option '{option}' is expected to be of type String, but is Array")
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { option: {} }), f"TypeError: Util.load_dump: Argument #2: Option '{option}' is expected to be of type String, but is Map")
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { option: False }), f"TypeError: Util.load_dump: Argument #2: Option '{option}' is expected to be of type String, but is Bool")

#@<> WL14387-TSFR_1_1_1 - s3BucketName - string option
TEST_STRING_OPTION("s3BucketName")

#@<> WL14387-TSFR_1_2_1 - s3BucketName and osBucketName cannot be used at the same time
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "s3BucketName": "one", "osBucketName": "two" }), "ValueError: Util.load_dump: Argument #2: The option 's3BucketName' cannot be used when the value of 'osBucketName' option is set")

#@<> WL14387-TSFR_1_1_3 - s3BucketName set to an empty string loads from a local directory
wipeout_server(session2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "s3BucketName": "", "resetProgress": True }), "should not fail")

#@<> s3CredentialsFile - string option
TEST_STRING_OPTION("s3CredentialsFile")

#@<> WL14387-TSFR_3_1_1_1 - s3CredentialsFile cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "s3CredentialsFile": "file" }), "ValueError: Util.load_dump: Argument #2: The option 's3CredentialsFile' cannot be used when the value of 's3BucketName' option is not set")

#@<> s3BucketName and s3CredentialsFile both set to an empty string loads from a local directory
wipeout_server(session2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "s3BucketName": "", "s3CredentialsFile": "", "resetProgress": True }), "should not fail")

#@<> s3ConfigFile - string option
TEST_STRING_OPTION("s3ConfigFile")

#@<> WL14387-TSFR_4_1_1_1 - s3ConfigFile cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "s3ConfigFile": "file" }), "ValueError: Util.load_dump: Argument #2: The option 's3ConfigFile' cannot be used when the value of 's3BucketName' option is not set")

#@<> s3BucketName and s3ConfigFile both set to an empty string loads from a local directory
wipeout_server(session2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "s3BucketName": "", "s3ConfigFile": "", "resetProgress": True }), "should not fail")

#@<> WL14387-TSFR_2_1_1 - s3Profile - string option
TEST_STRING_OPTION("s3Profile")

#@<> WL14387-TSFR_2_1_1_2 - s3Profile cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "s3Profile": "profile" }), "ValueError: Util.load_dump: Argument #2: The option 's3Profile' cannot be used when the value of 's3BucketName' option is not set")

#@<> WL14387-TSFR_2_1_2_1 - s3BucketName and s3Profile both set to an empty string loads from a local directory
wipeout_server(session2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "s3BucketName": "", "s3Profile": "", "resetProgress": True }), "should not fail")

#@<> s3EndpointOverride - string option
TEST_STRING_OPTION("s3EndpointOverride")

#@<> WL14387-TSFR_6_1_1 - s3EndpointOverride cannot be used without s3BucketName
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "s3EndpointOverride": "http://example.org" }), "ValueError: Util.load_dump: Argument #2: The option 's3EndpointOverride' cannot be used when the value of 's3BucketName' option is not set")

#@<> s3BucketName and s3EndpointOverride both set to an empty string loads from a local directory
wipeout_server(session2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "s3BucketName": "", "s3EndpointOverride": "", "resetProgress": True }), "should not fail")

#@<> s3EndpointOverride is missing a scheme
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "s3BucketName": "bucket", "s3EndpointOverride": "endpoint" }), "ValueError: Util.load_dump: Argument #2: The value of the option 's3EndpointOverride' is missing a scheme, expected: http:// or https://.")

#@<> s3EndpointOverride is using wrong scheme
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "s3BucketName": "bucket", "s3EndpointOverride": "FTp://endpoint" }), "ValueError: Util.load_dump: Argument #2: The value of the option 's3EndpointOverride' uses an invalid scheme 'FTp://', expected: http:// or https://.")

#@<> BUG#33788895 - log errors even if shell.options["logSql"] is "off"
old_log_sql = shell.options["logSql"]
shell.options["logSql"] = "off"

# dump
dump_dir = os.path.join(outdir, "bug_33788895")
session1.run_sql("CREATE USER admin@'%' IDENTIFIED BY 'pass'")
session1.run_sql("GRANT ALL ON *.* TO 'admin'@'%'")
shell.connect("mysql://admin:pass@{0}:{1}".format(__host, __mysql_sandbox_port1))

# dump instance
wipe_dir(dump_dir)
session1.run_sql("ALTER USER admin@'%' WITH MAX_QUERIES_PER_HOUR 100")
WIPE_SHELL_LOG()
EXPECT_THROWS(lambda: util.dump_instance(dump_dir, { "users": False }), "Fatal error during dump")
EXPECT_SHELL_LOG_MATCHES(re.compile(r"Info: util.dumpInstance\(\): tid=\d+: MySQL Error 1226 \(42000\): User 'admin' has exceeded the 'max_questions' resource \(current value: 100\), SQL: "))

# dump schemas
wipe_dir(dump_dir)
session1.run_sql("ALTER USER admin@'%' WITH MAX_QUERIES_PER_HOUR 80")
WIPE_SHELL_LOG()
EXPECT_THROWS(lambda: util.dump_schemas(["world"], dump_dir), "Fatal error during dump")
EXPECT_SHELL_LOG_MATCHES(re.compile(r"Info: util.dumpSchemas\(\): tid=\d+: MySQL Error 1226 \(42000\): User 'admin' has exceeded the 'max_questions' resource \(current value: 80\), SQL: "))

# dump tables
wipe_dir(dump_dir)
session1.run_sql("ALTER USER admin@'%' WITH MAX_QUERIES_PER_HOUR 70")
WIPE_SHELL_LOG()
tables = ["City", "Country"]
if __os_type == "windows":
    tables = [ t.lower() for t in tables ]
EXPECT_THROWS(lambda: util.dump_tables("world", tables, dump_dir), "Fatal error during dump")
EXPECT_SHELL_LOG_MATCHES(re.compile(r"Info: util.dumpTables\(\): tid=\d+: MySQL Error 1226 \(42000\): User 'admin' has exceeded the 'max_questions' resource \(current value: 70\), SQL: "))

session1.run_sql("DROP USER admin@'%'")

# load dump
dump_dir = os.path.join(outdir, "dump_instance")
wipeout_server(session2)
session2.run_sql("CREATE USER admin@'%' IDENTIFIED BY 'pass' WITH MAX_QUERIES_PER_HOUR 70")
session2.run_sql("GRANT ALL ON *.* TO 'admin'@'%'")
shell.connect("mysql://admin:pass@{0}:{1}".format(__host, __mysql_sandbox_port2))

WIPE_SHELL_LOG()
# we don't specify the error message here, it can vary depending on when the error is reported
EXPECT_THROWS(lambda: util.load_dump(dump_dir), "")
EXPECT_STDOUT_CONTAINS("User 'admin' has exceeded the 'max_questions' resource (current value: 70)")
EXPECT_SHELL_LOG_MATCHES(re.compile(r"Info: util.loadDump\(\): tid=\d+: MySQL Error 1226 \(42000\): User 'admin' has exceeded the 'max_questions' resource \(current value: 70\), SQL: "))

#@<> BUG#33788895 - cleanup
shell.options["logSql"] = old_log_sql

#@<> BUG#34141432 - shell may expose sensitive information via error messages
# constants
dump_dir = os.path.join(outdir, "bug_34141432")

# dump users and DDL, we're not interested in data
shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "users": True, "ddlOnly": True, "showProgress": False }), "Dump should not fail")

# overwrite the users file with malformed CREATE USER statement
testutil.create_file(os.path.join(dump_dir, "@.users.sql"), """
-- MySQLShell dump 2.0.1  Distrib Ver 8.0.30 for Linux on x86_64 - for MySQL 8.0.30 (Source distribution), for Linux (x86_64)
--
-- Host: localhost
-- ------------------------------------------------------
-- Server version	8.0.30
--
-- Dumping user accounts
--

-- begin user 'airportdb'@'%'
CREATE USER IF NOT EXISTS `airportdb`@`%` IDENTIFIED WITH 'mysql_native_password' USING '*AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA';
-- end user 'airportdb'@'%'

-- begin grants 'airportdb'@'%'
GRANT SELECT, INSERT, UPDATE, DELETE ON *.* TO `airportdb`@`%` WITH GRANT OPTION;
-- end grants 'airportdb'@'%'

-- End of dumping user accounts
""")

# load users, it should fail as CREATE USER statement has a syntax error, but password should not be exposed
shell.connect(__sandbox_uri2)
wipeout_server(session)
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "showProgress": False }), "You have an error in your SQL syntax")
EXPECT_STDOUT_NOT_CONTAINS("AA")

#@<> BUG#34408669 - shell should fall back to manually creating primary keys if sql_generate_invisible_primary_key cannot be set by the user {VER(>= 8.0.30)}
# constants
dump_dir = os.path.join(outdir, "bug_34408669")
tested_schema = "tested_schema"
tested_table = "tested_table"

# setup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [tested_schema])
session.run_sql("SET @@SESSION.sql_generate_invisible_primary_key = OFF")
session.run_sql("CREATE TABLE !.! (data INT)", [ tested_schema, tested_table ])
# dump DDL, we're not interested in data
EXPECT_NO_THROWS(lambda: util.dump_schemas([tested_schema], dump_dir, { "ddlOnly": True, "showProgress": False }), "Dump should not fail")

# connect, create a user which cannot change the sql_generate_invisible_primary_key variable
shell.connect(__sandbox_uri2)
wipeout_server(session)
session.run_sql("SET @@GLOBAL.sql_generate_invisible_primary_key = OFF")
session.run_sql("CREATE USER IF NOT EXISTS admin@'%' IDENTIFIED BY 'pass'")
session.run_sql("GRANT ALL ON *.* TO admin@'%'")
session.run_sql("REVOKE SUPER,SYSTEM_VARIABLES_ADMIN,SESSION_VARIABLES_ADMIN ON *.* FROM admin@'%'")

# connect as the created user, ask for PKs to be created
WIPE_SHELL_LOG()
shell.connect("mysql://admin:pass@{0}:{1}".format(__host, __mysql_sandbox_port2))
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "createInvisiblePKs": True, "showProgress": True }), "Load should not fail")
EXPECT_CONTAINS(["PRIMARY KEY"], session.run_sql("SHOW CREATE TABLE !.!", [ tested_schema, tested_table ]).fetch_all()[0][1])
EXPECT_SHELL_LOG_CONTAINS("The current user cannot set the sql_generate_invisible_primary_key session variable")

# load again, this time PKs should not be created
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "createInvisiblePKs": False, "showProgress": True, "resetProgress": True }), "Load should not fail")
EXPECT_NOT_CONTAINS(["PRIMARY KEY"], session.run_sql("SHOW CREATE TABLE !.!", [ tested_schema, tested_table ]).fetch_all()[0][1])

# try once again, this time global variable is ON
shell.connect(__sandbox_uri2)
session.run_sql("SET @@GLOBAL.sql_generate_invisible_primary_key = ON")
shell.connect("mysql://admin:pass@{0}:{1}".format(__host, __mysql_sandbox_port2))

# user requests PKs to be created, this should work
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "createInvisiblePKs": True, "showProgress": True, "resetProgress": True }), "Load should not fail")
EXPECT_CONTAINS(["PRIMARY KEY"], session.run_sql("SHOW CREATE TABLE !.!", [ tested_schema, tested_table ]).fetch_all()[0][1])

# user requests no primary keys to be created, but since they don't have required privileges it fails
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "createInvisiblePKs": False, "showProgress": True, "resetProgress": True }), "Access denied")

# user requests no primary keys to be created, env var is set, keys are created anyway
os.environ["MYSQLSH_ALLOW_ALWAYS_GIPK"] = "1"
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "createInvisiblePKs": False, "showProgress": True, "resetProgress": True }), "Load should not fail")
EXPECT_CONTAINS(["PRIMARY KEY"], session.run_sql("SHOW CREATE TABLE !.!", [ tested_schema, tested_table ]).fetch_all()[0][1])

#@<> BUG#34173126 - loading a dump when global auto-commit is off
# constants
dump_dir = os.path.join(outdir, "bug_34173126")
tested_schema = "tested_schema"
tested_table = "tested_table"

# setup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [tested_schema])
session.run_sql("CREATE TABLE !.! (id INT PRIMARY KEY, data INT)", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (1, 1), (2, 2), (3, 3), (4, 4), (5, 5)", [ tested_schema, tested_table ])
EXPECT_NO_THROWS(lambda: util.dump_schemas([tested_schema], dump_dir, { "showProgress": False }), "Dump should not fail")

# connect to the target instance, disable auto-commit and load
shell.connect(__sandbox_uri2)
wipeout_server(session)
original_global_autocommit = session.run_sql("SELECT @@GLOBAL.autocommit").fetch_one()[0]
session.run_sql("SET @@GLOBAL.autocommit = OFF")
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "Load should not fail")

# verification
compare_schema(session1, session2, tested_schema, check_rows=True)

#@<> BUG#34173126 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
shell.connect(__sandbox_uri2)
session.run_sql("SET @@GLOBAL.autocommit = ?", [original_global_autocommit])

#@<> BUG#34768224 - loading a view which uses another view with DEFINER set
# constants
dump_dir = os.path.join(outdir, "bug_34768224")
tested_schema = "tested_schema"
tested_table = "tested_table"
tested_view = "tested_view"
tested_user = "'user'@'localhost'"

# setup
shell.connect(__sandbox_uri1)
session.run_sql(f"DROP USER IF EXISTS {tested_user}")
session.run_sql(f"CREATE USER {tested_user}")
session.run_sql(f"GRANT ALL ON *.* TO {tested_user}")

session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [tested_schema])
session.run_sql("CREATE TABLE !.! (id INT PRIMARY KEY, data INT)", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (1, 1), (2, 2), (3, 3), (4, 4), (5, 5)", [ tested_schema, tested_table ])
session.run_sql(f"CREATE DEFINER = {tested_user} VIEW !.! AS SELECT * FROM !.!", [ tested_schema, tested_view, tested_schema, tested_table ])
session.run_sql(f"CREATE VIEW !.! AS SELECT * FROM !.!", [ tested_schema, tested_view + "_2", tested_schema, tested_view ])
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [tested_schema], "users": True, "excludeUsers": ["root"], "showProgress": False }), "Dump should not fail")

# connect to the target instance and load
shell.connect(__sandbox_uri2)
wipeout_server(session)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "showProgress": False }), "Load should not fail")

# verification
compare_schema(session1, session2, tested_schema, check_rows=True)

#@<> BUG#34768224 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql(f"DROP USER IF EXISTS {tested_user}")

#@<> BUG#34764157 - setup
# constants
dump_dir = os.path.join(outdir, "bug_34764157")
tested_schema = "tested_schema"
tested_table = "tested_table"
tested_view = "tested_view"
# show grants in 5.7 is going to list this routine using all lower case
tested_routine = "Tested_Routine"
tested_user = "'user'@'localhost'"

# setup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [tested_schema])
session.run_sql("CREATE TABLE !.! (id INT PRIMARY KEY, data INT)", [ tested_schema, tested_table ])
session.run_sql("CREATE VIEW !.! AS SELECT * FROM !.!", [ tested_schema, tested_view, tested_schema, tested_table ])
session.run_sql("CREATE PROCEDURE !.!() BEGIN END", [ tested_schema, tested_routine ])

session.run_sql(f"DROP USER IF EXISTS {tested_user}")
session.run_sql(f"CREATE USER {tested_user} IDENTIFIED BY 'pwd'")
session.run_sql(f"GRANT SELECT ON !.! TO {tested_user}", [tested_schema, tested_table])
session.run_sql(f"GRANT SELECT ON !.! TO {tested_user}", [tested_schema, tested_view])
session.run_sql(f"GRANT EXECUTE ON PROCEDURE !.! TO {tested_user}", [tested_schema, tested_routine])

#@<> BUG#34764157 - dumper commented out grants for included, existing views and routines
shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [tested_schema], "users": True, "excludeUsers": ["root"], "showProgress": False }), "Dump should not fail")

# connect to the target instance and load
shell.connect(__sandbox_uri2)
wipeout_server(session)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "showProgress": False }), "Load should not fail")

# verification
compare_schema(session1, session2, tested_schema, check_rows=True)
compare_user_grants(session1, session2, tested_user)

#@<> BUG#34764157 - dumper should detect invalid grants - table/view
# NOTE: it's not possible to test routine, as when routine is removed, grant is removed as well
shell.connect(__sandbox_uri1)
session.run_sql("DROP VIEW !.!", [ tested_schema, tested_view ])
invalid_grant = f"""GRANT SELECT ON `{tested_schema}`.`{tested_view}` TO {get_user_account_for_output(tested_user)}"""

# without MDS compatibility check
WIPE_OUTPUT()
wipe_dir(dump_dir)
EXPECT_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [tested_schema], "users": True, "excludeUsers": ["root"], "showProgress": False }), "Dump contains an invalid grant statement. Use the 'strip_invalid_grants' compatibility option to fix this.")
EXPECT_STDOUT_CONTAINS(f"ERROR: User {tested_user} has grant statement on a non-existent table ({invalid_grant})")

# with MDS compatibility check
WIPE_OUTPUT()
wipe_dir(dump_dir)
EXPECT_THROWS(lambda: util.dump_instance(dump_dir, { "compatibility": ["strip_definers"], "ocimds": True, "targetVersion": "8.0.30", "includeSchemas": [tested_schema], "users": True, "excludeUsers": ["root"], "showProgress": False }), "Compatibility issues were found")
EXPECT_STDOUT_CONTAINS(strip_invalid_grants(tested_user, invalid_grant, "table").error())

# succeeds with the compatibility option
WIPE_OUTPUT()
wipe_dir(dump_dir)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "compatibility": ["strip_invalid_grants"], "includeSchemas": [tested_schema], "users": True, "excludeUsers": ["root"], "showProgress": False }), "Dump should not fail")

# succeeds with the compatibility option - ocimds
WIPE_OUTPUT()
wipe_dir(dump_dir)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "compatibility": ["strip_invalid_grants", "strip_definers"], "ocimds": True, "targetVersion": "8.0.30", "includeSchemas": [tested_schema], "users": True, "excludeUsers": ["root"], "showProgress": False }), "Dump should not fail")
EXPECT_STDOUT_CONTAINS(strip_invalid_grants(tested_user, invalid_grant, "table").fixed())

# test load
shell.connect(__sandbox_uri2)
wipeout_server(session)
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "showProgress": False }), "Load should not fail")

#@<> BUG#34764157 - dumper should detect invalid grants - view is missing but grant is valid
shell.connect(__sandbox_uri1)
session.run_sql(f"GRANT CREATE ON !.! TO {tested_user}", [tested_schema, tested_view])

wipe_dir(dump_dir)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [tested_schema], "users": True, "excludeUsers": ["root"], "showProgress": False }), "Dump should not fail")

shell.connect(__sandbox_uri2)
wipeout_server(session)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "showProgress": False }), "Load should not fail")

#@<> BUG#34764157 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql(f"DROP USER IF EXISTS {tested_user}")

#@<> BUG#34876423 - load failed if table contained multiple indexes and one of them was specified on an AUTO_INCREMENT column
# constants
dump_dir = os.path.join(outdir, "bug_34876423")
tested_schema = "tested_schema"
tested_table = "tested_table"

# setup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA IF NOT EXISTS !", [tested_schema])
session.run_sql("CREATE TABLE !.! (num INT UNSIGNED PRIMARY KEY AUTO_INCREMENT, val varchar(32), KEY(val), KEY(val, num))", [ tested_schema, tested_table ])

# dump
shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [tested_schema], "showProgress": False }), "Dump should not fail")
# `val` index is going to be deferred, drop it and readd it to make sure the same CREATE TABLE statement is returned by both source and destination instance
session.run_sql("ALTER TABLE !.! DROP KEY `val`", [ tested_schema, tested_table ])
session.run_sql("ALTER TABLE !.! ADD KEY (`val`)", [ tested_schema, tested_table ])

# load
shell.connect(__sandbox_uri2)
wipeout_server(session)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "deferTableIndexes": "all", "showProgress": False }), "Load should not fail")

# verification
compare_schema(session1, session2, tested_schema, check_rows=True)

#@<> BUG#34876423 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])

#@<> BUG#34566034 - load failed if "deferTableIndexes": "all", "ignoreExistingObjects": True options were set and instance contained existing tables with indexes
# constants
dump_dir = os.path.join(outdir, "bug_34566034")
tested_schema = "tested_schema"
tested_table = "tested_table"

# setup
shell.connect(__sandbox_uri2)
session.run_sql("DROP SCHEMA IF EXISTS !", [ tested_schema ])
session.run_sql("CREATE SCHEMA !", [ tested_schema ])
session.run_sql("""
CREATE TABLE IF NOT EXISTS !.! (
  `cluster_id` CHAR(36) CHARACTER SET ascii COLLATE ascii_general_ci,
  `cluster_name` VARCHAR(63) NOT NULL,
  `description` TEXT,
  `options` JSON,
  `attributes` JSON,
  `cluster_type` ENUM('gr', 'ar') NOT NULL,
  `primary_mode` ENUM('pm', 'mm') NOT NULL DEFAULT 'pm',
  `router_options` JSON,
  `clusterset_id` VARCHAR(36) CHARACTER SET ascii COLLATE ascii_general_ci DEFAULT NULL,
  PRIMARY KEY(cluster_id),
  UNIQUE KEY(cluster_name)
) CHARSET = utf8mb4, ROW_FORMAT = DYNAMIC;
""", [ tested_schema, tested_table ])

# dump
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "showProgress": False }), "Dump should not fail")

# load
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "deferTableIndexes": "all", "ignoreExistingObjects": True, "showProgress": False }), "Load should not fail")
EXPECT_STDOUT_CONTAINS(f"NOTE: Schema `{tested_schema}` already contains a table named `{tested_table}`")
EXPECT_STDOUT_CONTAINS("NOTE: One or more objects in the dump already exist in the destination database but will be ignored because the 'ignoreExistingObjects' option was enabled.")

#@<> BUG#34566034 - cleanup
shell.connect(__sandbox_uri2)
wipeout_server(session)

#@<> BUG#35304391 - loader should notify if rows were replaced during load
# constants
dump_dir = os.path.join(outdir, "bug_35304391")
tested_schema = "tested_schema"
tested_table = "tested_table"

# setup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [ tested_schema ])
session.run_sql("CREATE SCHEMA ! DEFAULT CHARACTER SET latin1 COLLATE latin1_general_cs", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (id tinyint AUTO_INCREMENT PRIMARY KEY, alias varchar(12) NOT NULL UNIQUE KEY, name varchar(32) NOT NULL)", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! (alias, name) VALUES ('bond-007', 'James Bond'), ('Bond-007', 'James Bond')", [ tested_schema, tested_table ])

# dump
shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [ tested_schema ], "showProgress": False }), "Dump should not fail")

# load
shell.connect(__sandbox_uri2)
wipeout_server(session)
# load just the DDL
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadData": False, "resetProgress": True, "showProgress": False }), "Load should not fail")
# change collation to case-insensitive
session.run_sql("ALTER SCHEMA ! CHARACTER SET latin1 COLLATE latin1_general_ci", [ tested_schema ])
session.run_sql("ALTER TABLE !.! CONVERT TO CHARACTER SET latin1 COLLATE latin1_general_ci", [ tested_schema, tested_table ])
# load the data
WIPE_OUTPUT()
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadDdl": False, "resetProgress": True, "showProgress": False }), "Load should not fail")

# verification
EXPECT_STDOUT_CONTAINS("1 rows were replaced")

#@<> BUG#35304391 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])

#@<> WL#15887 - warning when loading to a different version than requested during dump {not __dbug_off}
# constants
dump_dir = os.path.join(outdir, "wl_15887")

# dump
shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "targetVersion": __mysh_version_no_extra, "ddlOnly": True, "showProgress": False }), "Dump should not fail")

# setup
testutil.dbug_set("+d,dump_loader_force_mds")

# load
shell.connect(__sandbox_uri2)
wipeout_server(session)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "ignoreVersion": True, "showProgress": False }), "Load should not fail")

# verification
if __mysh_version_no_extra == __version:
    # version match, no warning
    EXPECT_STDOUT_NOT_CONTAINS("'targetVersion'")
else:
    EXPECT_STDOUT_CONTAINS(f"Destination MySQL version is different than the value of the 'targetVersion' option set when the dump was created: {__mysh_version_no_extra}")

# cleanup
testutil.dbug_set("")

#@<> BUG#35822020 - loader is stuck if metadata files are missing
# constants
dump_dir = os.path.join(outdir, "bug_35822020")
tested_schema = "tested_schema"
tested_table = "tested_table"

class Expected_result(NamedTuple):
    pattern: re.Pattern
    success: bool
    msg: str = ""

# order is important
expected = [
    Expected_result(re.compile(r"@(\.(post|users))?\.sql"), True),  # loader ignores these files if they are missing
    Expected_result(re.compile(r".+\.idx"), True),  # these files are not used if dump is complete
    Expected_result(re.compile(r"@\.json"), False, "Cannot open file"),
    Expected_result(re.compile(r"@\.done\.json"), False, "Incomplete dump"),
    Expected_result(re.compile(r".+\.triggers\.sql"), False, "Cannot open file"),
    Expected_result(re.compile(r".+\.(json|sql)"), False, "Dump directory is corrupted, some of the metadata files are missing"),
    Expected_result(re.compile(r".+\.zst"), False, "Dump directory is corrupted, some of the data files are missing")
]

# setup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [ tested_schema ])
session.run_sql("CREATE SCHEMA !", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (a int)", [ tested_schema, tested_table ])
session.run_sql("CREATE TABLE !.! (a int) PARTITION BY RANGE (a) (PARTITION p0 VALUES LESS THAN (6))", [ tested_schema, tested_table + "_partitioned" ])
session.run_sql("CREATE VIEW !.tested_view AS SELECT * FROM !.!", [ tested_schema, tested_schema, tested_table ])
session.run_sql("CREATE EVENT !.tested_event ON SCHEDULE EVERY 1 YEAR DO BEGIN END", [ tested_schema ])
session.run_sql("CREATE FUNCTION !.tested_function() RETURNS INT DETERMINISTIC RETURN 1", [ tested_schema ])
session.run_sql("CREATE PROCEDURE !.tested_procedure() DETERMINISTIC BEGIN END", [ tested_schema ])
session.run_sql("CREATE TRIGGER !.tested_trigger AFTER DELETE ON !.! FOR EACH ROW BEGIN END", [ tested_schema, tested_schema, tested_table ])

EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "includeSchemas": [ tested_schema ], "users": True, "showProgress": False }), "Dump should not fail")

#@<> BUG#35822020 - test
shell.connect(__sandbox_uri2)
l = lambda: util.load_dump(dump_dir, { "loadUsers": True, "excludeUsers": [ "'root'@'%'" ], "resetProgress": True, "showProgress": False })

for f in os.listdir(dump_dir):
    print("------->", f)
    wipeout_server(session)
    e = None
    for ex in expected:
        if ex.pattern.fullmatch(f):
            e = ex
            break
    if e is None:
        testutil.fail(f"File is not handled by the test: {f}")
        continue
    p = os.path.join(dump_dir, f)
    os.rename(p, p + ".bak")
    if e.success:
        EXPECT_NO_THROWS(l, f"Load should not fail when file {f} is missing")
    else:
        EXPECT_THROWS(l, e.msg)
    os.rename(p + ".bak", p)

#@<> BUG#35822020 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])

#@<> WL15947 - setup
schema_name = "wl15947"
test_table_primary = "pk"
test_table_unique = "uni"
test_table_unique_null = "uni-null"
test_table_no_index = "no-index"
test_table_partitioned = "part"
test_table_empty = "empty"

def setup_db():
    session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
    session.run_sql("CREATE SCHEMA !", [schema_name])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT NOT NULL PRIMARY KEY)", [ schema_name, test_table_primary ])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT NOT NULL UNIQUE KEY)", [ schema_name, test_table_unique ])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT UNIQUE KEY)", [ schema_name, test_table_unique_null ])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT)", [ schema_name, test_table_no_index ])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT NOT NULL PRIMARY KEY) PARTITION BY RANGE (`id`) (PARTITION p0 VALUES LESS THAN (500), PARTITION p1 VALUES LESS THAN (1000), PARTITION p2 VALUES LESS THAN MAXVALUE);", [ schema_name, test_table_partitioned ])
    session.run_sql("CREATE TABLE !.! (`id` MEDIUMINT)", [ schema_name, test_table_empty ])
    session.run_sql(f"INSERT INTO !.! (`id`) VALUES {','.join(f'({i})' for i in range(1500))}", [ schema_name, test_table_primary ])
    session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_unique, schema_name, test_table_primary ])
    session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_unique_null, schema_name, test_table_primary ])
    session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_no_index, schema_name, test_table_primary ])
    session.run_sql("INSERT INTO !.! SELECT * FROM !.!;", [ schema_name, test_table_partitioned, schema_name, test_table_primary ])
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_primary ])
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_unique ])
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_unique_null ])
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_no_index ])
    session.run_sql("ANALYZE TABLE !.!;", [ schema_name, test_table_partitioned ])

def TEST_BOOL_OPTION(option):
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { option: None }), f"TypeError: Util.load_dump: Argument #2: Option '{option}' is expected to be of type Bool, but is Null")
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { option: "dummy" }), f"TypeError: Util.load_dump: Argument #2: Option '{option}' Bool expected, but value is String")
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { option: [] }), f"TypeError: Util.load_dump: Argument #2: Option '{option}' is expected to be of type Bool, but is Array")
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { option: {} }), f"TypeError: Util.load_dump: Argument #2: Option '{option}' is expected to be of type Bool, but is Map")

dump_dir = os.path.join(outdir, "wl15947")
no_data_dump_dir = dump_dir + "-nodata"

shell.connect(__sandbox_uri1)
setup_db()
util.dump_schemas([ "xtest", schema_name ], dump_dir, { "checksum": True, "where": { quote_identifier(schema_name, test_table_no_index): "id > 100" }, "showProgress": False })
util.dump_schemas([ "xtest", schema_name ], no_data_dump_dir, { "ddlOnly": True, "checksum": True, "showProgress": False })
shell.connect(__sandbox_uri2)

#@<> WL15947-TSFR_2_1 - help text
help_text = """
      - checksum: bool (default: false) - Verify tables against checksums that
        were computed during dump.
"""
EXPECT_TRUE(help_text in util.help("load_dump"))

#@<> WL15947-TSFR_2_1_1 - load without checksum option
wipeout_server(session2)
util.load_dump(dump_dir, { "resetProgress": True, "showProgress": False })
EXPECT_STDOUT_NOT_CONTAINS("checksum")

#@<> WL15947-TSFR_2_1_1 - load with checksum option set to False
wipeout_server(session2)
util.load_dump(dump_dir, { "checksum": False, "resetProgress": True, "showProgress": False })
EXPECT_STDOUT_NOT_CONTAINS("checksum")

#@<> WL15947-TSFR_2_1_2 - option type
TEST_BOOL_OPTION("checksum")

#@<> WL15947-TSFR_2_2_2 - manipulate checksum to contain data errors, load with dryRun
wipeout_server(session2)

checksum_file = checksum_file_path(dump_dir)
checksums = read_json(checksum_file)
checksums["data"][schema_name][test_table_primary]["partitions"][""]["0"]["checksum"] = "error"

with backup_file(checksum_file) as backup:
    write_json(checksum_file, checksums)
    backup.callback(lambda: os.remove(checksum_file))
    EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "dryRun": True, "checksum": True, "resetProgress": True, "showProgress": False }))
    # WL15947-TSFR_2_4_2_1
    EXPECT_STDOUT_CONTAINS("WARNING: Checksum information is not going to be verified, dryRun enabled.")
    # regular run - throws
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "checksum": True, "resetProgress": True, "showProgress": False }), "Error: Shell Error (53031): Util.load_dump: Checksum verification failed")

#@<> WL15947-TSFR_2_3_1 - file-related errors
wipeout_server(session2)

checksum_file = checksum_file_path(dump_dir)
checksums = read_json(checksum_file)

with backup_file(checksum_file) as backup:
    # no progress file
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "checksum": True, "resetProgress": True, "showProgress": False }), "RuntimeError: Util.load_dump: Cannot open file")
    # no read access
    if __os_type != "windows":
        with ExitStack() as stack:
            write_json(checksum_file, checksums)
            stack.callback(lambda: os.remove(checksum_file))
            os.chmod(checksum_file, 0o060)
            EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "checksum": True, "resetProgress": True, "showProgress": False }), "RuntimeError: Util.load_dump: Cannot open file")
    # corrupted file
    write_json(checksum_file, checksums)
    backup.callback(lambda: os.remove(checksum_file))
    with open(checksum_file, "a", encoding="utf-8") as f:
        f.write("error!")
    EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "checksum": True, "resetProgress": True, "showProgress": False }), "RuntimeError: Util.load_dump: Failed to parse")

#@<> WL15947 - load dump with no data
test = lambda: util.load_dump(no_data_dump_dir, { "checksum": True, "loadDdl": False, "resetProgress": True, "showProgress": False })

# load the full dump first
# WL15947-TSFR_2_2_1 - load data dumped with 'where'
# WL15947-TSFR_2_4_3 - load an empty table
wipeout_server(session2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "checksum": True, "resetProgress": True, "showProgress": False }))
# BUG#35983655 - display duration of checksum verification
EXPECT_STDOUT_CONTAINS(" checksums were verified in ")

# WL15947-TSFR_2_4_2 - checksum existing tables without loading the data
# this throws because both dumps were created with different contents (one had `where` option)
EXPECT_THROWS(test, "Shell Error (53031): Util.load_dump: Checksum verification failed")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_no_index}`. Mismatched number of rows, expected: 1500, actual: 1399.")

# WL15947-TSFR_2_4_1 - tables are empty - checksum errors
for table in [ test_table_primary, test_table_unique, test_table_unique_null, test_table_no_index, test_table_partitioned ]:
    session2.run_sql("TRUNCATE TABLE !.!", [ schema_name, table ])

WIPE_OUTPUT()
EXPECT_THROWS(test, "Error: Shell Error (53031): Util.load_dump: Checksum verification failed")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_primary}`. Mismatched number of rows, expected: 1500, actual: 0.")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_unique}`. Mismatched number of rows, expected: 1500, actual: 0.")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_unique_null}`. Mismatched number of rows, expected: 1500, actual: 0.")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_no_index}`. Mismatched number of rows, expected: 1500, actual: 0.")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p0`. Mismatched number of rows, expected: 500, actual: 0.")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p1`. Mismatched number of rows, expected: 500, actual: 0.")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p2`. Mismatched number of rows, expected: 500, actual: 0.")
EXPECT_STDOUT_CONTAINS("ERROR: 7 checksum verification errors were reported during the load.")

# WL15947-TSFR_2_4_1_1 - tables are missing
for table in [ test_table_unique, test_table_no_index ]:
    session2.run_sql("DROP TABLE !.!", [ schema_name, table ])

WIPE_OUTPUT()
EXPECT_THROWS(test, "Error: Shell Error (53031): Util.load_dump: Checksum verification failed")
EXPECT_STDOUT_CONTAINS(f"ERROR: Could not verify checksum of `{schema_name}`.`{test_table_unique}`: table does not exist")
EXPECT_STDOUT_CONTAINS(f"ERROR: Could not verify checksum of `{schema_name}`.`{test_table_no_index}`: table does not exist")
EXPECT_STDOUT_CONTAINS("ERROR: 7 checksum verification errors were reported during the load.")

#@<> WL15947 - checking checksum without loading the data
test = lambda: util.load_dump(dump_dir, { "checksum": True, "loadData": False, "loadDdl": False, "resetProgress": True, "showProgress": False })

# load the full dump first
# WL15947-TSFR_2_2_1 - load data dumped with 'where'
# WL15947-TSFR_2_4_3 - load an empty table
wipeout_server(session2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "checksum": True, "resetProgress": True, "showProgress": False }))

# WL15947-TSFR_2_4_2 - checksum existing tables without loading the data
# WL15947-TSFR_2_4_1_2 - load with data present works
EXPECT_NO_THROWS(test)

# WL15947-TSFR_2_4_1 - tables are empty - checksum errors
for table in [ test_table_primary, test_table_unique, test_table_unique_null, test_table_no_index, test_table_partitioned ]:
    session2.run_sql("TRUNCATE TABLE !.!", [ schema_name, table ])
WIPE_OUTPUT()

EXPECT_THROWS(test, "Error: Shell Error (53031): Util.load_dump: Checksum verification failed")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_primary}` (chunk 0)")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_unique}` (chunk 0)")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_unique_null}` (chunk 0)")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_no_index}`")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p0` (chunk 0)")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p1` (chunk 0)")
EXPECT_STDOUT_CONTAINS(f"ERROR: Checksum verification failed for: `{schema_name}`.`{test_table_partitioned}` partition `p2` (chunk 0)")
EXPECT_STDOUT_CONTAINS("ERROR: 7 checksum verification errors were reported during the load.")

# WL15947-TSFR_2_4_1_1 - tables are missing
for table in [ test_table_unique, test_table_no_index ]:
    session2.run_sql("DROP TABLE !.!", [ schema_name, table ])

WIPE_OUTPUT()
EXPECT_THROWS(test, "Error: Shell Error (53031): Util.load_dump: Checksum verification failed")
EXPECT_STDOUT_CONTAINS(f"ERROR: Could not verify checksum of `{schema_name}`.`{test_table_unique}` (chunk 0): table does not exist")
EXPECT_STDOUT_CONTAINS(f"ERROR: Could not verify checksum of `{schema_name}`.`{test_table_no_index}`: table does not exist")
EXPECT_STDOUT_CONTAINS("ERROR: 7 checksum verification errors were reported during the load.")

#@<> WL15947 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !;", [schema_name])

#@<> BUG#35830920 mysql_audit and mysql_firewall schemas should be automatically excluded when loading a dump into MHS - setup {not __dbug_off}
# BUG#37023079 - exclude mysql_option schema
# BUG#37278169 - exclude mysql_autopilot schema
# BUG#37637843 - exclude `mysql_rest_service_metadata` and `mysql_tasks` schemas
# create schemas
schema_names = [ "mysql_audit", "mysql_autopilot", "mysql_firewall", "mysql_option", "mysql_rest_service_metadata", "mysql_tasks" ]

def create_mhs_schemas(s):
    for schema_name in schema_names:
        s.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
        s.run_sql("CREATE SCHEMA !", [schema_name])
        s.run_sql("CREATE TABLE !.! (a int)", [schema_name, "test_table"])

create_mhs_schemas(session1)
wipeout_server(session2)
create_mhs_schemas(session2)

# create a MHS-compatible dump but without 'ocimds' option
shell.connect(__sandbox_uri1)
dump_dir = os.path.join(outdir, "bug_35830920")
EXPECT_NO_THROWS(lambda: util.dump_schemas(schema_names, dump_dir, { "ddlOnly": True, "showProgress": False }), "Dumping the instance should not fail")

#@<> BUG#35830920 - test {not __dbug_off}
shell.connect(__sandbox_uri2)

# loading into non-MHS should fail, because schemas already exist
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "Duplicate objects found in destination database")
testutil.rmfile(os.path.join(dump_dir, "load-progress*.json"))

# loading into MHS should automatically exclude the schemas and operation should succeed
testutil.dbug_set("+d,dump_loader_force_mds")
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "ignoreVersion": True, "showProgress": False }), "Loading should not fail")
testutil.dbug_set("")

#@<> BUG#35830920 - cleanup {not __dbug_off}
for schema_name in schema_names:
    session.run_sql("DROP SCHEMA !;", [schema_name])

#@<> BUG#35860654 - cannot dump a table with single generated column
# constants
dump_dir = os.path.join(outdir, "bug_35860654")
tested_schema = "tested_schema"
tested_table = "tested_table"

# setup
session1.run_sql("DROP SCHEMA IF EXISTS !", [ tested_schema ])
session1.run_sql("CREATE SCHEMA !", [ tested_schema ])
session1.run_sql("CREATE TABLE !.! (a date GENERATED ALWAYS AS (50399) STORED)", [ tested_schema, tested_table ])

#@<> BUG#35860654 - dumping with ocimds should fail, complaining that table doesn't have a PK
shell.connect(__sandbox_uri1)
EXPECT_THROWS(lambda: util.dump_schemas([ tested_schema ], dump_dir, { "ocimds": True, "showProgress": False }), "Compatibility issues were found")
EXPECT_STDOUT_CONTAINS(create_invisible_pks(tested_schema, tested_table).error())
wipe_dir(dump_dir)

#@<> BUG#35860654 - test
shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: util.dump_schemas([ tested_schema ], dump_dir, { "ocimds": True, "compatibility": [ "create_invisible_pks" ], "showProgress": False }), "dump should not throw")
EXPECT_STDOUT_CONTAINS(create_invisible_pks(tested_schema, tested_table).fixed())

shell.connect(__sandbox_uri2)
wipeout_server(session)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "createInvisiblePKs": True if __version_num > 80024 else False,"showProgress": False }), "load should not throw")

#@<> BUG#35860654 - cleanup
session1.run_sql("DROP SCHEMA IF EXISTS !", [ tested_schema ])

#@<> BUG#36119568 - load fails on Windows if global sql_mode is set to STRICT_ALL_TABLES
# constants
dump_dir = os.path.join(outdir, "bug_36119568")

# dump
shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "showProgress": False }), "Dump should not fail")

# load
shell.connect(__sandbox_uri2)
wipeout_server(session2)
session.run_sql("SET @saved_sql_mode = @@global.sql_mode")
session.run_sql("SET @@global.sql_mode = STRICT_ALL_TABLES")

EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "Load should not fail")

# cleanup
session.run_sql("SET @@global.sql_mode = @saved_sql_mode")

#@<> BUG#36127633 - reconnect session when connection is lost {not __dbug_off}
# setup
tested_schema = "test_schema"
tested_table = "test_table"
dump_dir = os.path.join(outdir, "bug_36127633")

shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA !", [tested_schema])
session.run_sql("""CREATE TABLE !.! (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    data BIGINT
)""", [ tested_schema, tested_table ])

# dump data
util.dump_instance(dump_dir, { "includeSchemas": [ tested_schema ], "users": False, "showProgress": False })

# connect to the destination server
shell.connect(__sandbox_uri2)
# wipe the destination server
wipeout_server(session2)
# trap on a comment that's a line in all of the SQL files, trap will hit after the third file (order of SQL files is: pre, schema, table, post)
testutil.set_trap("mysql", ["sql regex -- -+", "++match_counter > 3"], { "code": 2013, "msg": "Server lost", "state": "HY000" })

# try to load, comment fails each time, load fails as well
WIPE_OUTPUT()
WIPE_SHELL_LOG()
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "DBError: MySQL Error (2013): Util.load_dump: Server lost")
EXPECT_STDOUT_CONTAINS("ERROR: While executing postamble SQL: MySQL Error 2013 (HY000): Server lost")
# log mentions reconnections
EXPECT_SHELL_LOG_CONTAINS("Session disconnected: ")

testutil.clear_traps("mysql")

# wipe the destination server once again
wipeout_server(session2)
# trap on a comment that's a line in all of the SQL files, trap will hit on the fourth file, once
testutil.set_trap("mysql", ["sql regex -- -+", "++match_counter == 4"], { "code": 2013, "msg": "Server lost", "state": "HY000" })

# try to load, comment fails once, but then succeeds
WIPE_OUTPUT()
WIPE_SHELL_LOG()
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "resetProgress": True, "showProgress": False }), "load should succeed")
# log mentions reconnections
EXPECT_SHELL_LOG_CONTAINS("Session disconnected: ")

testutil.clear_traps("mysql")

# cleanup
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])

#@<> BUG#37593239 - load hanged when rebuilding indexes if user running the load didn't have SELECT privilege on performance_schema
# constants
dump_dir = os.path.join(outdir, "bug_37593239")
tested_schema = "tested_schema"
tested_table = "tested_table"

# setup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])
session.run_sql("CREATE SCHEMA !", [tested_schema])
session.run_sql("CREATE TABLE !.! (`id` int NOT NULL AUTO_INCREMENT, `text` longtext NOT NULL, PRIMARY KEY (`id`))", [ tested_schema, tested_table ])

session.run_sql("INSERT INTO !.! VALUES (null,'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA')", [ tested_schema, tested_table ])
session.run_sql("UPDATE !.! SET text = repeat(text,100) where id = 1", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (null,'BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB')", [ tested_schema, tested_table ])
session.run_sql("UPDATE !.! SET text = repeat(text,1000) where id = 2", [ tested_schema, tested_table ])
session.run_sql("INSERT INTO !.! VALUES (null,'CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC')", [ tested_schema, tested_table ])
session.run_sql("UPDATE !.! SET text = repeat(text,10000) where id = 3", [ tested_schema, tested_table ])

session.run_sql("ALTER TABLE !.! ADD FULLTEXT KEY `ftsidx` (`text`)", [ tested_schema, tested_table ])

# dump
shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: util.dump_schemas([tested_schema], dump_dir, { "showProgress": False }), "Dump should not fail")

# create the test user
shell.connect(__sandbox_uri2)
wipeout_server(session)

session.run_sql(f"CREATE USER {test_user_account} IDENTIFIED BY ?", [test_user_pwd])
session.run_sql(f"GRANT FILE ON *.* TO {test_user_account}")
session.run_sql(f"GRANT ALL ON !.* TO {test_user_account}", [tested_schema])

# load
shell.connect(test_user_uri(__mysql_sandbox_port2))
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "deferTableIndexes": "all", "showProgress": True }), "Load should not fail")

# verification
compare_schema(session1, session2, tested_schema, check_rows=True)

#@<> BUG#37593239 - cleanup
shell.connect(__sandbox_uri1)
session.run_sql("DROP SCHEMA IF EXISTS !", [tested_schema])

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
wipe_dir(outdir)
