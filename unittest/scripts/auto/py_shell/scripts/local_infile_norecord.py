#@<> entry point
## imports
import os
import os.path

## constants
schema_name = "local-schema"
table_name = "local-table"
file_name = os.path.join(__tmp_dir, "local-infile.tsv")

#@<> setup
shell.connect(__mysqluripwd)
session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
session.run_sql("CREATE SCHEMA !", [schema_name])
session.run_sql("CREATE TABLE !.! (id INT PRIMARY KEY, data TEXT)", [schema_name, table_name])

old_local_infile = session.run_sql("SELECT @@GLOBAL.local_infile").fetch_one()[0]
session.run_sql("SET @@GLOBAL.local_infile = ON")

session.close()

with open(file_name, "wb") as f:
    f.write(bytes("1\tQWERTY\n", "utf-8"))
    f.write(bytes("2\tasdfg\n", "utf-8"))
    f.write(bytes("3\tZxCvB\n", "utf-8"))

#@<> X session does not support local-infile
for value in ["0", "false", "1", "true"]:
    EXPECT_THROWS(lambda: shell.connect(__uripwd + "?local-infile=" + value), "RuntimeError: X Protocol: LOAD DATA LOCAL INFILE is not supported.")

#@<> classic session with local-infile disabled
for value in ["0", "false"]:
    EXPECT_NO_THROWS(lambda: shell.connect(__mysqluripwd + "?local-infile=" + value), "Classic session should be established.")
    EXPECT_THROWS(lambda: session.run_sql("LOAD DATA LOCAL INFILE ? INTO TABLE !.!", [file_name, schema_name, table_name]), "DBError: MySQL Error (2068): LOAD DATA LOCAL INFILE file request rejected due to restrictions on access.")
    EXPECT_NO_THROWS(lambda: shell.disconnect(), "Classic session should be closed.")

#@<> classic session with local-infile enabled
for value in ["1", "true"]:
    EXPECT_NO_THROWS(lambda: shell.connect(__mysqluripwd + "?local-infile=" + value), "Classic session should be established.")
    EXPECT_NO_THROWS(lambda: session.run_sql("LOAD DATA LOCAL INFILE ? INTO TABLE !.!", [file_name, schema_name, table_name]), "Load should succeed.")
    EXPECT_NO_THROWS(lambda: session.run_sql("TRUNCATE TABLE !.!", [schema_name, table_name]), "Truncate should succeed.")
    EXPECT_NO_THROWS(lambda: shell.disconnect(), "Classic session should be closed.")

#@<> cleanup
shell.connect(__mysqluripwd)
session.run_sql("DROP SCHEMA IF EXISTS !", [schema_name])
session.run_sql("SET @@GLOBAL.local_infile = ?", [old_local_infile])
session.close()

os.remove(file_name)
