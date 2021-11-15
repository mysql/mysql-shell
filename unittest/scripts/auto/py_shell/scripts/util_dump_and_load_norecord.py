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

outdir = __tmp_dir+"/ldtest"
try:
    testutil.rmdir(outdir, True)
except:
    pass
testutil.mkdir(outdir)

mysql_tmpdir = __tmp_dir+"/mysql"
try:
    testutil.rmdir(mysql_tmpdir, True)
except:
    pass
testutil.mkdir(mysql_tmpdir)

def prepare(sbport, options={}):
    # load test schemas
    uri = "mysql://root:root@localhost:%s" % sbport
    datadir = __tmp_dir+"/ldtest/datadirs%s" % sbport
    testutil.mkdir(datadir)
    # 8.0 seems to create dirs automatically but not 5.7
    if __version_num < 80000:
        testutil.mkdir(datadir+"/testindexdir")
        testutil.mkdir(datadir+"/test datadir")
    options.update({
        "loose_innodb_directories": datadir,
        "early_plugin_load": "keyring_file."+("dll" if __os_type == "windows" else "so"),
        "keyring_file_data": datadir+"/keyring",
        "local_infile": "1",
        "tmpdir": mysql_tmpdir,
        # small sort buffer to force stray filesorts to be triggered even if we don't have much data
        "sort_buffer_size": 32768
    })
    testutil.deploy_sandbox(sbport, "root", options)


prepare(__mysql_sandbox_port1)
session1 = mysql.get_session(__sandbox_uri1)
session1.run_sql("set names utf8mb4")
session1.run_sql("create schema world")
testutil.import_data(__sandbox_uri1, __data_path+"/sql/world.sql", "world")
testutil.import_data(__sandbox_uri1, __data_path+"/sql/misc_features.sql")
session1.run_sql("create schema tests")
# BUG#33079172 "SQL SECURITY INVOKER" INSERTED AT WRONG LOCATION
session1.run_sql("create procedure tests.tmp() IF @v > 0 THEN SELECT 1; ELSE SELECT 2; END IF;;")

prepare(__mysql_sandbox_port2)
session2 = mysql.get_session(__sandbox_uri2)
session2.run_sql("set names utf8mb4")


## Tests to ensure restricted users dumped with strip_restricted_grants can be loaded with a restricted user and not just with root

#@<> ensure accounts dumped in compat mode can be loaded {VER(>=8.0.0) and not __dbug_off}
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

def format_rows(rows):
    return "\n".join([r[0] for r in rows])

shell.connect(__sandbox_uri2)

# dump with root
util.dump_instance(__tmp_dir+"/ldtest/dump_root", {"compatibility":["strip_restricted_grants", "strip_definers"], "ocimds":True})

# CREATE USER for role is converted to CREATE ROLE
EXPECT_FILE_MATCHES(re.compile(r"CREATE ROLE IF NOT EXISTS ['`]administrator['`]@['`]%['`]"), os.path.join(__tmp_dir, "ldtest", "dump_root", "@.users.sql"))

# dump with admin user
shell.connect(f"mysql://admin:pass@localhost:{__mysql_sandbox_port2}")
# disable consistency b/c we won't be able to acquire a backup lock
util.dump_instance(__tmp_dir+"/ldtest/dump_admin", {"compatibility":["strip_restricted_grants", "strip_definers"], "ocimds":True, "consistent":False})

# CREATE USER for role is converted to CREATE ROLE
EXPECT_FILE_MATCHES(re.compile(r"CREATE ROLE IF NOT EXISTS ['`]administrator['`]@['`]%['`]"), os.path.join(__tmp_dir, "ldtest", "dump_admin", "@.users.sql"))

# load with admin user
reset_server(session2)
util.load_dump(__tmp_dir+"/ldtest/dump_admin", {"loadUsers":1, "loadDdl":0, "loadData":0, "excludeUsers":["root@%","root@localhost"]})

