#@{__version_num > 80018 or __version_num < 80000}

import random
import json

def rand_text(size):
    s = "["
    alphabet = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&()*+,-./:;<=>?@^_{|}~'
    for i in range(size-2):
        s += random.choice(alphabet)
    s += "]"
    return s


def TEST_LOAD(dump, net_buffer_length, max_binlog_cache_size, remove_progress=True):
    WIPE_STDOUT()
    if remove_progress:
        wipeout_server(session)
        testutil.rmfile(__tmp_dir+"/ldtest/"+dump+"/load-progress*")
    session.run_sql("set global max_binlog_cache_size=?", [max_binlog_cache_size])
    session.close()
    shell.connect(__sandbox_uri1+"?net-buffer-length=%s"%net_buffer_length)
    util.load_dump(__tmp_dir+"/ldtest/"+dump)
    for i in range(k_last_table):
        EXPECT_EQ(rcount[i], session.run_sql("select count(*) from testdb.data%s"%i).fetch_one()[0])
        EXPECT_EQ(checksum[i], session.run_sql("checksum table testdb.data%s"%i).fetch_one()[1])

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
        checksum.append(session.run_sql("checksum table testdb.data%i"%i).fetch_one()[1])
        rcount.append(session.run_sql("select count(*) from testdb.data%i"%i).fetch_one()[0])


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
session.run_sql("insert into testdb.data0 values (?)", ["8"+rand_text(chunk_size)])
# a bunch of smaller rows
session.run_sql("insert into testdb.data0 values (?)", ["9"+rand_text(10)])
session.run_sql("insert into testdb.data0 values (?)", ["a"+rand_text(34)])
session.run_sql("insert into testdb.data0 values (?)", ["b"+rand_text(12)])
session.run_sql("insert into testdb.data0 values (?)", ["c"+rand_text(150)])
# at the limit
session.run_sql("insert into testdb.data0 values (?)", ["d"+rand_text(chunk_size)])
session.run_sql("insert into testdb.data0 values (?)", ["e"+rand_text(chunk_size)])
session.run_sql("insert into testdb.data0 values (?)", ["f"+rand_text(12)])
session.run_sql("insert into testdb.data0 values (?)", ["g"+rand_text(chunk_size)])
session.run_sql("insert into testdb.data0 values ('\\n\\t\\\\')")

session.run_sql("create table testdb.data1 (n int)")
session.run_sql("insert into testdb.data1 values (1111111)" + ",(1111111)"*chunk_size)

session.run_sql("create table testdb.data2 (n int, pk int PRIMARY KEY AUTO_INCREMENT)")
session.run_sql("insert into testdb.data2 (n) values (1111111)" + ",(1111111)"*chunk_size)

session.run_sql("create table testdb.data3 (n int, pk int PRIMARY KEY AUTO_INCREMENT)")
session.run_sql("insert into testdb.data3 (n) values (1111111)" + ",(1111111)"*4)

set_test_table_count(4)

util.dump_instance(__tmp_dir+"/ldtest/dump-trxlimit", {"chunking":False})
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

EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 9 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 9 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> BUG#31961688 - fail after loading a sub-chunk from table which does not have PK, then resume {not __dbug_off}
# EXPECT: when resuming, table is truncated, all sub-chunks are loaded again
testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_END", "schema == testdb", "table == data0", "chunk == -1", "subchunk == 3"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_CONTAINS("NOTE: Truncating table `testdb`.`data0` because of resume and it has no PK or equivalent key")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 9 sub-chunks")

