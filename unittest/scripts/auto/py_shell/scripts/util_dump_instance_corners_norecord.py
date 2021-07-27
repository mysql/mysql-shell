# imports
import json
import os
import os.path
import re
import shutil
import stat
import urllib.parse

#@<> INCLUDE dump_utils.inc

#@<> deploy sandbox
testutil.deploy_raw_sandbox(__mysql_sandbox_port1, "root")

shell.connect(__sandbox_uri1)

dumpdir = os.path.join(__tmp_dir, "dumptests")
shutil.rmtree(dumpdir, True)
os.makedirs(dumpdir)

#@<> a table with a varchar column in the PK
session.run_sql("create schema vctest")
session.run_sql("create table vctest.uuid (pk varchar(36) primary key, data longtext charset ascii)")
for i in range(100):
    session.run_sql("insert into vctest.uuid values (uuid(), repeat(%s, 200*1000))"%(i%10))

session.run_sql("analyze table vctest.uuid")

session.run_sql("show table status in vctest")

## dump with chunks slightly bigger than a row
outdir = os.path.join(dumpdir, "vctest")
util.dump_schemas(["vctest"], outdir, {"bytesPerChunk": "4M", "compression":"none"})
CHECK_OUTPUT_SANITY(outdir, min_chunk_size=1000*1000, min_chunks=5)


#@<> a table with rows bigger than the chunk size
session.run_sql("create schema test")
session.run_sql("create table test.bigrows (pk int primary key auto_increment, data longtext charset ascii)")
for i in range(10):
    session.run_sql("insert into test.bigrows values (default, repeat('x', 200*1000))")

session.run_sql("analyze table test.bigrows")

session.run_sql("show table status in test")

## dump with chunks slightly bigger than a row
outdir = os.path.join(dumpdir, "bigrows2")
util.dump_schemas(["test"], outdir, {"bytesPerChunk": "300K", "compression":"none"})
# there are 10 rows with 200K per row each, we want 128K chunks, so we should get ~10 chunks ideally but fewer is ok (as long as not just 1)
CHECK_OUTPUT_SANITY(outdir, min_chunk_size=200000, min_chunks=5)


## dump with chunks smaller than the rows
outdir = os.path.join(dumpdir, "bigrows")
util.dump_schemas(["test"], outdir, {"bytesPerChunk": "128K", "compression":"none"})
CHECK_OUTPUT_SANITY(outdir, min_chunk_size=200000, min_chunks=10)

#@<> a table with small avg rows, but a few very large ones
session.run_sql("create schema test2")
session.run_sql("create table test2.mostsmall (pk int primary key auto_increment, data longtext charset ascii)")
for i in range(100):
    if i in [20,30,40]:
        session.run_sql("insert into test2.mostsmall values (default, repeat('x', 200*1000))")
    elif i % 2 == 0:
        session.run_sql("insert into test2.mostsmall values (default, repeat('x', 200))")
    else:
        session.run_sql("insert into test2.mostsmall values (default, null)")

session.run_sql("analyze table test2.mostsmall")

session.run_sql("show table status in test2")

## dump with chunks slightly bigger than a row
outdir = os.path.join(dumpdir, "mostsmall")
util.dump_schemas(["test2"], outdir, {"bytesPerChunk": "300K", "compression":"none"})
CHECK_OUTPUT_SANITY(outdir, min_chunk_size=128, min_chunks=5)

## dump with chunks smaller than the rows
outdir = os.path.join(dumpdir, "mostsmall2")
util.dump_schemas(["test2"], outdir, {"bytesPerChunk": "128K", "compression":"none"})
CHECK_OUTPUT_SANITY(outdir, min_chunk_size=100, min_chunks=5)

#@<> BUG#32955616 - setup
dump_dir = os.path.join(dumpdir, "bug32955616")
tested_schema = "test_schema"
tested_table = "test_table"
rows = 5
row_length = 1500000

session.run_sql("CREATE SCHEMA !;", [ tested_schema ])
session.run_sql("CREATE TABLE !.! (id INT AUTO_INCREMENT PRIMARY KEY, data LONGBLOB)", [ tested_schema, tested_table ])

for i in range(rows):
    session.run_sql(f"INSERT INTO !.! (data) VALUES ('{random_string(row_length)}')", [ tested_schema, tested_table ])

session.run_sql("ANALYZE TABLE !.!;", [ tested_schema, tested_table ])

#@<> BUG#32955616 - test
# table has rows much bigger than bytesPerChunk, dumper should create a chunk for each row
util.dump_schemas([ tested_schema ], dump_dir, { "bytesPerChunk": "128K", "compression": "none" })
CHECK_OUTPUT_SANITY(dump_dir, min_chunk_size = row_length, min_chunks = rows)

#@<> BUG#32955616 - cleanup
session.run_sql("DROP SCHEMA !;", [ tested_schema ])

#@<> cleanup
testutil.destroy_sandbox(__mysql_sandbox_port1)
