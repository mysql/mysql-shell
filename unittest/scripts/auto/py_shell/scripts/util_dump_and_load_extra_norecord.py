# Special case integration tests of dump and load (slow tests)

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
session1.run_sql("create schema tests")
# BUG#33079172 "SQL SECURITY INVOKER" INSERTED AT WRONG LOCATION
session1.run_sql("create procedure tests.tmp() IF @v > 0 THEN SELECT 1; ELSE SELECT 2; END IF;;")

prepare(__mysql_sandbox_port2)
session2 = mysql.get_session(__sandbox_uri2)
session2.run_sql("set names utf8mb4")

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

#@<> BUG#BUG33332497 ensure progress reporting is correct
# clean up after the previous test
testutil.rmfile(target+"/*")

shell.connect(__sandbox_uri2)
wipeout_server(session)

# copy just the first metadata file
copy(ordered[0])

# start the process which will load the dump asynchronously
proc = testutil.call_mysqlsh_async([__sandbox_uri2, "--py", "-e", f"util.load_dump('{target}', {{'waitDumpTimeout': 60, 'showProgress': True}})"])

# copy the remaining files
for f in ordered[1:]:
    copy(f)
    if f.endswith(".idx"):
        # .idx files make no difference so add more to save some time
        continue
    time.sleep(1)

# wait for the load to finish
testutil.wait_mysqlsh_async(proc)

# check that progress is going up
for progress in re.findall(r'(\d+) / (\d+) tables done', testutil.fetch_captured_stdout(False)):
    EXPECT_LE(int(progress[0]), int(progress[1]))

compare_servers(session1, session2, check_rows=True, check_users=False)

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

#@<> cleanup the schemas with lots of things
for i in range(500):
    session1.run_sql(f"drop user if exists rando_____________________{i}@'l{'o'*user_length}calhost'")

for s in range(500):
    dbname = f"randodb{s}{'_'*30}"
    session1.run_sql(f"drop schema if exists {dbname}")

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.destroy_sandbox(__mysql_sandbox_port2)
testutil.rmdir(__tmp_dir+"/ldtest", True)
