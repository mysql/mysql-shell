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

prepare(__mysql_sandbox_port2)
session2 = mysql.get_session(__sandbox_uri2)
session2.run_sql("set names utf8mb4")

#@<> Dump ddlOnly
shell.connect(__sandbox_uri1)
util.dump_instance(outdir+"/ddlonly", {"ddlOnly": True})

shell.connect(__sandbox_uri2)

#@<> load data which is not in the dump (fail)
EXPECT_THROWS(lambda: util.load_dump(outdir+"/ddlonly",
                                     {"loadData": True, "loadDdl": False}), "RuntimeError: Util.load_dump: Unknown database 'world'")

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

#@<> load ddl which is not in the dump (fail)
EXPECT_THROWS(lambda: util.load_dump(outdir+"/dataonly", {"loadData": False, "loadDdl": True}), "RuntimeError: Util.load_dump: Unknown database 'world'")


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
datafiles = []
for f in files:
    if f.endswith(".json") or f.endswith(".sql"):
        ddlfiles.append(f)
    else:
        datafiles.append(f)

# Copy ddl files and half of the data files first
for f in ddlfiles:
    copy(f)
n = int(len(datafiles)/2)
for f in datafiles[:n]:
    copy(f)

def copy_rest():
    time.sleep(5)
    for f in datafiles[n:]:
        copy(f)

#@<> load dump while dump still running

shell.connect(__sandbox_uri2)

# Copy the remaining files after a few secs
threading.Thread(target=copy_rest).start()

# Now at least half of the DDL files would be loaded
EXPECT_THROWS(lambda: util.load_dump(target, {"waitDumpTimeout": 10}), "Dump timeout")

# The dump should already be completely loaded by now
compare_servers(session1, session2, check_rows=True)

#@<> load dump after it's done

copy("@.done.json")
util.load_dump(target, {"waitDumpTimeout": 10})

compare_servers(session1, session2, check_rows=True)

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
def EXPECT_INCLUDE_EXCLUDE(options, included, excluded):
    opts = { "loadUsers": True, "showProgress": False, "resetProgress": True }
    opts.update(options)
    shell.connect(__sandbox_uri2)
    wipeout_server(session)
    WIPE_OUTPUT()
    EXPECT_NO_THROWS(lambda: util.load_dump(users_outdir, opts), "Loading the dump should not fail")
    all_users = {c[0] for c in session.run_sql("SELECT GRANTEE FROM information_schema.user_privileges;").fetch_all()}
    for i in included:
        EXPECT_TRUE(i in all_users, "User {0} should exist".format(i))
        EXPECT_STDOUT_NOT_CONTAINS("NOTE: Skipping CREATE/ALTER USER statements for user {0}".format(i.replace("'", "")))
        EXPECT_STDOUT_NOT_CONTAINS("NOTE: Skipping GRANT statements for user {0}".format(i.replace("'", "")))
    for e in excluded:
        EXPECT_TRUE(e not in all_users, "User {0} should not exist".format(e))
        EXPECT_STDOUT_CONTAINS("NOTE: Skipping CREATE/ALTER USER statements for user {0}".format(e.replace("'", "")))
        EXPECT_STDOUT_CONTAINS("NOTE: Skipping GRANT statements for user {0}".format(e.replace("'", "")))
    for u in included + excluded:
        if u.find('mysql') != -1:
            session.run_sql("DROP USER IF EXISTS {0};".format(u))

#@<> the `includeUsers` and `excludeUsers` options cannot be used when `loadUsers` is false
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": False, "includeUsers": ["third"] }), "ArgumentError: Util.load_dump: The 'includeUsers' option cannot be used if the 'loadUsers' option is set to false.")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": False, "excludeUsers": ["third"] }), "ArgumentError: Util.load_dump: The 'excludeUsers' option cannot be used if the 'loadUsers' option is set to false.")

#@<> test invalid user names
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": [""] }), "ArgumentError: Util.load_dump: User name must not be empty.")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "excludeUsers": [""] }), "ArgumentError: Util.load_dump: User name must not be empty.")

EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["@"] }), "ArgumentError: Util.load_dump: User name must not be empty.")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "excludeUsers": ["@"] }), "ArgumentError: Util.load_dump: User name must not be empty.")

EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["@@"] }), "ArgumentError: Util.load_dump: Invalid user name: @")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "excludeUsers": ["@@"] }), "ArgumentError: Util.load_dump: Invalid user name: @")

EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["foo@''nope"] }), "ArgumentError: Util.load_dump: Malformed hostname. Cannot use \"'\" or '\"' characters on the hostname without quotes")
EXPECT_THROWS(lambda: util.load_dump(users_outdir, { "loadUsers": True, "includeUsers": ["foo@''nope"] }), "ArgumentError: Util.load_dump: Malformed hostname. Cannot use \"'\" or '\"' characters on the hostname without quotes")

#@<> don't include or exclude any users, all accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": [], "excludeUsers": [] }, ["'first'@'localhost'", "'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"], [])

#@<> include non-existent user, no accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "includeUsers": ["third"] }, [], ["'first'@'localhost'", "'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"])

#@<> exclude non-existent user, all accounts are loaded
EXPECT_INCLUDE_EXCLUDE({ "excludeUsers": ["third"] }, ["'first'@'localhost'", "'first'@'10.11.12.13'", "'firstfirst'@'localhost'", "'second'@'localhost'", "'second'@'10.11.12.14'"], [])

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

#@<> Big blob in source, check max_allowed_packet in server and client


#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.rmdir(__tmp_dir+"/ldtest", True)
