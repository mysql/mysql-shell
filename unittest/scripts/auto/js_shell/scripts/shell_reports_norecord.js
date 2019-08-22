// Tests of WL#11263 \show and \watch command infrastructure

// -----------------------------------------------------------------------------
// WL11263_TSF12_1 - Validate that an exception is thrown using \show command without an active session.

//@ WL11263_TSF12_1 - run \show without a session
\show query SELECT 1

// WL11263_TSF12_2 - Validate that an exception is thrown using \watch command without an active session.

//@ WL11263_TSF12_2 - run \watch without a session [USE: WL11263_TSF12_1 - run \show without a session]
\watch query SELECT 1

//@ BUG#30083371 - help without session
\show query --help

//@ create a session
shell.connect(__uripwd);

//@ close the session immediately
session.close();

//@ WL11263_TSF12_1 - run \show with closed session [USE: WL11263_TSF12_1 - run \show without a session]
\show query SELECT 1

//@ WL11263_TSF12_2 - run \watch with closed session [USE: WL11263_TSF12_1 - run \show without a session]
\watch query SELECT 1

//@ BUG#30083371 - help with closed session [USE: BUG#30083371 - help without session]
\show query --help

// -----------------------------------------------------------------------------
//@ create a classic session
shell.connect(__mysqluripwd);

//@ BUG#30083371 - help with active session [USE: BUG#30083371 - help without session]
\show query --help

//@ classic session - switch to Python
\py

//@ run \show with classic session in Python
\show query SELECT 1

//@ classic session - switch to SQL mode
\sql

//@ run \show with classic session in SQL mode [USE: run \show with classic session in Python]
\show query SELECT 1

//@ classic session - switch to JavaScript
\js

//@ run \show with classic session in JavaScript [USE: run \show with classic session in Python]
\show query SELECT 1

//@ close the classic session [USE: close the session immediately]
session.close();

// -----------------------------------------------------------------------------
//@ create a new session for all the remaining tests [USE: create a session]
shell.connect(__uripwd);

// -----------------------------------------------------------------------------
// WL11263_TSF2_2 - Validate that the \show command can be used in all the scripting modes: JS, PY, SQL.

//@ WL11263_TSF2_2 - switch to Python
\py

//@ WL11263_TSF2_2 - run show command in Python
\show query SELECT 1

//@ WL11263_TSF2_2 - switch to SQL mode
\sql

//@ WL11263_TSF2_2 - run show command in SQL mode [USE: WL11263_TSF2_2 - run show command in Python]
\show query SELECT 1

//@ WL11263_TSF2_2 - switch to JavaScript
\js

//@ WL11263_TSF2_2 - run show command in JavaScript mode [USE: WL11263_TSF2_2 - run show command in Python]
\show query SELECT 1

// -----------------------------------------------------------------------------
//@ WL11263_TSF2_3 - Validate that using the \show command with an invalid report name throws an exception.
\show unknown_report

// -----------------------------------------------------------------------------
//@ WL11263_TSF2_4 - Validate that using the \show command without a report name list the reports available.
\show

// -----------------------------------------------------------------------------
//@ WL11263_TSF3_6 - Validate that using the \watch command with an invalid report name throws an exception. [USE: WL11263_TSF2_3 - Validate that using the \show command with an invalid report name throws an exception.]
\watch unknown_report

// -----------------------------------------------------------------------------
//@ WL11263_TSF3_7 - Validate that using the \watch command without a report name list the reports available. [USE: WL11263_TSF2_4 - Validate that using the \show command without a report name list the reports available.]
\watch

// -----------------------------------------------------------------------------
// WL11263_TSF5_4 - Call the \watch command with the --interval (-i) option setting a lower value than the lowest value allowed, analyze the behaviour (error expected).

//@ WL11263_TSF5_4 - use \watch command with an invalid --interval value (below threshold)
\watch query --interval 0.09

//@ WL11263_TSF5_4 - use \watch command with an invalid -i value (below threshold) [USE: WL11263_TSF5_4 - use \watch command with an invalid --interval value (below threshold)]
\watch query -i 0.09

// -----------------------------------------------------------------------------
// WL11263_TSF5_5 - Call the \watch command with the --interval (-i) option setting a lower value than the lowest value allowed, analyze the behaviour (error expected).

//@ WL11263_TSF5_5 - use \watch command with an invalid --interval value (above threshold)
\watch query --interval 86400.1

//@ WL11263_TSF5_5 - use \watch command with an invalid -i value (above threshold) [USE: WL11263_TSF5_5 - use \watch command with an invalid --interval value (above threshold)]
\watch query -i 86400.1

// -----------------------------------------------------------------------------
// WL11263_TSF2_5 - Validate that a report registered with an all lower-case name can be invoked by the \show command using all upper-case name.

//@ WL11263_TSF2_5 - register the report with all lower-case name
shell.registerReport('lowercasereportname', 'print', function (){ print('lower-case report name'); return { report: [] }; });

//@ WL11263_TSF2_5 - call the report using upper-case name
\show LOWERCASEREPORTNAME

//@ WL11263_TSF2_5 - register the report with all upper-case name
shell.registerReport('UPPERCASEREPORTNAME', 'print', function (){ print('upper-case report name'); return { report: [] }; });

//@ WL11263_TSF2_5 - call the report using lower-case name
\show uppercasereportname

// -----------------------------------------------------------------------------
// WL11263_TSF2_6 - Validate that a report registered with a name containing '_' character can be invoked by the \show command using a name with '_' character replaced by '-' character.

//@ WL11263_TSF2_6 - register the report with name containing '_'
shell.registerReport('report_with_underscore', 'print', function (){ print('report name with underscore'); return { report: [] }; });

//@ WL11263_TSF2_6 - call the report using hyphen
\show report-with-underscore

//@ WL11263_TSF2_6 - call the report - mix upper-case, underscore and hyphen
\show report-WITH_underscore

//@ WL11263_TSF2_6 - register the report with upper-case name containing '_'
shell.registerReport('UPPER_CASE_REPORT', 'print', function (){ print('upper-case report name with underscore'); return { report: [] }; });

//@ WL11263_TSF2_6 - call the upper-case report - mix upper-case, underscore and hyphen
\show upper_CASE-report

// -----------------------------------------------------------------------------
// Test invalid values of 'function' parameter

//@ try to register the report with function set to undefined
shell.registerReport('invalid_report', 'print', undefined)

//@ try to register the report with function set to null [USE: try to register the report with function set to undefined]
shell.registerReport('invalid_report', 'print', null)

//@ try to register the report with function set to invalid type [USE: try to register the report with function set to undefined]
shell.registerReport('invalid_report', 'print', 'function')

// -----------------------------------------------------------------------------
// Test invalid values of 'options' parameter

//@ try to register the report with options set to invalid type
shell.registerReport('invalid_report', 'print', function (){}, 'options')

// -----------------------------------------------------------------------------
//@ WL11263_TSF9_1 - Try to register a plugin report with a duplicated name, error is expected.
shell.registerReport('query', 'print', function (){})

// -----------------------------------------------------------------------------
//@ WL11263_TSF9_2 - Try to register a plugin report with an invalid identifier as its name, error is expected.
shell.registerReport('1', 'print', function (){})

// -----------------------------------------------------------------------------
// WL11263_TSF9_3 - Try to register a plugin report without giving a value to the 'name' parameter, error is expected.

//@ WL11263_TSF9_3 - The 'name' key is set to undefined
shell.registerReport(undefined, 'print', function (){})

//@ WL11263_TSF9_3 - The 'name' key is set to null [USE: WL11263_TSF9_3 - The 'name' key is set to undefined]
shell.registerReport(null, 'print', function (){})

//@ WL11263_TSF9_3 - The 'name' key is set to an invalid type [USE: WL11263_TSF9_3 - The 'name' key is set to undefined]
shell.registerReport({}, 'print', function (){})

//@ WL11263_TSF9_3 - The 'name' key is set to an empty string
shell.registerReport('', 'print', function (){})

// -----------------------------------------------------------------------------
// WL11263_TSF9_4 - Try to register a plugin report without giving a value to the 'type' parameter, error is expected.

//@ WL11263_TSF9_4 - The 'type' key is set to undefined
shell.registerReport('invalid_report', undefined, function (){})

//@ WL11263_TSF9_4 - The 'type' key is set to null [USE: WL11263_TSF9_4 - The 'type' key is set to undefined]
shell.registerReport('invalid_report', null, function (){})

//@ WL11263_TSF9_4 - The 'type' key is set to an invalid type [USE: WL11263_TSF9_4 - The 'type' key is set to undefined]
shell.registerReport('invalid_report', {}, function (){})

// -----------------------------------------------------------------------------
// WL11263_TSF9_5 - Try to register a plugin report with a value different than the supported as the value for the 'type' dictionary option, error is expected.

//@ WL11263_TSF9_5 - The 'name' key is set to an empty string
shell.registerReport('invalid_report', '', function (){})

//@ WL11263_TSF9_5 - The 'name' key is set to an invalid string [USE: WL11263_TSF9_5 - The 'name' key is set to an empty string]
shell.registerReport('invalid_report', 'unknown_type', function (){})

// -----------------------------------------------------------------------------
// Check if it's possible to register reports with allowed types

//@ Register report with valid type - 'print'
shell.registerReport('sample_print_type_report', 'print', function (){})

//@ Register report with valid type - 'report'
shell.registerReport('sample_report_type_report', 'report', function (){})

//@ Register report with valid type - 'list'
shell.registerReport('sample_list_type_report', 'list', function (){})

// -----------------------------------------------------------------------------
//@ WL11263_TSF9_6 - Validate that plugin report can be registered if the dictionary options 'description' and 'options' are not provided.
shell.registerReport('valid_report_without_brief_description_options_argc', 'print', function (){}, {})

// -----------------------------------------------------------------------------
// Try to register a plugin report giving an invalid value to the 'brief' dictionary option, error is expected.

