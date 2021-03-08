# Special case integration tests of dump and load

#@<> INCLUDE dump_utils.inc

#@<> Setup
from contextlib import ExitStack
import json
import os
import os.path
import random
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

session2.run_sql("create user testuser@'%'")
session2.run_sql("grant select, delete on *.* to testuser@'%'")
session2.run_sql("create user myuser@'%'")
session2.run_sql("create user myuser2@'%'")
session2.run_sql("create user myuser3@'%'")
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

# dump with admin user
shell.connect(f"mysql://admin:pass@localhost:{__mysql_sandbox_port2}")
# disable consistency b/c we won't be able to acquire a backup lock
util.dump_instance(__tmp_dir+"/ldtest/dump_admin", {"compatibility":["strip_restricted_grants", "strip_definers"], "ocimds":True, "consistent":False})

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
EXPECT_STDOUT_CONTAINS(".sql: Unknown database 'world'")

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
session.run_sql("USE schema1")
session.run_sql("CREATE definer=uuuser@localhost PROCEDURE mysp1() BEGIN END")
session.run_sql("CREATE definer=uuuuser@localhost FUNCTION myfun1() RETURNS INT NO SQL RETURN 1")
session.run_sql("CREATE definer=uuuuuser@localhost VIEW view1 AS select 1")
session.run_sql("CREATE TABLE table1 (pk INT PRIMARY KEY)");
session.run_sql("CREATE definer=uuuuuuser@localhost TRIGGER trigger1 BEFORE UPDATE ON table1 FOR EACH ROW BEGIN END")
session.run_sql("CREATE definer=uuuuuuuser@localhost EVENT event1 ON SCHEDULE EVERY 1 year DISABLE DO BEGIN END")

session.run_sql("CREATE USER uuuser@localhost")
session.run_sql("CREATE USER uuuuser@localhost")
session.run_sql("CREATE USER uuuuuser@localhost")
session.run_sql("CREATE USER uuuuuuser@localhost")
session.run_sql("CREATE USER uuuuuuuser@localhost")

session.run_sql("GRANT SELECT ON TABLE schema1.table1 TO uuuser@localhost")
session.run_sql("GRANT EXECUTE ON FUNCTION schema1.myfun1 TO uuuser@localhost")
session.run_sql("GRANT EXECUTE ON PROCEDURE schema1.mysp1 TO uuuser@localhost")

dump_dir = os.path.join(outdir, "user_load_order")
EXPECT_NO_THROWS(lambda: util.dump_instance(dump_dir), "Dump")

shell.connect(__sandbox_uri2)
wipeout_server(session2)

EXPECT_NO_THROWS(lambda: util.load_dump(dump_dir, {"loadUsers":True, "excludeUsers":["root@%"]}), "Load")

compare_servers(session1, session2)

#@<> load while dump is still running (prepare)
# We test this artificially by manually assembling the loaded dump
wipeout_server(session2)

source = outdir+"/fulldump"
target = outdir+"/partdump"


def copy(f):
    testutil.cpfile(os.path.join(source, f), os.path.join(target, f))


shell.connect(__sandbox_uri1)
util.dump_instance(source, {"bytesPerChunk": "128k"})

testutil.mkdir(target)

files = os.listdir(source)
files.sort()
files.remove("@.done.json")

# Copy global metadata files first
for f in files[:]:
    if f.startswith("@."):
        copy(f)
        files.remove(f)

ddlfiles = []
idxfiles = []
datafiles = []

for f in files:
    if f.endswith(".json") or f.endswith(".sql"):
        ddlfiles.append(f)
    elif f.endswith(".idx"):
        idxfiles.append(f)
    else:
        datafiles.append(f)

# Copy ddl, idx files and half of the data files first
for f in ddlfiles + idxfiles:
    copy(f)

n = int(len(datafiles)/2)
for f in datafiles[:n]:
    copy(f)

def copy_rest():
    time.sleep(2)
    for f in datafiles[n:]:
        copy(f)

#@<> load dump while dump still running

shell.connect(__sandbox_uri2)

# Copy the remaining files after a few secs
threading.Thread(target=copy_rest).start()

