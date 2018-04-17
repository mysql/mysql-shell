//@# Setup
||

//@# No GTID
|Query OK, 1 row affected|

//@# With GTID
|Query OK, 0 rows affected|
|Query OK, 1 row affected|
|GTIDs:|

//@# With GTID rollback
|Query OK, 0 rows affected|
|Query OK, 1 row affected|
|Query OK, 0 rows affected|

//@# With GTID commit
|Query OK, 0 rows affected|
|Query OK, 1 row affected|
|Query OK, 0 rows affected|
|GTIDs:|

//@# Multiple GTIDs
|Query OK, 0 rows affected|
|Query OK, 1 row affected|
|GTIDs: |

//@# disable GTID globally
|Query OK, 0 rows affected|
|Switching to JavaScript mode...|

//@# cmdline - print GTID only if interactive (no GTID)
|0|
|Query OK, 1 row affected|
|0|

//@#enable GTID globally
|Query OK, 0 rows affected|
|Switching to JavaScript mode...|

//@# cmdline - print GTID only if interactive
|0|
|0|
|Query OK, 1 row affected|
|GTIDs:|
|0|

//@# Cleanup
||