//@ The 'brief' key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'brief' : undefined})

//@ The 'brief' key is set to null
shell.registerReport('invalid_report', 'print', function (){}, {'brief' : null})

//@ The 'brief' key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'brief' : {}})

// -----------------------------------------------------------------------------
// Try to register a plugin report giving an invalid value to the 'details' dictionary option, error is expected.

//@ The 'details' key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'details' : undefined})

//@ The 'details' key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'details' : {}})

//@ The 'details' array holds an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'details' : [1]})

// -----------------------------------------------------------------------------
// Test invalid values of 'options' key

//@ The 'options' key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'options' : undefined})

//@ The 'options' key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : ''})

//@ The 'options' key is set to an array of invalid types
shell.registerReport('invalid_report', 'print', function (){}, {'options' : ['opt']})

// -----------------------------------------------------------------------------
// WL11263_TSF9_7 - If the dictionary option 'options' is given when registering a plugin report, validate that an exception is thrown if the 'name' key is not provided.

//@ WL11263_TSF9_7 - Missing 'name' key
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{}]})

//@ WL11263_TSF9_7 - The 'name' key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': undefined}]})

//@ WL11263_TSF9_7 - The 'name' key is set to null
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': null}]})

//@ WL11263_TSF9_7 - The 'name' key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': {}}]})

//@ WL11263_TSF9_7 - The 'name' key is set to an empty string
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': ''}]})

// -----------------------------------------------------------------------------
//@ WL11263_TSF9_8 - If the dictionary option 'options' is given when registering a plugin report, validate that an exception is thrown if the 'name' is not a valid identifier.
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': '1'}]})

// -----------------------------------------------------------------------------
// WL11263_TSF9_9 - If the dictionary option 'options' is given when registering a plugin report, set a duplicate value for the 'name' key, analyze the behaviour.

//@ WL11263_TSF9_9 - Option duplicates --help
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'help'}]})

//@ WL11263_TSF9_9 - Option duplicates --interval
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'interval'}]})

//@ WL11263_TSF9_9 - Option duplicates --nocls
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'nocls'}]})

//@ WL11263_TSF9_9 - Option duplicates --vertical in a 'list' type report
shell.registerReport('invalid_report', 'list', function (){}, {'options' : [{'name': 'vertical'}]})

//@ WL11263_TSF9_9 - Two options with the same name
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'option'}, {'name': 'option'}]})

//@ Report uses --vertical option in a 'print' type report - no error
shell.registerReport('print_type_report_with_vertical_option', 'print', function (){}, {'options' : [{'name': 'vertical'}]})

//@ Report uses --vertical option in a 'report' type report - no error
shell.registerReport('report_type_report_with_vertical_option', 'report', function (){}, {'options' : [{'name': 'vertical'}]})

// -----------------------------------------------------------------------------
// a list report which returns the value of option 'test'

function report_get_test_value(s, a, o) {
  return {'report' : [['value'], [o['test']]]}
}

// -----------------------------------------------------------------------------
// WL11263_TSF9_10 - If the dictionary option 'options' is given when registering a plugin report, validate that the 'name' key works as expected when calling the report.

//@ WL11263_TSF9_10 - Register the report
shell.registerReport('list_report_with_long_option', 'list', report_get_test_value, {'options' : [{'name': 'test'}]})

//@ WL11263_TSF9_10 - Execute the report
\show list_report_with_long_option --test "some text value"

// -----------------------------------------------------------------------------
// Test invalid values of 'shortcut' key

//@ The 'shortcut' key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'shortcut' : undefined}]})

//@ The 'shortcut' key is set to null
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'shortcut' : null}]})

//@ The 'shortcut' key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'shortcut' : {}}]})

//@ The 'shortcut' key is set to non-alphanumeric
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'shortcut' : '?'}]})

//@ The 'shortcut' key is set to more than one character
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'shortcut' : 'XY'}]})

// -----------------------------------------------------------------------------
// WL11263_TSF9_11 - If the dictionary option 'options' is given when registering a plugin report, set a duplicate value for the 'shortcut' key, analyze the behaviour.

//@ WL11263_TSF9_11 - Option duplicates -i
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'option', 'shortcut' : 'i'}]})

//@ WL11263_TSF9_11 - Option duplicates -E in a 'list' type report
shell.registerReport('invalid_report', 'list', function (){}, {'options' : [{'name': 'option', 'shortcut' : 'E'}]})

//@ WL11263_TSF9_11 - Two options with the same name
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'option1', 'shortcut' : 'o'}, {'name': 'option2', 'shortcut' : 'o'}]})

//@ Report uses -E option in a 'print' type report - no error
shell.registerReport('print_type_report_with_E_option', 'print', function (){}, {'options' : [{'name': 'option', 'shortcut' : 'E'}]})

//@ Report uses -E option in a 'report' type report - no error
shell.registerReport('report_type_report_with_E_option', 'report', function (){}, {'options' : [{'name': 'option', 'shortcut' : 'E'}]})

// -----------------------------------------------------------------------------
// WL11263_TSF9_12 - If the dictionary option 'options' is given when registering a plugin report, validate that if 'shortcut' key is given it works as the alias of the 'name' key when calling the report.

//@ WL11263_TSF9_12 - Register the report.
shell.registerReport('list_report_with_long_option_and_shortcut', 'list', report_get_test_value, {'options' : [{'name': 'test', 'shortcut' : 't'}]})

//@ WL11263_TSF9_12 - Execute the report with long option.
\show list_report_with_long_option_and_shortcut --test 1234567890

//@ WL11263_TSF9_12 - Execute the report with short option. [USE: WL11263_TSF9_12 - Execute the report with long option.]
\show list_report_with_long_option_and_shortcut -t 1234567890

// -----------------------------------------------------------------------------
// Test invalid values of 'brief' key

//@ The options['brief'] key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'brief' : undefined}]})

//@ The options['brief'] key is set to null
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'brief' : null}]})

//@ The options['brief'] key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'brief' : {}}]})

// -----------------------------------------------------------------------------
// Test invalid values of 'details' key

//@ The options['details'] key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'details' : undefined}]})

//@ The options['details'] key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'details' : {}}]})

//@ The options['details'] key holds an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'details' : [1]}]})

// -----------------------------------------------------------------------------
// WL11263_TSF9_13 - If the dictionary option 'options' is given when registering a plugin report, validate that the 'type' key just takes 'string', 'bool', 'integer', 'float' as valid values.

//@ WL11263_TSF9_13 - The 'type' key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'type' : undefined}]})

//@ WL11263_TSF9_13 - The 'type' key is set to null
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'type' : null}]})

//@ WL11263_TSF9_13 - The 'type' key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'type' : {}}]})

//@ WL11263_TSF9_13 - The 'type' key is set to an empty string
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'type' : ''}]})

//@ WL11263_TSF9_13 - The 'type' key is set to an invalid value
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'type' : 'invalid_type'}]})

//@ WL11263_TSF9_13 - The 'type' key has valid values
shell.registerReport('valid_report_with_options_with_valid_types', 'print', function (){}, {'options' : [{'name': 'one', 'type' : 'string'}, {'name': 'two', 'type' : 'bool'}, {'name': 'three', 'type' : 'integer'}, {'name': 'four', 'type' : 'float'}]})

// -----------------------------------------------------------------------------
// Test invalid values of 'required' key

//@ The 'required' key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'required' : undefined}]})

//@ The 'required' key is set to null
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'required' : null}]})

//@ The 'required' key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'required' : {}}]})

//@ Option cannot be 'required' if it's bool
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'type' : 'bool', 'required' : true}]})

// -----------------------------------------------------------------------------
// WL11263_TSF9_14 -  Validate that if the 'required' key is set to 'true' to an option, an exception is thrown if is not given when calling the report.

//@ WL11263_TSF9_14 - Register the report.
shell.registerReport('list_report_with_required_option', 'list', report_get_test_value, {'options' : [{'name': 'test', 'shortcut' : 't', 'required' : true}]})

//@ WL11263_TSF9_14 - Execute the report without required option.
\show list_report_with_required_option

//@ WL11263_TSF9_14 - Execute the report with required option.
\show list_report_with_required_option --test qwerty

//@ WL11263_TSF9_14 - Execute the report with required option using shortcut. [USE: WL11263_TSF9_14 - Execute the report with required option.]
\show list_report_with_required_option -t qwerty

// -----------------------------------------------------------------------------
// WL11263_TSF9_15 - Validate that if the 'required' key is not set or is set to 'false' to an option, an exception is not thrown if is not given when calling the report.

//@ WL11263_TSF9_15 - Register the report.
shell.registerReport('list_report_with_required_option_and_some_non_required_ones', 'list', report_get_test_value, {'options' : [{'name': 'test', 'required' : true}, {'name': 'one'}, {'name': 'two', 'required' : false}]})

//@ WL11263_TSF9_15 - Execute the report without non-required options.
\show list_report_with_required_option_and_some_non_required_ones --test 9876.54321

// -----------------------------------------------------------------------------
// Test invalid values of 'values' key

//@ The 'values' key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'values' : undefined}]})

//@ The 'values' key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'values' : {}}]})

//@ The 'values' list holds invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'values' : [{}]}]})

//@ The 'values' list cannot be defined for an 'integer' type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'type' : 'integer', 'values' : ["1"]}]})

//@ The 'values' list cannot be defined for a 'bool' type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'type' : 'bool', 'values' : ["false"]}]})

//@ The 'values' list cannot be defined for a 'float' type
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'type' : 'float', 'values' : ["3.14"]}]})

// -----------------------------------------------------------------------------
// WL11263_TSF9_16 - Validate that if the 'values' key is set to an option, an exception is thrown if a different value that the ones set is given when calling the report.

//@ WL11263_TSF9_16 - Register the report.
shell.registerReport('list_report_with_list_of_allowed_values', 'list', report_get_test_value, {'options' : [{'name': 'test', 'values' : ['one', 'two', 'three']}]})

