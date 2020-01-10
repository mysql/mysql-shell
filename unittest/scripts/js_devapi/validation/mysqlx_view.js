//@ Testing view name retrieving
|getName(): view1|
|name: view1|

//@ Testing session retrieving
|getSession(): <Session:|
|session: <Session:|

//@ Testing view schema retrieving
|getSchema(): <Schema:js_shell_test>|
|schema: <Schema:js_shell_test>|

//@<OUT> Testing view select
*************************** 1. row ***************************
my_name: John Doe

//@<OUT> Testing view update
*************************** 1. row ***************************
my_name: John Lock

//@<OUT> Testing view insert
*************************** 1. row ***************************
my_name: John Lock
*************************** 2. row ***************************
my_name: Jane Doe

//@ Testing view delete
|Query OK, 2 items affected|

//@ Testing existence
|Valid: true|
|Invalid: false|

//@ Testing view check
|Is View: true|


