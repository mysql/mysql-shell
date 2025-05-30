//@<> Call Shell with No Coloring
function callMysqlsh(command_line_args) {
    testutil.callMysqlsh(command_line_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
}

// WL14297-TSFR_9_1_1
//@ CLI shell Global --help
callMysqlsh(["--sql", "--", "--help"])

//@ CLI shell Global -h [USE:CLI shell Global --help]
callMysqlsh(["--sql", "--", "-h"])

// WL14297-TSFR_1_1_1
//@ CLI shell --help
callMysqlsh(["--", "shell", "--help"])

//@ CLI shell -h [USE:CLI shell --help]
callMysqlsh(["--", "shell", "--help"])

//@ CLI shell options --help
callMysqlsh(["--", "shell", "options", "--help"])

//@ CLI shell options -h [USE:CLI shell options --help]
callMysqlsh(["--", "shell", "options", "--help"])

//@ CLI util --help
callMysqlsh(["--sql", "--", "util", "--help"])

//@ CLI util -h [USE:CLI util --help]
callMysqlsh(["--", "util", "-h"])

// WL14297-TSFR_1_1_2
//@ CLI --help Unexisting Objects
callMysqlsh(["--", "test", "--help"])
callMysqlsh(["--", "test.wex", "--help"])

//@ CLI -h Unexisting Objects [USE:CLI --help Unexisting Objects]
callMysqlsh(["--", "test", "-h"])
callMysqlsh(["--", "test.wex", "-h"])

// WL14298 - TSFR_2_1_1
// WL14298 - TSFR_2_1_2
//@ CLI shell delete-all-credentials --help
callMysqlsh(["--", "shell", "delete-all-credentials", "--help"])

//@ CLI shell delete-credential --help
callMysqlsh(["--", "shell", "delete-credential", "--help"])

//@ CLI shell list-credential-helpers --help
callMysqlsh(["--", "shell", "list-credential-helpers", "--help"])

//@ CLI shell list-credentials --help
callMysqlsh(["--", "shell", "list-credentials", "--help"])

//@ CLI shell status --help
callMysqlsh(["--", "shell", "status", "--help"])

//@ CLI shell store-credential --help
callMysqlsh(["--", "shell", "store-credential", "--help"])


//@ CLI shell delete-all-credentials -h [USE:CLI shell delete-all-credentials --help]
callMysqlsh(["--", "shell", "delete-all-credentials", "-h"])

//@ CLI shell delete-credential -h [USE:CLI shell delete-credential --help]
callMysqlsh(["--", "shell", "delete-credential", "-h"])

//@ CLI shell list-credential-helpers -h [USE:CLI shell list-credential-helpers --help]
callMysqlsh(["--", "shell", "list-credential-helpers", "--help"])

//@ CLI shell list-credentials -h [USE:CLI shell list-credentials --help]
callMysqlsh(["--", "shell", "list-credentials", "-h"])

//@ CLI shell status -h [USE:CLI shell status --help]
callMysqlsh(["--", "shell", "status", "-h"])

//@ CLI shell store-credential -h [USE:CLI shell store-credential --help]
callMysqlsh(["--", "shell", "store-credential", "-h"])


//@ CLI shell deleteAllCredentials --help [USE:CLI shell delete-all-credentials --help]
callMysqlsh(["--", "shell", "deleteAllCredentials", "--help"])

//@ CLI shell deleteCredential --help [USE:CLI shell delete-credential --help]
callMysqlsh(["--", "shell", "deleteCredential", "--help"])

//@ CLI shell listCredentialHelpers --help [USE:CLI shell list-credential-helpers --help]
callMysqlsh(["--", "shell", "listCredential-helpers", "--help"])

//@ CLI shell listCredentials --help [USE:CLI shell list-credentials --help]
callMysqlsh(["--", "shell", "listCredentials", "--help"])

//@ CLI shell storeCredential --help [USE:CLI shell store-credential --help]
callMysqlsh(["--", "shell", "storeCredential", "--help"])


//@ CLI shell deleteAllCredentials -h [USE:CLI shell delete-all-credentials --help]
callMysqlsh(["--", "shell", "deleteAllCredentials", "-h"])

//@ CLI shell deleteCredential -h [USE:CLI shell delete-credential --help]
callMysqlsh(["--", "shell", "deleteCredential", "-h"])

//@ CLI shell listCredentialHelpers -h [USE:CLI shell list-credential-helpers --help]
callMysqlsh(["--", "shell", "listCredential-helpers", "-h"])

//@ CLI shell listCredentials -h [USE:CLI shell list-credentials --help]
callMysqlsh(["--", "shell", "listCredentials", "-h"])

//@ CLI shell storeCredential -h [USE:CLI shell store-credential --help]
callMysqlsh(["--", "shell", "storeCredential", "-h"])


//@ CLI shell options set-persist --help
callMysqlsh(["--", "shell", "options", "set-persist", "--help"])

//@ CLI shell options unset-persist --help
callMysqlsh(["--", "shell", "options", "unset-persist", "--help"])


//@ CLI shell options set-persist -h [USE:CLI shell options set-persist --help]
callMysqlsh(["--", "shell", "options", "set-persist", "-h"])

//@ CLI shell options unset-persist -h [USE:CLI shell options unset-persist --help]
callMysqlsh(["--", "shell", "options", "unset-persist", "-h"])


//@ CLI shell options setPersist --help [USE:CLI shell options set-persist --help]
callMysqlsh(["--", "shell", "options", "setPersist", "--help"])

//@ CLI shell options unsetPersist --help [USE:CLI shell options unset-persist --help]
callMysqlsh(["--", "shell", "options", "unsetPersist", "--help"])


//@ CLI shell options setPersist -h [USE:CLI shell options set-persist --help]
callMysqlsh(["--", "shell", "options", "setPersist", "-h"])

//@ CLI shell options unsetPersist -h [USE:CLI shell options unset-persist --help]
callMysqlsh(["--", "shell", "options", "unsetPersist", "-h"])


//@ CLI util check-for-server-upgrade --help
callMysqlsh(["--sql", "--", "util", "check-for-server-upgrade", "--help"])

//@ CLI util copy-instance --help
callMysqlsh(["--", "util", "copy-instance", "--help"])

//@ CLI util copy-schemas --help
callMysqlsh(["--", "util", "copy-schemas", "--help"])

//@ CLI util copy-tables --help
callMysqlsh(["--", "util", "copy-tables", "--help"])

//@ CLI util dump-binlogs --help
callMysqlsh(["--", "util", "dump-binlogs", "--help"])

//@ CLI util dump-instance --help
callMysqlsh(["--", "util", "dump-instance", "--help"])

//@ CLI util dump-schemas --help
callMysqlsh(["--", "util", "dump-schemas", "--help"])

//@ CLI util dump-tables --help
callMysqlsh(["--", "util", "dump-tables", "--help"])

//@ CLI util export-table --help
callMysqlsh(["--", "util", "export-table", "--help"])

//@ CLI util import-json --help
callMysqlsh(["--", "util", "import-json", "--help"])

//@ CLI util import-table --help
callMysqlsh(["--", "util", "import-table", "--help"])

//@ CLI util load-binlogs --help
callMysqlsh(["--", "util", "load-binlogs", "--help"])

//@ CLI util load-dump --help
callMysqlsh(["--", "util", "load-dump", "--help"])



//@ CLI util check-for-server-upgrade -h [USE:CLI util check-for-server-upgrade --help]
callMysqlsh(["--", "util", "check-for-server-upgrade", "-h"])

//@ CLI util dump-instance -h [USE:CLI util dump-instance --help]
callMysqlsh(["--", "util", "dump-instance", "-h"])

//@ CLI util dump-schemas -h [USE:CLI util dump-schemas --help]
callMysqlsh(["--", "util", "dump-schemas", "-h"])

//@ CLI util dump-tables -h [USE:CLI util dump-tables --help]
callMysqlsh(["--", "util", "dump-tables", "-h"])

//@ CLI util export-table -h [USE:CLI util export-table --help]
callMysqlsh(["--", "util", "export-table", "-h"])

//@ CLI util import-json -h [USE:CLI util import-json --help]
callMysqlsh(["--", "util", "import-json", "-h"])

//@ CLI util import-table -h [USE:CLI util import-table --help]
callMysqlsh(["--", "util", "import-table", "-h"])

//@ CLI util load-dump -h [USE:CLI util load-dump --help]
callMysqlsh(["--", "util", "load-dump", "-h"])



//@ CLI util checkForServerUpgrade --help [USE:CLI util check-for-server-upgrade --help]
callMysqlsh(["--", "util", "checkForServerUpgrade", "--help"])

//@ CLI util dumpInstance --help [USE:CLI util dump-instance --help]
callMysqlsh(["--", "util", "dumpInstance", "--help"])

//@ CLI util dumpSchemas --help [USE:CLI util dump-schemas --help]
callMysqlsh(["--", "util", "dumpSchemas", "--help"])

//@ CLI util dumpTables --help [USE:CLI util dump-tables --help]
callMysqlsh(["--", "util", "dumpTables", "--help"])

//@ CLI util exportTable --help [USE:CLI util export-table --help]
callMysqlsh(["--", "util", "exportTable", "--help"])

//@ CLI util importJson --help [USE:CLI util import-json --help]
callMysqlsh(["--", "util", "importJson", "--help"])

//@ CLI util importTable --help [USE:CLI util import-table --help]
callMysqlsh(["--", "util", "importTable", "--help"])

//@ CLI util loadDump --help [USE:CLI util load-dump --help]
callMysqlsh(["--", "util", "loadDump", "--help"])



//@ CLI util checkForServerUpgrade -h [USE:CLI util check-for-server-upgrade --help]
callMysqlsh(["--", "util", "checkForServerUpgrade", "-h"])

//@ CLI util dumpInstance -h [USE:CLI util dump-instance --help]
callMysqlsh(["--", "util", "dumpInstance", "-h"])

//@ CLI util dumpSchemas -h [USE:CLI util dump-schemas --help]
callMysqlsh(["--", "util", "dumpSchemas", "-h"])

//@ CLI util dumpTables -h [USE:CLI util dump-tables --help]
callMysqlsh(["--", "util", "dumpTables", "-h"])

//@ CLI util exportTable -h [USE:CLI util export-table --help]
callMysqlsh(["--", "util", "exportTable", "-h"])

//@ CLI util importJson -h [USE:CLI util import-json --help]
callMysqlsh(["--", "util", "importJson", "-h"])

//@ CLI util importTable -h [USE:CLI util import-table --help]
callMysqlsh(["--", "util", "importTable", "-h"])

//@ CLI util loadDump -h [USE:CLI util load-dump --help]
callMysqlsh(["--", "util", "loadDump", "-h"])


//@ CLI shell delete-all-secrets --help
callMysqlsh(["--", "shell", "delete-all-secrets", "--help"])

//@ CLI shell delete-secret --help
callMysqlsh(["--", "shell", "delete-secret", "--help"])

//@ CLI shell list-secrets --help
callMysqlsh(["--", "shell", "list-secrets", "--help"])

//@ CLI shell read-secret --help
callMysqlsh(["--", "shell", "read-secret", "--help"])

//@ CLI shell store-secret --help
callMysqlsh(["--", "shell", "store-secret", "--help"])


//@ CLI shell delete-all-secrets -h [USE: CLI shell delete-all-secrets --help]
callMysqlsh(["--", "shell", "delete-all-secrets", "-h"])

//@ CLI shell delete-secret -h [USE: CLI shell delete-secret --help]
callMysqlsh(["--", "shell", "delete-secret", "-h"])

//@ CLI shell list-secrets -h [USE: CLI shell list-secrets --help]
callMysqlsh(["--", "shell", "list-secrets", "-h"])

//@ CLI shell read-secret -h [USE: CLI shell read-secret --help]
callMysqlsh(["--", "shell", "read-secret", "--help"])

//@ CLI shell store-secret -h [USE: CLI shell store-secret --help]
callMysqlsh(["--", "shell", "store-secret", "-h"])


//@ CLI shell deleteAllSecrets --help [USE: CLI shell delete-all-secrets --help]
callMysqlsh(["--", "shell", "deleteAllSecrets", "--help"])

//@ CLI shell deleteSecret --help [USE: CLI shell delete-secret --help]
callMysqlsh(["--", "shell", "deleteSecret", "--help"])

//@ CLI shell listSecrets --help [USE: CLI shell list-secrets --help]
callMysqlsh(["--", "shell", "listSecrets", "--help"])

//@ CLI shell readSecret --help [USE: CLI shell read-secret --help]
callMysqlsh(["--", "shell", "readSecret", "--help"])

//@ CLI shell storeSecret --help [USE: CLI shell store-secret --help]
callMysqlsh(["--", "shell", "storeSecret", "--help"])


//@ CLI shell deleteAllSecrets -h [USE: CLI shell delete-all-secrets --help]
callMysqlsh(["--", "shell", "deleteAllSecrets", "-h"])

//@ CLI shell deleteSecret -h [USE: CLI shell delete-secret --help]
callMysqlsh(["--", "shell", "deleteSecret", "-h"])

//@ CLI shell listSecrets -h [USE: CLI shell list-secrets --help]
callMysqlsh(["--", "shell", "listSecrets", "-h"])

//@ CLI shell readSecret -h [USE: CLI shell read-secret --help]
callMysqlsh(["--", "shell", "readSecret", "-h"])

//@ CLI shell storeSecret -h [USE: CLI shell store-secret --help]
callMysqlsh(["--", "shell", "storeSecret", "-h"])
