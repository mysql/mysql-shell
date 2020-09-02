// WL11263_TSF12_1 - Validate that an exception is thrown using \show command without an active session.

//@ WL11263_TSF12_1 - run \show without a session
||Executing the report requires an existing, open session.

// WL11263_TSF12_2 - Validate that an exception is thrown using \watch command without an active session.

//@<OUT> BUG#30083371 - help without session
NAME
      query - Executes the SQL statement given as arguments.

SYNTAX
      \show query [OPTIONS] [ARGS]
      \watch query [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      Arguments:

      This report accepts 1-* arguments.

//@ create a session
|<Session:<<<__uri>>>>|

//@ close the session immediately
||

//@ create a classic session
|<ClassicSession:<<<__mysql_uri>>>>|

//@ classic session - switch to Python
|Switching to Python mode...|

//@<OUT> run \show with classic session in Python
+---+
| 1 |
+---+
| 1 |
+---+

//@ classic session - switch to SQL mode
|Switching to SQL mode... Commands end with ;|

//@ classic session - switch to JavaScript
|Switching to JavaScript mode...|

// WL11263_TSF2_2 - Validate that the \show command can be used in all the scripting modes: JS, PY, SQL.

//@ WL11263_TSF2_2 - switch to Python
|Switching to Python mode...|

//@<OUT> WL11263_TSF2_2 - run show command in Python
+---+
| 1 |
+---+
| 1 |
+---+

//@ WL11263_TSF2_2 - switch to SQL mode
|Switching to SQL mode... Commands end with ;|

//@ WL11263_TSF2_2 - switch to JavaScript
|Switching to JavaScript mode...|

//@ WL11263_TSF2_3 - Validate that using the \show command with an invalid report name throws an exception.
||Unknown report: unknown_report

//@ WL11263_TSF2_4 - Validate that using the \show command without a report name list the reports available.
|Available reports: query, thread, threads.|

//@ WL11263_TSF5_4 - use \watch command with an invalid --interval value (below threshold)
||The value of '--interval' option should be a float in range [0.1, 86400], got: '0.09'.

//@ WL11263_TSF5_5 - use \watch command with an invalid --interval value (above threshold)
||The value of '--interval' option should be a float in range [0.1, 86400], got: '86400.1'.

// -----------------------------------------------------------------------------
// WL11263_TSF2_5 - Validate that a report registered with an all lower-case name can be invoked by the \show command using all upper-case name.

//@ WL11263_TSF2_5 - register the report with all lower-case name
||

//@ WL11263_TSF2_5 - call the report using upper-case name
|lower-case report name|

//@ WL11263_TSF2_5 - register the report with all upper-case name
||

//@ WL11263_TSF2_5 - call the report using lower-case name
|upper-case report name|

// -----------------------------------------------------------------------------
// WL11263_TSF2_6 - Validate that a report registered with a name containing '_' character can be invoked by the \show command using a name with '_' character replaced by '-' character.

//@ WL11263_TSF2_6 - register the report with name containing '_'
||

//@ WL11263_TSF2_6 - call the report using hyphen
|report name with underscore|

//@ WL11263_TSF2_6 - call the report - mix upper-case, underscore and hyphen
|report name with underscore|

//@ WL11263_TSF2_6 - register the report with upper-case name containing '_'
||

//@ WL11263_TSF2_6 - call the upper-case report - mix upper-case, underscore and hyphen
|upper-case report name with underscore|

//@ try to register the report with function set to undefined
||Shell.registerReport: Argument #3 is expected to be a function (ArgumentError)

//@ try to register the report with options set to invalid type
||Shell.registerReport: Argument #4 is expected to be a map (ArgumentError)

//@ WL11263_TSF9_1 - Try to register a plugin report with a duplicated name, error is expected.
||Shell.registerReport: Duplicate report: query (ArgumentError)

//@ WL11263_TSF9_2 - Try to register a plugin report with an invalid identifier as its name, error is expected.
||Shell.registerReport: The function name '1' is not a valid identifier. (ArgumentError)

// WL11263_TSF9_3 - Try to register a plugin report without giving a value to the 'name' dictionary option, error is expected.

//@ WL11263_TSF9_3 - The 'name' key is set to undefined
||Shell.registerReport: Argument #1 is expected to be a string (ArgumentError)

//@ WL11263_TSF9_3 - The 'name' key is set to an empty string
||Shell.registerReport: The function name '' is not a valid identifier. (ArgumentError)

// WL11263_TSF9_4 - Try to register a plugin report without giving a value to the 'type' parameter, error is expected.

//@ WL11263_TSF9_4 - The 'type' key is set to undefined
||Shell.registerReport: Argument #2 is expected to be a string (ArgumentError)

// WL11263_TSF9_5 - Try to register a plugin report with a value different than the supported as the value for the 'type' dictionary option, error is expected.

//@ WL11263_TSF9_5 - The 'name' key is set to an empty string
||Shell.registerReport: Report type must be one of: list, report, print. (ArgumentError)

//@ Register report with valid type - 'print'
||

//@ Register report with valid type - 'report'
||

//@ Register report with valid type - 'list'
||

//@ WL11263_TSF9_6 - Validate that plugin report can be registered if the dictionary options 'description' and 'options' are not provided.
||

// Try to register a plugin report giving an invalid value to the 'brief' dictionary option, error is expected.

//@ The 'brief' key is set to undefined
||Shell.registerReport: Option 'brief' is expected to be of type String, but is Undefined (TypeError)

//@ The 'brief' key is set to null
||Shell.registerReport: Option 'brief' is expected to be of type String, but is Null (TypeError)

//@ The 'brief' key is set to an invalid type
||Shell.registerReport: Option 'brief' is expected to be of type String, but is Map (TypeError)

// Try to register a plugin report giving an invalid value to the 'details' dictionary option, error is expected.

//@ The 'details' key is set to undefined
||Shell.registerReport: Option 'details' is expected to be of type Array, but is Undefined (TypeError)

//@ The 'details' key is set to an invalid type
||Shell.registerReport: Option 'details' is expected to be of type Array, but is Map (TypeError)

//@ The 'details' array holds an invalid type
||Shell.registerReport: Option 'details' String expected, but value is Integer (TypeError)

//@ The 'options' key is set to undefined
||Shell.registerReport: Option 'options' is expected to be of type Array, but is Undefined (TypeError)

//@ The 'options' key is set to an invalid type
||Shell.registerReport: Option 'options' is expected to be of type Array, but is String (TypeError)

//@ The 'options' key is set to an array of invalid types
||Shell.registerReport: Invalid definition at option #1 (ArgumentError)

// WL11263_TSF9_7 - If the dictionary option 'options' is given when registering a plugin report, validate that an exception is thrown if the 'name' key is not provided.

//@ WL11263_TSF9_7 - Missing 'name' key
||Shell.registerReport: Missing required options at option #1: name (ArgumentError)

//@ WL11263_TSF9_7 - The 'name' key is set to undefined
||Shell.registerReport: Option 'name' is expected to be of type String, but is Undefined (TypeError)

//@ WL11263_TSF9_7 - The 'name' key is set to null
||Shell.registerReport: Option 'name' is expected to be of type String, but is Null (TypeError)

//@ WL11263_TSF9_7 - The 'name' key is set to an invalid type
||Shell.registerReport: Option 'name' is expected to be of type String, but is Map (TypeError)

//@ WL11263_TSF9_7 - The 'name' key is set to an empty string
||Shell.registerReport: parameter 'options', option #1 is not a valid identifier: ''. (ArgumentError)

//@ WL11263_TSF9_8 - If the dictionary option 'options' is given when registering a plugin report, validate that an exception is thrown if the 'name' is not a valid identifier.
||Shell.registerReport: parameter 'options', option #1 is not a valid identifier: '1'. (ArgumentError)

//@ WL11263_TSF9_9 - Option duplicates --help
||Shell.registerReport: Error while adding option 'help' its cmdline name '--help' clashes with existing option. (ArgumentError)

//@ WL11263_TSF9_9 - Option duplicates --interval
||Shell.registerReport: Option 'interval' is reserved for use with the \watch command. (ArgumentError)

//@ WL11263_TSF9_9 - Option duplicates --nocls
||Shell.registerReport: Option 'nocls' is reserved for use with the \watch command. (ArgumentError)

//@ WL11263_TSF9_9 - Option duplicates --vertical in a 'list' type report
||Shell.registerReport: Error while adding option 'vertical' its cmdline name '--vertical' clashes with existing option. (ArgumentError)

//@ WL11263_TSF9_9 - Two options with the same name
||Shell.registerReport: option 'option' is already defined. (ArgumentError)

//@ Report uses --vertical option in a 'print' type report - no error
||

//@ Report uses --vertical option in a 'report' type report - no error
||

//@ WL11263_TSF9_10 - Register the report
||

//@<OUT> WL11263_TSF9_10 - Execute the report
+-----------------+
| value           |
+-----------------+
| some text value |
+-----------------+

//@ The 'shortcut' key is set to undefined
||Shell.registerReport: Option 'shortcut' is expected to be of type String, but is Undefined (TypeError)

//@ The 'shortcut' key is set to null
||Shell.registerReport: Option 'shortcut' is expected to be of type String, but is Null (TypeError)

//@ The 'shortcut' key is set to an invalid type
||Shell.registerReport: Option 'shortcut' is expected to be of type String, but is Map (TypeError)

//@ The 'shortcut' key is set to non-alphanumeric
||Shell.registerReport: Short name of an option must be an alphanumeric character. (ArgumentError)

//@ The 'shortcut' key is set to more than one character
||Shell.registerReport: Short name of an option must be exactly one character long. (ArgumentError)

//@ WL11263_TSF9_11 - Option duplicates -i
||Shell.registerReport: Short name 'i' is reserved for use with the \watch command. (ArgumentError)

//@ WL11263_TSF9_11 - Option duplicates -E in a 'list' type report
||Shell.registerReport: Error while adding option 'option' its cmdline name '-E' clashes with existing option. (ArgumentError)

//@ WL11263_TSF9_11 - Two options with the same name
||Shell.registerReport: Error while adding option 'option2' its cmdline name '-o' clashes with option 'option1'. (ArgumentError)

//@ Report uses -E option in a 'print' type report - no error
||

//@ Report uses -E option in a 'report' type report - no error
||

//@ WL11263_TSF9_12 - Register the report.
||

//@<OUT> WL11263_TSF9_12 - Execute the report with long option.
+------------+
| value      |
+------------+
| 1234567890 |
+------------+

//@ The options['brief'] key is set to undefined
||Shell.registerReport: Option 'brief' is expected to be of type String, but is Undefined (TypeError)

//@ The options['brief'] key is set to null
||Shell.registerReport: Option 'brief' is expected to be of type String, but is Null (TypeError)

//@ The options['brief'] key is set to an invalid type
||Shell.registerReport: Option 'brief' is expected to be of type String, but is Map (TypeError)

//@ The options['details'] key is set to undefined
||Shell.registerReport: Option 'details' is expected to be of type Array, but is Undefined (TypeError)

//@ The options['details'] key is set to an invalid type
||Shell.registerReport: Option 'details' is expected to be of type Array, but is Map (TypeError)

//@ The options['details'] key holds an invalid type
||Shell.registerReport: Option 'details' String expected, but value is Integer (TypeError)

//@ WL11263_TSF9_13 - The 'type' key is set to undefined
||Shell.registerReport: Option 'type' is expected to be of type String, but is Undefined (TypeError)

//@ WL11263_TSF9_13 - The 'type' key is set to null
||Shell.registerReport: Option 'type' is expected to be of type String, but is Null (TypeError)

//@ WL11263_TSF9_13 - The 'type' key is set to an invalid type
||Shell.registerReport: Option 'type' is expected to be of type String, but is Map (TypeError)

//@ WL11263_TSF9_13 - The 'type' key is set to an empty string
||Shell.registerReport: Unsupported type used at option 'invalid_option'. Allowed types: bool, float, integer, string (ArgumentError)

//@ WL11263_TSF9_13 - The 'type' key is set to an invalid value
||Shell.registerReport: Unsupported type used at option 'invalid_option'. Allowed types: bool, float, integer, string (ArgumentError)

//@ WL11263_TSF9_13 - The 'type' key has valid values
||

//@ The 'required' key is set to undefined
||Shell.registerReport: Option 'required' is expected to be of type Bool, but is Undefined (TypeError)

//@ The 'required' key is set to null
||Shell.registerReport: Option 'required' is expected to be of type Bool, but is Null (TypeError)

//@ The 'required' key is set to an invalid type
||Shell.registerReport: Option 'required' is expected to be of type Bool, but is Map (TypeError)

//@ Option cannot be 'required' if it's bool
||Shell.registerReport: Option of type 'bool' cannot be required. (ArgumentError)

//@ WL11263_TSF9_14 - Register the report.
||

//@ WL11263_TSF9_14 - Execute the report without required option.
||list_report_with_required_option: missing required option(s): --test.

//@<OUT> WL11263_TSF9_14 - Execute the report with required option.
+--------+
| value  |
+--------+
| qwerty |
+--------+

//@ WL11263_TSF9_15 - Register the report.
||

//@<OUT> WL11263_TSF9_15 - Execute the report without non-required options.
+------------+
| value      |
+------------+
| 9876.54321 |
+------------+

//@ The 'values' key is set to undefined
||Shell.registerReport: Option 'values' is expected to be of type Array, but is Undefined (TypeError)

//@ The 'values' key is set to an invalid type
||Shell.registerReport: Option 'values' is expected to be of type Array, but is Map (TypeError)

//@ The 'values' list holds invalid type
||Shell.registerReport: Option 'values' String expected, but value is Map (TypeError)

//@ The 'values' list cannot be defined for an 'integer' type
||Shell.registerReport: Invalid options at integer option 'invalid_option': values (ArgumentError)

//@ The 'values' list cannot be defined for a 'bool' type
||Shell.registerReport: Invalid options at bool option 'invalid_option': values (ArgumentError)

//@ The 'values' list cannot be defined for a 'float' type
||Shell.registerReport: Invalid options at float option 'invalid_option': values (ArgumentError)

//@ WL11263_TSF9_16 - Register the report.
||

//@ WL11263_TSF9_16 - Execute the report with a disallowed value.
||list_report_with_list_of_allowed_values: option '--test' only accepts the following values: one, two, three.

//@<OUT> Execute the report with an allowed value.
+-------+
| value |
+-------+
| three |
+-------+

//@ Option definition cannot contain unknown keys
||Shell.registerReport: Invalid options at string option 'invalid_option': unknown_key (ArgumentError)

//@ The 'argc' key is set to undefined
||Shell.registerReport: Option 'argc' is expected to be of type String, but is Undefined (TypeError)

//@ The 'argc' key is set to null
||Shell.registerReport: Option 'argc' is expected to be of type String, but is Null (TypeError)

//@ The 'argc' key is set to an invalid type
||Shell.registerReport: Option 'argc' is expected to be of type String, but is Map (TypeError)

//@ WL11263_TSF9_17 - The 'argc' value contains a letter
||Shell.registerReport: Cannot convert 'x' to an unsigned integer. (ArgumentError)

//@ WL11263_TSF9_17 - The 'argc' value contains a question mark
||Shell.registerReport: Cannot convert '?' to an unsigned integer. (ArgumentError)

//@ WL11263_TSF9_17 - The 'argc' value contains two letters
||Shell.registerReport: Cannot convert 'y' to an unsigned integer. (ArgumentError)

//@ WL11263_TSF9_17 - The 'argc' value contains asterisk at wrong position
||Shell.registerReport: Cannot convert '*' to an unsigned integer. (ArgumentError)

//@ WL11263_TSF9_17 - The 'argc' value contains ill defined range
||Shell.registerReport: The lower limit of 'argc' cannot be greater than upper limit. (ArgumentError)

//@ WL11263_TSF9_17 - The 'argc' value contains ill defined range - third number
||Shell.registerReport: The value associated with the key named 'argc' has wrong format. (ArgumentError)

//@ WL11263_TSF9_17 - The 'argc' is set to 0
||

//@ WL11263_TSF9_17 - The 'argc' is set to 7
||

//@ WL11263_TSF9_17 - The 'argc' is set to *
||

//@ WL11263_TSF9_17 - The 'argc' is set to 0-2
||

//@ WL11263_TSF9_17 - The 'argc' is set to 1-2
||

//@ WL11263_TSF9_17 - The 'argc' is set to 0-*
||

//@ WL11263_TSF9_17 - The 'argc' is set to 1-*
||

//@ WL11263_TSF9_18 - Register the JavaScript report
||

//@ WL11263_TSF9_18 - Call JS report in JS mode
|This is a JavaScript report|

//@ WL11263_TSF9_18 - Switch to Python
|Switching to Python mode...|

//@ WL11263_TSF9_18 - Register the Python report
||

//@ WL11263_TSF9_18 - Call JS report in PY mode
|This is a JavaScript report|

//@ WL11263_TSF9_18 - Call PY report in PY mode
|This is a Python report|

//@ WL11263_TSF9_18 - Switch to SQL
|Switching to SQL mode... Commands end with ;|

//@ WL11263_TSF9_18 - Call JS report in SQL mode
|This is a JavaScript report|

//@ WL11263_TSF9_18 - Call PY report in SQL mode
|This is a Python report|

//@ WL11263_TSF9_18 - Switch to JavaScript
|Switching to JavaScript mode...|

//@ WL11263_TSF9_18 - Call PY report in JS mode
|This is a Python report|

//@ Register the JavaScript report which throws an exception
||

//@<ERR> Call JS report which throws in JS mode
reports.javascript_report_which_throws_an_exception: User-defined function threw an exception:
This is a JavaScript exception at (shell):1:89
in shell.registerReport('javascript_report_which_throws_an_exception', 'print', function (){throw 'This is a JavaScript exception'})
                                                                                            ^

//@ Switch to Python to create a report and call both of them
|Switching to Python mode...|

//@ Register the Python report which throws an exception
||

//@<ERR> Call JS report which throws in PY mode
reports.javascript_report_which_throws_an_exception: User-defined function threw an exception:
This is a JavaScript exception at (shell):1:89
in shell.registerReport('javascript_report_which_throws_an_exception', 'print', function (){throw 'This is a JavaScript exception'})
                                                                                            ^

//@<ERR> Call PY report which throws in PY mode
reports.python_report_which_throws_an_exception: User-defined function threw an exception:
Traceback (most recent call last):
  File "<string>", line 2, in python_report_which_throws_an_exception
Exception: This is a Python exception

//@ Switch to SQL to call both throwing reports
|Switching to SQL mode... Commands end with ;|

//@<ERR> Call JS report which throws in SQL mode
reports.javascript_report_which_throws_an_exception: User-defined function threw an exception:
This is a JavaScript exception at (shell):1:89
in shell.registerReport('javascript_report_which_throws_an_exception', 'print', function (){throw 'This is a JavaScript exception'})
                                                                                            ^

//@<ERR> Call PY report which throws in SQL mode
reports.python_report_which_throws_an_exception: User-defined function threw an exception:
Traceback (most recent call last):
  File "<string>", line 2, in python_report_which_throws_an_exception
Exception: This is a Python exception

//@ Switch to JavaScript to call Python report which throws
|Switching to JavaScript mode...|

//@<ERR> Call PY report which throws in JS mode
reports.python_report_which_throws_an_exception: User-defined function threw an exception:
Traceback (most recent call last):
  File "<string>", line 2, in python_report_which_throws_an_exception
Exception: This is a Python exception

//@ WL11263_TSF9_19 - register the report
||

//@<OUT> WL11263_TSF9_19 - Check output
before
Some random text
after

//@ WL11263_TSF9_20 - register the report
||

//@ WL11263_TSF9_20 - Check output
|Another random text|

//@ WL11263_TSF9_21 - register the report
||

//@ WL11263_TSF9_21 - Check output - no error
|Very interesting report|


//@ WL11263_TSF9_22 - register the report
||

//@ WL11263_TSF9_22 - Check output - error
||List report should return a list of lists.

//@ WL11263_TSF9_23 - register the report
||

//@ WL11263_TSF9_23 - Check output - error
||List report should contain at least one row.


//@ WL11263_TSF9_24 - register the report
||

//@ WL11263_TSF9_24 - Check output - error
||Option 'report' is expected to be of type Array, but is Undefined


//@ WL11263_TSF9_25 - register the report
||

//@ WL11263_TSF9_25 - Check output - error
||Option 'report' is expected to be of type Array, but is Undefined

//@ WL11263_TSF9_26 - register the report
||

//@ WL11263_TSF9_26 - Check output - error
||Report of type 'report' should contain exactly one element.

//@ WL11263_TSF9_27 - register the report
||

//@ WL11263_TSF9_27 - Check output - error
||Report of type 'report' should contain exactly one element.


//@ WL11263_TSF9_28 - register the lower-case report
||

//@ WL11263_TSF9_28 - try to register report with the same name but converted to upper-case
||Shell.registerReport: Name 'SAMPLE_REPORT_WITH_LOWER_CASE_NAME' conflicts with an existing report: sample_report_with_lower_case_name (ArgumentError)

//@ WL11263_TSF9_28 - register the upper-case report
||

//@ WL11263_TSF9_28 - try to register report with the same name but converted to lower-case
||Shell.registerReport: Name 'sample_report_with_upper_case_name' conflicts with an existing report: SAMPLE_REPORT_WITH_UPPER_CASE_NAME (ArgumentError)

//@ WL11263_TSF9_28 - it's not possible to register a report name containing hyphen
||Shell.registerReport: The function name 'sample_report_with-hyphen' is not a valid identifier. (ArgumentError)

//@ register the report - list_report_testing_various_options
||

//@ call list_report_testing_various_options with an unknown option
||list_report_testing_various_options: unknown option --unknown

//@ call list_report_testing_various_options with an unknown short option
||list_report_testing_various_options: unknown option -u

//@ call list_report_testing_various_options using the first option without value
||list_report_testing_various_options: option --one requires an argument

//@ call list_report_testing_various_options using the first option without value - short
||list_report_testing_various_options: option -1 requires an argument

//@<OUT> call list_report_testing_various_options using the first option
+------+--------+---------+
| name | type   | value   |
+------+--------+---------+
| one  | String | default |
+------+--------+---------+

//@ call list_report_testing_various_options using the second option without value
||list_report_testing_various_options: option --two requires an argument

//@ call list_report_testing_various_options using the second option without value - short
||list_report_testing_various_options: option -2 requires an argument

//@<OUT> call list_report_testing_various_options using the second option
+------+--------+-------+
| name | type   | value |
+------+--------+-------+
| two  | String | text  |
+------+--------+-------+

//@<OUT> call list_report_testing_various_options using the second option - int is converted to string
+------+--------+-------+
| name | type   | value |
+------+--------+-------+
| two  | String | 2     |
+------+--------+-------+

//@<OUT> call list_report_testing_various_options using the second option - bool is converted to string
+------+--------+-------+
| name | type   | value |
+------+--------+-------+
| two  | String | false |
+------+--------+-------+

//@<OUT> call list_report_testing_various_options using the second option - float is converted to string
+------+--------+-------+
| name | type   | value |
+------+--------+-------+
| two  | String | 3.14  |
+------+--------+-------+

//@ call list_report_testing_various_options not using the third option (it's not added to options dictionary when calling the function)
|Report returned no data.|

//@<OUT> call list_report_testing_various_options using the third option
+-------+------+-------+
| name  | type | value |
+-------+------+-------+
| three | Bool | true  |
+-------+------+-------+

//@ call list_report_testing_various_options using the third option and providing a value - it's treated as an argument
||list_report_testing_various_options: report is expecting 0 arguments, 1 provided. For usage information please run: \show list_report_testing_various_options --help

//@ call list_report_testing_various_options using the third option and providing a value - error
||list_report_testing_various_options: option --three does not require an argument

//@ call list_report_testing_various_options using the third option and providing a value - short - error
||list_report_testing_various_options: unknown option -a

//@ call list_report_testing_various_options using the fourth option without value
||list_report_testing_various_options: option --four requires an argument

//@ call list_report_testing_various_options using the fourth option without value - short
||list_report_testing_various_options: option -4 requires an argument

//@<OUT> call list_report_testing_various_options using the fourth option
+------+---------+-------+
| name | type    | value |
+------+---------+-------+
| four | Integer | 123   |
+------+---------+-------+

//@ call list_report_testing_various_options using the fourth option - string is not converted to int
||list_report_testing_various_options: cannot convert 'text' to a signed integer

//@ call list_report_testing_various_options using the fourth option - bool is not converted to int
||list_report_testing_various_options: cannot convert 'false' to a signed integer

//@ call list_report_testing_various_options using the fourth option - float is not converted to int
||list_report_testing_various_options: cannot convert '3.14' to a signed integer

//@ call list_report_testing_various_options using the fifth option without value
||list_report_testing_various_options: option --five requires an argument

//@ call list_report_testing_various_options using the fifth option without value - short
||list_report_testing_various_options: option -5 requires an argument

//@<OUT> call list_report_testing_various_options using the fifth option
+------+--------+---------+
| name | type   | value   |
+------+--------+---------+
| five | Number | 123.456 |
+------+--------+---------+

//@ call list_report_testing_various_options using the fifth option - string is not converted to float
||list_report_testing_various_options: cannot convert 'text' to a floating-point number

//@ call list_report_testing_various_options using the fifth option - bool is not converted to float
||list_report_testing_various_options: cannot convert 'false' to a floating-point number

//@<OUT> call list_report_testing_various_options using the fifth option - int is converted to float
+------+---------+-------+
| name | type    | value |
+------+---------+-------+
| five | Integer | 3     |
+------+---------+-------+

//@<OUT> call list_report_testing_various_options using --vertical option, should not be added to options
*************************** 1. row ***************************
 name: one
 type: String
value: text

//@<OUT> call list_report_testing_various_options using all options
+-------+---------+---------+
| name  | type    | value   |
+-------+---------+---------+
| five  | Number  | 654.321 |
| four  | Integer | 987     |
| one   | String  | text    |
| three | Bool    | true    |
| two   | String  | query   |
+-------+---------+---------+

//@ register the report - list_report_testing_argc_default
||

//@ call list_report_testing_argc_default with no arguments
|Report returned no data.|

//@ call list_report_testing_argc_default with an argument
||list_report_testing_argc_default: report is expecting 0 arguments, 1 provided. For usage information please run: \show list_report_testing_argc_default --help

//@ register the report - list_report_testing_argc_0
||

//@ call list_report_testing_argc_0 with no arguments
|Report returned no data.|

//@ call list_report_testing_argc_0 with an argument
||list_report_testing_argc_0: report is expecting 0 arguments, 1 provided. For usage information please run: \show list_report_testing_argc_0 --help


//@ register the report - list_report_testing_argc_1
||

//@ call list_report_testing_argc_1 with no arguments
||list_report_testing_argc_1: report is expecting 1 argument, 0 provided. For usage information please run: \show list_report_testing_argc_1 --help

//@<OUT> call list_report_testing_argc_1 with an argument
+------+---------+----------+
| name | type    | value    |
+------+---------+----------+
| argv | m.Array | ["arg0"] |
+------+---------+----------+

//@ call list_report_testing_argc_1 with two arguments
||list_report_testing_argc_1: report is expecting 1 argument, 2 provided. For usage information please run: \show list_report_testing_argc_1 --help

//@ register the report - list_report_testing_argc_asterisk
||

//@ call list_report_testing_argc_asterisk with no arguments
|Report returned no data.|

//@<OUT> call list_report_testing_argc_asterisk with an argument
+------+---------+----------+
| name | type    | value    |
+------+---------+----------+
| argv | m.Array | ["arg0"] |
+------+---------+----------+

//@<OUT> call list_report_testing_argc_asterisk with two arguments
+------+---------+------------------+
| name | type    | value            |
+------+---------+------------------+
| argv | m.Array | ["arg0", "arg1"] |
+------+---------+------------------+

//@ register the report - list_report_testing_argc_1_2
||

//@ call list_report_testing_argc_1_2 with no arguments
||list_report_testing_argc_1_2: report is expecting 1-2 arguments, 0 provided. For usage information please run: \show list_report_testing_argc_1_2 --help

//@<OUT> call list_report_testing_argc_1_2 with an argument
+------+---------+----------+
| name | type    | value    |
+------+---------+----------+
| argv | m.Array | ["arg0"] |
+------+---------+----------+

//@<OUT> call list_report_testing_argc_1_2 with two arguments
+------+---------+------------------+
| name | type    | value            |
+------+---------+------------------+
| argv | m.Array | ["arg0", "arg1"] |
+------+---------+------------------+

//@ call list_report_testing_argc_1_2 with three arguments
||list_report_testing_argc_1_2: report is expecting 1-2 arguments, 3 provided. For usage information please run: \show list_report_testing_argc_1_2 --help

//@ register the report - list_report_testing_argc_1_asterisk
||

//@ call list_report_testing_argc_1_asterisk with no arguments
||list_report_testing_argc_1_asterisk: report is expecting 1-* arguments, 0 provided. For usage information please run: \show list_report_testing_argc_1_asterisk --help

//@<OUT> call list_report_testing_argc_1_asterisk with an argument
+------+---------+----------+
| name | type    | value    |
+------+---------+----------+
| argv | m.Array | ["arg0"] |
+------+---------+----------+

//@<OUT> call list_report_testing_argc_1_asterisk with two arguments
+------+---------+------------------+
| name | type    | value            |
+------+---------+------------------+
| argv | m.Array | ["arg0", "arg1"] |
+------+---------+------------------+

//@<OUT> call list_report_testing_argc_1_asterisk with three arguments
+------+---------+--------------------------+
| name | type    | value                    |
+------+---------+--------------------------+
| argv | m.Array | ["arg0", "arg1", "arg2"] |
+------+---------+--------------------------+

//@ WL11263_TSF10_1 - register the report - ths_list_no_options_no_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_no_options_no_arguments
NAME
      ths_list_no_options_no_arguments - testing help

SYNTAX
      shell.reports.ths_list_no_options_no_arguments(session)

WHERE
      session: Object. A Session object to be used to execute the report.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_list_options_no_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_options_no_arguments
NAME
      ths_list_options_no_arguments - testing help

SYNTAX
      shell.reports.ths_list_options_no_arguments(session, argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 0 arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_list_no_options_one_argument
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_no_options_one_argument
NAME
      ths_list_no_options_one_argument - testing help

SYNTAX
      shell.reports.ths_list_no_options_one_argument(session, argv)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1 argument.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_list_no_options_unbound_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_no_options_unbound_arguments
NAME
      ths_list_no_options_unbound_arguments - testing help

SYNTAX
      shell.reports.ths_list_no_options_unbound_arguments(session[, argv])

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects any number of arguments.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_list_no_options_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_no_options_range_of_arguments
NAME
      ths_list_no_options_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_list_no_options_range_of_arguments(session, argv)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-2 arguments.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_list_no_options_unbound_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_no_options_unbound_range_of_arguments
NAME
      ths_list_no_options_unbound_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_list_no_options_unbound_range_of_arguments(session,
      argv)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-* arguments.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_list_options_one_argument
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_options_one_argument
NAME
      ths_list_options_one_argument - testing help

SYNTAX
      shell.reports.ths_list_options_one_argument(session, argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1 argument.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_list_options_unbound_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_options_unbound_arguments
NAME
      ths_list_options_unbound_arguments - testing help

SYNTAX
      shell.reports.ths_list_options_unbound_arguments(session, argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects any number of arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_list_options_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_options_range_of_arguments
NAME
      ths_list_options_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_list_options_range_of_arguments(session, argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-2 arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_list_options_range_of_arguments_details
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_options_range_of_arguments_details
NAME
      ths_list_options_range_of_arguments_details - testing help

SYNTAX
      shell.reports.ths_list_options_range_of_arguments_details(session, argv,
      options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-2 arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'list' type report.

      More details

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_list_options_unbound_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_list_options_unbound_range_of_arguments
NAME
      ths_list_options_unbound_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_list_options_unbound_range_of_arguments(session, argv,
      options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-* arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'list' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_report_no_options_no_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_no_options_no_arguments
NAME
      ths_report_no_options_no_arguments - testing help

SYNTAX
      shell.reports.ths_report_no_options_no_arguments(session)

WHERE
      session: Object. A Session object to be used to execute the report.

DESCRIPTION
      This is a 'report' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_report_options_no_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_options_no_arguments
NAME
      ths_report_options_no_arguments - testing help

SYNTAX
      shell.reports.ths_report_options_no_arguments(session, argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 0 arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'report' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_report_no_options_one_argument
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_no_options_one_argument
NAME
      ths_report_no_options_one_argument - testing help

SYNTAX
      shell.reports.ths_report_no_options_one_argument(session, argv)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1 argument.

DESCRIPTION
      This is a 'report' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_report_no_options_unbound_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_no_options_unbound_arguments
NAME
      ths_report_no_options_unbound_arguments - testing help

SYNTAX
      shell.reports.ths_report_no_options_unbound_arguments(session[, argv])

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects any number of arguments.

DESCRIPTION
      This is a 'report' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_report_no_options_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_no_options_range_of_arguments
NAME
      ths_report_no_options_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_report_no_options_range_of_arguments(session, argv)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-2 arguments.

DESCRIPTION
      This is a 'report' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_report_no_options_unbound_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_no_options_unbound_range_of_arguments
NAME
      ths_report_no_options_unbound_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_report_no_options_unbound_range_of_arguments(session,
      argv)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-* arguments.

DESCRIPTION
      This is a 'report' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_report_options_one_argument
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_options_one_argument
NAME
      ths_report_options_one_argument - testing help

SYNTAX
      shell.reports.ths_report_options_one_argument(session, argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1 argument.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'report' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_report_options_unbound_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_options_unbound_arguments
NAME
      ths_report_options_unbound_arguments - testing help

SYNTAX
      shell.reports.ths_report_options_unbound_arguments(session, argv,
      options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects any number of arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'report' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_report_options_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_options_range_of_arguments
NAME
      ths_report_options_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_report_options_range_of_arguments(session, argv,
      options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-2 arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'report' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_report_options_range_of_arguments_details
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_options_range_of_arguments_details
NAME
      ths_report_options_range_of_arguments_details - testing help

SYNTAX
      shell.reports.ths_report_options_range_of_arguments_details(session,
      argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-2 arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'report' type report.

      More details

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_report_options_unbound_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_report_options_unbound_range_of_arguments
NAME
      ths_report_options_unbound_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_report_options_unbound_range_of_arguments(session,
      argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-* arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'report' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_print_no_options_no_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_no_options_no_arguments
NAME
      ths_print_no_options_no_arguments - testing help

SYNTAX
      shell.reports.ths_print_no_options_no_arguments(session)

WHERE
      session: Object. A Session object to be used to execute the report.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_print_options_no_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_options_no_arguments
NAME
      ths_print_options_no_arguments - testing help

SYNTAX
      shell.reports.ths_print_options_no_arguments(session, argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 0 arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_print_no_options_one_argument
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_no_options_one_argument
NAME
      ths_print_no_options_one_argument - testing help

SYNTAX
      shell.reports.ths_print_no_options_one_argument(session, argv)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1 argument.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_print_no_options_unbound_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_no_options_unbound_arguments
NAME
      ths_print_no_options_unbound_arguments - testing help

SYNTAX
      shell.reports.ths_print_no_options_unbound_arguments(session[, argv])

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects any number of arguments.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_print_no_options_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_no_options_range_of_arguments
NAME
      ths_print_no_options_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_print_no_options_range_of_arguments(session, argv)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-2 arguments.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_print_no_options_unbound_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_no_options_unbound_range_of_arguments
NAME
      ths_print_no_options_unbound_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_print_no_options_unbound_range_of_arguments(session,
      argv)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-* arguments.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

//@ WL11263_TSF10_1 - register the report - ths_print_options_one_argument
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_options_one_argument
NAME
      ths_print_options_one_argument - testing help

SYNTAX
      shell.reports.ths_print_options_one_argument(session, argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1 argument.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_print_options_unbound_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_options_unbound_arguments
NAME
      ths_print_options_unbound_arguments - testing help

SYNTAX
      shell.reports.ths_print_options_unbound_arguments(session, argv, options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects any number of arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_print_options_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_options_range_of_arguments
NAME
      ths_print_options_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_print_options_range_of_arguments(session, argv,
      options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-2 arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_print_options_range_of_arguments_details
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_options_range_of_arguments_details
NAME
      ths_print_options_range_of_arguments_details - testing help

SYNTAX
      shell.reports.ths_print_options_range_of_arguments_details(session, argv,
      options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-2 arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'print' type report.

      More details

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@ WL11263_TSF10_1 - register the report - ths_print_options_unbound_range_of_arguments
||

//@<OUT> WL11263_TSF10_1 - show help - ths_print_options_unbound_range_of_arguments
NAME
      ths_print_options_unbound_range_of_arguments - testing help

SYNTAX
      shell.reports.ths_print_options_unbound_range_of_arguments(session, argv,
      options)

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 1-* arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - one String. Option with default type.
      - two (required) String. Option with "string" type.
      - three Bool. Option with "bool" type.
      - four Integer. Option with "integer" type.
      - five Float. Option with "float" type.

      The one option accepts the following values: a, b, c.

      Details of parameter three.


//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_no_options_no_arguments
NAME
      ths_list_no_options_no_arguments - testing help

SYNTAX
      \show ths_list_no_options_no_arguments [OPTIONS]
      \watch ths_list_no_options_no_arguments [OPTIONS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_options_no_arguments
NAME
      ths_list_options_no_arguments - testing help

SYNTAX
      \show ths_list_options_no_arguments --two=string|-2 [OPTIONS]
      \watch ths_list_options_no_arguments --two=string|-2 [OPTIONS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_no_options_one_argument
NAME
      ths_list_no_options_one_argument - testing help

SYNTAX
      \show ths_list_no_options_one_argument [OPTIONS] [ARGS]
      \watch ths_list_no_options_one_argument [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      Arguments:

      This report accepts 1 argument.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_no_options_unbound_arguments
NAME
      ths_list_no_options_unbound_arguments - testing help

SYNTAX
      \show ths_list_no_options_unbound_arguments [OPTIONS] [ARGS]
      \watch ths_list_no_options_unbound_arguments [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      Arguments:

      This report accepts any number of arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_no_options_range_of_arguments
NAME
      ths_list_no_options_range_of_arguments - testing help

SYNTAX
      \show ths_list_no_options_range_of_arguments [OPTIONS] [ARGS]
      \watch ths_list_no_options_range_of_arguments [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      Arguments:

      This report accepts 1-2 arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_no_options_unbound_range_of_arguments
NAME
      ths_list_no_options_unbound_range_of_arguments - testing help

SYNTAX
      \show ths_list_no_options_unbound_range_of_arguments [OPTIONS] [ARGS]
      \watch ths_list_no_options_unbound_range_of_arguments [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      Arguments:

      This report accepts 1-* arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_options_one_argument
NAME
      ths_list_options_one_argument - testing help

SYNTAX
      \show ths_list_options_one_argument --two=string|-2 [OPTIONS] [ARGS]
      \watch ths_list_options_one_argument --two=string|-2 [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1 argument.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_options_unbound_arguments
NAME
      ths_list_options_unbound_arguments - testing help

SYNTAX
      \show ths_list_options_unbound_arguments --two=string|-2 [OPTIONS] [ARGS]
      \watch ths_list_options_unbound_arguments --two=string|-2 [OPTIONS]
      [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts any number of arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_options_range_of_arguments
NAME
      ths_list_options_range_of_arguments - testing help

SYNTAX
      \show ths_list_options_range_of_arguments --two=string|-2 [OPTIONS]
      [ARGS]
      \watch ths_list_options_range_of_arguments --two=string|-2 [OPTIONS]
      [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1-2 arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_options_range_of_arguments_details
SYNTAX
      \show ths_list_options_range_of_arguments_details --two=string|-2
      [OPTIONS] [ARGS]
      \watch ths_list_options_range_of_arguments_details --two=string|-2
      [OPTIONS] [ARGS]

DESCRIPTION
      More details

      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1-2 arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_list_options_unbound_range_of_arguments
NAME
      ths_list_options_unbound_range_of_arguments - testing help

SYNTAX
      \show ths_list_options_unbound_range_of_arguments --two=string|-2
      [OPTIONS] [ARGS]
      \watch ths_list_options_unbound_range_of_arguments --two=string|-2
      [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --vertical, -E
                  Display records vertically.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1-* arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_no_options_no_arguments
NAME
      ths_report_no_options_no_arguments - testing help

SYNTAX
      \show ths_report_no_options_no_arguments [OPTIONS]
      \watch ths_report_no_options_no_arguments [OPTIONS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_options_no_arguments
NAME
      ths_report_options_no_arguments - testing help

SYNTAX
      \show ths_report_options_no_arguments --two=string|-2 [OPTIONS]
      \watch ths_report_options_no_arguments --two=string|-2 [OPTIONS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_no_options_one_argument
NAME
      ths_report_no_options_one_argument - testing help

SYNTAX
      \show ths_report_no_options_one_argument [OPTIONS] [ARGS]
      \watch ths_report_no_options_one_argument [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      Arguments:

      This report accepts 1 argument.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_no_options_unbound_arguments
NAME
      ths_report_no_options_unbound_arguments - testing help

SYNTAX
      \show ths_report_no_options_unbound_arguments [OPTIONS] [ARGS]
      \watch ths_report_no_options_unbound_arguments [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      Arguments:

      This report accepts any number of arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_no_options_range_of_arguments
NAME
      ths_report_no_options_range_of_arguments - testing help

SYNTAX
      \show ths_report_no_options_range_of_arguments [OPTIONS] [ARGS]
      \watch ths_report_no_options_range_of_arguments [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      Arguments:

      This report accepts 1-2 arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_no_options_unbound_range_of_arguments
NAME
      ths_report_no_options_unbound_range_of_arguments - testing help

SYNTAX
      \show ths_report_no_options_unbound_range_of_arguments [OPTIONS] [ARGS]
      \watch ths_report_no_options_unbound_range_of_arguments [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      Arguments:

      This report accepts 1-* arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_options_one_argument
NAME
      ths_report_options_one_argument - testing help

SYNTAX
      \show ths_report_options_one_argument --two=string|-2 [OPTIONS] [ARGS]
      \watch ths_report_options_one_argument --two=string|-2 [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1 argument.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_options_unbound_arguments
NAME
      ths_report_options_unbound_arguments - testing help

SYNTAX
      \show ths_report_options_unbound_arguments --two=string|-2 [OPTIONS]
      [ARGS]
      \watch ths_report_options_unbound_arguments --two=string|-2 [OPTIONS]
      [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts any number of arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_options_range_of_arguments
NAME
      ths_report_options_range_of_arguments - testing help

SYNTAX
      \show ths_report_options_range_of_arguments --two=string|-2 [OPTIONS]
      [ARGS]
      \watch ths_report_options_range_of_arguments --two=string|-2 [OPTIONS]
      [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1-2 arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_options_range_of_arguments_details
NAME
      ths_report_options_range_of_arguments_details - testing help

SYNTAX
      \show ths_report_options_range_of_arguments_details --two=string|-2
      [OPTIONS] [ARGS]
      \watch ths_report_options_range_of_arguments_details --two=string|-2
      [OPTIONS] [ARGS]

DESCRIPTION
      More details

      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1-2 arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_report_options_unbound_range_of_arguments
NAME
      ths_report_options_unbound_range_of_arguments - testing help

SYNTAX
      \show ths_report_options_unbound_range_of_arguments --two=string|-2
      [OPTIONS] [ARGS]
      \watch ths_report_options_unbound_range_of_arguments --two=string|-2
      [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1-* arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_no_options_no_arguments
NAME
      ths_print_no_options_no_arguments - testing help

SYNTAX
      \show ths_print_no_options_no_arguments [OPTIONS]
      \watch ths_print_no_options_no_arguments [OPTIONS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_options_no_arguments
NAME
      ths_print_options_no_arguments - testing help

SYNTAX
      \show ths_print_options_no_arguments --two=string|-2 [OPTIONS]
      \watch ths_print_options_no_arguments --two=string|-2 [OPTIONS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_no_options_one_argument
NAME
      ths_print_no_options_one_argument - testing help

SYNTAX
      \show ths_print_no_options_one_argument [OPTIONS] [ARGS]
      \watch ths_print_no_options_one_argument [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      Arguments:

      This report accepts 1 argument.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_no_options_unbound_arguments
NAME
      ths_print_no_options_unbound_arguments - testing help

SYNTAX
      \show ths_print_no_options_unbound_arguments [OPTIONS] [ARGS]
      \watch ths_print_no_options_unbound_arguments [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      Arguments:

      This report accepts any number of arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_no_options_range_of_arguments
NAME
      ths_print_no_options_range_of_arguments - testing help

SYNTAX
      \show ths_print_no_options_range_of_arguments [OPTIONS] [ARGS]
      \watch ths_print_no_options_range_of_arguments [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      Arguments:

      This report accepts 1-2 arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_no_options_unbound_range_of_arguments
NAME
      ths_print_no_options_unbound_range_of_arguments - testing help

SYNTAX
      \show ths_print_no_options_unbound_range_of_arguments [OPTIONS] [ARGS]
      \watch ths_print_no_options_unbound_range_of_arguments [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      Arguments:

      This report accepts 1-* arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_options_one_argument
NAME
      ths_print_options_one_argument - testing help

SYNTAX
      \show ths_print_options_one_argument --two=string|-2 [OPTIONS] [ARGS]
      \watch ths_print_options_one_argument --two=string|-2 [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1 argument.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_options_unbound_arguments
NAME
      ths_print_options_unbound_arguments - testing help

SYNTAX
      \show ths_print_options_unbound_arguments --two=string|-2 [OPTIONS]
      [ARGS]
      \watch ths_print_options_unbound_arguments --two=string|-2 [OPTIONS]
      [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts any number of arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_options_range_of_arguments
NAME
      ths_print_options_range_of_arguments - testing help

SYNTAX
      \show ths_print_options_range_of_arguments --two=string|-2 [OPTIONS]
      [ARGS]
      \watch ths_print_options_range_of_arguments --two=string|-2 [OPTIONS]
      [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1-2 arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_options_range_of_arguments_details
NAME
      ths_print_options_range_of_arguments_details - testing help

SYNTAX
      \show ths_print_options_range_of_arguments_details --two=string|-2
      [OPTIONS] [ARGS]
      \watch ths_print_options_range_of_arguments_details --two=string|-2
      [OPTIONS] [ARGS]

DESCRIPTION
      More details

      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1-2 arguments.

//@<OUT> WL11263_TSF18_1 - call \show --help - ths_print_options_unbound_range_of_arguments
NAME
      ths_print_options_unbound_range_of_arguments - testing help

SYNTAX
      \show ths_print_options_unbound_range_of_arguments --two=string|-2
      [OPTIONS] [ARGS]
      \watch ths_print_options_unbound_range_of_arguments --two=string|-2
      [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --one=string, -1
                  Option with default type. Allowed values: a, b, c.

      --two=string, -2
                  (required) Option with "string" type.

      --three, -3 Option with "bool" type.

      --four=integer, -4
                  Option with "integer" type.

      --five=float, -5
                  Option with "float" type.

      Details of parameter three.

      Arguments:

      This report accepts 1-* arguments.

//@<OUT> WL11263_TSF13_1 - call shell.reports.list_report_testing_various_options() in JS mode
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "one",
            "String",
            "text"
        ]
    ]
}

//@ WL11263_TSF13_1 - switch to Python
|Switching to Python mode...|

//@ WL11263_TSF13_1 - switch to back to JS
|Switching to JavaScript mode...|

//@ call shell.reports.list_report_testing_various_options without options
||reports.list_report_testing_various_options: Invalid number of arguments, expected 1 to 3 but got 0 (ArgumentError)


//@ call shell.reports.list_report_testing_various_options with session - undefined
||reports.list_report_testing_various_options: Argument #1 is expected to be an object (ArgumentError)

//@ call shell.reports.list_report_testing_various_options with session - null
||reports.list_report_testing_various_options: Argument #1 is expected to be an object (ArgumentError)

//@ call shell.reports.list_report_testing_various_options with session - string
||reports.list_report_testing_various_options: Argument #1 is expected to be an object (ArgumentError)


//@ call shell.reports.list_report_testing_various_options with options - undefined
||reports.list_report_testing_various_options: Argument #3 is expected to be a map (ArgumentError)

//@<OUT> call shell.reports.list_report_testing_various_options with options - null
{
    "report": [
        [
            "name",
            "type",
            "value"
        ]
    ]
}

//@ call shell.reports.list_report_testing_various_options with options - string
||reports.list_report_testing_various_options: Argument #3 is expected to be a map (ArgumentError)


//@ call shell.reports.list_report_testing_various_options with an unknown option
||reports.list_report_testing_various_options: Invalid options at Argument #3: unknown (ArgumentError)


//@<OUT> call shell.reports.list_report_testing_various_options using the first option without value
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "one",
            "Undefined",
            undefined
        ]
    ]
}

//@ call shell.reports.list_report_testing_various_options using the first option with int
||reports.list_report_testing_various_options: Argument #3, option 'one' is expected to be a string (ArgumentError)

//@<OUT> call shell.reports.list_report_testing_various_options using the first option with string
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "one",
            "String",
            "default"
        ]
    ]
}


//@<OUT> call shell.reports.list_report_testing_various_options using the second option without value
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "two",
            "Undefined",
            undefined
        ]
    ]
}

//@ call shell.reports.list_report_testing_various_options using the second option with int
||reports.list_report_testing_various_options: Argument #3, option 'two' is expected to be a string (ArgumentError)

//@<OUT> call shell.reports.list_report_testing_various_options using the second option with string
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "two",
            "String",
            "text"
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_various_options not using the third option
{
    "report": [
        [
            "name",
            "type",
            "value"
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_various_options using the third option - undefined
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "three",
            "Undefined",
            undefined
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_various_options using the third option with int
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "three",
            "Integer",
            3
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_various_options using the third option with float
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "three",
            "Number",
            3.14
        ]
    ]
}

//@ call shell.reports.list_report_testing_various_options using the third option with string
||reports.list_report_testing_various_options: Argument #3, option 'three' is expected to be a bool (ArgumentError)

//@<OUT> call shell.reports.list_report_testing_various_options using the third option with bool
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "three",
            "Bool",
            false
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_various_options using the third option with bool - true
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "three",
            "Bool",
            true
        ]
    ]
}


//@<OUT> call shell.reports.list_report_testing_various_options using the fourth option without value
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "four",
            "Undefined",
            undefined
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_various_options using the fourth option with bool
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "four",
            "Bool",
            false
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_various_options using the fourth option with float
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "four",
            "Number",
            3.14
        ]
    ]
}

//@ call shell.reports.list_report_testing_various_options using the fourth option with string
||reports.list_report_testing_various_options: Argument #3, option 'four' is expected to be an integer (ArgumentError)

//@<OUT> call shell.reports.list_report_testing_various_options using the fourth option with int
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "four",
            "Integer",
            4
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_various_options using the fifth option without value
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "five",
            "Undefined",
            undefined
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_various_options using the fifth option with bool
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "five",
            "Bool",
            false
        ]
    ]
}

//@ call shell.reports.list_report_testing_various_options using the fifth option with string
||reports.list_report_testing_various_options: Argument #3, option 'five' is expected to be a float (ArgumentError)

//@<OUT> call shell.reports.list_report_testing_various_options using the fifth option with int
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "five",
            "Integer",
            5
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_various_options using the fifth option with float
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "five",
            "Number",
            3.14
        ]
    ]
}


//@<OUT> call shell.reports.list_report_testing_various_options using all options
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "five",
            "Number",
            654.321
        ],
        [
            "four",
            "Integer",
            987
        ],
        [
            "one",
            "String",
            "text"
        ],
        [
            "three",
            "String",
            "true"
        ],
        [
            "two",
            "String",
            "query"
        ]
    ]
}

//@ call shell.reports.list_report_testing_argc_default with no arguments
||reports.list_report_testing_argc_default: Invalid number of arguments, expected 1 but got 2 (ArgumentError)


//@ call shell.reports.list_report_testing_argc_0 with no arguments
||reports.list_report_testing_argc_0: Invalid number of arguments, expected 1 but got 2 (ArgumentError)


//@ call shell.reports.list_report_testing_argc_1 with no arguments
||reports.list_report_testing_argc_1: Argument #2 'argv' is expecting 1 argument. (ArgumentError)

//@<OUT> call shell.reports.list_report_testing_argc_1 with an argument
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "argv",
            "m.Array",
            [
                "arg0"
            ]
        ]
    ]
}


//@<OUT> call shell.reports.list_report_testing_argc_asterisk with no arguments
{
    "report": [
        [
            "name",
            "type",
            "value"
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_argc_asterisk with an argument
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "argv",
            "m.Array",
            [
                "arg0"
            ]
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_argc_asterisk with two arguments
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "argv",
            "m.Array",
            [
                "arg0",
                "arg1"
            ]
        ]
    ]
}


//@ call shell.reports.list_report_testing_argc_1_2 with no arguments
||reports.list_report_testing_argc_1_2: Argument #2 'argv' is expecting 1-2 arguments. (ArgumentError)

//@<OUT> call shell.reports.list_report_testing_argc_1_2 with an argument
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "argv",
            "m.Array",
            [
                "arg0"
            ]
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_argc_1_2 with two arguments
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "argv",
            "m.Array",
            [
                "arg0",
                "arg1"
            ]
        ]
    ]
}


//@ call shell.reports.list_report_testing_argc_1_asterisk with no arguments
||reports.list_report_testing_argc_1_asterisk: Argument #2 'argv' is expecting 1-* arguments. (ArgumentError)

//@<OUT> call shell.reports.list_report_testing_argc_1_asterisk with an argument
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "argv",
            "m.Array",
            [
                "arg0"
            ]
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_argc_1_asterisk with two arguments
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "argv",
            "m.Array",
            [
                "arg0",
                "arg1"
            ]
        ]
    ]
}

//@<OUT> call shell.reports.list_report_testing_argc_1_asterisk with three arguments
{
    "report": [
        [
            "name",
            "type",
            "value"
        ],
        [
            "argv",
            "m.Array",
            [
                "arg0",
                "arg1",
                "arg2"
            ]
        ]
    ]
}

//@ WL11263_TSF14_1 - register report - returns undefined
||

//@ WL11263_TSF14_1 - call the report - undefined means an error
||Report should return a dictionary.

//@ WL11263_TSF14_1 - call the report - empty dictionary means an error
||Missing required options: report

//@ WL11263_TSF14_1 - call the report - unknown key means an error
||Invalid and missing options (invalid: unknown), (missing: report)

//@ WL11263_TSF14_1 - call the report - undefined report means an error
||Option 'report' is expected to be of type Array, but is Undefined

//@ WL11263_TSF14_1 - call the report - null report means an error
||Option 'report' is expected to be of type Array, but is Null

//@ WL11263_TSF14_1 - call the report - string report means an error
||Option 'report' is expected to be of type Array, but is String

//@<OUT> WL11263_TSF15_1 - call an existing report, expect tabular output
+-------+---------+---------+
| name  | type    | value   |
+-------+---------+---------+
| five  | Number  | 654.321 |
| four  | Integer | 987     |
| one   | String  | text    |
| three | Bool    | true    |
| two   | String  | query   |
+-------+---------+---------+

//@ WL11263_TSF15_2 - register a report
||

//@<OUT> WL11263_TSF15_2 - call the report
+-------+-------+
| left  | right |
+-------+-------+
| one   | two   |
| three | NULL  |
| four  | five  |
| seven | eight |
+-------+-------+

//@<OUT> WL11263_TSF16_1 - call an existing report, expect vertical output
*************************** 1. row ***************************
 name: five
 type: Number
value: 654.321
*************************** 2. row ***************************
 name: four
 type: Integer
value: 987
*************************** 3. row ***************************
 name: one
 type: String
value: text
*************************** 4. row ***************************
 name: three
 type: Bool
value: true
*************************** 5. row ***************************
 name: two
 type: String
value: query

//@<OUT> WL11263_TSF16_2 - call the existing report
*************************** 1. row ***************************
 left: one
right: two
*************************** 2. row ***************************
 left: three
right: NULL
*************************** 3. row ***************************
 left: four
right: five
*************************** 4. row ***************************
 left: seven
right: eight

//@ WL11263_TSF15_2 - register a 'print' report
||

//@ WL11263_TSF15_2 - call the 'print' report with --vertical option
||print_report_to_test_vertical_option: unknown option --vertical

//@ WL11263_TSF15_2 - call the 'print' report with -E option
||print_report_to_test_vertical_option: unknown option -E

//@ WL11263_TSF15_2 - register a 'report' report
||

//@ WL11263_TSF15_2 - call the 'report' type report with --vertical option
||report_type_report_to_test_vertical_option: unknown option --vertical

//@ WL11263_TSF15_2 - call the 'report' type report with -E option
||report_type_report_to_test_vertical_option: unknown option -E

//@ WL11263_TSF17_1 - register a 'report' type report
||

//@<OUT> WL11263_TSF17_1 - call the 'report' type report - test the output
---
- text
- 2
- 3.14
- |-
    multi
    line
    text
- 1: value
  2: 4
  3: 7.777
  4:
    - 1
    - 2
    - 3
  5:
    key: value

//@ The 'examples' key is set to undefined
||Shell.registerReport: Option 'examples' is expected to be of type Array, but is Undefined (TypeError)

//@ The 'examples' key is set to an invalid type
||Shell.registerReport: Option 'examples' is expected to be of type Array, but is String (TypeError)

//@ The 'examples' key is set to an array of invalid types
||Shell.registerReport: The value associated with the key named 'examples' should be a list of dictionaries. (ArgumentError)

//@ 'examples': Dictionary is missing the required key
||Shell.registerReport: Missing required options: description (ArgumentError)

//@ 'examples': description is null
||Shell.registerReport: Option 'description' is expected to be of type String, but is Null (TypeError)

//@ 'examples': description is undefined
||Shell.registerReport: Option 'description' is expected to be of type String, but is Undefined (TypeError)

//@ 'examples': description is an integer
||Shell.registerReport: Option 'description' is expected to be of type String, but is Integer (TypeError)

//@ 'examples': 'args': is set to undefined
||Shell.registerReport: Option 'args' is expected to be of type Array, but is Undefined (TypeError)

//@ 'examples': 'args': is set to an invalid type
||Shell.registerReport: Option 'args' is expected to be of type Array, but is String (TypeError)

//@ 'examples': 'args': is set to an array of invalid types
||Shell.registerReport: Option 'args' String expected, but value is Bool (TypeError)

//@ 'examples': 'options': is set to undefined
||Shell.registerReport: Option 'options' is expected to be of type Map, but is Undefined (TypeError)

//@ 'examples': 'options': is set to an invalid type
||Shell.registerReport: Option 'options' is expected to be of type Map, but is String (TypeError)

//@ 'examples': 'options': is set to a map of invalid types
||Shell.registerReport: Option 'options' String expected, but value is Integer (TypeError)

//@ 'examples': 'args': invalid number of arguments, expected 0, given 1
||Shell.registerReport: Report expects 0 arguments, example has: 1 (ArgumentError)

//@ 'examples': 'args': invalid number of arguments, expected 1, given 0
||Shell.registerReport: Report expects 1 argument, example has: 0 (ArgumentError)

//@ 'examples': 'args': invalid number of arguments, expected 1, given 2
||Shell.registerReport: Report expects 1 argument, example has: 2 (ArgumentError)

//@ 'examples': 'args': invalid number of arguments, expected 1-3, given 0
||Shell.registerReport: Report expects 1-3 arguments, example has: 0 (ArgumentError)

//@ 'examples': 'args': invalid number of arguments, expected 1-3, given 4
||Shell.registerReport: Report expects 1-3 arguments, example has: 4 (ArgumentError)

//@ 'examples': 'options': unknown long option
||Shell.registerReport: Option named 'op' used in example does not exist. (ArgumentError)

//@ 'examples': 'options': unknown short option
||Shell.registerReport: Option named 'x' used in example does not exist. (ArgumentError)

//@ 'examples': 'options': invalid value for Boolean option
||Shell.registerReport: Option named 'opt' used in example is a Boolean, should have value 'true' or 'false'. (ArgumentError)

//@ 'examples': 'options': invalid value for Boolean option - empty
||Shell.registerReport: Option named 'o' used in example is a Boolean, should have value 'true' or 'false'. (ArgumentError)

//@ 'examples': 'options': invalid value for String option - empty
||Shell.registerReport: Option named 'o' used in example cannot be an empty string. (ArgumentError)

//@ valid 'examples' value
||

//@<OUT> valid 'examples' - --help
NAME
      valid_examples_report -

SYNTAX
      \show valid_examples_report [OPTIONS] [ARGS]
      \watch valid_examples_report [OPTIONS] [ARGS]

DESCRIPTION
      Options:

      --help, -h  Display this help and exit.

      --opt[=string], -o


      --switch, -s


      --idx=integer, -I


      --precision=float, -p


      Arguments:

      This report accepts 0-3 arguments.

EXAMPLES
      \show valid_examples_report 1
            1st

      \show valid_examples_report 2 "3 4"
            2nd

      \show valid_examples_report "5 \"6\"" 7 "8 '9'"
            3rd

      \show valid_examples_report --opt a
            4th

      \show valid_examples_report -o b
            5th

      \show valid_examples_report --opt "c d"
            6th

      \show valid_examples_report --opt "e \"f\""
            7th

      \show valid_examples_report -o "g 'h'"
            8th

      \show valid_examples_report
            9th

      \show valid_examples_report --switch
            10th

      \show valid_examples_report
            11th

      \show valid_examples_report -s
            12th

      \show valid_examples_report --idx 4
            13th

      \show valid_examples_report -I 5
            14th

      \show valid_examples_report --precision 6.7
            15th

      \show valid_examples_report -p 8.910
            16th

      \show valid_examples_report --opt
            17th

      \show valid_examples_report -o
            18th

      \show valid_examples_report -o y x
            19th

//@<OUT> valid 'examples' - function help
NAME
      valid_examples_report

SYNTAX
      shell.reports.valid_examples_report(session[, argv][, options])

WHERE
      session: Object. A Session object to be used to execute the report.
      argv: Array. Extra arguments. Report expects 0-3 arguments.
      options: Dictionary. Options expected by the report.

DESCRIPTION
      This is a 'print' type report.

      The session parameter must be any of ClassicSession, Session.

      The options parameter accepts the following options:

      - opt String.
      - switch Bool.
      - idx Integer.
      - precision Float.

EXAMPLES
      valid_examples_report(session, ["1"], {})
            1st

      valid_examples_report(session, ["2", "3 4"], {})
            2nd

      valid_examples_report(session, ["5 \"6\"", "7", "8 '9'"], {})
            3rd

      valid_examples_report(session, [], {"opt": "a"})
            4th

      valid_examples_report(session, [], {"opt": "b"})
            5th

      valid_examples_report(session, [], {"opt": "c d"})
            6th

      valid_examples_report(session, [], {"opt": "e \"f\""})
            7th

      valid_examples_report(session, [], {"opt": "g 'h'"})
            8th

      valid_examples_report(session, [], {"switch": false})
            9th

      valid_examples_report(session, [], {"switch": true})
            10th

      valid_examples_report(session, [], {"switch": false})
            11th

      valid_examples_report(session, [], {"switch": true})
            12th

      valid_examples_report(session, [], {"idx": 4})
            13th

      valid_examples_report(session, [], {"idx": 5})
            14th

      valid_examples_report(session, [], {"precision": 6.7})
            15th

      valid_examples_report(session, [], {"precision": 8.910})
            16th

      valid_examples_report(session, [], {"opt": ""})
            17th

      valid_examples_report(session, [], {"opt": ""})
            18th

      valid_examples_report(session, ["x"], {"opt": "y"})
            19th

//@ 'empty': invalid value - null
||Shell.registerReport: Option 'empty' is expected to be of type Bool, but is Null (TypeError)

//@ 'empty': invalid value - undefined
||Shell.registerReport: Option 'empty' is expected to be of type Bool, but is Undefined (TypeError)

//@ 'empty': invalid value - string
||Shell.registerReport: Option 'empty' Bool expected, but value is String (TypeError)

//@ 'empty': invalid type - bool
||Shell.registerReport: Only option of type 'string' can accept empty values. (ArgumentError)

//@ valid 'empty' value
||

//@ call report with an empty value
|valid_report_with_empty_option - OK|