//@ WL11263_TSF9_16 - Execute the report with a disallowed value.
\show list_report_with_list_of_allowed_values --test four

//@ Execute the report with an allowed value.
\show list_report_with_list_of_allowed_values --test three

// -----------------------------------------------------------------------------

//@ Option definition cannot contain unknown keys
shell.registerReport('invalid_report', 'print', function (){}, {'options' : [{'name': 'invalid_option', 'unknown_key' : 'unknown value'}]})

// -----------------------------------------------------------------------------
// Test invalid values of 'argc' key

//@ The 'argc' key is set to undefined
shell.registerReport('invalid_report', 'print', function (){}, {'argc' : undefined})

//@ The 'argc' key is set to null
shell.registerReport('invalid_report', 'print', function (){}, {'argc' : null})

//@ The 'argc' key is set to an invalid type
shell.registerReport('invalid_report', 'print', function (){}, {'argc' : {}})

// -----------------------------------------------------------------------------
// WL11263_TSF9_17 - Validate that the dictionary option 'argc' only takes the following options as a valid value: a number, an asterisk, two numbers separated by a '-' or a number and an asterisk separated by a '-'. All the posible values must be given as string.

//@ WL11263_TSF9_17 - The 'argc' value contains a letter
shell.registerReport('invalid_report', 'print', function (){}, {'argc' : 'x'})

//@ WL11263_TSF9_17 - The 'argc' value contains a question mark
shell.registerReport('invalid_report', 'print', function (){}, {'argc' : '?'})

//@ WL11263_TSF9_17 - The 'argc' value contains two letters
shell.registerReport('invalid_report', 'print', function (){}, {'argc' : 'y-z'})

//@ WL11263_TSF9_17 - The 'argc' value contains asterisk at wrong position
shell.registerReport('invalid_report', 'print', function (){}, {'argc' : '*-7'})

//@ WL11263_TSF9_17 - The 'argc' value contains ill defined range
shell.registerReport('invalid_report', 'print', function (){}, {'argc' : '2-1'})

//@ WL11263_TSF9_17 - The 'argc' value contains ill defined range - third number
shell.registerReport('invalid_report', 'print', function (){}, {'argc' : '1-2-3'})

//@ WL11263_TSF9_17 - The 'argc' is set to 0
shell.registerReport('valid_report_with_argc_0', 'print', function (){}, {'argc' : '0'})

//@ WL11263_TSF9_17 - The 'argc' is set to 7
shell.registerReport('valid_report_with_argc_7', 'print', function (){}, {'argc' : '7'})

//@ WL11263_TSF9_17 - The 'argc' is set to *
shell.registerReport('valid_report_with_argc_asterisk', 'print', function (){}, {'argc' : '*'})

//@ WL11263_TSF9_17 - The 'argc' is set to 0-2
shell.registerReport('valid_report_with_argc_0_2', 'print', function (){}, {'argc' : '0-2'})

//@ WL11263_TSF9_17 - The 'argc' is set to 1-2
shell.registerReport('valid_report_with_argc_1_2', 'print', function (){}, {'argc' : '1-2'})

//@ WL11263_TSF9_17 - The 'argc' is set to 0-*
shell.registerReport('valid_report_with_argc_0_asterisk', 'print', function (){}, {'argc' : '0-*'})

//@ WL11263_TSF9_17 - The 'argc' is set to 1-*
shell.registerReport('valid_report_with_argc_1_asterisk', 'print', function (){}, {'argc' : '1-*'})

// -----------------------------------------------------------------------------
// WL11263_TSF9_18 - Validate that a plugin report already registered can be called in any scripting mode no matter the scripting language of the report.

//@ WL11263_TSF9_18 - Register the JavaScript report
shell.registerReport('javascript_report_to_be_called_from_various_scripting_modes', 'print', function (){println('This is a JavaScript report'); return {'report' : []}})

//@ WL11263_TSF9_18 - Call JS report in JS mode
\show javascript_report_to_be_called_from_various_scripting_modes

//@ WL11263_TSF9_18 - Switch to Python
\py

def python_report_to_be_called_from_various_scripting_modes(s):
  print('This is a Python report')
  return {'report' : []}

//@ WL11263_TSF9_18 - Register the Python report
shell.register_report('python_report_to_be_called_from_various_scripting_modes', 'print', python_report_to_be_called_from_various_scripting_modes)

//@ WL11263_TSF9_18 - Call JS report in PY mode
\show javascript_report_to_be_called_from_various_scripting_modes

//@ WL11263_TSF9_18 - Call PY report in PY mode
\show python_report_to_be_called_from_various_scripting_modes

//@ WL11263_TSF9_18 - Switch to SQL
\sql

//@ WL11263_TSF9_18 - Call JS report in SQL mode
\show javascript_report_to_be_called_from_various_scripting_modes

//@ WL11263_TSF9_18 - Call PY report in SQL mode
\show python_report_to_be_called_from_various_scripting_modes

//@ WL11263_TSF9_18 - Switch to JavaScript
\js

//@ WL11263_TSF9_18 - Call PY report in JS mode
\show python_report_to_be_called_from_various_scripting_modes

// -----------------------------------------------------------------------------
// Invoke reports which throw an exception (all scripting modes)

//@ Register the JavaScript report which throws an exception
shell.registerReport('javascript_report_which_throws_an_exception', 'print', function (){throw 'This is a JavaScript exception'})

//@ Call JS report which throws in JS mode
\show javascript_report_which_throws_an_exception

//@ Switch to Python to create a report and call both of them
\py

def python_report_which_throws_an_exception(s):
  raise Exception('This is a Python exception')

//@ Register the Python report which throws an exception
shell.register_report('python_report_which_throws_an_exception', 'print', python_report_which_throws_an_exception)

//@ Call JS report which throws in PY mode
\show javascript_report_which_throws_an_exception

//@ Call PY report which throws in PY mode
\show python_report_which_throws_an_exception

//@ Switch to SQL to call both throwing reports
\sql

//@ Call JS report which throws in SQL mode
\show javascript_report_which_throws_an_exception

//@ Call PY report which throws in SQL mode
\show python_report_which_throws_an_exception

//@ Switch to JavaScript to call Python report which throws
\js

//@ Call PY report which throws in JS mode
\show python_report_which_throws_an_exception

// -----------------------------------------------------------------------------
// WL11263_TSF9_19 - Create and register a plugin report type of 'print', after calling the report validate that all the output comes from the report and nothing is added by the shell.

//@ WL11263_TSF9_19 - register the report
shell.registerReport('print_report_which_outputs_some_text', 'print', function (){println('Some random text'); return {'report' : []}})

//@ WL11263_TSF9_19 - Check output
println('before')
\show print_report_which_outputs_some_text
println('after')

// -----------------------------------------------------------------------------
// WL11263_TSF9_20 - Create and register a plugin report type of 'print' that returns an empty list of json objects. After calling the report validate that no error is present.

//@ WL11263_TSF9_20 - register the report
shell.registerReport('print_report_which_outputs_some_other_text', 'print', function (){println('Another random text'); return {'report' : []}})

//@ WL11263_TSF9_20 - Check output
\show print_report_which_outputs_some_other_text

// -----------------------------------------------------------------------------
// WL11263_TSF9_21 - Create and register a plugin report type of 'print' that returns a list of json objects. Analyze the behaviour after calling the report.

//@ WL11263_TSF9_21 - register the report
shell.registerReport('print_report_which_returns_non_empty_list', 'print', function (){println('Very interesting report'); return {'report' : [1, 2, 3]}})

//@ WL11263_TSF9_21 - Check output - no error
\show print_report_which_returns_non_empty_list

// -----------------------------------------------------------------------------
// WL11263_TSF9_22 - Create and register a plugin report type of 'list' that returns a list of dictionaries. Analyze the behaviour after calling the report.

//@ WL11263_TSF9_22 - register the report
shell.registerReport('list_report_which_returns_list_of_dictionaries', 'list', function (){return {'report' : [{}]}})

//@ WL11263_TSF9_22 - Check output - error
\show list_report_which_returns_list_of_dictionaries

// -----------------------------------------------------------------------------
// WL11263_TSF9_23 - Create and register a plugin report type of 'list' that returns an empty list. Expect an exception after calling the report.

//@ WL11263_TSF9_23 - register the report
shell.registerReport('list_report_which_returns_an_empty_list', 'list', function (){return {'report' : []}})

//@ WL11263_TSF9_23 - Check output - error
\show list_report_which_returns_an_empty_list

// -----------------------------------------------------------------------------
// WL11263_TSF9_24 - Create and register a plugin report type of 'list' that returns nothing. Expect an exception after calling the report.

//@ WL11263_TSF9_24 - register the report
shell.registerReport('list_report_which_returns_nothing', 'list', function (){return {'report' : undefined}})

//@ WL11263_TSF9_24 - Check output - error
\show list_report_which_returns_nothing

// -----------------------------------------------------------------------------
// WL11263_TSF9_25 - Create and register a plugin report type of 'report' that returns nothing. Expect an exception after calling the report.

//@ WL11263_TSF9_25 - register the report
shell.registerReport('report_type_report_which_returns_nothing', 'report', function (){return {'report' : undefined}})

//@ WL11263_TSF9_25 - Check output - error
\show report_type_report_which_returns_nothing

// -----------------------------------------------------------------------------
// WL11263_TSF9_26 - Create and register a plugin report type of 'report' that returns a list with more than one element. Expect an exception after calling the report.

//@ WL11263_TSF9_26 - register the report
shell.registerReport('report_type_report_which_returns_list_with_more_than_one_element', 'report', function (){return {'report' : [[], []]}})

//@ WL11263_TSF9_26 - Check output - error
\show report_type_report_which_returns_list_with_more_than_one_element

// -----------------------------------------------------------------------------
// WL11263_TSF9_27 - Create and register a plugin report type of 'report' that returns an empty list. Expect an exception after calling the report.