EXPECT_EQ("""GRANT USAGE ON *.* TO `myuser`@`%`
GRANT SELECT, SHOW VIEW ON `mysql`.* TO `myuser`@`%`
GRANT ALL PRIVILEGES ON `test_schema`.* TO `myuser`@`%`""",
        format_rows(session2.run_sql("show grants for myuser@'%'").fetch_all()))

EXPECT_EQ("""GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO `myuser2`@`%`
GRANT APPLICATION_PASSWORD_ADMIN,CONNECTION_ADMIN,REPLICATION_APPLIER,RESOURCE_GROUP_ADMIN,RESOURCE_GROUP_USER,XA_RECOVER_ADMIN ON *.* TO `myuser2`@`%`
REVOKE INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `mysql`.* FROM `myuser2`@`%`
REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `myuser2`@`%`""",
          format_rows(session2.run_sql("show grants for myuser2@'%'").fetch_all()))

EXPECT_EQ("""GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT APPLICATION_PASSWORD_ADMIN,CONNECTION_ADMIN,REPLICATION_APPLIER,RESOURCE_GROUP_ADMIN,RESOURCE_GROUP_USER,XA_RECOVER_ADMIN ON *.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT SELECT, SHOW VIEW ON `mysql`.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT ALL PRIVILEGES ON `test_schema`.* TO `myuser3`@`%` WITH GRANT OPTION
REVOKE INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `mysql`.* FROM `myuser3`@`%`
REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `myuser3`@`%`""",
          format_rows(session2.run_sql("show grants for myuser3@'%'").fetch_all()))

reset_server(session2)
util.load_dump(__tmp_dir+"/ldtest/dump_root", {"loadUsers":1, "loadDdl":0, "loadData":0, "excludeUsers":["root@%","root@localhost"]})

EXPECT_EQ("""GRANT USAGE ON *.* TO `myuser`@`%`
GRANT SELECT, SHOW VIEW ON `mysql`.* TO `myuser`@`%`
GRANT ALL PRIVILEGES ON `test_schema`.* TO `myuser`@`%`""",
          format_rows(session2.run_sql("show grants for myuser@'%'").fetch_all()))

EXPECT_EQ("""GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO `myuser2`@`%`
GRANT APPLICATION_PASSWORD_ADMIN,CONNECTION_ADMIN,REPLICATION_APPLIER,RESOURCE_GROUP_ADMIN,RESOURCE_GROUP_USER,XA_RECOVER_ADMIN ON *.* TO `myuser2`@`%`
REVOKE INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `mysql`.* FROM `myuser2`@`%`
REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `myuser2`@`%`""",
          format_rows(session2.run_sql("show grants for myuser2@'%'").fetch_all()))

EXPECT_EQ("""GRANT SELECT, INSERT, UPDATE, DELETE, CREATE, DROP, PROCESS, REFERENCES, INDEX, ALTER, SHOW DATABASES, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, REPLICATION SLAVE, REPLICATION CLIENT, CREATE VIEW, SHOW VIEW, CREATE ROUTINE, ALTER ROUTINE, CREATE USER, EVENT, TRIGGER, CREATE ROLE, DROP ROLE ON *.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT APPLICATION_PASSWORD_ADMIN,CONNECTION_ADMIN,REPLICATION_APPLIER,RESOURCE_GROUP_ADMIN,RESOURCE_GROUP_USER,XA_RECOVER_ADMIN ON *.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT SELECT, SHOW VIEW ON `mysql`.* TO `myuser3`@`%` WITH GRANT OPTION
GRANT ALL PRIVILEGES ON `test_schema`.* TO `myuser3`@`%` WITH GRANT OPTION
REVOKE INSERT, UPDATE, DELETE, CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, EXECUTE, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `mysql`.* FROM `myuser3`@`%`
REVOKE CREATE, DROP, REFERENCES, INDEX, ALTER, CREATE TEMPORARY TABLES, LOCK TABLES, CREATE VIEW, CREATE ROUTINE, ALTER ROUTINE, EVENT, TRIGGER ON `sys`.* FROM `myuser3`@`%`""",
          format_rows(session2.run_sql("show grants for myuser3@'%'").fetch_all()))

testutil.dbug_set("")

