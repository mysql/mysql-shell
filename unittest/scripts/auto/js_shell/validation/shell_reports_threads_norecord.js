//@ create a new session
|<ClassicSession:<<<__mysql_uri>>>>|

//@ create test user
||

//@ create test connection
||

//@ get IDs
||

//@ create database
||

//@ WL11651-TSFR1_1 - Validate that a new report named threads is available in the Shell.
|, threads.|

//@ WL11651-TSFR1_2 - Run the threads report, validate that the info displayed belongs just to the user that executes the report.
||

//@ display help for threads report
||

//@ WL11651-TSFR2_2 - Run the threads report with the --foreground (-f) option, validate that the info displayed list all foreground threads.
||

//@ WL11651-TSFR3_2 - Run the threads report with the --background (-b) option, validate that the info displayed list all background threads.
||

//@ WL11651-TSFR4_2 - Run the threads report with the --all (-A) option, validate that the info displayed list all the threads: foreground and background.
||

//@ WL11651-TSFR4_3 - Run the threads report with the --foreground and --background options, validate that the info displayed list all the threads: foreground and background.
||

//@<OUT> WL11651-TSFR5_1 - Run the threads report, validate that the output of the report is presented as a list-type report.
+--------------+---------+
| user         | command |
+--------------+---------+
| threads_test | Sleep   |
+--------------+---------+

//@<OUT> WL11651-TSFR5_1 - Run the threads report, validate that the output of the report is presented as a list-type report - vertical.
*************************** 1. row ***************************
   user: threads_test
command: Sleep

//@ WL11651-TSFR6_1 - Run the threads report, validate that the default columns in the report are: tid, cid, user, host, db, command, time, state, txstate, info, nblocking.
||

//@ WL11651-TSFR6_2 - Run the threads report with --background option, validate that the default columns in the report are: tid, name, nio, ioltncy, iominltncy, ioavgltncy, iomaxltncy.
||

//@ WL11651-TSFR7_2 - null
||reports.threads: Option 'format' is expected to be of type String, but is Null (TypeError)

//@ WL11651-TSFR7_2 - undefined
||reports.threads: Option 'format' is expected to be of type String, but is Undefined (TypeError)

//@ WL11651-TSFR7_2 - bool
||reports.threads: Argument #3, option 'format' is expected to be a string (TypeError)

//@<OUT> WL11651-TSFR7_2 - string
{
    "report": [
        [
            "TID",
            "CID"
        ],
        [
            <<<__session_ids.tid>>>,
            <<<__session_ids.cid>>>
        ]
    ]
}

//@ WL11651-TSFR7_3 - Validate that the --format (-o) option can be used to specify the order of the columns in the report.
||

//@ WL11651-TSFR7_4 - Validate that the --format (-o) option can be used to specify alias for the columns in the report.
||

//@ WL11651-TSFR7_5 - Validate that all the columns listed in FR5 can be requested using the --format (-o) option.
||

//@ status - run without variable name
||reports.threads: Cannot use 'status' in 'format' parameter: Column 'status' requires a variable name, i.e.: status.NAME

//@ system - run without variable name
||reports.threads: Cannot use 'system' in 'format' parameter: Column 'system' requires a variable name, i.e.: system.NAME

//@ WL11651-TSFR7_6 - empty value
||threads: option --format requires an argument

//@ WL11651-TSFR7_6 - empty value - function call
||reports.threads: The 'format' parameter cannot be empty. (ArgumentError)

//@ WL11651-TSFR7_6 - invalid format
||reports.threads: Cannot use 'tid cid' in 'format' parameter: Unknown column name: tid cid

//@ WL11651-TSFR7_7 - Use the --format (-o) option to request columns different than the ones listed in FR5, validate that an exception is thrown.
||reports.threads: Cannot use 'invalidcolumn' in 'format' parameter: Unknown column name: invalidcolumn

//@ WL11651-TSFR7_8 - Use the --format (-o) option to request columns with certain prefix, validate that all columns matching this prefix are displayed and the alias is ignored.
||

//@ WL11651-TSFR7_8 - prefix does not match any column
||reports.threads: Cannot use 'k' in 'format' parameter: Unknown column name: k

//@ WL11651-TSFR7_9 - Use the --format (-o) option to request a column listed in FR5 but using an upper-case name.
||

//@ WL11651-TSFR8_2 - Validate that all the columns listed in FR5 can be used as a key to filter the results of the report.
||

//@ WL11651-TSFR8_3 - Validate that the use of key names different than the columns listed in FR5 causes an exception.
||reports.threads: Failed to parse 'where' parameter: Unknown column name: invalidcolumn

//@ WL11651-TSFR8_4 - Try to use non SQL values as the value for a key using the --where option, an exception should be thrown.
||reports.threads: Failed to parse 'where' parameter: Unknown column name: one

//@ WL11651-TSFR8_5 - Validate that the following relational operator can be used with the --where option: =, !=, <>, >, >=, <, <=, LIKE.
||

//@ WL11651-TSFR8_6 - Validate that the --where option support the following logical operators: AND, OR, NOT. (case insensitive)
||

//@ WL11651-TSFR8_7 - Validate that the --where option support multiple logical operations.
||

//@ WL11651-TSFR8_9 - empty value
||threads: option --where requires an argument

//@ WL11651-TSFR8_9 - empty value - function call
||reports.threads: The 'where' parameter cannot be empty. (ArgumentError)

//@<ERR> WL11651-TSFR8_10 - Validate that an exception is thrown if the --where option receive an argument that doesn't comply with the form specified.
reports.threads: Failed to parse 'where' parameter: Expected end of expression, at position 4,
in: one two
        ^^^

//@ WL11651-TSFR9_2 - When the --order-by option is not used, validate that the report is ordered by the tid column.
||

//@ WL11651-TSFR9_3 - When the --order-by option is used, validate that the report is ordered by the column specified.
||

//@ WL11651-TSFR9_4 - Use multiple columns with the --order-by option, validate that the report is ordered by the columns specified.
||

//@ WL11651-TSFR9_5 - Validate that the --order-by only accepts the columns listed in FR5 as valid columns to order the report.
||

//@ WL11651-TSFR9_X - Validate that the --order-by throws an exception if column is unknown.
||reports.threads: Cannot use 'invalidcolumn' as a column to order by: Unknown column name: invalidcolumn

//@ WL11651-TSFR10_2 - When using the --desc option, validate that the output is sorted in descending order.
||

//@ WL11651-TSFR10_3 - When not using the --desc option, validate that the output is sorted in ascending order.
||

//@ WL11651-TSFR11_2 - When using the --limit option, validate that the output is limited to the number of threads specified.
||

//@ cleanup - delete the database
||

//@ cleanup - delete the user
||

//@ cleanup - disconnect test session
||

//@ cleanup - disconnect the session
||