#@<> BUG#31961688 - fail before start of the data load has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, all sub-chunks are loaded
testutil.set_trap("dump_loader", ["op == BEFORE_LOAD_START", "schema == testdb", "table == data2", "chunk == -1"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")

#@<> BUG#31961688 - fail after start of the data load has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, all sub-chunks are loaded
testutil.set_trap("dump_loader", ["op == AFTER_LOAD_START", "schema == testdb", "table == data2", "chunk == -1"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")

#@<> BUG#31961688 - fail before start of the sub-chunk transaction has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, all sub-chunks are loaded again
testutil.set_trap("dump_loader", ["op == BEFORE_LOAD_SUBCHUNK_START", "schema == testdb", "table == data2", "chunk == -1", "subchunk == 0"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")

#@<> BUG#31961688 - fail after start of the sub-chunk transaction has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, all sub-chunks are loaded again
testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_START", "schema == testdb", "table == data2", "chunk == -1", "subchunk == 0"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")

#@<> BUG#31961688 - fail before end of the sub-chunk transaction has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, all sub-chunks are loaded again
testutil.set_trap("dump_loader", ["op == BEFORE_LOAD_SUBCHUNK_END", "schema == testdb", "table == data2", "chunk == -1", "subchunk == 0"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")

#@<> BUG#31961688 - fail after end of the sub-chunk transaction has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, sub-chunks which were finished are not loaded again
for subchunk in range(13):
  print("Testing sub-chunk: " + str(subchunk))
  testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_END", "schema == testdb", "table == data2", "chunk == -1", "subchunk == " + str(subchunk)], {"msg": "Injected exception"})
  EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
  EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")
  testutil.clear_traps("dump_loader")
  TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
  EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
  EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in {0} sub-chunks".format(13 - subchunk))

#@<> BUG#31961688 - fail after the last end of the sub-chunk transaction has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, an empty sub-chunk is loaded
testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_END", "schema == testdb", "table == data2", "chunk == -1", "subchunk == 13"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> BUG#31961688 - fail before end of the data load has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, an empty sub-chunk is loaded
testutil.set_trap("dump_loader", ["op == BEFORE_LOAD_END", "schema == testdb", "table == data2", "chunk == -1"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> BUG#31961688 - fail after end of the data load has been logged, then resume {not __dbug_off}
# EXPECT: when resuming, no sub-chunks are loaded
testutil.set_trap("dump_loader", ["op == AFTER_LOAD_END", "schema == testdb", "table == data2", "chunk == -1"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_NOT_CONTAINS("testdb@data2.tsv.zst")

#@<> BUG#31961688 - table which is not sub-chunked does not generate sub-chunking events {not __dbug_off}
# EXPECT: trap should not be triggered
testutil.set_trap("dump_loader", ["op == BEFORE_LOAD_SUBCHUNK_START", "schema == testdb", "table == data3", "chunk == -1"], {"msg": "Injected exception"})

EXPECT_NO_THROWS(lambda:TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit), "trap should not be triggered")

testutil.clear_traps("dump_loader")

#@<> BUG#31961688 - test uncompressed files {not __dbug_off}
util.dump_instance(os.path.join(__tmp_dir, "ldtest", "dump-trxlimit-none"), {"chunking": False, "compression": "none"})
hack_in_bytes_per_chunk("dump-trxlimit-none", chunk_size)

testutil.set_trap("dump_loader", ["op == AFTER_LOAD_SUBCHUNK_END", "schema == testdb", "table == data2", "chunk == -1", "subchunk == 5"], {"msg": "Injected exception"})

EXPECT_THROWS(lambda:TEST_LOAD("dump-trxlimit-none", trx_size_limit, trx_size_limit), "RuntimeError: Util.load_dump: Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv: Injected exception")

testutil.clear_traps("dump_loader")

TEST_LOAD("dump-trxlimit-none", trx_size_limit, trx_size_limit, False)
EXPECT_STDOUT_CONTAINS("NOTE: Load progress file detected. Load will be resumed from where it was left, assuming no external updates were made.")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 8 sub-chunks")

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

util.dump_instance(__tmp_dir+"/ldtest/dump-toobig", {"chunking":False})
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

util.dump_instance(__tmp_dir+"/ldtest/dump-toobig2", {"chunking":False})
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

util.dump_instance(__tmp_dir+"/ldtest/bug32072961", { "chunking": False })
TEST_LOAD("bug32072961", 1024 * 1024, 4 * 1024 * 1024 * 1024)

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
testutil.rmdir(__tmp_dir+"/ldtest", True)
