// Display the help for MySQL Shell (mysqlsh --help) and verify that the option --pager is present

//@ help should contain information about --pager option
testutil.callMysqlsh(["--help"]);
