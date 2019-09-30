var _defaultSandboxDir = shell.options.sandboxDir
var _defaultOciConfigFile = shell.options['oci.configFile']
const options_file = os.path.join(testutil.getUserConfigPath(), "test_options.json");

//@ shell classic connection
shell.connect(__mysqluripwd)

//@ autocomplete.nameCache update and set back to default using shell.options
shell.options.setPersist("autocomplete.nameCache", false);
shell.options["autocomplete.nameCache"]
os.loadTextFile(options_file);
shell.options.unsetPersist("autocomplete.nameCache")
shell.options["autocomplete.nameCache"]

//@ batchContinueOnError update and set back to default using shell.options
shell.options.set("batchContinueOnError", true);
shell.options["batchContinueOnError"]
shell.options.unsetPersist("batchContinueOnError")
shell.options["batchContinueOnError"]

//@ devapi.dbObjectHandles update and set back to default using shell.options
shell.options.setPersist("devapi.dbObjectHandles", false);
shell.options["devapi.dbObjectHandles"]
os.loadTextFile(options_file);
shell.options.unsetPersist("devapi.dbObjectHandles")
shell.options["devapi.dbObjectHandles"]

//@ history.autoSave update and set back to default using shell.options
shell.options.setPersist("history.autoSave", true);
shell.options["history.autoSave"]
os.loadTextFile(options_file);
shell.options.unsetPersist("history.autoSave");
shell.options["history.autoSave"]

//@ history.maxSize update and set back to default using shell.options
shell.options.setPersist("history.maxSize", 10);
shell.options["history.maxSize"]
os.loadTextFile(options_file);
shell.options.unsetPersist("history.maxSize");
shell.options["history.maxSize"]

//@ history.sql.ignorePattern update and set back to default using shell.options
shell.options.setPersist("history.sql.ignorePattern", "*PATTERN*");
shell.options["history.sql.ignorePattern"]
os.loadTextFile(options_file);
shell.options.unsetPersist("history.sql.ignorePattern");
shell.options["history.sql.ignorePattern"]

//@ interactive update and set back to default using shell.options
shell.options["interactive"] = "true"
shell.options["interactive"]
shell.options.unsetPersist("interactive");
shell.options["interactive"]

//@ logLevel update and set back to default using shell.options
shell.options.setPersist("logLevel", 8);
shell.options["logLevel"]
os.loadTextFile(options_file);
shell.options.unsetPersist("logLevel");
shell.options["logLevel"]

//@ outputFormat update and set back to default using shell.options
shell.options.setPersist("outputFormat", "tabbed");
shell.options["outputFormat"]
os.loadTextFile(options_file);
shell.options.unsetPersist("outputFormat")
shell.options["outputFormat"]

//@ resultFormat update and set back to default using shell.options
shell.options.setPersist("resultFormat", "json/raw");
shell.options["resultFormat"]
os.loadTextFile(options_file);
shell.options.unsetPersist("resultFormat")
shell.options["resultFormat"]

//@ passwordsFromStdin update and set back to default using shell.options
shell.options.setPersist("passwordsFromStdin", true);
shell.options["passwordsFromStdin"]
os.loadTextFile(options_file);
shell.options.unsetPersist("passwordsFromStdin")
shell.options["passwordsFromStdin"]

//@ sandboxDir update and set back to default using shell.options
shell.options.setPersist("sandboxDir", "\\sandboxDir");
shell.options["sandboxDir"]
os.loadTextFile(options_file);
shell.options.unsetPersist("sandboxDir");
shell.options["sandboxDir"]

//@ showWarnings update and set back to default using shell.options
shell.options.setPersist("showWarnings", false);
shell.options["showWarnings"]
os.loadTextFile(options_file);
shell.options.unsetPersist("showWarnings");
shell.options["showWarnings"]

//@ useWizards update and set back to default using shell.options
shell.options.setPersist("useWizards", false);
shell.options["useWizards"]
os.loadTextFile(options_file);
shell.options.unsetPersist("useWizards");
shell.options["useWizards"]

//@ defaultMode update and set back to default using shell.options
shell.options.setPersist("defaultMode", "py");
shell.options["defaultMode"]
os.loadTextFile(options_file);
shell.options.unsetPersist("defaultMode");
shell.options["defaultMode"]

//@ dba.gtidWaitTimeout update and set back to default using shell.options
// WL#11862 - FR6_4
shell.options.setPersist("dba.gtidWaitTimeout", "180");
shell.options["dba.gtidWaitTimeout"]
os.loadTextFile(options_file);
shell.options.unsetPersist("dba.gtidWaitTimeout");
shell.options["dba.gtidWaitTimeout"]

//@ dba.logSql update and set back to default using shell.options
// WL#13294
shell.options.setPersist("dba.logSql", "1");
shell.options["dba.logSql"]
os.loadTextFile(options_file);
shell.options.unsetPersist("dba.logSql");
shell.options["dba.logSql"]

//@ autocomplete.nameCache update and set back to default using \option
\option --persist autocomplete.nameCache = false
\option autocomplete.nameCache
os.loadTextFile(options_file);
\option --unset --persist autocomplete.nameCache
\option autocomplete.nameCache

//@ batchContinueOnError update and set back to default using \option
\option batchContinueOnError = true
\option batchContinueOnError
\option --unset --persist batchContinueOnError
\option batchContinueOnError

//@ devapi.dbObjectHandles update and set back to default using \option
\option --persist devapi.dbObjectHandles = false
\option devapi.dbObjectHandles
os.loadTextFile(options_file);
\option --unset --persist devapi.dbObjectHandles
\option devapi.dbObjectHandles

