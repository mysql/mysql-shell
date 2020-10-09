# Special case integration tests of dump and load

#@<> INCLUDE dump_utils.inc

#@<> Setup
import os
import threading
import time

outdir = __tmp_dir+"/ldtest"
try:
    testutil.rmdir(outdir, True)
except:
    pass
testutil.mkdir(outdir)


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
        "local_infile": "1"
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
session2.run_sql("grant all on mysql.* to myuser@'%'")

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

reset_server(session2)
util.load_dump(__tmp_dir+"/ldtest/dump_root", {"loadUsers":1, "loadDdl":0, "loadData":0, "excludeUsers":["root@%","root@localhost"]})

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

compare_servers(session1, session2, check_rows=False)
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

compare_servers(session1, session2)

wipeout_server(session2)

#@<> load ddl which is not in the dump (fail/no-op)
util.load_dump(outdir+"/dataonly", {"loadData": False, "loadDdl": True})


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

compare_servers(session1, session2, check_rows=True)

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
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": False, "includeUsers": ["third"] }), "ValueError: Util.load_dump: The 'includeUsers' option cannot be used if the 'loadUsers' option is set to false.")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": False, "excludeUsers": ["third"] }), "ValueError: Util.load_dump: The 'excludeUsers' option cannot be used if the 'loadUsers' option is set to false.")

#@<> test invalid user names
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": [""] }), "ValueError: Util.load_dump: User name must not be empty.")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "excludeUsers": [""] }), "ValueError: Util.load_dump: User name must not be empty.")

EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["@"] }), "ValueError: Util.load_dump: User name must not be empty.")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "excludeUsers": ["@"] }), "ValueError: Util.load_dump: User name must not be empty.")

EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["@@"] }), "ValueError: Util.load_dump: Invalid user name: @")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "excludeUsers": ["@@"] }), "ValueError: Util.load_dump: Invalid user name: @")

EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["foo@''nope"] }), "ValueError: Util.load_dump: Malformed hostname. Cannot use \"'\" or '\"' characters on the hostname without quotes")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["foo@''nope"] }), "ValueError: Util.load_dump: Malformed hostname. Cannot use \"'\" or '\"' characters on the hostname without quotes")

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

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.rmdir(__tmp_dir+"/ldtest", True)
