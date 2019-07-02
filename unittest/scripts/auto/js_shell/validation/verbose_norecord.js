//@<OUT> Tests verbose output of the shell (level 0)
0

//@<OUT> Tests verbose output of the shell (level 1)
1

//@<ERR> Tests verbose output of the shell (level 1)
verbose: [[*]]: error: ==error
verbose: [[*]]: warning: ==warning
verbose: [[*]]: ==info

//@<OUT> Tests verbose output of the shell (level 2)
2

//@<ERR> Tests verbose output of the shell (level 2)
verbose: [[*]]: error: ==error
verbose: [[*]]: warning: ==warning
verbose: [[*]]: ==info
verbose: [[*]]: ==debug

//@<OUT> Tests verbose output of the shell (level 3)
3

//@<ERR> Tests verbose output of the shell (level 3)
verbose: [[*]]: error: ==error
verbose: [[*]]: warning: ==warning
verbose: [[*]]: ==info
verbose: [[*]]: ==debug
verbose: [[*]]: ==debug2

//@<OUT> Tests verbose output of the shell (level 4)
4

//@<ERR> Tests verbose output of the shell (level 4)
verbose: [[*]]: error: ==error
verbose: [[*]]: warning: ==warning
verbose: [[*]]: ==info
verbose: [[*]]: ==debug
verbose: [[*]]: ==debug2
verbose: [[*]]: ==debug3

//@# Check default value of --verbose on startup
|verbose= 0|
|0|
|verbose= 1|
|0|
|verbose= 2|
|0|
