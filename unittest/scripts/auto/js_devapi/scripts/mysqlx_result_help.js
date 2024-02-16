//@ Initialization
shell.connect(__uripwd);
var schema = session.createSchema('result_help');
var collection = schema.createCollection('sample');
var result = collection.add({_id:'sample', one:1}).execute();
session.close();

//@ Result help
result.help();

//@ Result help, \? [USE:Result help]
\? Result

//@ Help on affectedItemsCount
result.help('affectedItemsCount');

//@ Help on affectedItemsCount, \? [USE:Help on affectedItemsCount]
\? Result.affectedItemsCount

//@ Help on autoIncrementValue
result.help('autoIncrementValue');

//@ Help on autoIncrementValue, \? [USE:Help on autoIncrementValue]
\? Result.autoIncrementValue

//@ Help on executionTime
result.help('executionTime');

//@ Help on executionTime, \? [USE:Help on executionTime]
\? Result.executionTime

//@ Help on generatedIds
result.help('generatedIds');

//@ Help on generatedIds, \? [USE:Help on generatedIds]
\? Result.generatedIds

//@ Help on warningsCount
result.help('warningsCount');

//@ Help on warningsCount, \? [USE:Help on warningsCount]
\? Result.warningsCount

//@ Help on warnings
result.help('warnings');

//@ Help on warnings, \? [USE:Help on warnings]
\? Result.warnings

//@ Help on getAffectedItemsCount
result.help('getAffectedItemsCount');

//@ Help on getAffectedItemsCount, \? [USE:Help on getAffectedItemsCount]
\? Result.getAffectedItemsCount

//@ Help on getAutoIncrementValue
result.help('getAutoIncrementValue');

//@ Help on getAutoIncrementValue, \? [USE:Help on getAutoIncrementValue]
\? Result.getAutoIncrementValue

//@ Help on getExecutionTime
result.help('getExecutionTime');

//@ Help on getExecutionTime, \? [USE:Help on getExecutionTime]
\? Result.getExecutionTime

//@ Help on getGeneratedIds
result.help('getGeneratedIds');

//@ Help on getGeneratedIds, \? [USE:Help on getGeneratedIds]
\? Result.getGeneratedIds

//@ Help on getWarningsCount
result.help('getWarningsCount');

//@ Help on getWarningsCount, \? [USE:Help on getWarningsCount]
\? Result.getWarningsCount

//@ Help on getWarnings
result.help('getWarnings');

//@ Help on getWarnings, \? [USE:Help on getWarnings]
\? Result.getWarnings

//@ Help on help
result.help('help');

//@ Help on help, \? [USE:Help on help]
\? Result.help

//@ Finalization
shell.connect(__uripwd);
session.dropSchema('result_help');
session.close();
