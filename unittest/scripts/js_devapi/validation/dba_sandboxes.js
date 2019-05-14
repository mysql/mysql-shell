//@ Deploy sandbox in dir with space
||

//@<OUT> BUG#29634828 AdminAPI should handle localhost and sandboxes better
+---------------+
| @@report_host |
+---------------+
| 127.0.0.1     |
+---------------+

//@ Stop sandbox in dir with space
||

//@ Delete sandbox in dir with space
||

//@ Deploy sandbox in dir with long path
||

//@ Stop sandbox in dir with long path
||

//@ Delete sandbox in dir with long path
||

//@ Deploy sandbox in dir with non-ascii characters.
||

//@ Stop sandbox in dir with non-ascii characters.
||

//@ Delete sandbox in dir with non-ascii characters.
||

//@ SETUP BUG@29725222 add restart support to sandboxes {VER(>= 8.0.17)}
||

//@ BUG@29725222 test that restart works {VER(>= 8.0.17)}
|Query OK, 0 rows affected |

//@ TEARDOWN BUG@29725222 add restart support to sandboxes {VER(>= 8.0.17)}
||
