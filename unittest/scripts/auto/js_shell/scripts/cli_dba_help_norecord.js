//@<> Call Shell with No Coloring
function callMysqlsh(command_line_args) {
    testutil.callMysqlsh(command_line_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
}

//@ CLI dba --help
callMysqlsh(["--", "dba", "--help"])

//@ CLI dba check-instance-configuration --help
callMysqlsh(["--", "dba", "check-instance-configuration", "--help"])

//@ CLI dba configure-instance --help
callMysqlsh(["--", "dba", "configure-instance", "--help"])

//@ CLI dba configure-local-instance --help
callMysqlsh(["--", "dba", "configure-local-instance", "--help"])

//@ CLI dba configure-replica-set-instance --help
callMysqlsh(["--", "dba", "configure-replica-set-instance", "--help"])

//@ CLI dba create-cluster --help
callMysqlsh(["--", "dba", "create-cluster", "--help"])

//@ CLI dba create-replica-set --help
callMysqlsh(["--", "dba", "create-replica-set", "--help"])

//@ CLI dba delete-sandbox-instance --help
callMysqlsh(["--", "dba", "delete-sandbox-instance", "--help"])

//@ CLI dba drop-metadata-schema --help
callMysqlsh(["--", "dba", "drop-metadata-schema", "--help"])

//@ CLI dba deploy-sandbox-instance --help
callMysqlsh(["--", "dba", "deploy-sandbox-instance", "--help"])

//@ CLI dba kill-sandbox-instance --help
callMysqlsh(["--", "dba", "kill-sandbox-instance", "--help"])

//@ CLI dba reboot-cluster-from-complete-outage --help
callMysqlsh(["--", "dba", "reboot-cluster-from-complete-outage", "--help"])

//@ CLI dba start-sandbox-instance --help
callMysqlsh(["--", "dba", "start-sandbox-instance", "--help"])

//@ CLI dba stop-sandbox-instance --help
callMysqlsh(["--", "dba", "stop-sandbox-instance", "--help"])

//@ CLI dba upgrade-metadata --help
callMysqlsh(["--", "dba", "upgrade-metadata", "--help"])