reset_server(session2)

#@<> Dump ddlOnly
shell.connect(__sandbox_uri1)
util.dump_instance(outdir+"/ddlonly", {"ddlOnly": True})

shell.connect(__sandbox_uri2)

#@<> load data which is not in the dump (fail)
EXPECT_THROWS(lambda: util.load_dump(outdir+"/ddlonly",
                                     {"loadData": True, "loadDdl": False, "excludeSchemas":["all_features","all_features2"]}), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_MATCHES(re.compile(r"ERROR: \[Worker00\d\] While executing DDL script for `.+`\.`.+`: Unknown database 'world'"))

testutil.rmfile(outdir+"/ddlonly/load-progress*.json")

#@<> load ddl normally
util.load_dump(outdir+"/ddlonly")

compare_servers(session1, session2, check_rows=False, check_users=False)
#@<> Dump dataOnly
shell.connect(__sandbox_uri1)
util.dump_instance(outdir+"/dataonly", {"dataOnly": True})

shell.connect(__sandbox_uri2)

#@<> load data assuming tables already exist

# will fail because tables already exist
EXPECT_THROWS(lambda: util.load_dump(outdir+"/dataonly",
                                     {"dryRun": True}), "RuntimeError: Util.load_dump: Duplicate objects found in destination database")

# will pass because we're explicitly only loading data
util.load_dump(outdir+"/dataonly", {"dryRun": True, "loadDdl": False})

util.load_dump(outdir+"/dataonly", {"ignoreExistingObjects": True})

compare_servers(session1, session2, check_users=False)

wipeout_server(session2)


#@<> load ddl which is not in the dump (fail/no-op)
util.load_dump(outdir+"/dataonly", {"loadData": False, "loadDdl": True})


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

session.run_sql("GRANT SELECT ON TABLE schema1.table1 TO uuuser@localhost")
session.run_sql("GRANT EXECUTE ON FUNCTION schema1.myfun1 TO uuuser@localhost")
session.run_sql("GRANT ALTER ROUTINE ON PROCEDURE schema1.mysp1 TO uuuser@localhost")
session.run_sql("GRANT ALTER ON schema2.* TO uuuser@localhost")

dump_dir = os.path.join(outdir, "user_load_order")
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir), "Dump")

shell.connect(__sandbox_uri2)
wipeout_server(session2)

EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, {"loadUsers":True, "excludeUsers":["root@%"]}), "Load")

compare_servers(session1, session2)

#@<> BUG#33406711 - filtering objects should also filter relevant privileges
# setup
expected_accounts = snapshot_accounts(session1)
del expected_accounts["root@%"]
dump_dir = os.path.join(outdir, "bug_33406711")

# dump excluding one of the schemas, load with no filters, schema-related privilege is not present
wipeout_server(session2)

shell.connect(__sandbox_uri1)
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "excludeSchemas": [ "schema2" ], "showProgress": False }), "Dump")

shell.connect(__sandbox_uri2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "loadUsers": True, "excludeUsers": [ "root@%" ], "showProgress": False }), "Load")

# privileges for schema2 are not included
expected_accounts["uuuser@localhost"]["grants"] = [grant for grant in expected_accounts["uuuser@localhost"]["grants"] if "schema2" not in grant]
# root is not re-created
actual_accounts = snapshot_accounts(session2)
del actual_accounts["root@%"]
# validation
EXPECT_EQ(expected_accounts, actual_accounts)

# use the same dump, exclude a table when loading
wipeout_server(session2)
testutil.rmfile(os.path.join(dump_dir, "load-progress*.json"))

shell.connect(__sandbox_uri2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "excludeTables": [ "schema1.table1" ], "loadUsers": True, "excludeUsers": [ "root@%" ], "showProgress": False }), "Load")
EXPECT_STDOUT_MATCHES(re.compile(r"NOTE: Filtered statement with excluded database object: GRANT SELECT ON `schema1`.`table1` TO ['`]uuuser['`]@['`]localhost['`]; -> \(skipped\)"))