//@ history.autoSave update and set back to default using \option
\option --persist history.autoSave = true
\option history.autoSave
os.loadTextFile(options_file);
\option --unset --persist history.autoSave
\option history.autoSave

//@ history.maxSize update and set back to default using \option
\option --persist history.maxSize = 10
\option history.maxSize
os.loadTextFile(options_file);
\option --unset --persist history.maxSize
\option history.maxSize

//@ history.sql.ignorePattern update and set back to default using \option
\option --persist history.sql.ignorePattern = *PATTERN*
\option history.sql.ignorePattern
os.loadTextFile(options_file);
\option --unset --persist history.sql.ignorePattern
\option history.sql.ignorePattern

//@ interactive update and set back to default using \option
\option interactive = true
\option interactive
\option --unset --persist interactive
\option interactive

//@ logLevel update and set back to default using \option
\option --persist logLevel = 8
\option logLevel
os.loadTextFile(options_file);
\option --unset --persist logLevel
\option logLevel

//@ outputFormat update and set back to default using \option
\option --persist outputFormat = tabbed
\option outputFormat
os.loadTextFile(options_file);
\option --unset --persist outputFormat
\option outputFormat

//@ resultFormat update and set back to default using \option
\option --persist resultFormat = json/raw
\option resultFormat
os.loadTextFile(options_file);
\option --unset --persist resultFormat
\option resultFormat

//@ passwordsFromStdin update and set back to default using \option
\option --persist passwordsFromStdin = true
\option passwordsFromStdin
os.loadTextFile(options_file);
\option --unset --persist passwordsFromStdin
\option passwordsFromStdin

//@ sandboxDir update and set back to default using \option
\option --persist sandboxDir = \sandboxDir
\option sandboxDir
os.loadTextFile(options_file);
\option --unset --persist sandboxDir
\option sandboxDir

//@ showWarnings update and set back to default using \option
\option --persist showWarnings = false
\option showWarnings
os.loadTextFile(options_file);
\option --unset --persist showWarnings
\option showWarnings

//@ useWizards update and set back to default using \option
\option --persist useWizards = false
\option useWizards
os.loadTextFile(options_file);
\option --unset --persist useWizards
\option useWizards

//@ defaultMode update and set back to default using \option
\option --persist defaultMode = py
\option defaultMode
os.loadTextFile(options_file);
\option --unset --persist defaultMode
\option defaultMode

//@ dba.gtidWaitTimeout update and set back to default using \option
// WL#11862 - FR6_3
\option --persist dba.gtidWaitTimeout = 120
\option dba.gtidWaitTimeout
os.loadTextFile(options_file);
\option --unset --persist dba.gtidWaitTimeout
\option dba.gtidWaitTimeout

//@ dba.logSql update and set back to default using \option
// WL#13294
\option --persist dba.logSql = 2
\option dba.logSql
os.loadTextFile(options_file);
\option --unset --persist dba.logSql
\option dba.logSql

//@ credentialStore.helper update and set back to default using \option
\option --persist credentialStore.helper = plaintext
\option credentialStore.helper
os.loadTextFile(options_file);
\option --unset --persist credentialStore.helper
\option credentialStore.helper

//@ credentialStore.savePasswords update and set back to default using \option
\option --persist credentialStore.savePasswords = always
\option credentialStore.savePasswords
os.loadTextFile(options_file);
\option --unset --persist credentialStore.savePasswords
\option credentialStore.savePasswords

//@ credentialStore.excludeFilters update and set back to default using \option
\option --persist credentialStore.excludeFilters = "[\"user@*\"]"
\option credentialStore.excludeFilters
os.loadTextFile(options_file);
\option --unset --persist credentialStore.excludeFilters
\option credentialStore.excludeFilters

//@ pager update and set back to default using \option
\option --persist pager = "more -10"
\option pager
os.loadTextFile(options_file);
\option --unset --persist pager
\option pager

//@ List all the options using \option
\option -l

//@ List all the options using \option and show-origin
\option --list --show-origin

//@ List an option which origin is Compiled default
\option --list --show-origin

//@ List an option which origin is User defined
\option logLevel=8
\option -l --show-origin

//@ Verify error messages
shell.options.defaultMode = 1
\option defaultMode = 1
shell.options.unset(InvalidOption)
\option --unset InvalidOption
\option -h InvalidOption
\option --help InvalidOption

//@ Verify option dba.gtidWaitTimeout
// WL#11862 - FR6_2
\option dba.gtidWaitTimeout = 0.2
\option dba.gtidWaitTimeout = -1
\option dba.gtidWaitTimeout = "Hello world"
\option dba.gtidWaitTimeout = 0
\option dba.gtidWaitTimeout = 1
\option --unset dba.gtidWaitTimeout

//@ Verify option dba.logSql
// WL#13294
\option dba.logSql = 0.2
\option dba.logSql = -1
\option dba.logSql = 3
\option dba.logSql = "not valid"
\option dba.logSql = 0
\option dba.logSql = 2
\option --unset dba.logSql

//@ Verify option verbose
\option verbose = 0.2
\option verbose = -1
\option verbose = 5
\option verbose = "not valid"
\option verbose = 0
\option verbose = 5
\option --unset verbose

//@ Configuration operation available in SQL mode
\sql
\option logLevel
\option logLevel=0
\option logLevel
\option --unset logLevel
\option logLevel

//@ List all the options using \option for SQL mode
\option --list
\js

//@ List all the options using \option and show-origin for SQL mode
\sql
\option --list --show-origin
\js
