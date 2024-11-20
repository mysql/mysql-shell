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

//@ CLI clusterset dissolve --help
callMysqlsh(["--", "clusterset", "dissolve", "--help"])

//@ CLI clusterset rejoin-cluster --help
callMysqlsh(["--", "clusterset", "rejoin-cluster", "--help"])

//@ CLI clusterset set-primary-cluster --help
callMysqlsh(["--", "clusterset", "set-primary-cluster", "--help"])

//@ CLI clusterset force-primary-cluster --help
callMysqlsh(["--", "clusterset", "force-primary-cluster", "--help"])

//@ CLI clusterset status --help
callMysqlsh(["--", "clusterset", "status", "--help"])

//@ CLI clusterset router-options --help
callMysqlsh(["--", "clusterset", "router-options", "--help"])

//@ CLI clusterset execute --help
callMysqlsh(["--", "clusterset", "execute", "--help"])

//@ CLI clusterset create-routing-guideline --help
callMysqlsh(["--", "clusterset", "create-routing-guideline", "--help"])

//@ CLI clusterset get-routing-guideline --help
callMysqlsh(["--", "clusterset", "get_routing_guideline", "--help"])

//@ CLI clusterset remove-routing-guideline --help
callMysqlsh(["--", "clusterset", "remove_routing_guideline", "--help"])

//@ CLI clusterset routing-guidelines --help
callMysqlsh(["--", "clusterset", "routing_guidelines", "--help"])

//@ CLI clusterset import-routing-guideline --help
callMysqlsh(["--", "clusterset", "import-routing-guideline", "--help"])
