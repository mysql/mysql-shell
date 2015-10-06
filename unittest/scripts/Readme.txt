Testing Full Scripts
--------------------

This folder contains test data for specific test groups.

Every test group will contain a folder on this path, devap is the first test group available.

A test group folder contains three subfolders:
- scripts: contains "Test Scripts"
- setup: contains "Setup Scripts"
- validation: contains "Validation Scripts"


Test Scripts
------------
A test script is any script that needs to be tested using the shellcore.
A test script must contain a set of "Assumptions" for successful execution.
These assumptions will be defined at the top of the script with a comment like:
// Assumption: <comma separated list of assumptions>

example in devapi/parameter_binding.js:
// Assumptions: test schema assigned to db, my_collection collection exists

These assumptions are meant to be satisfied by a "Setup Script" which will be executed prior to the execution of the test script.

Scripts can be tested by executing them in batch mode or in interactive mode.

Batch Mode: The whole script will be processed at once and the validation script will be done after the whole test script has been executed.

Interactive Mode: The script is processed in chunks or groups of lines of code. A Chunk is defined by a comment like:

//@ Example of running a chunk

All the lines of code under that comment will be grouped together until a new chunk definition is found.

The validation script MUST also define the validations that will be executed after the chunk is executed.

The way to define such validations is by grouping them with an identical chunk comment.

//@# Example of running a chunk line by line
Every line on the chunk will be executed one by one until a new chunk definition is found.

This can be useful when the code in the chunk will produce errors, when an error is produced the lines after the error are not executed.

With this approach, since every line is executed separatedly, even they produce errors, the subsequent lines will be executed.

This can be useful i.e. like to group error validations.


Setup Script
------------
A setup script is meant to satisfy the assumptions required by the test scripts in a test group.

The setup script will contain a function to satisfy every existing assumption.

Its processing logic is as simple as iterating over the assumptions defined on a test script and calling the corresponding function.

The assumption for every executed test case will be available on the setup script on a variable named __assumptions__.


Validation Script
-----------------

A validation script will be used to test the results of executing a test script, it is composed of lines in the next format:

code to execute|expected output|expected error

code to execute: Contains a statement to be executed after the code in the test script has been executed.
expected output: Contains text to be searched on the stdout, if not found an error will be raised.
expected error: Contains text to be searched on the stderrif not found an error will be raised

Remarks:
- When the test scripts are executed in batch mode the line number is ignored, all the validations occur after the test script was executed.
- If there's no "code to execute" the validations will occur over the stdour and stderr produced by the execution of the test script.
- Tipically expected output and expected error are exclusive.

REMEMBER: if you are testing a script in interactive mode, you must define the validation groups for each executed chunk on the test script.

Usage
-----
To create a unit test validating scripts you must:
Create a test class inheriting from Shell_script_tester
Call set_config_folder(name) passing as name the test group being tested.
Call set_setup_script(name) if applicable, this is to indicate the setup script to be used to satisfy the assumptions on the test script.
Call either validate_batch(name) or validate_interactive(name) for every test script that will use the configured setup script.

See test_devapi_samples_t.cc for usage example of batch executed script.
See shell_js_mysqlx_session_t.cc for usaje example of interactive executed script.
