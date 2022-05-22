#@{__version_num > 80018 or __version_num < 80000}

import os.path
import random
import json

def rand_text(size):
    return "["+ "".join(random.choices('0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&()*+,-./:;<=>?@^_{|}~', k=size-2)) +"]"


def TEST_LOAD(dump, net_buffer_length, max_binlog_cache_size, remove_progress = True, options = {}):
    WIPE_SHELL_LOG()
    WIPE_STDOUT()
    if remove_progress:
        wipeout_server(session)
        testutil.rmfile(__tmp_dir+"/ldtest/"+dump+"/load-progress*")
    session.run_sql("set global max_binlog_cache_size=?", [max_binlog_cache_size])
    session.close()
    shell.connect(__sandbox_uri1+"?net-buffer-length=%s"%net_buffer_length)
    if "showProgress" not in options:
        options["showProgress"] = False
    util.load_dump(__tmp_dir+"/ldtest/"+dump, options)
    for i in range(k_last_table):
        EXPECT_EQ(rcount[i], session.run_sql(f'select count(*) from testdb.data{i}').fetch_one()[0])
        EXPECT_EQ(checksum[i], session.run_sql(f'checksum table testdb.data{i}').fetch_one()[1])

k_last_table = 2

checksum = []
rcount = []

def set_test_table_count(num):
    global k_last_table
    global checksum
    global rcount
    k_last_table = num
    checksum = []
    rcount = []
    for i in range(k_last_table):
        checksum.append(session.run_sql(f'checksum table testdb.data{i}').fetch_one()[1])
        rcount.append(session.run_sql(f'select count(*) from testdb.data{i}').fetch_one()[0])


def hack_in_bytes_per_chunk(dump, bytes_per_chunk):
    md = json.loads(open(__tmp_dir+"/ldtest/"+dump+"/@.json").read())
    md["bytesPerChunk"] = bytes_per_chunk
    with open(__tmp_dir+"/ldtest/"+dump+"/@.json", "w") as f:
        f.write(json.dumps(md))


trx_size_limit = 1024*32
chunk_size = trx_size_limit//2


#@<> INCLUDE dump_utils.inc

#@<> Setup
try:
  testutil.rmdir(__tmp_dir+"/ldtest", True)
except:
  pass
testutil.mkdir(__tmp_dir+"/ldtest")

testutil.deploy_sandbox(__mysql_sandbox_port1, "root")

shell.connect(__sandbox_uri1)

session.run_sql("set global local_infile=1")

