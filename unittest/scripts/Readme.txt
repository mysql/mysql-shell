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

example in dwvapi/parameter_binding.js:
// Assumptions: test schema assigned to db, my_collection collection exists

These assumptions are meant to be satisfied by a "Setup Script" which will be executed prior to the execution of the test script.


Setup Script
------------
A setup script is meant to satisfy the assumptions required by the test scripts in a test group.

The setup script will contain a function to satisfy every existing assumption.

Its processing logic is as simple as iterating over the assumptions defined on a test script and calling the corresponding function.

The assumption for every executed test case will be available on the setup script on a variable named __assumptions__.


Validation Script
-----------------

A validation script will be used to test the results of executing a test script, it is composed of lines in the next format:

line number|code to execute|expected output|expected error

code to execute: Contains a statement to be executed after the indicated "line number" has been executed in the test script.
expected output: Contains text to be searched on the stdout, if not found an error will be raised 
expected error: Contains text to be searched on the stderrif not found an error will be raised

Remarks:
- When the test scripts are executed in batch mode the line number is ignored, all the validations occur after the test script was executed.
- If there's no "code to execute" the validations will occur over the stdour and stderr produced by the execution of the test script.
- Tipically expected output and expected error are exclusive.


Usage
-----
To create a unit test validating scripts you must:
Create a test class inheriting from Shell_script_tester
Call set_config_folder(name) passing as name the test group being tested.
Call set_setup_script(name) to indicate the setup script to be used on the script testing.
Call either validate_batch(name) or validate_interactive(name) for every test script that will use the configured setup script.

See test_devapi_samples_t.cc for usage example.


