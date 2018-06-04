shell.connect(__mysqluripwd);
var result = session.runSql('select 1');
session.close();

//@ ClassicResult help
result.help();

//@ ClassicResult help, \? [USE:ClassicResult help]
\? classicresult

//@ Help on affectedRowCount
result.help('affectedRowCount')

//@ Help on affectedRowCount, \? [USE:Help on affectedRowCount]
\? classicresult.affectedRowCount

//@ Help on autoIncrementValue
result.help('autoIncrementValue')

//@ Help on autoIncrementValue, \? [USE:Help on autoIncrementValue]
\? classicresult.autoIncrementValue

//@ Help on columnCount
result.help('columnCount')

//@ Help on columnCount, \? [USE:Help on columnCount]
\? classicresult.columnCount

//@ Help on columnNames
result.help('columnNames')

//@ Help on columnNames, \? [USE:Help on columnNames]
\? classicresult.columnNames

//@ Help on columns
result.help('columns')

//@ Help on columns, \? [USE:Help on columns]
\? classicresult.columns

//@ Help on executionTime
result.help('executionTime')

//@ Help on executionTime, \? [USE:Help on executionTime]
\? classicresult.executionTime

//@ Help on info
result.help('info')

//@ Help on info, \? [USE:Help on info]
\? classicresult.info

//@ Help on warningCount
result.help('warningCount')

//@ Help on warningCount, \? [USE:Help on warningCount]
\? classicresult.warningCount

//@ Help on warnings
result.help('warnings')

//@ Help on warnings, \? [USE:Help on warnings]
\? classicresult.warnings

//@ Help on fetchAll
result.help('fetchAll')

//@ Help on fetchAll, \? [USE:Help on fetchAll]
\? classicresult.fetchAll

//@ Help on fetchOne
result.help('fetchOne')

//@ Help on fetchOne, \? [USE:Help on fetchOne]
\? classicresult.fetchOne

//@ Help on getAffectedRowCount
result.help('getAffectedRowCount')

//@ Help on getAffectedRowCount, \? [USE:Help on getAffectedRowCount]
\? classicresult.getAffectedRowCount

//@ Help on getAutoIncrementValue
result.help('getAutoIncrementValue')

//@ Help on getAutoIncrementValue, \? [USE:Help on getAutoIncrementValue]
\? classicresult.getAutoIncrementValue

//@ Help on getColumnCount
result.help('getColumnCount')

//@ Help on getColumnCount, \? [USE:Help on getColumnCount]
\? classicresult.getColumnCount

//@ Help on getColumnNames
result.help('getColumnNames')

//@ Help on getColumnNames, \? [USE:Help on getColumnNames]
\? classicresult.getColumnNames

//@ Help on getColumns
result.help('getColumns')

//@ Help on getColumns, \? [USE:Help on getColumns]
\? classicresult.getColumns

//@ Help on getExecutionTime
result.help('getExecutionTime')

//@ Help on getExecutionTime, \? [USE:Help on getExecutionTime]
\? classicresult.getExecutionTime

//@ Help on getInfo
result.help('getInfo')

//@ Help on getInfo, \? [USE:Help on getInfo]
\? classicresult.getInfo

//@ Help on getWarningCount
result.help('getWarningCount')

//@ Help on getWarningCount, \? [USE:Help on getWarningCount]
\? classicresult.getWarningCount

//@ Help on getWarnings
result.help('getWarnings')

//@ Help on getWarnings, \? [USE:Help on getWarnings]
\? classicresult.getWarnings

//@ Help on hasData
result.help('hasData')

//@ Help on hasData, \? [USE:Help on hasData]
\? classicresult.hasData

//@ Help on help
result.help('help')

//@ Help on help, \? [USE:Help on help]
\? classicresult.help

//@ Help on nextDataSet
result.help('nextDataSet')

//@ Help on nextDataSet, \? [USE:Help on nextDataSet]
\? classicresult.nextDataSet