#@<> load dump with chunks bigger than max_binlog_cache_size (prep)
# Bug#31878733
# create dump with large chunks at various row boundaries
session.run_sql("create schema testdb")
# session.run_sql("create table testdb.empty (txt longtext)")
session.run_sql("create table testdb.data0 (txt longtext charset ascii)")
# a few rows that are small enough to fit
session.run_sql("insert into testdb.data0 values (?)", ["1"+rand_text(500)])
session.run_sql("insert into testdb.data0 values (?)", ["2"+rand_text(500)])
session.run_sql("insert into testdb.data0 values (null)")
session.run_sql("insert into testdb.data0 values (?)", ["4"+rand_text(500)])
session.run_sql("insert into testdb.data0 values (?)", ["5"+rand_text(500)])
# 2 big rows that will require the chunk to be flushed at the previous row
session.run_sql("insert into testdb.data0 values (?)", ["6"+rand_text(chunk_size//2)])
session.run_sql("insert into testdb.data0 values (?)", ["7"+rand_text(chunk_size//2)])
# single row chunk
session.run_sql("insert into testdb.data0 values (?)", ["8"+rand_text(chunk_size - 2)])
# a bunch of smaller rows
session.run_sql("insert into testdb.data0 values (?)", ["9"+rand_text(10)])
session.run_sql("insert into testdb.data0 values (?)", ["a"+rand_text(34)])
session.run_sql("insert into testdb.data0 values (?)", ["b"+rand_text(12)])
session.run_sql("insert into testdb.data0 values (?)", ["c"+rand_text(150)])
# at the limit, subtracting two bytes: one for the row identifier and one for the newline character
session.run_sql("insert into testdb.data0 values (?)", ["d"+rand_text(chunk_size - 2)])
session.run_sql("insert into testdb.data0 values (?)", ["e"+rand_text(chunk_size - 2)])
session.run_sql("insert into testdb.data0 values (?)", ["f"+rand_text(12)])
session.run_sql("insert into testdb.data0 values (?)", ["g"+rand_text(chunk_size - 2)])
session.run_sql("insert into testdb.data0 values ('\\n\\t\\\\')")

session.run_sql("create table testdb.data1 (n int)")
session.run_sql("insert into testdb.data1 values (1111111)" + ",(1111111)"*chunk_size)

session.run_sql("create table testdb.data2 (n int, pk int PRIMARY KEY AUTO_INCREMENT)")
session.run_sql("insert into testdb.data2 (n) values (1111111)" + ",(1111111)"*chunk_size)

session.run_sql("create table testdb.data3 (n int, pk int PRIMARY KEY AUTO_INCREMENT)")
session.run_sql("insert into testdb.data3 (n) values (1111111)" + ",(1111111)"*4)

set_test_table_count(4)

util.dump_instance(__tmp_dir+"/ldtest/dump-trxlimit", { "chunking": False, "showProgress": False })
# set bytesPerChunk to 128K to take effect during load
hack_in_bytes_per_chunk("dump-trxlimit", chunk_size)

#@<> net_buffer = trx_limit
TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit)

#@<> net_buffer = trx_limit 2
TEST_LOAD("dump-trxlimit", trx_size_limit*4, trx_size_limit*4)

#@<> net_buffer < trx_limit
TEST_LOAD("dump-trxlimit", 2048, trx_size_limit)

#@<> net_buffer < trx_limit 2
TEST_LOAD("dump-trxlimit", 4096, trx_size_limit*3)

#@<> net_buffer < trx_limit 3
TEST_LOAD("dump-trxlimit", 4096, trx_size_limit*10)

#@<> net_buffer < trx_limit 4
TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit*10)

#@<> net_buffer > trx_limit
TEST_LOAD("dump-trxlimit", trx_size_limit*2, trx_size_limit)

#@<> net_buffer > trx_limit 2
TEST_LOAD("dump-trxlimit", trx_size_limit*3, trx_size_limit)

#@<> net_buffer > trx_limit 3
TEST_LOAD("dump-trxlimit", trx_size_limit*4, trx_size_limit*2)

EXPECT_SHELL_LOG_CONTAINS("testdb@data0.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 9 sub-chunks")
EXPECT_SHELL_LOG_CONTAINS("testdb@data1.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 9 sub-chunks")
EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")
EXPECT_SHELL_LOG_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> BUG#31961688 - fail after loading a sub-chunk from table which does not have PK, then resume {not __dbug_off}
# EXPECT: when resuming, table is truncated, all sub-chunks are loaded again
testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_END", "schema == testdb", "table == data0", "chunk == -1", "subchunk == 3"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_CONTAINS("NOTE: Truncating table `testdb`.`data0` because of resume and it has no PK or equivalent key")
EXPECT_SHELL_LOG_CONTAINS("testdb@data0.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 9 sub-chunks")

#@<> BUG#31961688 - fail before start of the data load has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, all sub-chunks are loaded
testutil.set_trap("dump_loader", ["op == BEFORE_LOAD_START", "schema == testdb", "table == data2", "chunk == -1"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")

#@<> BUG#31961688 - fail after start of the data load has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, all sub-chunks are loaded
testutil.set_trap("dump_loader", ["op == AFTER_LOAD_START", "schema == testdb", "table == data2", "chunk == -1"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")

#@<> BUG#31961688 - fail before start of the sub-chunk transaction has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, all sub-chunks are loaded again
testutil.set_trap("dump_loader", ["op == BEFORE_LOAD_SUBCHUNK_START", "schema == testdb", "table == data2", "chunk == -1", "subchunk == 0"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")

#@<> BUG#31961688 - fail after start of the sub-chunk transaction has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, all sub-chunks are loaded again
testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_START", "schema == testdb", "table == data2", "chunk == -1", "subchunk == 0"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")

#@<> BUG#31961688 - fail before end of the sub-chunk transaction has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, all sub-chunks are loaded again
testutil.set_trap("dump_loader", ["op == BEFORE_LOAD_SUBCHUNK_END", "schema == testdb", "table == data2", "chunk == -1", "subchunk == 0"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")

#@<> BUG#31961688 - fail after end of the sub-chunk transaction has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, sub-chunks which were finished are not loaded again
for subchunk in range(13):
  print("Testing sub-chunk: " + str(subchunk))
  testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_END", "schema == testdb", "table == data2", "chunk == -1", "subchunk == " + str(subchunk)], {"msg": "Injected exception"})
  EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
  EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")
  testutil.clear_traps("dump_loader")
  TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
  EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
  EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in {0} sub-chunks".format(13 - subchunk))

#@<> BUG#31961688 - fail after the last end of the sub-chunk transaction has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, an empty sub-chunk is loaded
testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_END", "schema == testdb", "table == data2", "chunk == -1", "subchunk == 13"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv.zst: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> BUG#31961688 - fail before end of the data load has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, an empty sub-chunk is loaded
testutil.set_trap("dump_loader", ["op == BEFORE_LOAD_END", "schema == testdb", "table == data2", "chunk == -1"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv.zst: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> BUG#31961688 - table which is not sub-chunked does not generate sub-chunking events {not __dbug_off}
# EXPECT: trap should not be triggered
testutil.set_trap("dump_loader", ["op == BEFORE_LOAD_SUBCHUNK_START", "schema == testdb", "table == data3", "chunk == -1"], {"msg": "Injected exception"})

EXPECT_NO_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "trap should not be triggered")

testutil.clear_traps("dump_loader")

#@<> BUG#31961688 - test uncompressed files {not __dbug_off}
util.dump_instance(os.path.join(__tmp_dir, "ldtest", "dump-trxlimit-none"), { "chunking": False, "compression": "none", "showProgress": False })
hack_in_bytes_per_chunk("dump-trxlimit-none", chunk_size)

testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_END", "schema == testdb", "table == data2", "chunk == -1", "subchunk == 5"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit-none", trx_size_limit, trx_size_limit), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit-none", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 8 sub-chunks")

##

# TODO load from a 1.0.1 dump

#@<> impossible load: row too big

# this table will generate a chunk with a row that's too big to be loaded
wipeout_server(session)
session.run_sql("create schema testdb")
session.run_sql("create table testdb.data0 (txt longtext charset ascii)")
session.run_sql("insert into testdb.data0 values (?)", [rand_text(1)])
session.run_sql("insert into testdb.data0 values (null)")
session.run_sql("insert into testdb.data0 values (?)", [rand_text(trx_size_limit*10)])
session.run_sql("insert into testdb.data0 values (?)", [rand_text(trx_size_limit*2)])
session.run_sql("insert into testdb.data0 values (?)", [rand_text(20)])

set_test_table_count(1)

util.dump_instance(__tmp_dir+"/ldtest/dump-toobig", { "chunking": False, "showProgress": False })
hack_in_bytes_per_chunk("dump-toobig", chunk_size)

wipeout_server(session)

#@<> row > max_binlog_cache_size - fail from chunk too big
# note: use 4098 as binlog_cache_size b/c setting it to trx_size_limit is somehow allowing bigger transactions through
EXPECT_THROWS(lambda:TEST_LOAD("dump-toobig", 4098, 4098), "Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: MySQL Error 1197 (HY000): Multi-statement transaction required more than 'max_binlog_cache_size' bytes of storage")

wipeout_server(session)
testutil.rmfile(__tmp_dir+"/ldtest/dump-toobig/load-progress*")

#@<> row > max_binlog_cache_size < net-buffer-length - fail from chunk too big
EXPECT_THROWS(lambda:TEST_LOAD("dump-toobig", trx_size_limit, 4098), "Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: MySQL Error 1197 (HY000): Multi-statement transaction required more than 'max_binlog_cache_size' bytes of storage")

# WL14577 - information should be printed before loading a row which is too big, and after failure, if a sub-chunk contained oversized row
EXPECT_STDOUT_MATCHES(re.compile(r"WARNING: \[Worker\d+\] testdb@data0.tsv.zst: Attempting to load a row longer than maxBytesPerTransaction\."))
EXPECT_STDOUT_MATCHES(re.compile(r"NOTE: \[Worker\d+\] testdb@data0.tsv.zst: This error has been reported for a sub-chunk which has at least one row longer than maxBytesPerTransaction \(16384 bytes\)\."))

wipeout_server(session)
testutil.rmfile(__tmp_dir+"/ldtest/dump-toobig/load-progress*")



#@<> impossible load: deceptively small row sizes

# each row of this table will be 255+ bytes long, but the dump data is just 1 byte
# this is a corner case where the dump size and estimated transaction size
# will be much smaller than the actual transaction size, which will break the
# sub-chunking logic. In practice, if a user falls into this case, they should
# specify a smaller chunk size.
session.run_sql("set global max_binlog_cache_size=?", [1024*1024*1024])
wipeout_server(session)
shell.connect(__sandbox_uri1)
session.run_sql("create schema testdb")
session.run_sql("create table testdb.data0 (n char(255))")
s = "insert into testdb.data0 values ('.')"
s += ",('.')"*trx_size_limit
session.run_sql(s)

set_test_table_count(1)

util.dump_instance(__tmp_dir+"/ldtest/dump-toobig2", { "chunking": False, "showProgress": False })
hack_in_bytes_per_chunk("dump-toobig2", chunk_size)

wipeout_server(session)

#@<> rowset > max_binlog_cache_size - fail from chunk too big
EXPECT_THROWS(lambda:TEST_LOAD("dump-toobig2", 4098, trx_size_limit//2), "Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: MySQL Error 1197 (HY000): Multi-statement transaction required more than 'max_binlog_cache_size' bytes of storage")

wipeout_server(session)
testutil.rmfile(__tmp_dir+"/ldtest/dump-toobig2/load-progress*")

#@<> rowset > max_binlog_cache_size < net-buffer-length - fail from chunk too big
EXPECT_THROWS(lambda:TEST_LOAD("dump-toobig2", trx_size_limit, trx_size_limit//2), "Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: MySQL Error 1197 (HY000): Multi-statement transaction required more than 'max_binlog_cache_size' bytes of storage")

wipeout_server(session)
testutil.rmfile(__tmp_dir+"/ldtest/dump-toobig2/load-progress*")

#@<> rowset > max_binlog_cache_size - should work if we reduce bytesPerChunk
hack_in_bytes_per_chunk("dump-toobig2", chunk_size//2)

TEST_LOAD("dump-toobig2", 4098, trx_size_limit//2)

wipeout_server(session)
testutil.rmfile(__tmp_dir+"/ldtest/dump-toobig2/load-progress*")

#@<> rowset > max_binlog_cache_size < net-buffer-length - should work if we reduce bytesPerChunk
TEST_LOAD("dump-toobig2", trx_size_limit, trx_size_limit//2)

wipeout_server(session)
testutil.rmfile(__tmp_dir+"/ldtest/dump-toobig2/load-progress*")

#@<> BUG#32072961
# data in the first row is bigger than net-buffer-length, but it is possible to fit multiple rows into a single transaction
# the find_last_row_boundary_before() method will be called twice, first time with `length` (equal to net-buffer-length), second time with the transaction size
# when find_last_row_boundary_before() was using indexes from .idx file in order to find row boundary within the specified limit, it was not checking if the given index does not exceed the size of data in buffer, resulting in segmentation fault
# test table has many long rows, to ensure that test would crash the testsuite in case of regression (index will point far away from data buffer)
shell.connect(__sandbox_uri1)
wipeout_server(session)

session.run_sql("create schema testdb")
session.run_sql("create table testdb.data0 (data longblob)")

for i in range(70):
    session.run_sql("insert into testdb.data0 values (repeat('a', 1024 * 1024 + 1))")

set_test_table_count(1)

util.dump_instance(__tmp_dir+"/ldtest/bug32072961", { "chunking": False, "showProgress": False })
TEST_LOAD("bug32072961", 1024 * 1024, 4 * 1024 * 1024 * 1024)

#@<> WL14577 - load dump with small maxBytesPerTransaction, rows are loaded one by one
TEST_LOAD("bug32072961", 1024 * 1024, 4 * 1024 * 1024 * 1024, options = { "maxBytesPerTransaction" : "128k" })

#@<> WL14577 - setup
shell.connect(__sandbox_uri1)
wipeout_server(session)

session.run_sql("create schema testdb")

def create_table(idx, size):
    session.run_sql(f"CREATE TABLE testdb.data{idx} (id INT AUTO_INCREMENT PRIMARY KEY, data varchar(1024))")
    session.run_sql(f"""INSERT INTO testdb.data{idx} (data) VALUES {",".join("('a')" for i in range(size))}""")
    session.run_sql(f"UPDATE testdb.data{idx} SET data = repeat('x', 1024)")

# table with less than 128k data
create_table(0, 100)

# table with less than 384k data
create_table(1, 350)

# table with more than 384k data
create_table(2, 400)

# table which has a single row which exceeds the maxBytesPerTransaction used in tests below
session.run_sql("CREATE TABLE testdb.data3 (data longblob)")
session.run_sql("INSERT INTO testdb.data3 (data) VALUES ('a')")
session.run_sql(f"UPDATE testdb.data3 SET data = repeat('x', 1024 * 1024)")

set_test_table_count(4)

output_folder = "wl14577"
output_path = os.path.join(__tmp_dir, "ldtest", output_folder)

# set limits to high values, we're not testing these here
net_buffer_length = 5 * 1024 * 1024  # 5MB
max_binlog_cache_size = 4 * 1024 * 1024 * 1024  # 4GB

util.dump_instance(output_path, { "chunking": False, "showProgress": False })

# artificially set the bytesPerChunk value to 256k
hack_in_bytes_per_chunk(output_folder, 256000)

#@<> WL14577 - maxBytesPerTransaction, a new string option
EXPECT_THROWS(lambda: util.load_dump(output_path, { "maxBytesPerTransaction" : 1234 }), "TypeError: Util.load_dump: Argument #2: Option 'maxBytesPerTransaction' is expected to be of type String, but is Integer")
EXPECT_THROWS(lambda: util.load_dump(output_path, { "maxBytesPerTransaction" : "" }), "ValueError: Util.load_dump: Argument #2: The option 'maxBytesPerTransaction' cannot be set to an empty string.")

#@<> WL14577-TSFR_1_1 - 1
help_text="""
      - maxBytesPerTransaction: string (default taken from dump) - Specifies
        the maximum number of bytes that can be loaded from a dump data file
        per single LOAD DATA statement. Supports unit suffixes: k (kilobytes),
        M (Megabytes), G (Gigabytes). Minimum value: 4096. If this option is
        not specified explicitly, the value of the bytesPerChunk dump option is
        used, but only in case of the files with data size greater than 1.5 *
        bytesPerChunk.
"""

\h util.load_dump

EXPECT_STDOUT_CONTAINS(help_text)

#@<> WL14577-TSFR_1_1 - 2
util.help('load_dump')
EXPECT_STDOUT_CONTAINS(help_text)

#@<> WL14577-TSFR_1_2
TEST_LOAD(output_folder, net_buffer_length, max_binlog_cache_size, options = { "maxBytesPerTransaction" : "128k" })
# one table is smaller than maxBytesPerTransaction, two are bigger
EXPECT_SHELL_LOG_MATCHES(re.compile(r"testdb@data0.tsv.zst: Records: \d+  Deleted: 0  Skipped: 0  Warnings: 0$", re.MULTILINE))
EXPECT_SHELL_LOG_MATCHES(re.compile(r"testdb@data1.tsv.zst: Records: \d+  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in \d+ sub-chunks$", re.MULTILINE))
EXPECT_SHELL_LOG_MATCHES(re.compile(r"testdb@data2.tsv.zst: Records: \d+  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in \d+ sub-chunks$", re.MULTILINE))

# WL14577-TSFR_3_1 - row longer than maxBytesPerTransaction, the whole file is subchunked
EXPECT_STDOUT_MATCHES(re.compile(r"WARNING: \[Worker\d+\] testdb@data3.tsv.zst: Attempting to load a row longer than maxBytesPerTransaction."))
EXPECT_SHELL_LOG_CONTAINS("testdb@data3.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - flushed sub-chunk 1")
EXPECT_SHELL_LOG_MATCHES(re.compile(r"testdb@data3.tsv.zst: Records: \d+  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in \d+ sub-chunks$", re.MULTILINE))

#@<> WL14577-TSFR_1_3
TEST_LOAD(output_folder, net_buffer_length, max_binlog_cache_size)
# hacked value of bytesPerChunk (256k) is used, two tables are smaller than 384k (1.5 * bytesPerChunk), one is bigger
EXPECT_SHELL_LOG_MATCHES(re.compile(r"testdb@data0.tsv.zst: Records: \d+  Deleted: 0  Skipped: 0  Warnings: 0$", re.MULTILINE))
EXPECT_SHELL_LOG_MATCHES(re.compile(r"testdb@data1.tsv.zst: Records: \d+  Deleted: 0  Skipped: 0  Warnings: 0$", re.MULTILINE))
EXPECT_SHELL_LOG_MATCHES(re.compile(r"testdb@data2.tsv.zst: Records: \d+  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in \d+ sub-chunks$", re.MULTILINE))

#@<> WL14577 - invalid values
for value in [ "xyz", "1xyz", "2Mhz", "0.1k", "0,3M" ]:
    EXPECT_THROWS(lambda: util.load_dump(output_path, { "maxBytesPerTransaction" : value }), f'ValueError: Util.load_dump: Argument #2: Wrong input number "{value}"')

EXPECT_THROWS(lambda: util.load_dump(output_path, { "maxBytesPerTransaction" : "-1G" }), 'ValueError: Util.load_dump: Argument #2: Input number "-1G" cannot be negative')

#@<> WL14577-TSFR_1_4
for value in ("1000k", "1M", "1G", "128000"):
    TEST_LOAD(output_folder, net_buffer_length, max_binlog_cache_size, options = { "maxBytesPerTransaction" : value })

#@<> WL14577-TSFR_1_5
for value in ("4k", "4095", "3999", "1", "0"):
    EXPECT_THROWS(lambda: util.load_dump(output_path, { "maxBytesPerTransaction" : value }), f"ValueError: Util.load_dump: Argument #2: The value of 'maxBytesPerTransaction' option must be greater than or equal to 4096 bytes.")

EXPECT_THROWS(lambda: util.load_dump(output_path, { "maxBytesPerTransaction" : "-1" }), 'ValueError: Util.load_dump: Argument #2: Input number "-1" cannot be negative')

#@<> WL14577-TSFR_2_1 {not __dbug_off}
# throw exception after loading the first subchunk of table `data1`
testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_END", "schema == testdb", "table == data1", "chunk == -1", "subchunk == 1"], {"msg": "Injected exception"})
EXPECT_THROWS(lambda: TEST_LOAD(output_folder, net_buffer_length, max_binlog_cache_size, options = { "maxBytesPerTransaction" : "128k" }), "Error: Shell Error (53005): Util.load_dump: Error loading dump")
EXPECT_SHELL_LOG_MATCHES(re.compile(r"testdb@data1.tsv.zst: Records: \d+  Deleted: 0  Skipped: 0  Warnings: 0 - flushed sub-chunk 1$", re.MULTILINE))
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Injected exception")
testutil.clear_traps("dump_loader")

# load once again, do not set maxBytesPerTransaction, table `data1` is loaded without subchunking
TEST_LOAD(output_folder, net_buffer_length, max_binlog_cache_size, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_SHELL_LOG_MATCHES(re.compile(r"testdb@data1.tsv.zst: Records: \d+  Deleted: 0  Skipped: 0  Warnings: 0$", re.MULTILINE))

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.rmdir(__tmp_dir+"/ldtest", True)
