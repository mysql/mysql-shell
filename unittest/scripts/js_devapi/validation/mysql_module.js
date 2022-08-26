//@<ERR> getClassicSession errors
mysql.getClassicSession: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
mysql.getClassicSession: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
mysql.getClassicSession: Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary (TypeError)
mysql.getClassicSession: Argument #2 is expected to be a string (TypeError)

//@<ERR> getSession errors
mysql.getSession: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
mysql.getSession: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
mysql.getSession: Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary (TypeError)
mysql.getSession: Argument #2 is expected to be a string (TypeError)


//@<ERR> parseStatementAst errors
mysql.parseStatementAst: Argument #1 is expected to be a string (TypeError)
mysql.parseStatementAst: Argument #1 is expected to be a string (TypeError)

//@ parseStatementAst
||mysql.parseStatementAst: mismatched input 'this' expecting {<EOF>, ALTER_SYMBOL, ANALYZE_SYMBOL, BEGIN_SYMBOL, BINLOG_SYMBOL, CACHE_SYMBOL, CALL_SYMBOL, CHANGE_SYMBOL, CHECKSUM_SYMBOL, CHECK_SYMBOL, COMMIT_SYMBOL, CREATE_SYMBOL, DEALLOCATE_SYMBOL, DELETE_SYMBOL, DESC_SYMBOL, DESCRIBE_SYMBOL, DO_SYMBOL, DROP_SYMBOL, EXECUTE_SYMBOL, EXPLAIN_SYMBOL, FLUSH_SYMBOL, GET_SYMBOL, GRANT_SYMBOL, HANDLER_SYMBOL, HELP_SYMBOL, IMPORT_SYMBOL, INSERT_SYMBOL, INSTALL_SYMBOL, KILL_SYMBOL, LOAD_SYMBOL, LOCK_SYMBOL, OPTIMIZE_SYMBOL, PREPARE_SYMBOL, PURGE_SYMBOL, RELEASE_SYMBOL, RENAME_SYMBOL, REPAIR_SYMBOL, REPLACE_SYMBOL, RESET_SYMBOL, RESIGNAL_SYMBOL, REVOKE_SYMBOL, ROLLBACK_SYMBOL, SAVEPOINT_SYMBOL, SELECT_SYMBOL, SET_SYMBOL, SHOW_SYMBOL, SHUTDOWN_SYMBOL, SIGNAL_SYMBOL, START_SYMBOL, STOP_SYMBOL, TABLE_SYMBOL, TRUNCATE_SYMBOL, UNINSTALL_SYMBOL, UNLOCK_SYMBOL, UPDATE_SYMBOL, USE_SYMBOL, VALUES_SYMBOL, WITH_SYMBOL, XA_SYMBOL, CLONE_SYMBOL, RESTART_SYMBOL, '('} (RuntimeError)
||mysql.parseStatementAst: no viable alternative at input 'SELECT' (RuntimeError)

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
mysql.splitScript: Argument #1 is expected to be a string (TypeError)
mysql.splitScript: Argument #1 is expected to be a string (TypeError)