# privileges for schema1.table1 are not included
expected_accounts["uuuser@localhost"]["grants"] = [grant for grant in expected_accounts["uuuser@localhost"]["grants"] if "table1" not in grant]
# root is not re-created
actual_accounts = snapshot_accounts(session2)
del actual_accounts["root@%"]
# validation
EXPECT_EQ(expected_accounts, actual_accounts)

# use the same dump, exclude a table and a routine when loading
wipeout_server(session2)
testutil.rmfile(os.path.join(dump_dir, "load-progress*.json"))

shell.connect(__sandbox_uri2)
EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "excludeRoutines": [ "schema1.myfun1" ], "excludeTables": [ "schema1.table1" ], "loadUsers": True, "excludeUsers": [ "root@%" ], "showProgress": False }), "Load")
EXPECT_STDOUT_MATCHES(re.compile(r"NOTE: Filtered statement with excluded database object: GRANT SELECT ON `schema1`.`table1` TO ['`]uuuser['`]@['`]localhost['`]; -> \(skipped\)"))
EXPECT_STDOUT_MATCHES(re.compile(r"NOTE: Filtered statement with excluded database object: GRANT EXECUTE ON FUNCTION `schema1`.`myfun1` TO ['`]uuuser['`]@['`]localhost['`]; -> \(skipped\)"))

# privileges for schema1.myfun1 are not included
expected_accounts["uuuser@localhost"]["grants"] = [grant for grant in expected_accounts["uuuser@localhost"]["grants"] if "myfun1" not in grant]
# root is not re-created
actual_accounts = snapshot_accounts(session2)
del actual_accounts["root@%"]
# validation
EXPECT_EQ(expected_accounts, actual_accounts)

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
# account which is excluded when loading into MDS
session.run_sql("CREATE USER IF NOT EXISTS 'ocimonitor'@'localhost' IDENTIFIED BY 'pwd';")

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
def EXPECT_INCLUDE_EXCLUDE(options, included, excluded, expected_exception=None):
    opts = { "loadUsers": True, "showProgress": False, "resetProgress": True }
    opts.update(options)
    shell.connect(__sandbox_uri2)
    try:
        reset_server(session)
    except NameError:
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
        EXPECT_STDOUT_NOT_CONTAINS("NOTE: Skipping GRANT statements for user {0}".format(i))
    for e in excluded:
        EXPECT_TRUE(e not in all_users, "User {0} should not exist".format(e))
        EXPECT_STDOUT_CONTAINS("NOTE: Skipping CREATE/ALTER USER statements for user {0}".format(e))
        EXPECT_STDOUT_CONTAINS("NOTE: Skipping GRANT statements for user {0}".format(e))
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
EXPECT_STDOUT_CONTAINS("NOTE: Skipping GRANT statements for user 'mysql.sys'@'localhost'")

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

#@<> include and exclude the same username, no accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first"], "excludeUsers": ["first"] }, [], ["'first'@'localhost'", "'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

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

#@<> include using two usernames, exclude one of them, two accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second"], "excludeUsers": ["second"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'"], ["'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using two usernames, exclude one of the accounts, three accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second"], "excludeUsers": ["second@localhost"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'", "'second'@'10.11.12.14'"], ["'firstfirst'@'localhost'", "'second'@'localhost'"])

#@<> include using an username and an account, three accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second@localhost"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'", "'second'@'localhost'"], ["'firstfirst'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using an username and an account, exclude using username, two accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second@localhost"], "excludeUsers": ["second"]  }, ["'first'@'localhost'", "'first'@'10.11.12.13'"], ["'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using an username and an account, exclude using an account, two accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "second@localhost"], "excludeUsers": ["second@localhost"]  }, ["'first'@'localhost'", "'first'@'10.11.12.13'"], ["'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> include using an username and non-existing username, exclude using a non-existing username, two accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["first", "third"], "excludeUsers": ["fourth"]  }, ["'first'@'localhost'", "'first'@'10.11.12.13'"], ["'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> don't include or exclude anything, mysql.sys is always excluded (and it already exists)
EXPECT_INCLUDE_EXCLUDE({ "ignoreExistingObjects": True }, [], [])
EXPECT_STDOUT_CONTAINS("NOTE: Skipping CREATE/ALTER USER statements for user 'mysql.sys'@'localhost'")
EXPECT_STDOUT_CONTAINS("NOTE: Skipping GRANT statements for user 'mysql.sys'@'localhost'")

