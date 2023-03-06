#@{__version_num > 80018 or __version_num < 80000}

import os.path
import random
import json

def rand_text(size):
    return "["+ "".join(random.choices('0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&()*+,-./:;<=>?@^_{|}~', k=size-2)) +"]"

def TEST_LOAD(net_buffer_length, max_binlog_cache_size, options = {}):
    WIPE_SHELL_LOG()
    WIPE_STDOUT()
    if "maxBytesPerTransaction" not in options:
        global max_bytes_per_trx
        options["maxBytesPerTransaction"] = str(max_bytes_per_trx)
    if "chunking" not in options:
        options["chunking"] = False
    if "showProgress" not in options:
        options["showProgress"] = False
    if "users" not in options:
        options["users"] = False
    wipeout_server(tgt_session)
    tgt_session.run_sql(f"set global max_binlog_cache_size={max_binlog_cache_size}")
    setup_session(__sandbox_uri1)
    util.copy_instance(f"{__sandbox_uri2}?net-buffer-length={net_buffer_length}", options)
    for i in range(k_last_table):
        EXPECT_EQ(checksum[i], tgt_session.run_sql(f'checksum table testdb.data{i}').fetch_one()[1])
        EXPECT_EQ(rcount[i], tgt_session.run_sql(f'select count(*) from testdb.data{i}').fetch_one()[0])

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
        checksum.append(src_session.run_sql(f'checksum table testdb.data{i}').fetch_one()[1])
        rcount.append(src_session.run_sql(f'select count(*) from testdb.data{i}').fetch_one()[0])

trx_size_limit = 1024*32
chunk_size = trx_size_limit//2
max_bytes_per_trx = chunk_size

#@<> INCLUDE dump_utils.inc
#@<> INCLUDE copy_utils.inc

#@<> Setup
setup_copy_tests(2)

