//@ {VER(>=8.0.33)}
let result_formats=["table", "tabbed", "vertical", "json", "ndjson", "json/raw", "json/array", "json/pretty"];
let DUMP_STATEMENT_ID = 'Statement ID:';
let JSON_STATEMENT_ID = '"statementId":';

function verify_statement_id_in_api_dump(present) {
    for (format of result_formats) {
        let result = session.runSql("select 1");
        let count = shell.dumpRows(result, format);

        if (present) {
            EXPECT_STDOUT_CONTAINS(DUMP_STATEMENT_ID);
        } else {
            EXPECT_STDOUT_NOT_CONTAINS(DUMP_STATEMENT_ID);
        }
        WIPE_OUTPUT();
    }
}


function verify_statement_id_in_dump(target, present) {
    for (format of result_formats) {
        testutil.callMysqlsh([target, "-ifull", "--result-format", format, "--sql", "-e", "select 1"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);

        if (present) {
            EXPECT_STDOUT_CONTAINS(DUMP_STATEMENT_ID);
        } else {
            EXPECT_STDOUT_NOT_CONTAINS(DUMP_STATEMENT_ID);
        }
        WIPE_OUTPUT();
    }
}


//@<> Setup
testutil.deployRawSandbox(__mysql_sandbox_port1, "root", {session_track_system_variables:"sql_mode"});
testutil.deployRawSandbox(__mysql_sandbox_port2, "root", {session_track_system_variables:"statement_id"});

//@<> Disabled by default, no Statement ID Printed
shell.connect(__sandbox_uri1);
verify_statement_id_in_api_dump(false);
session.close();
verify_statement_id_in_dump(__sandbox_uri1, false);

//@<> Disabled by default, JSON output does not contain statementId in the JSON object
testutil.callMysqlsh([__sandbox_uri1, "-ifull", "--json", "--sql", "-e", "select 1"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
EXPECT_STDOUT_NOT_CONTAINS(JSON_STATEMENT_ID);

//@<> Disabled by default, enable Statement ID Session Tracker using "*"
shell.connect(__sandbox_uri1);
session.runSql("set session_track_system_variables='*'");
verify_statement_id_in_api_dump(true);
session.close();


//@<> Disabled by default, enable Statement ID Session Tracker using "statement_id"
shell.connect(__sandbox_uri1);
session.runSql("set session_track_system_variables='statement_id'");
verify_statement_id_in_api_dump(true);
session.close();


//@<> Enabled by default, Statement ID Printed
shell.connect(__sandbox_uri2);
verify_statement_id_in_api_dump(true);
session.close();
verify_statement_id_in_dump(__sandbox_uri2, true);


//@<> Enabled by default, disable Statement ID Session Tracker using ""
shell.connect(__sandbox_uri2);
session.runSql("set session_track_system_variables=''");
verify_statement_id_in_api_dump(false);
session.close();


//@<> Enabled by default, JSON output contains statementId in the JSON object
testutil.callMysqlsh([__sandbox_uri2, "-ifull", "--json", "--sql", "-e", "select 1"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor"]);
EXPECT_STDOUT_CONTAINS(JSON_STATEMENT_ID);

//@<> Disabled by default, result.getStatementId returns empty string
shell.connect(__sandbox_uri1);
let res1 = session.runSql("select 1");
let rows1 = res1.fetchAll();
EXPECT_EQ(res1.statementId, "");
EXPECT_EQ(res1.getStatementId(), "");
session.close();


//@<> Enabled by default, result.getStatementId returns non empty string
shell.connect(__sandbox_uri2);
let res2 = session.runSql("select 1");
let rows2 = res2.fetchAll();
EXPECT_NE(res2.statementId, "");
EXPECT_NE(res2.getStatementId(), "");
session.close();


//@<> Cleanup
testutil.destroySandbox(__mysql_sandbox_port1);
testutil.destroySandbox(__mysql_sandbox_port2);
