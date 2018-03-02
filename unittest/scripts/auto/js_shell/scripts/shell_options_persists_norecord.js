var _defaultSandboxDir = shell.options.sandboxDir

//@ shell classic connection
shell.connect(__mysqluripwd)

//@ autocomplete.nameCache update and set back to default using shell.options
shell.options["autocomplete.nameCache"] = "false"
shell.options["autocomplete.nameCache"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("autocomplete.nameCache")
shell.options["autocomplete.nameCache"]

//@ batchContinueOnError update and set back to default using shell.options
shell.options["batchContinueOnError"] = "true"
shell.options["batchContinueOnError"]
shell.options.unset("batchContinueOnError")
shell.options["batchContinueOnError"]

//@ devapi.dbObjectHandles update and set back to default using shell.options
shell.options["devapi.dbObjectHandles"] = "false"
shell.options["devapi.dbObjectHandles"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("devapi.dbObjectHandles")
shell.options["devapi.dbObjectHandles"]

//@ history.autoSave update and set back to default using shell.options
shell.options["history.autoSave"] = "true"
shell.options["history.autoSave"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("history.autoSave")
shell.options["history.autoSave"]

//@ history.maxSize update and set back to default using shell.options
shell.options["history.maxSize"] = 10
shell.options["history.maxSize"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("history.maxSize")
shell.options["history.maxSize"]

//@ history.sql.ignorePattern update and set back to default using shell.options
shell.options["history.sql.ignorePattern"] = "*PATTERN*"
shell.options["history.sql.ignorePattern"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("history.sql.ignorePattern")
shell.options["history.sql.ignorePattern"]

//@ interactive update and set back to default using shell.options
shell.options["interactive"] = "true"
shell.options["interactive"]
shell.options.unset("interactive")
shell.options["interactive"]

//@ logLevel update and set back to default using shell.options
shell.options["logLevel"] = 8
shell.options["logLevel"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("logLevel")
shell.options["logLevel"]

//@ outputFormat update and set back to default using shell.options
shell.options["outputFormat"] = "json/raw"
shell.options["outputFormat"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("outputFormat")
shell.options["outputFormat"]

//@ passwordsFromStdin update and set back to default using shell.options
shell.options["passwordsFromStdin"] = "true"
shell.options["passwordsFromStdin"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("passwordsFromStdin")
shell.options["passwordsFromStdin"]

//@ sandboxDir update and set back to default using shell.options
shell.options["sandboxDir"] = "\\sandboxDir"
shell.options["sandboxDir"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("sandboxDir")
shell.options["sandboxDir"]

//@ showWarnings update and set back to default using shell.options
shell.options["showWarnings"] = "false"
shell.options["showWarnings"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("showWarnings")
shell.options["showWarnings"]

//@ useWizards update and set back to default using shell.options
shell.options["useWizards"] = "false"
shell.options["useWizards"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("useWizards")
shell.options["useWizards"]

//@ defaultMode update and set back to default using shell.options
shell.options["defaultMode"] = "py"
shell.options["defaultMode"]
os.load_text_file(os.get_user_config_path() + "/test_options.json")
shell.options.unset("defaultMode")
shell.options["defaultMode"]

//@ autocomplete.nameCache update and set back to default using \option
\option autocomplete.nameCache = false
\option autocomplete.nameCache
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset autocomplete.nameCache
\option autocomplete.nameCache

//@ batchContinueOnError update and set back to default using \option
\option batchContinueOnError = true
\option batchContinueOnError
\option --unset batchContinueOnError
\option batchContinueOnError

//@ devapi.dbObjectHandles update and set back to default using \option
\option devapi.dbObjectHandles = false
\option devapi.dbObjectHandles
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset devapi.dbObjectHandles
\option devapi.dbObjectHandles

//@ history.autoSave update and set back to default using \option
\option history.autoSave = true
\option history.autoSave
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset history.autoSave
\option history.autoSave

//@ history.maxSize update and set back to default using \option
\option history.maxSize = 10
\option history.maxSize
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset history.maxSize
\option history.maxSize

//@ history.sql.ignorePattern update and set back to default using \option
\option history.sql.ignorePattern = *PATTERN*
\option history.sql.ignorePattern
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset history.sql.ignorePattern
\option history.sql.ignorePattern

//@ interactive update and set back to default using \option
\option interactive = true
\option interactive
\option --unset interactive
\option interactive

//@ logLevel update and set back to default using \option
\option logLevel = 8
\option logLevel
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset logLevel
\option logLevel

//@ outputFormat update and set back to default using \option
\option outputFormat = json/raw
\option outputFormat
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset outputFormat
\option outputFormat

//@ passwordsFromStdin update and set back to default using \option
\option passwordsFromStdin = true
\option passwordsFromStdin
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset passwordsFromStdin
\option passwordsFromStdin

//@ sandboxDir update and set back to default using \option
\option sandboxDir = \sandboxDir
\option sandboxDir
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset sandboxDir
\option sandboxDir

//@ showWarnings update and set back to default using \option
\option showWarnings = false
\option showWarnings
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset showWarnings
\option showWarnings

//@ useWizards update and set back to default using \option
\option useWizards = false
\option useWizards
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset useWizards
\option useWizards

//@ defaultMode update and set back to default using \option
\option defaultMode = py
\option defaultMode
os.load_text_file(os.get_user_config_path() + "/test_options.json")
\option --unset defaultMode
\option defaultMode

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