#@<> load dump with chunks bigger than max_binlog_cache_size (prep)
wipeout_server(src_session)
# create dump with large chunks at various row boundaries
src_session.run_sql("create schema testdb")
src_session.run_sql("create table testdb.data0 (txt longtext charset ascii)")
# a few rows that are small enough to fit
src_session.run_sql("insert into testdb.data0 values (?)", ["1"+rand_text(500)])
src_session.run_sql("insert into testdb.data0 values (?)", ["2"+rand_text(500)])
src_session.run_sql("insert into testdb.data0 values (null)")
src_session.run_sql("insert into testdb.data0 values (?)", ["4"+rand_text(500)])
src_session.run_sql("insert into testdb.data0 values (?)", ["5"+rand_text(500)])
# 2 big rows that will require the chunk to be flushed at the previous row
src_session.run_sql("insert into testdb.data0 values (?)", ["6"+rand_text(chunk_size//2)])
src_session.run_sql("insert into testdb.data0 values (?)", ["7"+rand_text(chunk_size//2)])
# single row chunk
src_session.run_sql("insert into testdb.data0 values (?)", ["8"+rand_text(chunk_size - 2)])
# a bunch of smaller rows
src_session.run_sql("insert into testdb.data0 values (?)", ["9"+rand_text(10)])
src_session.run_sql("insert into testdb.data0 values (?)", ["a"+rand_text(34)])
src_session.run_sql("insert into testdb.data0 values (?)", ["b"+rand_text(12)])
src_session.run_sql("insert into testdb.data0 values (?)", ["c"+rand_text(150)])
# at the limit, subtracting two bytes: one for the row identifier and one for the newline character
src_session.run_sql("insert into testdb.data0 values (?)", ["d"+rand_text(chunk_size - 2)])
src_session.run_sql("insert into testdb.data0 values (?)", ["e"+rand_text(chunk_size - 2)])
src_session.run_sql("insert into testdb.data0 values (?)", ["f"+rand_text(12)])
src_session.run_sql("insert into testdb.data0 values (?)", ["g"+rand_text(chunk_size - 2)])
src_session.run_sql("insert into testdb.data0 values ('\\n\\t\\\\')")

src_session.run_sql("create table testdb.data1 (n int)")
src_session.run_sql("insert into testdb.data1 values (1111111)" + ",(1111111)"*chunk_size)

src_session.run_sql("create table testdb.data2 (n int, pk int PRIMARY KEY AUTO_INCREMENT)")
src_session.run_sql("insert into testdb.data2 (n) values (1111111)" + ",(1111111)"*chunk_size)

src_session.run_sql("create table testdb.data3 (n int, pk int PRIMARY KEY AUTO_INCREMENT)")
src_session.run_sql("insert into testdb.data3 (n) values (1111111)" + ",(1111111)"*4)

set_test_table_count(4)

# WL15298_TSFR_4_5_3
# WL15298_TSFR_4_5_4
# WL15298_TSFR_4_5_5

#@<> net_buffer = trx_limit
TEST_LOAD(trx_size_limit, trx_size_limit)

#@<> net_buffer = trx_limit 2
TEST_LOAD(trx_size_limit*4, trx_size_limit*4)

#@<> net_buffer < trx_limit
TEST_LOAD(2048, trx_size_limit)

#@<> net_buffer < trx_limit 2
TEST_LOAD(4096, trx_size_limit*3)

#@<> net_buffer < trx_limit 3
TEST_LOAD(4096, trx_size_limit*10)

#@<> net_buffer < trx_limit 4
TEST_LOAD(trx_size_limit, trx_size_limit*10)

#@<> net_buffer > trx_limit
TEST_LOAD(trx_size_limit*2, trx_size_limit)

#@<> net_buffer > trx_limit 2
TEST_LOAD(trx_size_limit*3, trx_size_limit)

#@<> net_buffer > trx_limit 3
TEST_LOAD(trx_size_limit*4, trx_size_limit*2)

EXPECT_SHELL_LOG_CONTAINS("testdb@data0.tsv: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 9 sub-chunks")
EXPECT_SHELL_LOG_CONTAINS("testdb@data1.tsv: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 9 sub-chunks")
EXPECT_SHELL_LOG_CONTAINS("testdb@data2.tsv: Records: 382  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 14 sub-chunks")
EXPECT_SHELL_LOG_CONTAINS("testdb@data3.tsv: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> impossible load: row too big

# this table will generate a chunk with a row that's too big to be loaded
wipeout_server(src_session)
src_session.run_sql("create schema testdb")
src_session.run_sql("create table testdb.data0 (txt longtext charset ascii)")
src_session.run_sql("insert into testdb.data0 values (?)", [rand_text(1)])
src_session.run_sql("insert into testdb.data0 values (null)")
src_session.run_sql("insert into testdb.data0 values (?)", [rand_text(trx_size_limit*10)])
src_session.run_sql("insert into testdb.data0 values (?)", [rand_text(trx_size_limit*2)])
src_session.run_sql("insert into testdb.data0 values (?)", [rand_text(20)])

set_test_table_count(1)

#@<> row > max_binlog_cache_size - fail from chunk too big
# note: use 4098 as binlog_cache_size b/c setting it to trx_size_limit is somehow allowing bigger transactions through
EXPECT_THROWS(lambda:TEST_LOAD(4098, 4098), "Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv: MySQL Error 1197 (HY000): Multi-statement transaction required more than 'max_binlog_cache_size' bytes of storage")

#@<> row > max_binlog_cache_size < net-buffer-length - fail from chunk too big
EXPECT_THROWS(lambda:TEST_LOAD(trx_size_limit, 4098), "Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv: MySQL Error 1197 (HY000): Multi-statement transaction required more than 'max_binlog_cache_size' bytes of storage")

#@<> impossible load: deceptively small row sizes

# each row of this table will be 255+ bytes long, but the dump data is just 1 byte
# this is a corner case where the dump size and estimated transaction size
# will be much smaller than the actual transaction size, which will break the
# sub-chunking logic. In practice, if a user falls into this case, they should
# specify a smaller chunk size.
src_session.run_sql("set global max_binlog_cache_size=?", [1024*1024*1024])
wipeout_server(src_session)
src_session.run_sql("create schema testdb")
src_session.run_sql("create table testdb.data0 (n char(255))")
s = "insert into testdb.data0 values ('.')"
s += ",('.')"*trx_size_limit
src_session.run_sql(s)

set_test_table_count(1)

#@<> rowset > max_binlog_cache_size - fail from chunk too big
EXPECT_THROWS(lambda:TEST_LOAD(4098, trx_size_limit//2), "Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv: MySQL Error 1197 (HY000): Multi-statement transaction required more than 'max_binlog_cache_size' bytes of storage")

#@<> rowset > max_binlog_cache_size < net-buffer-length - fail from chunk too big
EXPECT_THROWS(lambda:TEST_LOAD(trx_size_limit, trx_size_limit//2), "Error loading dump")
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv: MySQL Error 1197 (HY000): Multi-statement transaction required more than 'max_binlog_cache_size' bytes of storage")

#@<> rowset > max_binlog_cache_size - should work if we reduce bytesPerChunk
TEST_LOAD(4098, trx_size_limit//2, { "maxBytesPerTransaction": str(chunk_size//2) })

#@<> rowset > max_binlog_cache_size < net-buffer-length - should work if we reduce bytesPerChunk
TEST_LOAD(trx_size_limit, trx_size_limit//2, { "maxBytesPerTransaction": str(chunk_size//2) })

#@<> Cleanup
cleanup_copy_tests()
