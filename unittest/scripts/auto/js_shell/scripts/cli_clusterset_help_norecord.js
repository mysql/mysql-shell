//@<> Call Shell with No Coloring
function callMysqlsh(command_line_args) {
    testutil.callMysqlsh(command_line_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
}

//@ CLI clusterset --help
callMysqlsh(["--", "clusterset", "--help"])

//@ CLI clusterset create-replica-cluster --help
callMysqlsh(["--", "clusterset", "create-replica-cluster", "--help"])

//@ CLI clusterset remove-cluster --help
callMysqlsh(["--", "clusterset", "remove-cluster", "--help"])

//@ CLI clusterset describe --help
callMysqlsh(["--", "clusterset", "describe", "--help"])

//@ CLI clusterset rejoin-cluster --help
callMysqlsh(["--", "clusterset", "rejoin-cluster", "--help"])

//@ CLI clusterset set-primary-cluster --help
callMysqlsh(["--", "clusterset", "set-primary-cluster", "--help"])

//@ CLI clusterset force-primary-cluster --help
callMysqlsh(["--", "clusterset", "force-primary-cluster", "--help"])

//@ CLI clusterset status --help
callMysqlsh(["--", "clusterset", "status", "--help"])
