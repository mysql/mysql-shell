#@{__version_num > 80018 or __version_num < 80000}

import os
import random

def rand_text(size):
    return "["+ "".join(random.choices('0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&()*+,-./:;<=>?@^_{|}~', k=size-2)) +"]"


def TEST_LOAD(dump, net_buffer_length, max_binlog_cache_size):
    WIPE_STDOUT()
    session.run_sql("set global max_binlog_cache_size=?", [max_binlog_cache_size])
    session.close()
    shell.connect(__sandbox_uri1+"?net-buffer-length=%s"%net_buffer_length)
    for i in range(k_last_table):
        session.run_sql("truncate table testdb.data%s" % i)
    for i in range(k_last_table):
        util.import_table(f"{TESTDIRROOT}/{dump}/testdb@data{i}*.tsv.zst", {
            "schema": "testdb",
            "table": f"data{i}",
            "maxBytesPerTransaction": str(max_binlog_cache_size)
        })
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


trx_size_limit = 1024*32
chunk_size = trx_size_limit//2


#@<> INCLUDE dump_utils.inc

#@<> Setup
TESTDIRROOT = os.path.join(__tmp_dir, "subctest")
try:
  testutil.rmdir(TESTDIRROOT, True)
except:
  pass
testutil.mkdir(TESTDIRROOT)

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

util.dump_instance(os.path.join(TESTDIRROOT, "dump-trxlimit"), {"chunking":False})

#@<> net_buffer = trx_limit
TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit)
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 2  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 5 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 5 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 1552  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 7 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> net_buffer = trx_limit 2
TEST_LOAD("dump-trxlimit", trx_size_limit*4, trx_size_limit*4)
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 17  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 2 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 6230  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 2 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")


#@<> net_buffer < trx_limit
TEST_LOAD("dump-trxlimit", 2048, trx_size_limit)
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 2  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 5 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 5 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 1552  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 7 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> net_buffer < trx_limit 2
TEST_LOAD("dump-trxlimit", 4096, trx_size_limit*3)
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 17  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Records: 4097  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 2 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 1549  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 3 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> net_buffer < trx_limit 3
TEST_LOAD("dump-trxlimit", 4096, trx_size_limit*10)
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 17  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Records: 16385  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 16385  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> net_buffer < trx_limit 4
TEST_LOAD("dump-trxlimit", trx_size_limit, trx_size_limit*10)
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 17  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Records: 16385  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 16385  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> net_buffer > trx_limit
TEST_LOAD("dump-trxlimit", trx_size_limit*2, trx_size_limit)
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 2  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 5 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 5 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 1552  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 7 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> net_buffer > trx_limit 2
TEST_LOAD("dump-trxlimit", trx_size_limit*3, trx_size_limit)
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 2  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 5 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 5 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 1552  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 7 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> net_buffer > trx_limit 3
TEST_LOAD("dump-trxlimit", trx_size_limit*4, trx_size_limit*2)
EXPECT_STDOUT_CONTAINS("testdb@data0.tsv.zst: Records: 4  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 2 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data1.tsv.zst: Records: 1  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 3 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data2.tsv.zst: Records: 1549  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 4 sub-chunks")
EXPECT_STDOUT_CONTAINS("testdb@data3.tsv.zst: Records: 5  Deleted: 0  Skipped: 0  Warnings: 0 - loading finished in 1 sub-chunks")

#@<> Negavtive maxBytesPerTransaction
EXPECT_THROWS(lambda: util.import_table(f"{TESTDIRROOT}/dump-trxlimit/testdb@data1*.tsv.zst", {
    "schema": "testdb",
    "table": "data1",
    "maxBytesPerTransaction": "-1"}),
    'ValueError: Util.import_table: Argument #2: Input number "-1" cannot be negative'
)

#@<> maxBytesPerTransaction less than minimal (4096) value
EXPECT_THROWS(lambda: util.import_table(f"{TESTDIRROOT}/dump-trxlimit/testdb@data1*.tsv.zst", {
    "schema": "testdb",
    "table": "data1",
    "maxBytesPerTransaction": "2048"}),
    "ValueError: Util.import_table: Argument #2: The value of 'maxBytesPerTransaction' option must be greater than or equal to 4096 bytes."
)

#@<> Cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
