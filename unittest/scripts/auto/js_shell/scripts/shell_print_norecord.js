//@<> Setup
type_expectation = {
    "info": "sample",
    "error": "ERROR: sample",
    "warning": "WARNING: sample",
    "note": "NOTE: sample",
    "status": "sample",
}

//@<> Test JavaScript formatting
for (element in type_expectation) {
    expected = type_expectation[element];

    rc = testutil.callMysqlsh(["--js", "-e", `shell.print('sample', '${element}')`],"", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
    EXPECT_OUTPUT_CONTAINS(expected)
    WIPE_OUTPUT()
}

//@<> Test Python Mode formatting
for (element in type_expectation) {
    expected = type_expectation[element];

    rc = testutil.callMysqlsh(["--py", "-e", `shell.print('sample', '${element}')`],"", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
    EXPECT_OUTPUT_CONTAINS(expected)
    WIPE_OUTPUT()
}

type_expectation = {
    "info": '{"info":"sample\\n"}',
    "error": '{"error":"sample\\n"}',
    "warning": '{"warning":"sample\\n"}',
    "note": '{"note":"sample\\n"}',
    "status": '{"status":"sample\\n"}',
}

//@<> Test JavaScript formatting (JSON)
for (element in type_expectation) {
    expected = type_expectation[element];

    rc = testutil.callMysqlsh(["--js", "--json=raw", "-e", `shell.print('sample', '${element}')`],"", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
    EXPECT_OUTPUT_CONTAINS(expected)
    WIPE_OUTPUT()
}

//@<> Test Python Mode formatting (JOSN)
for (element in type_expectation) {
    expected = type_expectation[element];

    rc = testutil.callMysqlsh(["--py", "--json=raw", "-e", `shell.print('sample', '${element}')`],"", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
    EXPECT_OUTPUT_CONTAINS(expected)
    WIPE_OUTPUT()
}



// NOTE: these tests don't work because the test system also uses the <<<>>> pattern to replace "variables" with values
// associated to them in the execution context, however, the functionality was manually verified as indicated here.
//@<> Test API name case JS
// testutil.callMysqlsh(["--js", "-e", "shell.print('shell.<<<someApiName>>>')"],"", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
// EXPECT_OUTPUT_CONTAINS("shell.someApiName")
// WIPE_OUTPUT()


//@<> Test API name case PY
// testutil.callMysqlsh(["--py", "-e", "shell.print('shell.<<<someApiName>>>')"],"", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
// EXPECT_OUTPUT_CONTAINS("shell.some_api_name")
// WIPE_OUTPUT()


