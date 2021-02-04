//@<> Call Shell with No Coloring
function callMysqlsh(command_line_args) {
    testutil.callMysqlsh(command_line_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
}

//@ CLI replicaset --help
callMysqlsh(["--", "rs", "--help"])

//@ CLI replicaset add-instance --help
callMysqlsh(["--", "rs", "add-instance", "--help"])

//@ CLI replicaset force-primary-instance --help
callMysqlsh(["--", "rs", "force-primary-instance", "--help"])

//@ CLI replicaset list-routers --help
callMysqlsh(["--", "rs", "list-routers", "--help"])

//@ CLI replicaset options --help
callMysqlsh(["--", "rs", "options", "--help"])

//@ CLI replicaset rejoin-instance --help
callMysqlsh(["--", "rs", "rejoin-instance", "--help"])

//@ CLI replicaset remove-instance --help
callMysqlsh(["--", "rs", "remove-instance", "--help"])

//@ CLI replicaset remove-router-metadata --help
callMysqlsh(["--", "rs", "remove-router-metadata", "--help"])

//@ CLI replicaset set-instance-option --help
callMysqlsh(["--", "rs", "set-instance-option", "--help"])

//@ CLI replicaset set-option --help
callMysqlsh(["--", "rs", "set-option", "--help"])

//@ CLI replicaset set-primary-instance --help
callMysqlsh(["--", "rs", "set-primary-instance", "--help"])

//@ CLI replicaset setup-admin-account --help
callMysqlsh(["--", "rs", "setup-admin-account", "--help"])

//@ CLI replicaset setup-router-account --help
callMysqlsh(["--", "rs", "setup-router-account", "--help"])

//@ CLI replicaset status --help
callMysqlsh(["--", "rs", "status", "--help"])
