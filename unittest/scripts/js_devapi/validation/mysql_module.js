//@<ERR> getClassicSession errors
Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary (TypeError)
Argument #2 is expected to be a string (TypeError)

//@<ERR> getSession errors
Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary (TypeError)
Argument #2 is expected to be a string (TypeError)


//@<ERR> parseStatementAst errors
Argument #1 is expected to be a string (TypeError)
Argument #1 is expected to be a string (TypeError)

//@ parseStatementAst
||mismatched input 'this' expecting {<EOF>, '(', ALTER_SYMBOL, ANALYZE_SYMBOL, BEGIN_SYMBOL, BINLOG_SYMBOL, CACHE_SYMBOL, CALL_SYMBOL, CHANGE_SYMBOL, CHECKSUM_SYMBOL, CHECK_SYMBOL, COMMIT_SYMBOL, CREATE_SYMBOL, DEALLOCATE_SYMBOL, DELETE_SYMBOL, DESC_SYMBOL, DESCRIBE_SYMBOL, DO_SYMBOL, DROP_SYMBOL, EXECUTE_SYMBOL, EXPLAIN_SYMBOL, FLUSH_SYMBOL, GET_SYMBOL, GRANT_SYMBOL, HANDLER_SYMBOL, HELP_SYMBOL, IMPORT_SYMBOL, INSERT_SYMBOL, INSTALL_SYMBOL, KILL_SYMBOL, LOAD_SYMBOL, LOCK_SYMBOL, OPTIMIZE_SYMBOL, PREPARE_SYMBOL, PURGE_SYMBOL, RELEASE_SYMBOL, RENAME_SYMBOL, REPAIR_SYMBOL, REPLACE_SYMBOL, RESET_SYMBOL, RESIGNAL_SYMBOL, REVOKE_SYMBOL, ROLLBACK_SYMBOL, SAVEPOINT_SYMBOL, SELECT_SYMBOL, SET_SYMBOL, SHOW_SYMBOL, SHUTDOWN_SYMBOL, SIGNAL_SYMBOL, START_SYMBOL, STOP_SYMBOL, TABLE_SYMBOL, TRUNCATE_SYMBOL, UNINSTALL_SYMBOL, UNLOCK_SYMBOL, UPDATE_SYMBOL, USE_SYMBOL, VALUES_SYMBOL, WITH_SYMBOL, XA_SYMBOL, CLONE_SYMBOL, RESTART_SYMBOL} (RuntimeError)
||no viable alternative at input 'SELECT' (RuntimeError)

//@<OUT> splitScript
[
    "select 1"
]
[
    "select 1",
    "select 2"
]
[
    "select 1",
    "select 2;select3"
]
[
    "SELECT 1 ",
    "SELECT 2 ",
    "SELECT 3; SELECT 4"
]

//@<ERR> splitScript errors
Argument #1 is expected to be a string (TypeError)
Argument #1 is expected to be a string (TypeError)
