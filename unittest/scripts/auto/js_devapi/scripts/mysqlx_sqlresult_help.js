//@ Initialization
shell.connect(__uripwd);
var result = session.sql("select 1").execute();
session.close();

//@ SqlResult help
result.help();

//@ SqlResult help, \? [USE:SqlResult help]
\? SqlResult

//@ Help on affectedItemsCount
result.help('affectedItemsCount');

//@ Help on affectedItemsCount, \? [USE:Help on affectedItemsCount]
\? SqlResult.affectedItemsCount

//@ Help on affectedRowCount
result.help('affectedRowCount');

//@ Help on affectedRowCount, \? [USE:Help on affectedRowCount]
\? SqlResult.affectedRowCount

//@ Help on autoIncrementValue
result.help('autoIncrementValue');

//@ Help on autoIncrementValue, \? [USE:Help on autoIncrementValue]
\? SqlResult.autoIncrementValue

//@ Help on columnCount
result.help('columnCount');

//@ Help on columnCount, \? [USE:Help on columnCount]
\? SqlResult.columnCount

//@ Help on columnNames
result.help('columnNames');

//@ Help on columnNames, \? [USE:Help on columnNames]
\? SqlResult.columnNames

//@ Help on columns
result.help('columns');

//@ Help on columns, \? [USE:Help on columns]
\? SqlResult.columns

//@ Help on executionTime
result.help('executionTime');

//@ Help on executionTime, \? [USE:Help on executionTime]
\? SqlResult.executionTime

//@ Help on warningCount
result.help('warningCount');

//@ Help on warningCount, \? [USE:Help on warningCount]
\? SqlResult.warningCount

//@ Help on warnings
result.help('warnings');

//@ Help on warnings, \? [USE:Help on warnings]
\? SqlResult.warnings

//@ Help on warningsCount
result.help('warningsCount');

//@ Help on warningsCount, \? [USE:Help on warningsCount]
\? SqlResult.warningsCount

//@ Help on fetchAll
result.help('fetchAll');

//@ Help on fetchAll, \? [USE:Help on fetchAll]
\? SqlResult.fetchAll

//@ Help on fetchOne
result.help('fetchOne');

//@ Help on fetchOne, \? [USE:Help on fetchOne]
\? SqlResult.fetchOne

//@ Help on fetchOneObject
result.help('fetchOneObject');

//@ Help on fetchOneObject, \? [USE:Help on fetchOneObject]
\? SqlResult.fetchOneObject

//@ Help on getAffectedItemsCount
result.help('getAffectedItemsCount');

//@ Help on getAffectedItemsCount, \? [USE:Help on getAffectedItemsCount]
\? SqlResult.getAffectedItemsCount

//@ Help on getAffectedRowCount
result.help('getAffectedRowCount');

//@ Help on getAffectedRowCount, \? [USE:Help on getAffectedRowCount]
\? SqlResult.getAffectedRowCount

//@ Help on getAutoIncrementValue
result.help('getAutoIncrementValue');

//@ Help on getAutoIncrementValue, \? [USE:Help on getAutoIncrementValue]
\? SqlResult.getAutoIncrementValue

//@ Help on getColumnCount
result.help('getColumnCount');

//@ Help on getColumnCount, \? [USE:Help on getColumnCount]
\? SqlResult.getColumnCount

//@ Help on getColumnNames
result.help('getColumnNames');

//@ Help on getColumnNames, \? [USE:Help on getColumnNames]
\? SqlResult.getColumnNames

//@ Help on getColumns
result.help('getColumns');

//@ Help on getColumns, \? [USE:Help on getColumns]
\? SqlResult.getColumns

//@ Help on getExecutionTime
result.help('getExecutionTime');

//@ Help on getExecutionTime, \? [USE:Help on getExecutionTime]
\? SqlResult.getExecutionTime

//@ Help on getWarningCount
result.help('getWarningCount');

//@ Help on getWarningCount, \? [USE:Help on getWarningCount]
\? SqlResult.getWarningCount

//@ Help on getWarnings
result.help('getWarnings');

//@ Help on getWarnings, \? [USE:Help on getWarnings]
\? SqlResult.getWarnings

//@ Help on getWarningsCount
result.help('getWarningsCount');

//@ Help on getWarningsCount, \? [USE:Help on getWarningsCount]
\? SqlResult.getWarningsCount

//@ Help on hasData
result.help('hasData');

//@ Help on hasData, \? [USE:Help on hasData]
\? SqlResult.hasData

//@ Help on help
result.help('help');

//@ Help on help, \? [USE:Help on help]
\? SqlResult.help

//@ Help on nextDataSet
result.help('nextDataSet');

//@ Help on nextDataSet, \? [USE:Help on nextDataSet]
\? SqlResult.nextDataSet
