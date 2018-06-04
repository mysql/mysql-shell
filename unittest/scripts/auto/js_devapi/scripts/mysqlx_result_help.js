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

//@ Help on affectedItemCount
result.help('affectedItemCount');

//@ Help on affectedItemCount, \? [USE:Help on affectedItemCount]
\? Result.affectedItemCount

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

//@ Help on warningCount
result.help('warningCount');

//@ Help on warningCount, \? [USE:Help on warningCount]
\? Result.warningCount

//@ Help on warnings
result.help('warnings');

//@ Help on warnings, \? [USE:Help on warnings]
\? Result.warnings

//@ Help on getAffectedItemCount
result.help('getAffectedItemCount');

//@ Help on getAffectedItemCount, \? [USE:Help on getAffectedItemCount]
\? Result.getAffectedItemCount

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

//@ Help on getWarningCount
result.help('getWarningCount');

//@ Help on getWarningCount, \? [USE:Help on getWarningCount]
\? Result.getWarningCount

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
