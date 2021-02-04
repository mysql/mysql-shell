//@<> Call Shell with No Coloring
function callMysqlsh(command_line_args) {
    testutil.callMysqlsh(command_line_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
}

//@ CLI cluster --help
callMysqlsh(["--", "cluster", "--help"])

//@ CLI cluster add-instance --help
callMysqlsh(["--", "cluster", "add-instance", "--help"])

//@ CLI cluster check-instance-state --help
callMysqlsh(["--", "cluster", "check-instance-state", "--help"])

//@ CLI cluster describe --help
callMysqlsh(["--", "cluster", "describe", "--help"])

//@ CLI cluster dissolve --help
callMysqlsh(["--", "cluster", "dissolve", "--help"])

//@ CLI cluster force-quorum-using-partition-of --help
callMysqlsh(["--", "cluster", "force-quorum-using-partition-of", "--help"])

//@ CLI cluster list-routers --help
callMysqlsh(["--", "cluster", "list-routers", "--help"])

//@ CLI cluster options --help
callMysqlsh(["--", "cluster", "options", "--help"])

//@ CLI cluster rejoin-instance --help
callMysqlsh(["--", "cluster", "rejoin-instance", "--help"])

//@ CLI cluster remove-instance --help
callMysqlsh(["--", "cluster", "remove-instance", "--help"])

//@ CLI cluster remove-router-metadata --help
callMysqlsh(["--", "cluster", "remove-router-metadata", "--help"])

//@ CLI cluster rescan --help
callMysqlsh(["--", "cluster", "rescan", "--help"])

//@ CLI cluster reset-recovery-accounts-password --help
callMysqlsh(["--", "cluster", "reset-recovery-accounts-password", "--help"])

//@ CLI cluster set-instance-option --help
callMysqlsh(["--", "cluster", "set-instance-option", "--help"])

//@ CLI cluster set-option --help
callMysqlsh(["--", "cluster", "set-option", "--help"])

//@ CLI cluster set-primary-instance --help
callMysqlsh(["--", "cluster", "set-primary-instance", "--help"])

//@ CLI cluster setup-admin-account --help
callMysqlsh(["--", "cluster", "setup-admin-account", "--help"])

//@ CLI cluster setup-router-account --help
callMysqlsh(["--", "cluster", "setup-router-account", "--help"])

//@ CLI cluster status --help
callMysqlsh(["--", "cluster", "status", "--help"])

//@ CLI cluster switch-to-single-primary-mode --help
callMysqlsh(["--", "cluster", "switch-to-single-primary-mode", "--help"])