#@<> don't include or exclude anything when loading into MDS, ocimonitor is always excluded {VER(>=8.0.0) and not __dbug_off}
testutil.dbug_set("+d,dump_loader_force_mds")

EXPECT_INCLUDE_EXCLUDE({ "ignoreExistingObjects": True, 'ignoreVersion': True }, [], ["'ocimonitor'@'localhost'"])
EXPECT_STDOUT_CONTAINS("NOTE: Skipping CREATE/ALTER USER statements for user 'mysql.sys'@'localhost'")
EXPECT_STDOUT_CONTAINS("NOTE: Skipping GRANT statements for user 'mysql.sys'@'localhost'")

testutil.dbug_set("")

#@<> cleanup tests with include/exclude users
shell.connect(__sandbox_uri1)
session.run_sql("DROP USER 'first'@'localhost';")
session.run_sql("DROP USER 'first'@'10.11.12.13';")
session.run_sql("DROP USER 'firstfirst'@'localhost';")
session.run_sql("DROP USER 'second'@'localhost';")
session.run_sql("DROP USER 'second'@'10.11.12.14';")
session.run_sql("DROP USER 'mysql.sys-ex'@'localhost';")
session.run_sql("DROP USER 'ocimonitor'@'localhost';")

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
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir, { "ocimds": True, "compatibility": ["ignore_missing_pks", "strip_restricted_grants", "strip_definers"], "ddlOnly": True, "showProgress": False }), "Dumping the instance should not fail")

# when loading into non-MDS instance, if skipBinlog is set, but user doesn't have required privileges, exception should be thrown
wipeout_server(session2)

session2.run_sql("CREATE USER IF NOT EXISTS admin@'%' IDENTIFIED BY 'pass'")
session2.run_sql("GRANT ALL ON *.* TO 'admin'@'%'")
sql_log_bin_privileges = [ 'SUPER' ]
if __version_num >= 80000:
    sql_log_bin_privileges.append('SYSTEM_VARIABLES_ADMIN')
if __version_num >= 80014:
    sql_log_bin_privileges.append('SESSION_VARIABLES_ADMIN')
session2.run_sql("REVOKE {0} ON *.* FROM 'admin'@'%'".format(", ".join(sql_log_bin_privileges)))

shell.connect("mysql://admin:pass@{0}:{1}".format(__host, __mysql_sandbox_port2))

# join all but the last privilege with ', ', append ' or ' if there is more than one privilege, append the last privilege
missing_privileges = ', '.join(sql_log_bin_privileges[:-1]) + (' or ' if len(sql_log_bin_privileges) > 1 else '') + sql_log_bin_privileges[-1]
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "skipBinlog": True, "showProgress": False, "resetProgress": True  }), "RuntimeError: Util.load_dump: 'SET sql_log_bin=0' failed with error - MySQL Error 1227 (42000): Access denied; you need (at least one of) the {0} privilege(s) for this operation".format(missing_privileges))

# when loading into MDS instance, if skipBinlog is set, but user doesn't have required privileges, exception should be thrown
testutil.dbug_set("+d,dump_loader_force_mds")

EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "skipBinlog": True, "showProgress": False, "resetProgress": True  }), "RuntimeError: Util.load_dump: 'SET sql_log_bin=0' failed with error - MySQL Error 1227 (42000): Access denied; you need (at least one of) the {0} privilege(s) for this operation".format(missing_privileges))

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

EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "showProgress": False }), "RuntimeError: Util.load_dump: Dump is not MDS compatible")
EXPECT_STDOUT_CONTAINS("ERROR: Destination is a MySQL Database Service instance but the dump was produced without the compatibility option. Please enable the 'ocimds' option when dumping your database. Alternatively, enable the 'ignoreVersion' option to load anyway.")

