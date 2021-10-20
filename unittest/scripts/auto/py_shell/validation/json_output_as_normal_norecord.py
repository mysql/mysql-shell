#@<OUT> Normal Dump
Initializing...
Acquiring global read lock
Global read lock acquired
Initializing - done
Gathering information...
Gathering information - done
All transactions have been started
Locking instance for backup
Global read lock has been released
Writing global DDL files
Writing users DDL
Writing schema metadata...
Writing DDL...
Writing table metadata...
Running data dump using 4 threads.
Dumping data...
{{NOTE: |}}Table statistics not available for `schema_c`.`table_[[*]]`, chunking operation may be not optimal. Please consider running 'ANALYZE TABLE `schema_c`.`table_[[*]]`;' first.
{{NOTE: |}}Table statistics not available for `schema_c`.`table_[[*]]`, chunking operation may be not optimal. Please consider running 'ANALYZE TABLE `schema_c`.`table_[[*]]`;' first.
{{NOTE: |}}Table statistics not available for `schema_c`.`table_[[*]]`, chunking operation may be not optimal. Please consider running 'ANALYZE TABLE `schema_c`.`table_[[*]]`;' first.
Writing schema metadata - done
Writing DDL - done
Writing table metadata - done
Starting data dump
Dumping data - done
Dump duration: [[*]]
Total duration: [[*]]
Schemas dumped: 1
Tables dumped: 3
Uncompressed data size: [[*]] bytes
Compressed data size: [[*]] bytes
Compression ratio: [[*]]
Rows written: [[*]]
Bytes written: [[*]] bytes
Average uncompressed throughput: [[*]]
Average compressed throughput: [[*]]


#@<OUT> JSON Dump
{"status":"Initializing...\n"}
{"info":"Acquiring global read lock\n"}
{"info":"Global read lock acquired\n"}
{"status":"Initializing - done\n"}
{"status":"Gathering information...\n"}
{"status":"Gathering information - done\n"}
{"info":"All transactions have been started\n"}
{"info":"Locking instance for backup\n"}
{"info":"Global read lock has been released\n"}
{"status":"Writing global DDL files\n"}
{"status":"Writing users DDL\n"}
{"status":"Writing schema metadata...\n"}
{"status":"Writing DDL...\n"}
{"status":"Writing table metadata...\n"}
{"status":"Running data dump using 4 threads.\n"}
{"status":"Dumping data...\n"}
{"note":"Table statistics not available for `schema_c`.`table_[[*]]`, chunking operation may be not optimal. Please consider running 'ANALYZE TABLE `schema_c`.`table_[[*]]`;' first.\n"}
{"note":"Table statistics not available for `schema_c`.`table_[[*]]`, chunking operation may be not optimal. Please consider running 'ANALYZE TABLE `schema_c`.`table_[[*]]`;' first.\n"}
{"note":"Table statistics not available for `schema_c`.`table_[[*]]`, chunking operation may be not optimal. Please consider running 'ANALYZE TABLE `schema_c`.`table_[[*]]`;' first.\n"}
{"status":"Writing schema metadata - done\n"}
{"status":"Writing DDL - done\n"}
{"status":"Writing table metadata - done\n"}
{"status":"Starting data dump\n"}
{"status":"Dumping data - done\n"}
{"status":"Dump duration: [[*]]\n"}
{"status":"Total duration: [[*]]\n"}
{"status":"Schemas dumped: [[*]]\n"}
{"status":"Tables dumped: [[*]]\n"}
{"status":"Uncompressed data size: [[*]]\n"}
{"status":"Compressed data size: [[*]]\n"}
{"status":"Compression ratio: [[*]]\n"}
{"status":"Rows written: [[*]]\n"}
{"status":"Bytes written: [[*]]\n"}
{"status":"Average uncompressed throughput: [[*]]\n"}
{"status":"Average compressed throughput: [[*]]\n"}