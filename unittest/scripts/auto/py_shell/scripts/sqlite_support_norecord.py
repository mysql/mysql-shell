import sqlite3
import os
import sys
sys.stdout = sys.__stdout__

def is_sqlite3(path):
    with open(path, "rb") as fh:
        magic = b"SQLite format 3\000"
        sqlite_header_size = 100
        content = fh.read(sqlite_header_size)
        if len(content) < sqlite_header_size:
            return False
        return magic == content[:len(magic)]

def drop_db(dbpath):
    if os.path.exists(dbpath) and os.path.isfile(dbpath):
        os.remove(dbpath)


def test_full_text_search(module_name):
    try:
        db = sqlite3.connect(":memory:")
        with db:
            db.execute("create virtual table recipe using {}(name, ingredients)".format(module_name))
            db.executescript("""
                insert into recipe (name, ingredients) values ('broccoli stew', 'broccoli peppers cheese tomatoes');
                insert into recipe (name, ingredients) values ('pumpkin stew', 'pumpkin onions garlic celery');
                insert into recipe (name, ingredients) values ('broccoli pie', 'broccoli cheese onions flour');
                insert into recipe (name, ingredients) values ('pumpkin pie', 'pumpkin sugar flour butter');
                """)
            r = db.execute("select rowid, name, ingredients from recipe where name match 'pie'").fetchall()
            EXPECT_EQ([(3, 'broccoli pie', 'broccoli cheese onions flour'), (4, 'pumpkin pie', 'pumpkin sugar flour butter')], r)
    except sqlite3.OperationalError as e:
        print("{}:".format(module_name), e)
    finally:
        db.close()


#@<> SQLite module and library information
print("sqlite3.version:", sqlite3.version)
print("sqlite3.version_info:", sqlite3.version_info)
print("sqlite3.sqlite_version:", sqlite3.sqlite_version)
print("sqlite3.sqlite_version_info:", sqlite3.sqlite_version_info)
print("sqlite3.__file__:", sqlite3.__file__)
try:
    import _sqlite3
    print("_sqlite3.__file__:", _sqlite3.__file__)
except Exception:
    pass

try:
    import _ssl
    print("_ssl.__file__:", _ssl.__file__)
except Exception:
    pass


#@<> sqlite module compile options info
db = sqlite3.connect(":memory:")
with db:
    r = db.execute("pragma compile_options").fetchall()
    print("Compile options:")
    print("\n".join(map(lambda x: ";".join(x), r)))
db.close()


#@<> clean test environment
dbpath = "simple.sqlite"
drop_db(dbpath)


#@<> create simple db
db = sqlite3.connect(dbpath)
with db:
    db.execute("create table people (name text primary key)")
    db.execute("insert into people values ('Adam'), ('Bob'), ('Grace')")
db.close()


#@<> check if header is correct
EXPECT_EQ(True, is_sqlite3(dbpath))


#@<> read from simple db
db = sqlite3.connect(dbpath)
with db:
    r = db.execute("select * from people").fetchall()
    EXPECT_EQ([('Adam',), ('Bob',), ('Grace',)], r)
db.close()


#@<> drop simple db
drop_db(dbpath)


#@<> json support
try:
    db = sqlite3.connect(":memory:")
    with db:
        r = db.execute("select json_array(1,2,'3',4)").fetchall()
        EXPECT_EQ([('[1,2,"3",4]',)], r)
        r = db.execute("""select json(' { "this" : "is", "a": [ "test" ] } ')""").fetchall()
        EXPECT_EQ([('{"this":"is","a":["test"]}',)], r)
        r = db.execute("""select json_object('ex',json('[52,3.14159]'))""").fetchall()
        EXPECT_EQ([('{"ex":[52,3.14159]}',)], r)
except sqlite3.OperationalError as e:
    print("json:", e)
finally:
    db.close()


#@<> Full-text search v3 support
test_full_text_search("fts3")


#@<> Full-text search v4 support
test_full_text_search("fts4")


#@<> Full-text search v5 support
test_full_text_search("fts5")


#@<> R-tree support
try:
    db = sqlite3.connect(":memory:")
    with db:
        db.execute("""CREATE VIRTUAL TABLE denylist USING rtree_i32(id, low_ip, high_ip, low_port, high_port);""")
        # deny ip range 127.0.0.1-127.0.255.255 with port range 0-1024
        db.execute("""INSERT INTO denylist VALUES(1, 0x7F000001, 0x7F00ffff, 0, 1024)""")
        check_ip = 0x7f000101  # 127.0.1.1
        http_port = 80
        ephemeral_port = 46327
        r = db.execute("""SELECT * FROM denylist WHERE low_ip <= ? and ? <= high_ip and low_port <= ? and ? <= high_port""", (check_ip, check_ip, http_port, http_port)).fetchall()
        EXPECT_EQ([(1, 2130706433, 2130771967, 0, 1024)], r)
        r = db.execute("""SELECT * FROM denylist WHERE low_ip <= ? and ? <= high_ip and low_port <= ? and ? <= high_port""", (check_ip, check_ip, ephemeral_port, ephemeral_port)).fetchall()
        EXPECT_EQ([], r)
except sqlite3.OperationalError as e:
    print("rtree:", e)
finally:
    db.close()

print(flush=True)
