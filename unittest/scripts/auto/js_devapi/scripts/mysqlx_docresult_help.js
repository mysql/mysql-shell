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

//@ Help on affectedItemsCount
result.help('affectedItemsCount');

//@ Help on affectedItemsCount, \? [USE:Help on affectedItemsCount]
\? DocResult.affectedItemsCount

//@ Help on executionTime
result.help('executionTime');

//@ Help on executionTime, \? [USE:Help on executionTime]
\? DocResult.executionTime

//@ Help on warnings
result.help('warnings');

//@ Help on warnings, \? [USE:Help on warnings]
\? DocResult.warnings

//@ Help on warningsCount
result.help('warningsCount');

//@ Help on warningsCount, \? [USE:Help on warningsCount]
\? DocResult.warningsCount

//@ Help on fetchAll
result.help('fetchAll');

//@ Help on fetchAll, \? [USE:Help on fetchAll]
\? DocResult.fetchAll

//@ Help on fetchOne
result.help('fetchOne');

//@ Help on fetchOne, \? [USE:Help on fetchOne]
\? DocResult.fetchOne

//@ Help on getAffectedItemsCount
result.help('getAffectedItemsCount');

//@ Help on getAffectedItemsCount, \? [USE:Help on getAffectedItemsCount]
\? DocResult.getAffectedItemsCount

//@ Help on getExecutionTime
result.help('getExecutionTime');

//@ Help on getExecutionTime, \? [USE:Help on getExecutionTime]
\? DocResult.getExecutionTime

//@ Help on getWarnings
result.help('getWarnings');

//@ Help on getWarnings, \? [USE:Help on getWarnings]
\? DocResult.getWarnings

//@ Help on getWarningsCount
result.help('getWarningsCount');

//@ Help on getWarningsCount, \? [USE:Help on getWarningsCount]
\? DocResult.getWarningsCount

//@ Help on help
result.help('help');

//@ Help on help, \? [USE:Help on help]
\? DocResult.help

//@ Finalization
shell.connect(__uripwd);
session.dropSchema('docresult_help');
session.close();