//@ WL11263_TSF9_27 - register the report
shell.registerReport('report_type_report_which_returns_an_empty_list', 'report', function (){return {'report' : []}})

//@ WL11263_TSF9_27 - Check output - error
\show report_type_report_which_returns_an_empty_list

// -----------------------------------------------------------------------------
// WL11263_TSF9_28 - Create and register a report with all lower-case name. Register another report with the same name, but converted to upper-case, expect an exception.

//@ WL11263_TSF9_28 - register the lower-case report
shell.registerReport('sample_report_with_lower_case_name', 'print', function (){})

//@ WL11263_TSF9_28 - try to register report with the same name but converted to upper-case
shell.registerReport('SAMPLE_REPORT_WITH_LOWER_CASE_NAME', 'print', function (){})

//@ WL11263_TSF9_28 - register the upper-case report
shell.registerReport('SAMPLE_REPORT_WITH_UPPER_CASE_NAME', 'print', function (){})

//@ WL11263_TSF9_28 - try to register report with the same name but converted to lower-case
shell.registerReport('sample_report_with_upper_case_name', 'print', function (){})

//@ WL11263_TSF9_28 - it's not possible to register a report name containing hyphen
shell.registerReport('sample_report_with-hyphen', 'print', function (){})

// -----------------------------------------------------------------------------
// Test invoking the reports with various options

var options = [
  {
    'name' : 'one',
    'brief' : 'Option with default type.',
    'shortcut' : '1'
  },
  {
    'name' : 'two',
    'brief' : 'Option with "string" type.',
    'shortcut' : '2',
    'type' : 'string'
  },
  {
    'name' : 'three',
    'brief' : 'Option with "bool" type.',
    'shortcut' : '3',
    'type' : 'bool'
  },
  {
    'name' : 'four',
    'brief' : 'Option with "integer" type.',
    'shortcut' : '4',
    'type' : 'integer'
  },
  {
    'name' : 'five',
    'brief' : 'Option with "float" type.',
    'shortcut' : '5',
    'type' : 'float'
  }
];

var report = function (s, a, o) {
  var ret = [['name', 'type', 'value']];

  if (a && a.length > 0) {
    ret.push(['argv', type(a), a]);
  }

  for (var key in o) {
    if (!o.hasOwnProperty(key)) continue;

    ret.push([key, type(o[key]), o[key]]);
  }

  return {'report' : ret};
};

//@ register the report - list_report_testing_various_options
shell.registerReport('list_report_testing_various_options', 'list', report, {'brief' : 'testing options', 'options' : options})

//@ call list_report_testing_various_options with an unknown option
\show list_report_testing_various_options --unknown

//@ call list_report_testing_various_options with an unknown option and value [USE: call list_report_testing_various_options with an unknown option]
\show list_report_testing_various_options --unknown value

//@ call list_report_testing_various_options with an unknown short option
\show list_report_testing_various_options -u

//@ call list_report_testing_various_options with an unknown short option and value [USE: call list_report_testing_various_options with an unknown short option]
\show list_report_testing_various_options -u value

//@ call list_report_testing_various_options using the first option without value
\show list_report_testing_various_options --one

//@ call list_report_testing_various_options using the first option without value - short
\show list_report_testing_various_options -1

//@ call list_report_testing_various_options using the first option
\show list_report_testing_various_options --one default

//@ call list_report_testing_various_options using the first option = [USE: call list_report_testing_various_options using the first option]
\show list_report_testing_various_options --one=default

//@ call list_report_testing_various_options using the first short option [USE: call list_report_testing_various_options using the first option]
\show list_report_testing_various_options -1 default

//@ call list_report_testing_various_options using the first short option = [USE: call list_report_testing_various_options using the first option]
\show list_report_testing_various_options -1default


//@ call list_report_testing_various_options using the second option without value
\show list_report_testing_various_options --two

//@ call list_report_testing_various_options using the second option without value - short
\show list_report_testing_various_options -2

//@ call list_report_testing_various_options using the second option
\show list_report_testing_various_options --two text

//@ call list_report_testing_various_options using the second option = [USE: call list_report_testing_various_options using the second option]
\show list_report_testing_various_options --two=text

//@ call list_report_testing_various_options using the second short option [USE: call list_report_testing_various_options using the second option]
\show list_report_testing_various_options -2 text

//@ call list_report_testing_various_options using the second short option = [USE: call list_report_testing_various_options using the second option]
\show list_report_testing_various_options -2text

//@ call list_report_testing_various_options using the second option - int is converted to string
\show list_report_testing_various_options --two 2

//@ call list_report_testing_various_options using the second option - bool is converted to string
\show list_report_testing_various_options --two false

//@ call list_report_testing_various_options using the second option - float is converted to string
\show list_report_testing_various_options --two 3.14


//@ call list_report_testing_various_options not using the third option (it's not added to options dictionary when calling the function)
\show list_report_testing_various_options

//@ call list_report_testing_various_options using the third option
\show list_report_testing_various_options --three

//@ call list_report_testing_various_options using the third option - short [USE: call list_report_testing_various_options using the third option]
\show list_report_testing_various_options -3

//@ call list_report_testing_various_options using the third option and providing a value - it's treated as an argument
\show list_report_testing_various_options --three arg

//@ call list_report_testing_various_options using the third option and providing a value - short - it's treated as an argument [USE: call list_report_testing_various_options using the third option and providing a value - it's treated as an argument]
\show list_report_testing_various_options -3 arg

//@ call list_report_testing_various_options using the third option and providing a value - error
\show list_report_testing_various_options --three=arg

//@ call list_report_testing_various_options using the third option and providing a value - short - error
\show list_report_testing_various_options -3arg


//@ call list_report_testing_various_options using the fourth option without value
\show list_report_testing_various_options --four

//@ call list_report_testing_various_options using the fourth option without value - short
\show list_report_testing_various_options -4

//@ call list_report_testing_various_options using the fourth option
\show list_report_testing_various_options --four 123

//@ call list_report_testing_various_options using the fourth option = [USE: call list_report_testing_various_options using the fourth option]
\show list_report_testing_various_options --four=123

//@ call list_report_testing_various_options using the fourth short option [USE: call list_report_testing_various_options using the fourth option]
\show list_report_testing_various_options -4 123

//@ call list_report_testing_various_options using the fourth short option = [USE: call list_report_testing_various_options using the fourth option]
\show list_report_testing_various_options -4123

//@ call list_report_testing_various_options using the fourth option - string is not converted to int
\show list_report_testing_various_options --four text

//@ call list_report_testing_various_options using the fourth option - bool is not converted to int
\show list_report_testing_various_options --four false

//@ call list_report_testing_various_options using the fourth option - float is not converted to int
\show list_report_testing_various_options --four 3.14


//@ call list_report_testing_various_options using the fifth option without value
\show list_report_testing_various_options --five

//@ call list_report_testing_various_options using the fifth option without value - short
\show list_report_testing_various_options -5

//@ call list_report_testing_various_options using the fifth option
\show list_report_testing_various_options --five 123.456

//@ call list_report_testing_various_options using the fifth option = [USE: call list_report_testing_various_options using the fifth option]
\show list_report_testing_various_options --five=123.456

//@ call list_report_testing_various_options using the fifth short option [USE: call list_report_testing_various_options using the fifth option]
\show list_report_testing_various_options -5 123.456

//@ call list_report_testing_various_options using the fifth short option = [USE: call list_report_testing_various_options using the fifth option]
\show list_report_testing_various_options -5123.456

//@ call list_report_testing_various_options using the fifth option - string is not converted to float
\show list_report_testing_various_options --five text

//@ call list_report_testing_various_options using the fifth option - bool is not converted to float
\show list_report_testing_various_options --five false

//@ call list_report_testing_various_options using the fifth option - int is converted to float
\show list_report_testing_various_options --five 3


//@ call list_report_testing_various_options using --vertical option, should not be added to options
\show list_report_testing_various_options --vertical --one text

//@ call list_report_testing_various_options using --vertical option, should not be added to options - order does not matter [USE: call list_report_testing_various_options using --vertical option, should not be added to options]
\show list_report_testing_various_options --one text --vertical


//@ call list_report_testing_various_options using all options
\show list_report_testing_various_options --one text --two query --three --four 987 --five 654.321

//@ call list_report_testing_various_options using all options - order does not matter [USE: call list_report_testing_various_options using all options]
\show list_report_testing_various_options --five 654.321 --four 987 --three --two query --one text

// -----------------------------------------------------------------------------
// Test invoking the reports with various number of arguments

//@ register the report - list_report_testing_argc_default
shell.registerReport('list_report_testing_argc_default', 'list', report)

//@ call list_report_testing_argc_default with no arguments
\show list_report_testing_argc_default

//@ call list_report_testing_argc_default with an argument
\show list_report_testing_argc_default arg0


//@ register the report - list_report_testing_argc_0
shell.registerReport('list_report_testing_argc_0', 'list', report, {'argc' : '0'})

//@ call list_report_testing_argc_0 with no arguments
\show list_report_testing_argc_0

//@ call list_report_testing_argc_0 with an argument
\show list_report_testing_argc_0 arg0


//@ register the report - list_report_testing_argc_1
shell.registerReport('list_report_testing_argc_1', 'list', report, {'argc' : '1'})

//@ call list_report_testing_argc_1 with no arguments
\show list_report_testing_argc_1

//@ call list_report_testing_argc_1 with an argument
\show list_report_testing_argc_1 arg0

//@ call list_report_testing_argc_1 with two arguments
\show list_report_testing_argc_1 arg0 arg1


//@ register the report - list_report_testing_argc_asterisk
shell.registerReport('list_report_testing_argc_asterisk', 'list', report, {'argc' : '*'})

//@ call list_report_testing_argc_asterisk with no arguments
\show list_report_testing_argc_asterisk