# loading non-MDS dump into MDS with the 'ignoreVersion' option enabled should succeed
wipeout_server(session2)
WIPE_OUTPUT()

EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, { "ignoreVersion": True, "showProgress": False }), "Loading with 'ignoreVersion' should succeed")
EXPECT_STDOUT_CONTAINS("WARNING: Destination is a MySQL Database Service instance but the dump was produced without the compatibility option. The 'ignoreVersion' option is enabled, so loading anyway. If this operation fails, create the dump once again with the 'ocimds' option enabled.")

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
EXPECT_STDOUT_CONTAINS("WARNING: The dump contains tables without Primary Keys and it is loaded with the 'createInvisiblePKs' option set to false, this dump cannot be loaded into an MySQL Database Service instance with High Availability.")

# all tables have primary keys, no warning
EXPECT_PK(dump_just_pk_dir, { "createInvisiblePKs": False, "ignoreVersion": True }, False)
EXPECT_STDOUT_NOT_CONTAINS("createInvisiblePKs")

testutil.dbug_set("")

#@<> WL14506-FR4.5 - If the createInvisiblePKs option is set to true and the dump which contains tables without primary keys is loaded into MDS, a warning must be reported stating that Inbound Replication into an MDS HA instance cannot be used with this dump and the load process must continue. {VER(>= 8.0.24) and not __dbug_off}
# WL14506-TSFR_4_7
testutil.dbug_set("+d,dump_loader_force_mds")

EXPECT_PK(dump_no_pks_dir, { "createInvisiblePKs": True, "ignoreVersion": True }, True)
EXPECT_STDOUT_CONTAINS("WARNING: The dump contains tables without Primary Keys and it is loaded with the 'createInvisiblePKs' option set to true, Inbound Replication into an MySQL Database Service instance with High Availability (at the time of the release of MySQL Shell 8.0.24) cannot be used with this dump.")

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
    try:
        testutil.rmdir(dump_dir, True)
    except:
        pass
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
            EXPECT_SHELL_LOG_MATCHES(re.compile(f"\\[Worker00\\d\\] {encode_partition_basename(schema_name, table, partition)}.*tsv.zst: Records: \\d+  Deleted: 0  Skipped: 0  Warnings: 0"))
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
EXPECT_STDOUT_CONTAINS(f"NOTE: Skipping GRANT statements for user '{loader_user}'@'{host_with_netmask}'")

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
    dump_dir = os.path.join(outdir, "wl14244")
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
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeRoutines": [ "routine" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse routine filter 'routine'. The routine must be in the following form: schema.routine, with optional backtick quotes.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeRoutines": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse routine filter 'schema.@'. The routine must be in the following form: schema.routine, with optional backtick quotes.")

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
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeRoutines": [ "routine" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse routine filter 'routine'. The routine must be in the following form: schema.routine, with optional backtick quotes.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeRoutines": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse routine filter 'schema.@'. The routine must be in the following form: schema.routine, with optional backtick quotes.")

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
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeEvents": [ "event" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse event filter 'event'. The event must be in the following form: schema.event, with optional backtick quotes.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeEvents": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse event filter 'schema.@'. The event must be in the following form: schema.event, with optional backtick quotes.")

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
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeEvents": [ "event" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse event filter 'event'. The event must be in the following form: schema.event, with optional backtick quotes.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeEvents": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse event filter 'schema.@'. The event must be in the following form: schema.event, with optional backtick quotes.")

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
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeTriggers": [ "trigger" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse trigger filter 'trigger'. The filter must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "includeTriggers": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse trigger filter 'schema.@'. The filter must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes.")

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
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeTriggers": [ "trigger" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse trigger filter 'trigger'. The filter must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes.")
EXPECT_THROWS(lambda: util.load_dump(dump_dir, { "excludeTriggers": [ "schema.@" ] }), "ValueError: Util.load_dump: Argument #2: Can't parse trigger filter 'schema.@'. The filter must be in the following form: schema.table or schema.table.trigger, with optional backtick quotes.")

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

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.rmdir(__tmp_dir+"/ldtest", True)
