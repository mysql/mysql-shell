// connect
shell.connect(__uripwd);

WIPE_SHELL_LOG();

//@ switch to "table" output format
shell.options.outputFormat = "table";

//@ check if there are any deprecated warnings (1)
session.sql('select 1, 2, 3;');
EXPECT_SHELL_LOG_NOT_CONTAINS("Warning: 'SqlResult.warningCount' is deprecated, use 'SqlResult.warningsCount' instead.");
EXPECT_SHELL_LOG_NOT_CONTAINS("Warning: 'SqlResult.nextDataSet' is deprecated, use 'SqlResult.nextResult' instead.");

WIPE_SHELL_LOG();

//@ switch to "json" output format
shell.options.outputFormat = "json";

//@ check if there are any deprecated warnings (2)
session.sql('select 1, 2, 3;');
EXPECT_SHELL_LOG_NOT_CONTAINS("Warning: 'SqlResult.warningCount' is deprecated, use 'SqlResult.warningsCount' instead.");
EXPECT_SHELL_LOG_NOT_CONTAINS("Warning: 'SqlResult.affectedRowCount' is deprecated, use 'SqlResult.affectedItemsCount' instead.");

//@ cleanup
session.close();