//@ call list_report_testing_argc_asterisk with an argument
\show list_report_testing_argc_asterisk arg0

//@ call list_report_testing_argc_asterisk with two arguments
\show list_report_testing_argc_asterisk arg0 arg1


//@ register the report - list_report_testing_argc_1_2
shell.registerReport('list_report_testing_argc_1_2', 'list', report, {'argc' : '1-2'})

//@ call list_report_testing_argc_1_2 with no arguments
\show list_report_testing_argc_1_2

//@ call list_report_testing_argc_1_2 with an argument
\show list_report_testing_argc_1_2 arg0

//@ call list_report_testing_argc_1_2 with two arguments
\show list_report_testing_argc_1_2 arg0 arg1

//@ call list_report_testing_argc_1_2 with three arguments
\show list_report_testing_argc_1_2 arg0 arg1 arg2


//@ register the report - list_report_testing_argc_1_asterisk
shell.registerReport('list_report_testing_argc_1_asterisk', 'list', report, {'argc' : '1-*'})

//@ call list_report_testing_argc_1_asterisk with no arguments
\show list_report_testing_argc_1_asterisk

//@ call list_report_testing_argc_1_asterisk with an argument
\show list_report_testing_argc_1_asterisk arg0

//@ call list_report_testing_argc_1_asterisk with two arguments
\show list_report_testing_argc_1_asterisk arg0 arg1

//@ call list_report_testing_argc_1_asterisk with three arguments
\show list_report_testing_argc_1_asterisk arg0 arg1 arg2

// -----------------------------------------------------------------------------
// WL11263_TSF10_1 - Validate that the information of any new registered plugin report is displayed by the \help command, example: \help report_name

// make an option accept certain values
options[0]['values'] = ['a', 'b', 'c'];

// make at option required
options[1]['required'] = true;

// add a description
options[2]['details'] = ['Details of parameter three.'];

//@ WL11263_TSF10_1 - register the report - ths_list_no_options_no_arguments
shell.registerReport('ths_list_no_options_no_arguments', 'list', function(){}, {'brief' : 'testing help'})

//@ WL11263_TSF10_1 - show help - ths_list_no_options_no_arguments
\help ths_list_no_options_no_arguments


//@ WL11263_TSF10_1 - register the report - ths_list_options_no_arguments
shell.registerReport('ths_list_options_no_arguments', 'list', function(){}, {'brief' : 'testing help', 'options' : options})

//@ WL11263_TSF10_1 - show help - ths_list_options_no_arguments
\help ths_list_options_no_arguments


//@ WL11263_TSF10_1 - register the report - ths_list_no_options_one_argument
shell.registerReport('ths_list_no_options_one_argument', 'list', function(){}, {'brief' : 'testing help', 'argc' : '1'})

//@ WL11263_TSF10_1 - show help - ths_list_no_options_one_argument
\help ths_list_no_options_one_argument

//@ WL11263_TSF10_1 - register the report - ths_list_no_options_unbound_arguments
shell.registerReport('ths_list_no_options_unbound_arguments', 'list', function(){}, {'brief' : 'testing help', 'argc' : '*'})

//@ WL11263_TSF10_1 - show help - ths_list_no_options_unbound_arguments
\help ths_list_no_options_unbound_arguments

//@ WL11263_TSF10_1 - register the report - ths_list_no_options_range_of_arguments
shell.registerReport('ths_list_no_options_range_of_arguments', 'list', function(){}, {'brief' : 'testing help', 'argc' : '1-2'})

//@ WL11263_TSF10_1 - show help - ths_list_no_options_range_of_arguments
\help ths_list_no_options_range_of_arguments

//@ WL11263_TSF10_1 - register the report - ths_list_no_options_unbound_range_of_arguments
shell.registerReport('ths_list_no_options_unbound_range_of_arguments', 'list', function(){}, {'brief' : 'testing help', 'argc' : '1-*'})

//@ WL11263_TSF10_1 - show help - ths_list_no_options_unbound_range_of_arguments
\help ths_list_no_options_unbound_range_of_arguments


//@ WL11263_TSF10_1 - register the report - ths_list_options_one_argument
shell.registerReport('ths_list_options_one_argument', 'list', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '1'})

//@ WL11263_TSF10_1 - show help - ths_list_options_one_argument
\help ths_list_options_one_argument

//@ WL11263_TSF10_1 - register the report - ths_list_options_unbound_arguments
shell.registerReport('ths_list_options_unbound_arguments', 'list', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '*'})

//@ WL11263_TSF10_1 - show help - ths_list_options_unbound_arguments
\help ths_list_options_unbound_arguments

//@ WL11263_TSF10_1 - register the report - ths_list_options_range_of_arguments
shell.registerReport('ths_list_options_range_of_arguments', 'list', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '1-2'})

//@ WL11263_TSF10_1 - show help - ths_list_options_range_of_arguments
\help ths_list_options_range_of_arguments

//@ WL11263_TSF10_1 - register the report - ths_list_options_range_of_arguments_details
shell.registerReport('ths_list_options_range_of_arguments_details', 'list', function(){}, {'brief' : 'testing help', 'details' : ['More details'], 'options' : options, 'argc' : '1-2'})

//@ WL11263_TSF10_1 - show help - ths_list_options_range_of_arguments_details
\help ths_list_options_range_of_arguments_details

//@ WL11263_TSF10_1 - register the report - ths_list_options_unbound_range_of_arguments
shell.registerReport('ths_list_options_unbound_range_of_arguments', 'list', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '1-*'})

//@ WL11263_TSF10_1 - show help - ths_list_options_unbound_range_of_arguments
\help ths_list_options_unbound_range_of_arguments


//@ WL11263_TSF10_1 - register the report - ths_report_no_options_no_arguments
shell.registerReport('ths_report_no_options_no_arguments', 'report', function(){}, {'brief' : 'testing help'})

//@ WL11263_TSF10_1 - show help - ths_report_no_options_no_arguments
\help ths_report_no_options_no_arguments


//@ WL11263_TSF10_1 - register the report - ths_report_options_no_arguments
shell.registerReport('ths_report_options_no_arguments', 'report', function(){}, {'brief' : 'testing help', 'options' : options})

//@ WL11263_TSF10_1 - show help - ths_report_options_no_arguments
\help ths_report_options_no_arguments


//@ WL11263_TSF10_1 - register the report - ths_report_no_options_one_argument
shell.registerReport('ths_report_no_options_one_argument', 'report', function(){}, {'brief' : 'testing help', 'argc' : '1'})

//@ WL11263_TSF10_1 - show help - ths_report_no_options_one_argument
\help ths_report_no_options_one_argument

//@ WL11263_TSF10_1 - register the report - ths_report_no_options_unbound_arguments
shell.registerReport('ths_report_no_options_unbound_arguments', 'report', function(){}, {'brief' : 'testing help', 'argc' : '*'})

//@ WL11263_TSF10_1 - show help - ths_report_no_options_unbound_arguments
\help ths_report_no_options_unbound_arguments

//@ WL11263_TSF10_1 - register the report - ths_report_no_options_range_of_arguments
shell.registerReport('ths_report_no_options_range_of_arguments', 'report', function(){}, {'brief' : 'testing help', 'argc' : '1-2'})

//@ WL11263_TSF10_1 - show help - ths_report_no_options_range_of_arguments
\help ths_report_no_options_range_of_arguments

//@ WL11263_TSF10_1 - register the report - ths_report_no_options_unbound_range_of_arguments
shell.registerReport('ths_report_no_options_unbound_range_of_arguments', 'report', function(){}, {'brief' : 'testing help', 'argc' : '1-*'})

//@ WL11263_TSF10_1 - show help - ths_report_no_options_unbound_range_of_arguments
\help ths_report_no_options_unbound_range_of_arguments


//@ WL11263_TSF10_1 - register the report - ths_report_options_one_argument
shell.registerReport('ths_report_options_one_argument', 'report', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '1'})

//@ WL11263_TSF10_1 - show help - ths_report_options_one_argument
\help ths_report_options_one_argument

//@ WL11263_TSF10_1 - register the report - ths_report_options_unbound_arguments
shell.registerReport('ths_report_options_unbound_arguments', 'report', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '*'})

//@ WL11263_TSF10_1 - show help - ths_report_options_unbound_arguments
\help ths_report_options_unbound_arguments

//@ WL11263_TSF10_1 - register the report - ths_report_options_range_of_arguments
shell.registerReport('ths_report_options_range_of_arguments', 'report', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '1-2'})

//@ WL11263_TSF10_1 - show help - ths_report_options_range_of_arguments
\help ths_report_options_range_of_arguments

//@ WL11263_TSF10_1 - register the report - ths_report_options_range_of_arguments_details
shell.registerReport('ths_report_options_range_of_arguments_details', 'report', function(){}, {'brief' : 'testing help', 'details' : ['More details'], 'options' : options, 'argc' : '1-2'})

//@ WL11263_TSF10_1 - show help - ths_report_options_range_of_arguments_details
\help ths_report_options_range_of_arguments_details

//@ WL11263_TSF10_1 - register the report - ths_report_options_unbound_range_of_arguments
shell.registerReport('ths_report_options_unbound_range_of_arguments', 'report', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '1-*'})

//@ WL11263_TSF10_1 - show help - ths_report_options_unbound_range_of_arguments
\help ths_report_options_unbound_range_of_arguments


//@ WL11263_TSF10_1 - register the report - ths_print_no_options_no_arguments
shell.registerReport('ths_print_no_options_no_arguments', 'print', function(){}, {'brief' : 'testing help'})

//@ WL11263_TSF10_1 - show help - ths_print_no_options_no_arguments
\help ths_print_no_options_no_arguments


//@ WL11263_TSF10_1 - register the report - ths_print_options_no_arguments
shell.registerReport('ths_print_options_no_arguments', 'print', function(){}, {'brief' : 'testing help', 'options' : options})

//@ WL11263_TSF10_1 - show help - ths_print_options_no_arguments
\help ths_print_options_no_arguments


