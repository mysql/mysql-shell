# This is unit test file for WL13362 Support for importTable from multiple compressed and uncompressed files
import os

target_port = __mysql_port
target_xport = __port
target_schema = 'wl13362'
uri = "mysql://" + __mysqluripwd
xuri = "mysqlx://" + __uripwd

if __os_type != "windows":
    def filename_for_output(filename):
        return filename
else:
    def filename_for_output(filename):
        long_path_prefix = r"\\?" "\\"
        return long_path_prefix + filename.replace("/", "\\")

#@<> Setup test
shell.connect(uri)
session.run_sql('DROP SCHEMA IF EXISTS ' + target_schema)
session.run_sql('CREATE SCHEMA ' + target_schema)
session.run_sql('USE ' + target_schema)
session.run_sql('DROP TABLE IF EXISTS `lorem`')
session.run_sql("CREATE TABLE `lorem` (`id` int primary key, `part` text) ENGINE=InnoDB CHARSET=utf8mb4")

#@<> Set local_infile to true
session.run_sql('SET GLOBAL local_infile = true')

#@<> Retrieve directory content
chunked_dir = os.path.join(__import_data_path, "chunked")
dircontent = os.listdir(chunked_dir)
raw_files = sorted(list(filter(lambda x: x.endswith(".tsv"), dircontent)))
gz_files = sorted(list(filter(lambda x: x.endswith(".gz"), dircontent)))
zst_files = sorted(list(filter(lambda x: x.endswith(".zst"), dircontent)))
print("raw_files", raw_files)
print("gz_files", gz_files)
print("zst_files", zst_files)

#@<> Variations about empty file list
rc = testutil.call_mysqlsh([uri, '--schema=' + target_schema, '--', 'util', 'import-table', '', '--table=lorem'])
EXPECT_EQ(1, rc)
EXPECT_STDOUT_CONTAINS("Util.importTable: File list cannot be empty.")

rc = testutil.call_mysqlsh([uri, '--schema=' + target_schema, '--', 'util', 'import-table', '', '', '--table=lorem'])
EXPECT_EQ(1, rc)
EXPECT_STDOUT_CONTAINS("Util.importTable: File list cannot be empty.")

EXPECT_THROWS(lambda: util.import_table('', {'table': 'lorem'}), "RuntimeError: Util.import_table: File list cannot be empty.")
EXPECT_THROWS(lambda: util.import_table('', '', {'table': 'lorem'}), "RuntimeError: Util.import_table: File list cannot be empty.")
EXPECT_THROWS(lambda: util.import_table([], {'table': 'lorem'}), "RuntimeError: Util.import_table: File list cannot be empty.")
EXPECT_THROWS(lambda: util.import_table([''], {'table': 'lorem'}), "RuntimeError: Util.import_table: File list cannot be empty.")
EXPECT_THROWS(lambda: util.import_table(['', ''], {'table': 'lorem'}), "RuntimeError: Util.import_table: File list cannot be empty.")

