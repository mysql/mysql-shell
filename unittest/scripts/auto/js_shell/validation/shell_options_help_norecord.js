//@<OUT> autocomplete.nameCache option help text
 autocomplete.nameCache  Enable database name caching for autocompletion.

//@<OUT> batchContinueOnError option help text
 batchContinueOnError  To use in SQL batch mode, forces processing to continue
                       if an error is found.

//@<OUT> defaultCompress option help text
 defaultCompress  Enable compression in client/server protocol by default in
                  global shell sessions.

//@<OUT> defaultMode option help text
 defaultMode  Specifies the shell mode to use when shell is started - one of
              sql, js or py.

//@<OUT> devapi.dbObjectHandles option help text
 devapi.dbObjectHandles  Enable table and collection name handles for the
                         DevAPI db object.

//@<OUT> history.autoSave option help text
 history.autoSave  Shell's history autosave.

//@<OUT> history.maxSize option help text
 history.maxSize  Shell's history maximum size

//@<OUT> history.sql.ignorePattern option help text
 history.sql.ignorePattern  Shell's history ignore list.

//@<OUT> interactive option help text
 interactive  Enables interactive mode

//@<OUT> logLevel option help text
 logLevel  The log level value must be an integer between 1 and 8 or any of
           [none, internal, error, warning, info, debug, debug2, debug3]
           respectively.

//@<OUT> outputFormat option help text
 outputFormat  outputFormat option has been deprecated, please use resultFormat
               to set result format and --json command line option to wrap
               output in JSON instead.

//@<OUT> resultFormat option help text
 resultFormat  Determines format of results. Valid values: [table, tabbed,
               vertical, json, ndjson, json/raw, json/array, json/pretty].

//@<OUT> passwordsFromStdin option help text
 passwordsFromStdin  Read passwords from stdin instead of the tty.

//@<OUT> sandboxDir option help text
 sandboxDir  Default sandbox directory

//@<OUT> showWarnings option help text
 showWarnings  Automatically display SQL warnings on SQL mode if available.

//@<OUT> useWizards option help text
 useWizards  Enables wizard mode.

//@<OUT> dba.gtidWaitTimeout option help text
 dba.gtidWaitTimeout  Timeout value in seconds to wait for GTIDs to be
                      synchronized.

//@<OUT> dba.logSql option help text
 dba.logSql  Log SQL statements executed by AdminAPI operations: 0 - logging
             disabled; 1 - log statements other than SELECT and SHOW; 2 - log
             all statements.

//@<OUT> Verify the help text when using filter
 history.autoSave           Shell's history autosave.
 history.maxSize            Shell's history maximum size
 history.sql.ignorePattern  Shell's history ignore list.

//@<OUT> verbose output
 verbose  Enable diagnostic message output to the console: 0 - display no
          messages; 1 - display error, warning and informational messages; 2,
          3, 4 - include higher levels of debug messages. If level is not
          given, 1 is assumed.