//@ WL11263_TSF10_1 - register the report - ths_print_no_options_one_argument
shell.registerReport('ths_print_no_options_one_argument', 'print', function(){}, {'brief' : 'testing help', 'argc' : '1'})

//@ WL11263_TSF10_1 - show help - ths_print_no_options_one_argument
\help ths_print_no_options_one_argument

//@ WL11263_TSF10_1 - register the report - ths_print_no_options_unbound_arguments
shell.registerReport('ths_print_no_options_unbound_arguments', 'print', function(){}, {'brief' : 'testing help', 'argc' : '*'})

//@ WL11263_TSF10_1 - show help - ths_print_no_options_unbound_arguments
\help ths_print_no_options_unbound_arguments

//@ WL11263_TSF10_1 - register the report - ths_print_no_options_range_of_arguments
shell.registerReport('ths_print_no_options_range_of_arguments', 'print', function(){}, {'brief' : 'testing help', 'argc' : '1-2'})

//@ WL11263_TSF10_1 - show help - ths_print_no_options_range_of_arguments
\help ths_print_no_options_range_of_arguments

//@ WL11263_TSF10_1 - register the report - ths_print_no_options_unbound_range_of_arguments
shell.registerReport('ths_print_no_options_unbound_range_of_arguments', 'print', function(){}, {'brief' : 'testing help', 'argc' : '1-*'})

//@ WL11263_TSF10_1 - show help - ths_print_no_options_unbound_range_of_arguments
\help ths_print_no_options_unbound_range_of_arguments


//@ WL11263_TSF10_1 - register the report - ths_print_options_one_argument
shell.registerReport('ths_print_options_one_argument', 'print', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '1'})

//@ WL11263_TSF10_1 - show help - ths_print_options_one_argument
\help ths_print_options_one_argument

//@ WL11263_TSF10_1 - register the report - ths_print_options_unbound_arguments
shell.registerReport('ths_print_options_unbound_arguments', 'print', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '*'})

//@ WL11263_TSF10_1 - show help - ths_print_options_unbound_arguments
\help ths_print_options_unbound_arguments

//@ WL11263_TSF10_1 - register the report - ths_print_options_range_of_arguments
shell.registerReport('ths_print_options_range_of_arguments', 'print', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '1-2'})

//@ WL11263_TSF10_1 - show help - ths_print_options_range_of_arguments
\help ths_print_options_range_of_arguments

//@ WL11263_TSF10_1 - register the report - ths_print_options_range_of_arguments_details
shell.registerReport('ths_print_options_range_of_arguments_details', 'print', function(){}, {'brief' : 'testing help', 'details' : ['More details'], 'options' : options, 'argc' : '1-2'})

//@ WL11263_TSF10_1 - show help - ths_print_options_range_of_arguments_details
\help ths_print_options_range_of_arguments_details

//@ WL11263_TSF10_1 - register the report - ths_print_options_unbound_range_of_arguments
shell.registerReport('ths_print_options_unbound_range_of_arguments', 'print', function(){}, {'brief' : 'testing help', 'options' : options, 'argc' : '1-*'})

//@ WL11263_TSF10_1 - show help - ths_print_options_unbound_range_of_arguments
\help ths_print_options_unbound_range_of_arguments

// -----------------------------------------------------------------------------
// WL11263_TSF18_1 - Validate that the commands \show and \watch display the information about an specific report when the --help is given, example \show myreport --help.

//@ WL11263_TSF18_1 - call \show --help - ths_list_no_options_no_arguments
\show ths_list_no_options_no_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_list_options_no_arguments
\show ths_list_options_no_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_list_no_options_one_argument
\show ths_list_no_options_one_argument --help

//@ WL11263_TSF18_1 - call \show --help - ths_list_no_options_unbound_arguments
\show ths_list_no_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_list_no_options_range_of_arguments
\show ths_list_no_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_list_no_options_unbound_range_of_arguments
\show ths_list_no_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_list_options_one_argument
\show ths_list_options_one_argument --help

//@ WL11263_TSF18_1 - call \show --help - ths_list_options_unbound_arguments
\show ths_list_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_list_options_range_of_arguments
\show ths_list_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_list_options_range_of_arguments_details
\show ths_list_options_range_of_arguments_details --help

//@ WL11263_TSF18_1 - call \show --help - ths_list_options_unbound_range_of_arguments
\show ths_list_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_report_no_options_no_arguments
\show ths_report_no_options_no_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_report_options_no_arguments
\show ths_report_options_no_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_report_no_options_one_argument
\show ths_report_no_options_one_argument --help

//@ WL11263_TSF18_1 - call \show --help - ths_report_no_options_unbound_arguments
\show ths_report_no_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_report_no_options_range_of_arguments
\show ths_report_no_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_report_no_options_unbound_range_of_arguments
\show ths_report_no_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_report_options_one_argument
\show ths_report_options_one_argument --help

//@ WL11263_TSF18_1 - call \show --help - ths_report_options_unbound_arguments
\show ths_report_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_report_options_range_of_arguments
\show ths_report_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_report_options_range_of_arguments_details
\show ths_report_options_range_of_arguments_details --help

//@ WL11263_TSF18_1 - call \show --help - ths_report_options_unbound_range_of_arguments
\show ths_report_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_print_no_options_no_arguments
\show ths_print_no_options_no_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_print_options_no_arguments
\show ths_print_options_no_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_print_no_options_one_argument
\show ths_print_no_options_one_argument --help

//@ WL11263_TSF18_1 - call \show --help - ths_print_no_options_unbound_arguments
\show ths_print_no_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_print_no_options_range_of_arguments
\show ths_print_no_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_print_no_options_unbound_range_of_arguments
\show ths_print_no_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \show --help - ths_print_options_one_argument
\show ths_print_options_one_argument --help

//@ WL11263_TSF18_1 - call \show --help - ths_print_options_unbound_arguments
\show ths_print_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_print_options_range_of_arguments
\show ths_print_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \show --help - ths_print_options_range_of_arguments_details
\show ths_print_options_range_of_arguments_details --help

//@ WL11263_TSF18_1 - call \show --help - ths_print_options_unbound_range_of_arguments
\show ths_print_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_list_no_options_no_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_list_no_options_no_arguments]
\watch ths_list_no_options_no_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_list_options_no_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_list_options_no_arguments]
\watch ths_list_options_no_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_list_no_options_one_argument [USE: WL11263_TSF18_1 - call \show --help - ths_list_no_options_one_argument]
\watch ths_list_no_options_one_argument --help

//@ WL11263_TSF18_1 - call \watch --help - ths_list_no_options_unbound_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_list_no_options_unbound_arguments]
\watch ths_list_no_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_list_no_options_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_list_no_options_range_of_arguments]
\watch ths_list_no_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_list_no_options_unbound_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_list_no_options_unbound_range_of_arguments]
\watch ths_list_no_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_list_options_one_argument [USE: WL11263_TSF18_1 - call \show --help - ths_list_options_one_argument]
\watch ths_list_options_one_argument --help

//@ WL11263_TSF18_1 - call \watch --help - ths_list_options_unbound_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_list_options_unbound_arguments]
\watch ths_list_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_list_options_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_list_options_range_of_arguments]
\watch ths_list_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_list_options_range_of_arguments_details [USE: WL11263_TSF18_1 - call \show --help - ths_list_options_range_of_arguments_details]
\watch ths_list_options_range_of_arguments_details --help

//@ WL11263_TSF18_1 - call \watch --help - ths_list_options_unbound_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_list_options_unbound_range_of_arguments]
\watch ths_list_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_report_no_options_no_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_report_no_options_no_arguments]
\watch ths_report_no_options_no_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_report_options_no_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_report_options_no_arguments]
\watch ths_report_options_no_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_report_no_options_one_argument [USE: WL11263_TSF18_1 - call \show --help - ths_report_no_options_one_argument]
\watch ths_report_no_options_one_argument --help

//@ WL11263_TSF18_1 - call \watch --help - ths_report_no_options_unbound_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_report_no_options_unbound_arguments]
\watch ths_report_no_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_report_no_options_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_report_no_options_range_of_arguments]
\watch ths_report_no_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_report_no_options_unbound_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_report_no_options_unbound_range_of_arguments]
\watch ths_report_no_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_report_options_one_argument [USE: WL11263_TSF18_1 - call \show --help - ths_report_options_one_argument]
\watch ths_report_options_one_argument --help

//@ WL11263_TSF18_1 - call \watch --help - ths_report_options_unbound_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_report_options_unbound_arguments]
\watch ths_report_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_report_options_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_report_options_range_of_arguments]
\watch ths_report_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_report_options_range_of_arguments_details [USE: WL11263_TSF18_1 - call \show --help - ths_report_options_range_of_arguments_details]
\watch ths_report_options_range_of_arguments_details --help

//@ WL11263_TSF18_1 - call \watch --help - ths_report_options_unbound_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_report_options_unbound_range_of_arguments]
\watch ths_report_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_print_no_options_no_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_print_no_options_no_arguments]
\watch ths_print_no_options_no_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_print_options_no_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_print_options_no_arguments]
\watch ths_print_options_no_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_print_no_options_one_argument [USE: WL11263_TSF18_1 - call \show --help - ths_print_no_options_one_argument]
\watch ths_print_no_options_one_argument --help

//@ WL11263_TSF18_1 - call \watch --help - ths_print_no_options_unbound_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_print_no_options_unbound_arguments]
\watch ths_print_no_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_print_no_options_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_print_no_options_range_of_arguments]
\watch ths_print_no_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_print_no_options_unbound_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_print_no_options_unbound_range_of_arguments]
\watch ths_print_no_options_unbound_range_of_arguments --help


//@ WL11263_TSF18_1 - call \watch --help - ths_print_options_one_argument [USE: WL11263_TSF18_1 - call \show --help - ths_print_options_one_argument]
\watch ths_print_options_one_argument --help