#@<> Single file - cli
rc = testutil.call_mysqlsh([uri, '--schema=' + target_schema, '--', 'util', 'import-table', chunked_dir + os.path.sep + raw_files[0], '--table=lorem'])
EXPECT_EQ(0, rc)
EXPECT_STDOUT_CONTAINS(raw_files[0] + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Single file
util.import_table(chunked_dir + os.path.sep + raw_files[0], {'schema': target_schema, 'table': 'lorem'})
EXPECT_STDOUT_CONTAINS(raw_files[0] + ": Records: 100  Deleted: 0  Skipped: 100  Warnings: 100")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 100  Deleted: 0  Skipped: 100  Warnings: 100")

#@<> Single file - array arg
util.import_table(chunked_dir + os.path.sep + raw_files[0], {'schema': target_schema, 'table': 'lorem'})
EXPECT_STDOUT_CONTAINS(raw_files[0] + ": Records: 100  Deleted: 0  Skipped: 100  Warnings: 100")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 100  Deleted: 0  Skipped: 100  Warnings: 100")

#@<> Wildcard to single file - cli
rc = testutil.call_mysqlsh([uri, '--', 'util', 'import-table', chunked_dir + os.path.sep + 'lorem_aa*', '--schema=' + target_schema, '--table=lorem'])
EXPECT_EQ(0, rc)
EXPECT_STDOUT_CONTAINS("lorem_aaa.tsv: Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("1 file(s) (2.49 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Wildcard to single file
util.import_table(chunked_dir + os.path.sep + 'lorem_aa*', {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("lorem_aaa.tsv: Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("1 file(s) (2.49 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Wildcard to single file - array arg
util.import_table([chunked_dir + os.path.sep + 'lorem_aa*'], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("lorem_aaa.tsv: Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("1 file(s) (2.49 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Expand wildcard to 0 files - cli
rc = testutil.call_mysqlsh([uri, '--', 'util', 'import-table', chunked_dir + os.path.sep + 'lorem_xx*', '--schema=' + target_schema, '--table=lorem'])
EXPECT_EQ(0, rc)
EXPECT_STDOUT_CONTAINS("0 file(s) (0 bytes) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Expand wildcard to 0 files
util.import_table(chunked_dir + os.path.sep + 'lorem_xx*', {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("0 file(s) (0 bytes) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Expand wildcard to 0 files - array arg
util.import_table([chunked_dir + os.path.sep + 'lorem_xx*'], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("0 file(s) (0 bytes) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Import single non-existing file
EXPECT_THROWS(lambda: util.import_table([chunked_dir + os.path.sep + 'lorem_xxx_xxx.tsv'], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True}),
    "Cannot open file '"+ filename_for_output(chunked_dir + os.path.sep + 'lorem_xxx_xxx.tsv') +"': No such file or directory"
)

#@<> Import multiple non-existing files
util.import_table([chunked_dir + os.path.sep + 'lorem_xxx_xxx.tsv', 'lorem_xxx_ccc.tsv'], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("ERROR: File " + filename_for_output(chunked_dir + os.path.sep + 'lorem_xxx_xxx.tsv') + " does not exists.")
EXPECT_STDOUT_CONTAINS("ERROR: File " + filename_for_output(os.path.join(os.path.abspath(os.path.curdir), "lorem_xxx_ccc.tsv")) + " does not exists.")
EXPECT_STDOUT_CONTAINS("0 file(s) (0 bytes) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 0  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Wildcard files from non-existing directory
util.import_table([os.path.join(chunked_dir, "nonexisting", 'lorem_*.tsv')], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("ERROR: Directory " + filename_for_output(os.path.join(chunked_dir, "nonexisting")) + " does not exists.")

#@<> Select single file from non-existing directory
EXPECT_THROWS(lambda: util.import_table([os.path.join(chunked_dir, "nonexisting", 'lorem_abc.tsv')], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True}),
    "Util.import_table: Cannot open file '" + filename_for_output(os.path.join(chunked_dir, "nonexisting", 'lorem_abc.tsv')) + "': No such file or directory"
)

#@<> Select multiple files from non-existing directory
util.import_table([os.path.join(chunked_dir, "nonexisting", 'lorem_abc.tsv'), os.path.join(chunked_dir, "nonexisting", 'lorem_abq.tsv')], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("ERROR: File " + filename_for_output(os.path.join(chunked_dir, "nonexisting", 'lorem_abc.tsv')) + " does not exists.")
EXPECT_STDOUT_CONTAINS("ERROR: File " + filename_for_output(os.path.join(chunked_dir, "nonexisting", 'lorem_abq.tsv')) + " does not exists.")

#@<> Select multiple files with single wildcard from directory
util.import_table([os.path.join(chunked_dir, 'lorem_a*.tsv')], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
for name in [f for f in raw_files if f.startswith("lorem_a") and f.endswith(".tsv")]:
    EXPECT_STDOUT_CONTAINS(name + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("6 file(s) (14.75 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 600  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Select multiple files with multiple wildcards from directory
util.import_table([os.path.join(chunked_dir, 'lorem_a*.tsv'), os.path.join(chunked_dir, 'lorem_b*.tsv.gz'), os.path.join(chunked_dir, 'lorem_c*.tsv.zst')], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
for name in [f for f in dircontent if (f.startswith("lorem_a") and f.endswith(".tsv")) or (f.startswith("lorem_b") and f.endswith(".tsv.gz")) or (f.startswith("lorem_c") and f.endswith(".tsv.zst"))]:
    EXPECT_STDOUT_CONTAINS(name + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("17 file(s) (42.16 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 1700  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Single compressed file
util.import_table([os.path.join(chunked_dir, zst_files[0])], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS(zst_files[0] + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("File '" + filename_for_output(os.path.join(chunked_dir, zst_files[0])) + "' (1.26 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Multiple compressed files
util.import_table([os.path.join(chunked_dir, zst_files[0]), os.path.join(chunked_dir, zst_files[1]), os.path.join(chunked_dir, gz_files[0])], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS(zst_files[0] + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS(zst_files[1] + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS(gz_files[0] + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS("3 file(s) (7.48 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 300  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Mixed file list input
util.import_table([os.path.join(chunked_dir, "nonexisting_a.csv"), os.path.join(chunked_dir, "lorem_a*"), os.path.join(chunked_dir, "lorem_b*"), '', os.path.join(chunked_dir, zst_files[0]), os.path.join(chunked_dir, zst_files[1]), os.path.join(chunked_dir, gz_files[0])], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True})
EXPECT_STDOUT_CONTAINS("ERROR: File " + filename_for_output(os.path.join(chunked_dir, "nonexisting_a.csv")) + " does not exists.")
EXPECT_STDOUT_CONTAINS(zst_files[0] + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS(zst_files[1] + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
EXPECT_STDOUT_CONTAINS(gz_files[0] + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")
for name in [f for f in dircontent if f.startswith("lorem_a") or f.startswith("lorem_b")]:
    EXPECT_STDOUT_CONTAINS(name + ": Records: 100  Deleted: 0  Skipped: 0  Warnings: 0")

EXPECT_STDOUT_CONTAINS("19 file(s) (47.15 KB) was imported in ")
EXPECT_STDOUT_CONTAINS("Total rows affected in " + target_schema + ".lorem: Records: 1900  Deleted: 0  Skipped: 0  Warnings: 0")

#@<> Multifile import does not support byterPerChunk option
EXPECT_THROWS(lambda: util.import_table([os.path.join(chunked_dir, "nonexisting_a.csv"), os.path.join(chunked_dir, "lorem_a*"), '', os.path.join(chunked_dir, zst_files[0]), os.path.join(chunked_dir, zst_files[1]), os.path.join(chunked_dir, gz_files[0])], {'schema': target_schema, 'table': 'lorem', 'replaceDuplicates': True, 'bytesPerChunk': '1M'}),
    "Util.import_table: Invalid options: bytesPerChunk"
)

#@<> Missing target table + single path with wildcard
EXPECT_THROWS(lambda: util.import_table(os.path.join(chunked_dir, "lorem*.tsv"), {'schema': target_schema}),
    "Util.import_table: Target table is not set. The target table for the import operation must be provided in the options."
)

#@<> Missing target table + multiple expanded paths
EXPECT_THROWS(lambda: util.import_table([os.path.join(chunked_dir, raw_files[0]), os.path.join(chunked_dir, raw_files[1])], {'schema': target_schema}),
    "Util.import_table: Target table is not set. The target table for the import operation must be provided in the options."
)

#@<> Cleanup
session.close()
