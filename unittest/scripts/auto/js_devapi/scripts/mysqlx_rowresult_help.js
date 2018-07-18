//@ Initialization
shell.connect(__uripwd + '/mysql');
var result = db.user.select().execute();
session.close();

//@ RowResult help
result.help();

//@ RowResult help, \? [USE:RowResult help]
\? RowResult

//@ Help on affectedItemsCount
result.help('affectedItemsCount');

//@ Help on affectedItemsCount, \? [USE:Help on affectedItemsCount]
\? RowResult.affectedItemsCount

//@ Help on columnCount
result.help('columnCount');

//@ Help on columnCount, \? [USE:Help on columnCount]
\? RowResult.columnCount

//@ Help on columnNames
result.help('columnNames');

//@ Help on columnNames, \? [USE:Help on columnNames]
\? RowResult.columnNames

//@ Help on columns
result.help('columns');

//@ Help on columns, \? [USE:Help on columns]
\? RowResult.columns

//@ Help on executionTime
result.help('executionTime');

//@ Help on executionTime, \? [USE:Help on executionTime]
\? RowResult.executionTime

//@ Help on warningCount
result.help('warningCount');

//@ Help on warningCount, \? [USE:Help on warningCount]
\? RowResult.warningCount

//@ Help on warnings
result.help('warnings');

//@ Help on warnings, \? [USE:Help on warnings]
\? RowResult.warnings

//@ Help on warningsCount
result.help('warningsCount');

//@ Help on warningsCount, \? [USE:Help on warningsCount]
\? RowResult.warningsCount

//@ Help on fetchAll
result.help('fetchAll');

//@ Help on fetchAll, \? [USE:Help on fetchAll]
\? RowResult.fetchAll

//@ Help on fetchOne
result.help('fetchOne');

//@ Help on fetchOne, \? [USE:Help on fetchOne]
\? RowResult.fetchOne

//@ Help on getAffectedItemsCount
result.help('getAffectedItemsCount');

//@ Help on getAffectedItemsCount, \? [USE:Help on getAffectedItemsCount]
\? RowResult.getAffectedItemsCount

//@ Help on getColumnCount
result.help('getColumnCount');

//@ Help on getColumnCount, \? [USE:Help on getColumnCount]
\? RowResult.getColumnCount

//@ Help on getColumnNames
result.help('getColumnNames');

//@ Help on getColumnNames, \? [USE:Help on getColumnNames]
\? RowResult.getColumnNames

//@ Help on getColumns
result.help('getColumns');

//@ Help on getColumns, \? [USE:Help on getColumns]
\? RowResult.getColumns

//@ Help on getExecutionTime
result.help('getExecutionTime');

//@ Help on getExecutionTime, \? [USE:Help on getExecutionTime]
\? RowResult.getExecutionTime

//@ Help on getWarningCount
result.help('getWarningCount');

//@ Help on getWarningCount, \? [USE:Help on getWarningCount]
\? RowResult.getWarningCount

//@ Help on getWarnings
result.help('getWarnings');

//@ Help on getWarnings, \? [USE:Help on getWarnings]
\? RowResult.getWarnings

//@ Help on getWarningsCount
result.help('getWarningsCount');

//@ Help on getWarningsCount, \? [USE:Help on getWarningsCount]
\? RowResult.getWarningsCount

//@ Help on help
result.help('help');

//@ Help on help, \? [USE:Help on help]
\? RowResult.help

//@ Finalization
session.close();
