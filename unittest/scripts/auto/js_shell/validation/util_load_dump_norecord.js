//@<ERR> Bad Options (should fail)
Util.loadDump: Argument #1 is expected to be a string (TypeError)
Util.loadDump: Argument #1 is expected to be a string (TypeError)
Util.loadDump: Argument #1 is expected to be a string (TypeError)
Util.loadDump: Cannot open file '[[*]]<<<filename_for_output("ldtest/dump/@.sql/@.json")>>>': No[[*]] directory (RuntimeError)
Util.loadDump: Cannot open file '[[*]]<<<filename_for_output("ldtest/@.json")>>>': No such file or directory (RuntimeError)
Util.loadDump: Cannot open file '[[*]]<<<filename_for_output("ldtest/@.json")>>>': No such file or directory (RuntimeError)
Util.loadDump: Argument #2 is expected to be a map (TypeError)
Util.loadDump: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
Util.loadDump: Argument #2: The option 'osNamespace' cannot be used when the value of 'osBucketName' option is not set. (ArgumentError)
Util.loadDump: Argument #2: The option 'ociConfigFile' cannot be used when the value of 'osBucketName' option is not set. (ArgumentError)
Util.loadDump: Cannot open file: /badpath. (RuntimeError)
Util.loadDump: Argument #2: Invalid options: bogus (ArgumentError)
Util.loadDump: Argument #2: Option 'includeSchemas' is expected to be of type Array, but is String (TypeError)
Util.loadDump: Argument #2: At least one of loadData, loadDdl or loadUsers options must be enabled (ArgumentError)
Util.loadDump: Argument #2: At least one of loadData, loadDdl or loadUsers options must be enabled (ArgumentError)
Util.loadDump: Argument #2: Option 'waitDumpTimeout' Float expected, but value is String (TypeError)
Util.loadDump: Argument #2: Option 'analyzeTables' is expected to be of type String, but is Bool (TypeError)
Util.loadDump: Argument #2: Invalid value '' for analyzeTables option, allowed values: 'histogram', 'off' and 'on'. (ArgumentError)
Util.loadDump: Argument #2: Invalid value 'xxx' for analyzeTables option, allowed values: 'histogram', 'off' and 'on'. (ArgumentError)
Util.loadDump: Argument #2: Invalid value 'xxx' for deferTableIndexes option, allowed values: 'all', 'fulltext' and 'off'. (ArgumentError)
Util.loadDump: Argument #2: Invalid value '' for deferTableIndexes option, allowed values: 'all', 'fulltext' and 'off'. (ArgumentError)
Util.loadDump: Argument #2: Option 'deferTableIndexes' is expected to be of type String, but is Bool (TypeError)
Util.loadDump: Argument #2: Invalid value 'xxx' for updateGtidSet option, allowed values: 'append', 'off' and 'replace'. (ArgumentError)
Util.loadDump: Argument #2: Invalid value '' for updateGtidSet option, allowed values: 'append', 'off' and 'replace'. (ArgumentError)
Util.loadDump: Argument #2: Option 'updateGtidSet' is expected to be of type String, but is Bool (TypeError)

//@# progressFile errors should be reported before opening the dump
|Loading DDL and Data from '<<<__tmp_dir>>>/ldtest/dump' using 4 threads.|
|Opening dump...|
|Target is MySQL |
||
||Util.loadDump: Cannot open file '<<<__os_type=="windows"?'\\\\?\\invalid\\unwritable':'/invalid/unwritable'>>>': No such file or directory