//@ WL11263_TSF18_1 - call \watch --help - ths_print_options_unbound_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_print_options_unbound_arguments]
\watch ths_print_options_unbound_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_print_options_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_print_options_range_of_arguments]
\watch ths_print_options_range_of_arguments --help

//@ WL11263_TSF18_1 - call \watch --help - ths_print_options_range_of_arguments_details [USE: WL11263_TSF18_1 - call \show --help - ths_print_options_range_of_arguments_details]
\watch ths_print_options_range_of_arguments_details --help

//@ WL11263_TSF18_1 - call \watch --help - ths_print_options_unbound_range_of_arguments [USE: WL11263_TSF18_1 - call \show --help - ths_print_options_unbound_range_of_arguments]
\watch ths_print_options_unbound_range_of_arguments --help

// -----------------------------------------------------------------------------
// WL11263_TSF13_1 - Validate that any plugin report registered can be used through shell.reports.* as functions.

// reuse list_report_testing_various_options report

//@ WL11263_TSF13_1 - call shell.reports.list_report_testing_various_options() in JS mode
shell.reports.list_report_testing_various_options(session, null, {'one' : 'text'})

//@ WL11263_TSF13_1 - switch to Python
\py

//@ WL11263_TSF13_1 - call shell.reports.list_report_testing_various_options() in PY mode [USE: WL11263_TSF13_1 - call shell.reports.list_report_testing_various_options() in JS mode]
shell.reports.list_report_testing_various_options(session, [], {'one' : 'text'})

//@ WL11263_TSF13_1 - switch to back to JS
\js

// -----------------------------------------------------------------------------
// call method, test validation of options

//@ call shell.reports.list_report_testing_various_options without options
shell.reports.list_report_testing_various_options()


//@ call shell.reports.list_report_testing_various_options with session - undefined
shell.reports.list_report_testing_various_options(undefined, null, {})

//@ call shell.reports.list_report_testing_various_options with session - null
shell.reports.list_report_testing_various_options(null, null, {})

//@ call shell.reports.list_report_testing_various_options with session - string
shell.reports.list_report_testing_various_options('session', null, {})


//@ call shell.reports.list_report_testing_various_options with options - undefined
shell.reports.list_report_testing_various_options(session, null, undefined)

//@ call shell.reports.list_report_testing_various_options with options - null
shell.reports.list_report_testing_various_options(session, null, null)

//@ call shell.reports.list_report_testing_various_options with options - string
shell.reports.list_report_testing_various_options(session, null, 'options')


//@ call shell.reports.list_report_testing_various_options with an unknown option
shell.reports.list_report_testing_various_options(session, null, {'unknown' : undefined})

//@ call shell.reports.list_report_testing_various_options with an unknown option and value [USE: call shell.reports.list_report_testing_various_options with an unknown option]
shell.reports.list_report_testing_various_options(session, null, {'unknown' : 'value'})


//@ call shell.reports.list_report_testing_various_options using the first option without value
shell.reports.list_report_testing_various_options(session, null, {'one' : undefined})

//@ call shell.reports.list_report_testing_various_options using the first option with int
shell.reports.list_report_testing_various_options(session, null, {'one' : 1})

//@ call shell.reports.list_report_testing_various_options using the first option with bool [USE: call shell.reports.list_report_testing_various_options using the first option with int]
shell.reports.list_report_testing_various_options(session, null, {'one' : true})

//@ call shell.reports.list_report_testing_various_options using the first option with float [USE: call shell.reports.list_report_testing_various_options using the first option with int]
shell.reports.list_report_testing_various_options(session, null, {'one' : 3.14})

//@ call shell.reports.list_report_testing_various_options using the first option with string
shell.reports.list_report_testing_various_options(session, null, {'one' : 'default'})


//@ call shell.reports.list_report_testing_various_options using the second option without value
shell.reports.list_report_testing_various_options(session, null, {'two' : undefined})

//@ call shell.reports.list_report_testing_various_options using the second option with int
shell.reports.list_report_testing_various_options(session, null, {'two' : 2})

//@ call shell.reports.list_report_testing_various_options using the second option with bool [USE: call shell.reports.list_report_testing_various_options using the second option with int]
shell.reports.list_report_testing_various_options(session, null, {'two' : false})

//@ call shell.reports.list_report_testing_various_options using the second option with float [USE: call shell.reports.list_report_testing_various_options using the second option with int]
shell.reports.list_report_testing_various_options(session, null, {'two' : 3.14})

//@ call shell.reports.list_report_testing_various_options using the second option with string
shell.reports.list_report_testing_various_options(session, null, {'two' : 'text'})


//@ call shell.reports.list_report_testing_various_options not using the third option
shell.reports.list_report_testing_various_options(session, null, {})

//@ call shell.reports.list_report_testing_various_options using the third option - undefined [USE: call shell.reports.list_report_testing_various_options not using the third option]
shell.reports.list_report_testing_various_options(session, null, {'three' : undefined})

//@ call shell.reports.list_report_testing_various_options using the third option with int
shell.reports.list_report_testing_various_options(session, null, {'three' : 3})

//@ call shell.reports.list_report_testing_various_options using the third option with float
shell.reports.list_report_testing_various_options(session, null, {'three' : 3.14})

//@ call shell.reports.list_report_testing_various_options using the third option with string
shell.reports.list_report_testing_various_options(session, null, {'three' : 'text'})

//@ call shell.reports.list_report_testing_various_options using the third option with bool
shell.reports.list_report_testing_various_options(session, null, {'three' : false})

//@ call shell.reports.list_report_testing_various_options using the third option with bool - true
shell.reports.list_report_testing_various_options(session, null, {'three' : true})


//@ call shell.reports.list_report_testing_various_options using the fourth option without value
shell.reports.list_report_testing_various_options(session, null, {'four' : undefined})

//@ call shell.reports.list_report_testing_various_options using the fourth option with bool
shell.reports.list_report_testing_various_options(session, null, {'four' : false})

//@ call shell.reports.list_report_testing_various_options using the fourth option with float
shell.reports.list_report_testing_various_options(session, null, {'four' : 3.14})

//@ call shell.reports.list_report_testing_various_options using the fourth option with string
shell.reports.list_report_testing_various_options(session, null, {'four' : 'text'})

//@ call shell.reports.list_report_testing_various_options using the fourth option with int
shell.reports.list_report_testing_various_options(session, null, {'four' : 4})


//@ call shell.reports.list_report_testing_various_options using the fifth option without value
shell.reports.list_report_testing_various_options(session, null, {'five' : undefined})

//@ call shell.reports.list_report_testing_various_options using the fifth option with bool
shell.reports.list_report_testing_various_options(session, null, {'five' : false})

//@ call shell.reports.list_report_testing_various_options using the fifth option with string
shell.reports.list_report_testing_various_options(session, null, {'five' : 'text'})

//@ call shell.reports.list_report_testing_various_options using the fifth option with int
shell.reports.list_report_testing_various_options(session, null, {'five' : 5})

//@ call shell.reports.list_report_testing_various_options using the fifth option with float
shell.reports.list_report_testing_various_options(session, null, {'five' : 3.14})


//@ call shell.reports.list_report_testing_various_options using all options
shell.reports.list_report_testing_various_options(session, null, {'one' : 'text', 'two' : 'query', 'three' : 'true', 'four' : 987, 'five' : 654.321})

// -----------------------------------------------------------------------------
// Test calling the methods with various number of arguments

//@ call shell.reports.list_report_testing_argc_default with no arguments
shell.reports.list_report_testing_argc_default(session, null)

//@ call shell.reports.list_report_testing_argc_default with no arguments - empty array [USE: call shell.reports.list_report_testing_argc_default with no arguments]
shell.reports.list_report_testing_argc_default(session, [])

//@ call shell.reports.list_report_testing_argc_default with an argument [USE: call shell.reports.list_report_testing_argc_default with no arguments]
shell.reports.list_report_testing_argc_default(session, ['arg0'])


//@ call shell.reports.list_report_testing_argc_0 with no arguments
shell.reports.list_report_testing_argc_0(session, null)

//@ call shell.reports.list_report_testing_argc_0 with no arguments - empty array [USE: call shell.reports.list_report_testing_argc_0 with no arguments]
shell.reports.list_report_testing_argc_0(session, [])

//@ call shell.reports.list_report_testing_argc_0 with an argument [USE: call shell.reports.list_report_testing_argc_0 with no arguments]
shell.reports.list_report_testing_argc_0(session, ['arg0'])


//@ call shell.reports.list_report_testing_argc_1 with no arguments
shell.reports.list_report_testing_argc_1(session, null)

//@ call shell.reports.list_report_testing_argc_1 with no arguments - empty array [USE: call shell.reports.list_report_testing_argc_1 with no arguments]
shell.reports.list_report_testing_argc_1(session, [])

//@ call shell.reports.list_report_testing_argc_1 with an argument
shell.reports.list_report_testing_argc_1(session, ['arg0'])

//@ call shell.reports.list_report_testing_argc_1 with two arguments [USE: call shell.reports.list_report_testing_argc_1 with no arguments]
shell.reports.list_report_testing_argc_1(session, ['arg0', 'arg1'])


//@ call shell.reports.list_report_testing_argc_asterisk with no arguments
shell.reports.list_report_testing_argc_asterisk(session, null)

//@ call shell.reports.list_report_testing_argc_asterisk with no arguments - empty array [USE: call shell.reports.list_report_testing_argc_asterisk with no arguments]
shell.reports.list_report_testing_argc_asterisk(session, [])

//@ call shell.reports.list_report_testing_argc_asterisk with an argument
shell.reports.list_report_testing_argc_asterisk(session, ['arg0'])

//@ call shell.reports.list_report_testing_argc_asterisk with two arguments
shell.reports.list_report_testing_argc_asterisk(session, ['arg0', 'arg1'])


