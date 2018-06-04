//@ Initialization
shell.connect(__uripwd);
var schema = session.createSchema('docresult_help');
var collection = schema.createCollection('sample');
var result = collection.find().execute();
session.close();

//@ DocResult help
result.help();

//@ DocResult help, \? [USE:DocResult help]
\? DocResult

//@ Help on executionTime
result.help('executionTime');

//@ Help on executionTime, \? [USE:Help on executionTime]
\? DocResult.executionTime

//@ Help on warningCount
result.help('warningCount');

//@ Help on warningCount, \? [USE:Help on warningCount]
\? DocResult.warningCount

//@ Help on warnings
result.help('warnings');

//@ Help on warnings, \? [USE:Help on warnings]
\? DocResult.warnings

//@ Help on fetchAll
result.help('fetchAll');

//@ Help on fetchAll, \? [USE:Help on fetchAll]
\? DocResult.fetchAll

//@ Help on fetchOne
result.help('fetchOne');

//@ Help on fetchOne, \? [USE:Help on fetchOne]
\? DocResult.fetchOne

//@ Help on getExecutionTime
result.help('getExecutionTime');

//@ Help on getExecutionTime, \? [USE:Help on getExecutionTime]
\? DocResult.getExecutionTime

//@ Help on getWarningCount
result.help('getWarningCount');

//@ Help on getWarningCount, \? [USE:Help on getWarningCount]
\? DocResult.getWarningCount

//@ Help on getWarnings
result.help('getWarnings');

//@ Help on getWarnings, \? [USE:Help on getWarnings]
\? DocResult.getWarnings

//@ Help on help
result.help('help');

//@ Help on help, \? [USE:Help on help]
\? DocResult.help

//@ Finalization
shell.connect(__uripwd);
session.dropSchema('docresult_help');
session.close();