# Now at least half of the DDL files would be loaded
EXPECT_THROWS(lambda: util.load_dump(target, {"waitDumpTimeout": 5}), "Dump timeout")

#@<> load dump after it's done

copy("@.done.json")
util.load_dump(target, {"waitDumpTimeout": 10})

compare_servers(session1, session2, check_rows=True, check_users=False)

#@<> load incomplete dumps by retrying
testutil.rmfile(target+"/*")

shell.connect(__sandbox_uri2)
wipeout_server(session)

# order of files is:
# @.*, schema.*, schema@table.*, schema@table@chunk*, @.done.json

source_files = os.listdir(source)
ordered = ["@.done.json", "@.json"]
for f in source_files:
    if f.startswith("@.") and f not in ordered:
        ordered.append(f)
for f in source_files:
    if "@" not in f and f not in ordered:
        ordered.append(f)
for f in source_files:
    if f.count("@") == 1 and "tsv" not in f and f not in ordered:
        ordered.append(f)
for f in source_files:
    if ".idx" in f and f not in ordered:
        ordered.append(f)
for f in source_files:
    if f not in ordered:
        ordered.append(f)
del ordered[0]
ordered.append("@.done.json")
#print("\n".join(ordered))
assert set(source_files) == set(ordered)

shell.connect(__sandbox_uri2)

# copy files and retry 1 by 1
for f in ordered:
    copy(f)
    if f.endswith(".idx"):
        # .idx files make no difference so add more to save some time
        continue
    #print(os.listdir(target))
    if f == "@.done.json":
        util.load_dump(target, {"waitDumpTimeout": 0.001})
    else:
        if not EXPECT_THROWS(lambda: util.load_dump(target, {"waitDumpTimeout": 0.001}), "Dump timeout"):
            break

### BUG#32430402 showMetadata option

#@<> setup showMetadata tests
binlog_info_header = "---"
binlog_file = ""
binlog_position = 0
gtid_executed = ""
binlog_info = session1.run_sql("SHOW MASTER STATUS").fetch_one()

if binlog_info is not None:
    if len(binlog_info[0]) > 0:
        binlog_file = binlog_info[0]
        binlog_position = str(binlog_info[1])
    if len(binlog_info[4]) > 0:
        gtid_executed = binlog_info[4]

metadata_file = os.path.join(outdir, "fulldump", "@.json")

wipeout_server(session2)
shell.connect(__sandbox_uri2)

def set_binlog_info(file, position, gtid):
    with open(metadata_file, "r+", encoding="utf-8") as json_file:
        # read contents
        metadata = json.load(json_file)
        # modify data
        if file is None:
            metadata.pop('binlogFile', None)
        else:
            metadata['binlogFile'] = file
        if position is None:
            metadata.pop('binlogPosition', None)
        else:
            metadata['binlogPosition'] = position
        if gtid is None:
            metadata.pop('gtidExecuted', None)
        else:
            metadata['gtidExecuted'] = gtid
        # write back to file
        json_file.seek(0)
        json.dump(metadata, json_file)
        json_file.truncate()

def EXPECT_BINLOG_INFO(file, position, gtid, options = {}):
    WIPE_STDOUT()
    for option in [ "showMetadata", "dryRun" ]:
        if option not in options:
            options[option] = True
    util.load_dump(os.path.join(outdir, "fulldump"), options)
    EXPECT_STDOUT_CONTAINS(binlog_info_header)
    EXPECT_STDOUT_CONTAINS(testutil.yaml({ "Dump_metadata": { "Binlog_file": file, "Binlog_position": position, "Executed_GTID_set": gtid } }))

#@<> showMetadata defaults to false
util.load_dump(os.path.join(outdir, "fulldump"), { "dryRun": True })
EXPECT_STDOUT_NOT_CONTAINS(binlog_info_header)

#@<> showMetadata displays expected information
EXPECT_BINLOG_INFO(binlog_file, binlog_position, gtid_executed)

#@<> create backup of @.json
testutil.cpfile(metadata_file, metadata_file + ".bak")

#@<> no binary log information
set_binlog_info(None, None, None)
EXPECT_BINLOG_INFO("", 0, "", { "loadData": True, "loadDdl": False, "loadUsers": False })