//@ call shell.reports.list_report_testing_argc_1_2 with no arguments
shell.reports.list_report_testing_argc_1_2(session, null)

//@ call shell.reports.list_report_testing_argc_1_2 with no arguments - empty array [USE: call shell.reports.list_report_testing_argc_1_2 with no arguments]
shell.reports.list_report_testing_argc_1_2(session, [])

//@ call shell.reports.list_report_testing_argc_1_2 with an argument
shell.reports.list_report_testing_argc_1_2(session, ['arg0'])

//@ call shell.reports.list_report_testing_argc_1_2 with two arguments
shell.reports.list_report_testing_argc_1_2(session, ['arg0', 'arg1'])

//@ call shell.reports.list_report_testing_argc_1_2 with three arguments [USE: call shell.reports.list_report_testing_argc_1_2 with no arguments]
shell.reports.list_report_testing_argc_1_2(session, ['arg0', 'arg1', 'arg2'])


//@ call shell.reports.list_report_testing_argc_1_asterisk with no arguments
shell.reports.list_report_testing_argc_1_asterisk(session, null)

//@ call shell.reports.list_report_testing_argc_1_asterisk with no arguments - empty array [USE: call shell.reports.list_report_testing_argc_1_asterisk with no arguments]
shell.reports.list_report_testing_argc_1_asterisk(session, [])

//@ call shell.reports.list_report_testing_argc_1_asterisk with an argument
shell.reports.list_report_testing_argc_1_asterisk(session, ['arg0'])

//@ call shell.reports.list_report_testing_argc_1_asterisk with two arguments
shell.reports.list_report_testing_argc_1_asterisk(session, ['arg0', 'arg1'])

//@ call shell.reports.list_report_testing_argc_1_asterisk with three arguments
shell.reports.list_report_testing_argc_1_asterisk(session, ['arg0', 'arg1', 'arg2'])

// -----------------------------------------------------------------------------
// WL11263_TSF14_1 - Return a different value from a method used to register a plugin report that the one expected by the shell. Call the report and analyze the behaviour.

//@ WL11263_TSF14_1 - register report - returns undefined
shell.registerReport('invalid_return_type_undefined', 'list', function(){return undefined;})

//@ WL11263_TSF14_1 - call the report - undefined means an error
\show invalid_return_type_undefined

//@ WL11263_TSF14_1 - register report - returns null [USE: WL11263_TSF14_1 - register report - returns undefined]
shell.registerReport('invalid_return_type_null', 'list', function(){return null;})

//@ WL11263_TSF14_1 - call the report - null means an error [USE: WL11263_TSF14_1 - call the report - undefined means an error]
\show invalid_return_type_null

//@ WL11263_TSF14_1 - register report - returns string [USE: WL11263_TSF14_1 - register report - returns undefined]
shell.registerReport('invalid_return_type_string', 'list', function(){return 'text';})

//@ WL11263_TSF14_1 - call the report - string means an error [USE: WL11263_TSF14_1 - call the report - undefined means an error]
\show invalid_return_type_string

//@ WL11263_TSF14_1 - register report - returns empty dictionary [USE: WL11263_TSF14_1 - register report - returns undefined]
shell.registerReport('invalid_return_type_empty_dictionary', 'list', function(){return {};})

//@ WL11263_TSF14_1 - call the report - empty dictionary means an error
\show invalid_return_type_empty_dictionary

//@ WL11263_TSF14_1 - register report - returns dictionary with unknown key [USE: WL11263_TSF14_1 - register report - returns undefined]
shell.registerReport('invalid_return_type_dictionary_unknown_key', 'list', function(){return {'unknown' : 'text'};})

//@ WL11263_TSF14_1 - call the report - unknown key means an error
\show invalid_return_type_dictionary_unknown_key

//@ WL11263_TSF14_1 - register report - returns report which is undefined [USE: WL11263_TSF14_1 - register report - returns undefined]
shell.registerReport('invalid_return_type_undefined_report', 'list', function(){return {'report' : undefined};})

//@ WL11263_TSF14_1 - call the report - undefined report means an error
\show invalid_return_type_undefined_report

//@ WL11263_TSF14_1 - register report - returns report which is null [USE: WL11263_TSF14_1 - register report - returns undefined]
shell.registerReport('invalid_return_type_null_report', 'list', function(){return {'report' : null};})

//@ WL11263_TSF14_1 - call the report - null report means an error
\show invalid_return_type_null_report

//@ WL11263_TSF14_1 - register report - returns report which is string [USE: WL11263_TSF14_1 - register report - returns undefined]
shell.registerReport('invalid_return_type_string_report', 'list', function(){return {'report' : 'text'};})

//@ WL11263_TSF14_1 - call the report - string report means an error
\show invalid_return_type_string_report

// -----------------------------------------------------------------------------
// WL11263_TSF15_1 - Validate that the commands \show and \watch display the reports type of 'list' in a tabular form by default.

//@ WL11263_TSF15_1 - call an existing report, expect tabular output
\show list_report_testing_various_options --one text --two query --three --four 987 --five 654.321

// -----------------------------------------------------------------------------
// WL11263_TSF15_2 - Validate that if a plugin report is type of 'list', if the number of elements in a row are less that the column names they are considered as null values.

//@ WL11263_TSF15_2 - register a report
shell.registerReport('list_report_with_missing_and_extra_values', 'list', function(){return {'report' : [['left', 'right'], ['one', 'two'], ['three'], ['four', 'five', 'six'], ['seven', 'eight']]};})

//@ WL11263_TSF15_2 - call the report
\show list_report_with_missing_and_extra_values

// -----------------------------------------------------------------------------
// WL11263_TSF16_1 - Validate that the commands \show and \watch display the reports type of 'list' in vertical form if executed with '--vertical' or '-E' options.

//@ WL11263_TSF16_1 - call an existing report, expect vertical output
\show list_report_testing_various_options --vertical --one text --two query --three --four 987 --five 654.321

//@ WL11263_TSF16_1 - call an existing report, expect vertical output -E [USE: WL11263_TSF16_1 - call an existing report, expect vertical output]
\show list_report_testing_various_options -E --one text --two query --three --four 987 --five 654.321

// -----------------------------------------------------------------------------
// WL11263_TSF16_2 - Validate that if a plugin report is type of 'list', if the number of elements in a row are more that the column names they are ignored.

//@ WL11263_TSF16_2 - call the existing report
\show list_report_with_missing_and_extra_values --vertical

//@ WL11263_TSF16_2 - call the existing report -E [USE: WL11263_TSF16_2 - call the existing report]
\show list_report_with_missing_and_extra_values -E

// -----------------------------------------------------------------------------
// WL11263_TSF16_3 - Try to use --vertical, -E with non "list" type reports.

//@ WL11263_TSF15_2 - register a 'print' report
shell.registerReport('print_report_to_test_vertical_option', 'print', function(){return {'report' : []};})

//@ WL11263_TSF15_2 - call the 'print' report with --vertical option
\show print_report_to_test_vertical_option --vertical

//@ WL11263_TSF15_2 - call the 'print' report with -E option
\show print_report_to_test_vertical_option -E


//@ WL11263_TSF15_2 - register a 'report' report
shell.registerReport('report_type_report_to_test_vertical_option', 'report', function(){return {'report' : [[1, 2, 3]]};})

//@ WL11263_TSF15_2 - call the 'report' type report with --vertical option
\show report_type_report_to_test_vertical_option --vertical

//@ WL11263_TSF15_2 - call the 'report' type report with -E option
\show report_type_report_to_test_vertical_option -E

// -----------------------------------------------------------------------------
// WL11263_TSF17_1 - Validate that the commands \show and \watch display the reports type of 'report' in YAML format by default.

//@ WL11263_TSF17_1 - register a 'report' type report
shell.registerReport('report_type_report_to_test_YAML_output', 'report', function(){return {'report' : [['text', 2, 3.14, 'multi\nline\ntext', {'1' : 'value', '2' : 4, '3' : 7.777, '4' : [1, 2, 3], '5' : {'key' : 'value'}}]]};})

//@ WL11263_TSF17_1 - call the 'report' type report - test the output
\show report_type_report_to_test_YAML_output

// -----------------------------------------------------------------------------
// test 'query' report

//@ 'query' - create database
\show query CREATE DATABASE report_test

//@ 'query' - show tables, no result
\show query SHOW TABLES IN report_test

//@ 'query' - create sample table
\show query CREATE TABLE report_test.sample (one VARCHAR(20), two INT, three INT UNSIGNED, four FLOAT, five DOUBLE, six DECIMAL, seven DATE, eight DATETIME, nine BIT, ten BLOB, eleven GEOMETRY, twelve JSON, thirteen TIME, fourteen ENUM('a', 'b'), fifteen SET('c', 'd'))

//@ 'query' - show tables, result contains the table
\show query SHOW TABLES IN report_test

//@ 'query' - show description of sample table
\show query DESC report_test.sample

//@ 'query' - insert some values
\show query INSERT INTO report_test.sample VALUES ('z', -1, 2, 3.14, -6.28, 123, '2018-11-23', '2018-11-23 08:06:34', 0, 'BLOB', NULL, '{\"a\" : 3}', '08:06:34', 'b', 'c,d')

//@ 'query' - query for values
\show query SELECT * FROM report_test.sample

//@ 'query' - delete the database
\show query DROP DATABASE report_test

//@ 'query' - --help
\show query --help

//@ 'query' - help system
\help shell.reports.query

//@ 'query' - call method with null arguments
shell.reports.query(session, null)

//@ 'query' - call method with empty arguments [USE: 'query' - call method with null arguments]
shell.reports.query(session, [])

//@ 'query' - call method with null options
shell.reports.query(session, ['argv0'], null)

//@ 'query' - call method with empty options [USE: 'query' - call method with null options]
shell.reports.query(session, ['argv0'], {})

// -----------------------------------------------------------------------------
//@ cleanup - disconnect the session [USE: close the session immediately]
session.close();