#@<> only executed GTID set
set_binlog_info(None, None, "gtid_executed")
EXPECT_BINLOG_INFO("", 0, "gtid_executed", { "loadData": False, "loadDdl": True, "loadUsers": False })

#@<> executed GTID set + empty binlog file + zero binlog position
set_binlog_info("", 0, "gtid_executed")
EXPECT_BINLOG_INFO("", 0, "gtid_executed", { "loadData": False, "loadDdl": False, "loadUsers": True })

#@<> executed GTID set + empty binlog file + non-zero binlog position
set_binlog_info("", 5, "gtid_executed")
EXPECT_BINLOG_INFO("", 5, "gtid_executed")

#@<> everything is available
set_binlog_info("binlog.file", 1234, "gtid_executed")
EXPECT_BINLOG_INFO("binlog.file", "1234", "gtid_executed")

#@<> restore backup of @.json
testutil.cpfile(metadata_file + ".bak", metadata_file)

### BUG#32430402 showMetadata option -- END

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

users_outdir = os.path.join(outdir, "users")
util.dump_instance(users_outdir, { "users": True, "ddlOnly": True, "showProgress": False })

# helper function
def EXPECT_INCLUDE_EXCLUDE(options, included, excluded, expected_exception=None):
    opts = { "loadUsers": True, "showProgress": False, "resetProgress": True }
    opts.update(options)
    shell.connect(__sandbox_uri2)
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
        if u.find('mysql') != -1:
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

#@<> exclude non-existent user (and root@%), all accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "excludeUsers": ["third", "'root'@'%'"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"], [])

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

#@<> cleanup tests with include/exclude users
shell.connect(__sandbox_uri1)
session.run_sql("DROP USER 'first'@'localhost';")
session.run_sql("DROP USER 'first'@'10.11.12.13';")
session.run_sql("DROP USER 'firstfirst'@'localhost';")
session.run_sql("DROP USER 'second'@'localhost';")
session.run_sql("DROP USER 'second'@'10.11.12.14';")

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

#@<> Check that dumping lots of things won't trigger a filesort, which could be a problem if the source has no disk space left
session1.run_sql("set global sort_buffer_size=32768")

# create lots of users
user_length = 20 if __version_num < 80017 else 200
for i in range(500):
    session1.run_sql(f"create user rando_____________________{i}@'l{'o'*user_length}calhost'")

# create lots of schemas, tables and columns
for s in range(500):
    dbname = f"randodb{s}{'_'*30}"
    session1.run_sql(f"create schema {dbname}")

for t in range(500):
    sql = f"create table {dbname}.table{t} (pk int primary key"
    for c in range(100):
        sql += f", column{c} int"
    sql += ")"
    session1.run_sql(sql)

if __version_num > 80000:
    for t in range(500):
        sql = f"ANALYZE TABLE {dbname}.table{t} UPDATE HISTOGRAM ON `column0`;"
        session1.run_sql(sql)

for i in range(500):
    sql = f"create event {dbname}.event{i} on schedule at '2035-12-31 20:01:23' do set @a=5;"
    session1.run_sql(sql)
    sql = f"create function {dbname}.function{i}() RETURNS INT DETERMINISTIC RETURN 0"
    session1.run_sql(sql)
    sql = f"create procedure {dbname}.procedure{i}() BEGIN END"
    session1.run_sql(sql)
    sql = f"create trigger {dbname}.trigger{i} BEFORE INSERT ON table0 FOR EACH ROW SET @sum = @sum + NEW.column0"
    session1.run_sql(sql)

with ExitStack() as stack:
    if __os_type != "windows":
        # make the tmpdir read-only to force an error when tmpfiles are created (like when filesort is used) during dump
        os.chmod(mysql_tmpdir, 0o550)
        # allow writing to tmpdir for loading (executed when leaving the 'with' scope)
        stack.callback(lambda: os.chmod(mysql_tmpdir, 0o770))
    shell.connect(__sandbox_uri1)
    dump_dir = os.path.join(outdir, "lotsofstuff")
    util.dump_instance(dump_dir)

shell.connect(__sandbox_uri2)
wipeout_server(session2)

util.load_dump(dump_dir)

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

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.rmdir(__tmp_dir+"/ldtest", True)
