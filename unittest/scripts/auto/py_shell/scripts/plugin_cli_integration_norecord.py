#@<> Initialization
import os

user_path = testutil.get_user_config_path()
plugins_path = os.path.join(user_path, "plugins")
plugin_folder_path = os.path.join(plugins_path, "cli_tester")
plugin_path =  os.path.join(plugin_folder_path, "init.py")
testutil.mkdir(plugin_folder_path, True)

def call_mysqlsh(command_line_args):
    testutil.call_mysqlsh(command_line_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

plugin_code = """
def cli_tester(stritem, strlist, namedstr, options):
    print("String Parameter: " + stritem)

    for item in strlist:
        print("String List Parameter Item: " + item)

    print("String Named Item: " + namedstr)

    print("String Option: " + options["str"])

    for data in options["strlist"]:
      print("String List Option Item: " +  data)


def interactive_tester():
    print("Interactive mode function success!!!")

obj = shell.create_extension_object()
shell.add_extension_object_member(obj, "test", cli_tester, {
  "brief": "Testing cmd line args.",
  "cli": True,
  "parameters":[
      {
          "name": "stritem",
          "brief": "string parameter",
          "type": "string",
      },
      {
          "name": "strlist",
          "brief": "string list parameter",
          "type": "array",
          "itemtype": "string"
      },
      {
          "name": "namedstr",
          "brief": "string named parameter (for being after a list parameter)",
          "type": "string",
      },
      {
          "name":"options",
          "brief": "options to test string handling",
          "type": "dictionary",
          "options": [
              {
                  "name":"str",
                  "brief": "string option",
                  "type": "string"
              },
              {
                  "name":"strlist",
                  "brief": "string list option",
                  "type": "array",
                  "itemtype": "string"
              }
          ]
      }
  ]
})

shell.add_extension_object_member(obj, "testInteractive", interactive_tester, {
  "brief": "Testing interactive function.",
  "cli": False
})

empty_child = shell.create_extension_object()

def grand_child_function(stritem):
    print("Unique Parameter: " + stritem)

def print_more_nested_info(param_a):
    print("Interactive Parameter In Nested Function: " + param_a)

grand_child = shell.create_extension_object()
shell.add_extension_object_member(grand_child, "grandChildFunction", grand_child_function, {
  "brief": "Testing cmd line args in nested objects.",
  "cli": True,
  "parameters":[
      {
          "name": "stritem",
          "brief": "string parameter",
          "type": "string",
      }
    ]
});

shell.add_extension_object_member(grand_child, "printMoreNestedInfo", print_more_nested_info, {
  "brief": "Testing cmd line args in nested objects.",
  "parameters":[
      {
          "name": "param_a",
          "brief": "string parameter",
          "type": "string",
      }
    ]
});

shell.add_extension_object_member(empty_child, "grandChild", grand_child, {
  "brief": "Grand child object exposing a function."
})

shell.add_extension_object_member(obj, "emptyChild", empty_child, {
  "brief": "Empty object, exposes no functions but child does."
})

shell.register_global('cli_tester', obj, {"brief": "CLI Integration Testing Plugin"})
"""
testutil.create_file(plugin_path, plugin_code)

# WL14297 - TSFR_1_1_3
# WL14297 - TSFR_10_1_1
# WL14297 - TSFR_11_1_3
#@ CLI --help
call_mysqlsh(["--", "--help"])

#@ CLI -h [USE:CLI --help]
call_mysqlsh(["--", "-h"])

#@ CLI plugin --help
call_mysqlsh(["--", "cli_tester", "--help"])

#@ CLI plugin -h [USE:CLI plugin --help]
call_mysqlsh(["--", "cli_tester", "-h"])

#@ CLI plugin function --help
call_mysqlsh(["--", "cli_tester", "test", "--help"])

#@ CLI plugin function -h [USE:CLI plugin function --help]
call_mysqlsh(["--", "cli_tester", "test", "-h"])

#@ Interactive plugin function --help
call_mysqlsh(["--", "cli_tester", "test-interactive", "--help"])

#@ Interactive plugin function -h [USE:Interactive plugin function --help]
call_mysqlsh(["--", "cli_tester", "test-interactive", "-h"])

#@ Interactive plugin function help
call_mysqlsh(["-i", "-e", "cli_tester.help(\"testInteractive\")"])

#@ CLI plugin nested child --help
call_mysqlsh(["--", "cli_tester", "emptyChild", "--help"])

#@ CLI plugin nested child -h [USE:CLI plugin nested child --help]
call_mysqlsh(["--", "cli_tester", "emptyChild", "-h"])

#@ CLI plugin nested grand child --help
call_mysqlsh(["--", "cli_tester", "emptyChild", "grandChild", "--help"])

#@ CLI plugin nested grand child -h [USE:CLI plugin nested grand child --help]
call_mysqlsh(["--", "cli_tester", "emptyChild", "grandChild", "-h"])

#@ CLI plugin nested grand child function --help
call_mysqlsh(["--", "cli_tester", "emptyChild", "grandChild", "--help"])

#@ CLI plugin nested grand child function -h [USE:CLI plugin nested grand child function --help]
call_mysqlsh(["--", "cli_tester", "emptyChild", "grandChild", "-h"])

#@ Test string list handling
# - In list parameters, comma is used as item delimiter
call_mysqlsh(["--", "cli_tester", "test", "1,2,3,4,5", "1,2", "3", "4,5", "--namedstr=1,2,3,4,5", "--str=1,2,3,4,5", "--strlist=1,2", "--strlist=3", "--strlist=4,5"])

#@ Test string list quoted args for interpreter
# - In list parameters quoted values are turned into a single item
call_mysqlsh(["--", "cli_tester", "test", "1,2,3,4,5", "\"1,2\"", "3", "\"4,5\"", "--namedstr=1,2,3,4,5", "--str=1,2,3,4,5", "--strlist=\"1,2\"", "--strlist=3", "--strlist=\"4,5\""])

#@ Escaped comma in list parsing
# - In list parameters, the comma is NOT used as item delimiter
call_mysqlsh(["--", "cli_tester", "test", "1,2,3,4,5", "1\\,2", "3", "4\\,5", "--namedstr=1,2,3,4,5", "--str=1,2,3,4,5", "--strlist=1\\,2", "--strlist=3", "--strlist=4\\,5"])

#@ Escaped quoting: \", \'
# - In simple strings the escaped char (\") will be turned into the quote (").
# - In list parameters it will be used as item delimiter
call_mysqlsh(["--", "cli_tester", "test", "\"1\",2,3,4,5", "\"1\",2", "3", "\"4\",5", "--namedstr=1,2,\"3\",4,5", "--str=1,2,\"3\",4,5", "--strlist=\"1\",2", "--strlist=3", "--strlist=\"4\",\"5\""])
call_mysqlsh(["--", "cli_tester", "test", "\'1\',2,3,4,5", "\'1\',2", "3", "\'4\',5", "--namedstr=1,2,\'3\',4,5", "--str=1,2,\'3\',4,5", "--strlist=\'1\',2", "--strlist=3", "--strlist=\'4\',\'5\'"])

#@ Escaped equal: \=
# - It is turned into the target character (=)
call_mysqlsh(["--", "cli_tester", "test", "\\=1,2,3,4,5", "1\\=2", "3", "4\\=5", "--namedstr=\\=1,2,3,4\\=5", "--str=\\=1,2,3,4\\=5", "--strlist=1\\=2", "--strlist=3", "--strlist=4\\=5"])

#@ CLI calling plugin nested grand child function
call_mysqlsh(["--", "cli_tester", "emptyChild", "grandChild", "grand-child-function", "Success!!"])

#<> Calling non CLI plugin function
call_mysqlsh(["-i", "-e" "cli_tester.testInteractive()"])
EXPECT_STDOUT_CONTAINS("Interactive mode function success!!!")

#<> Calling non CLI plugin function from CLI
call_mysqlsh(["--", "cli_tester", "test-interactive"])
EXPECT_STDOUT_CONTAINS("ERROR: Invalid operation for cli_tester object: test-interactive")

#<> Calling non CLI nested plugin function
call_mysqlsh(["-i", "-e" "cli_tester.emptyChild.grandChild.printMoreNestedInfo('Success!')"])
EXPECT_STDOUT_CONTAINS("Interactive Parameter In Nested Function: Success!")

#<> Calling non CLI nested plugin function from CLI
call_mysqlsh(["--", "cli_tester", "emptyChild", "grandChild", "print-more-nested-info", "Failure"])
EXPECT_STDOUT_CONTAINS("ERROR: Invalid operation for grandChild object: print-more-nested-info")



#@<> CLI mapper properly takes default values from plugin optional parameters
# NOTE: The default values for the print_defaults function differ with the ones
# registered in the plugin on purpose, this will be used to demonstrate who is
# passing the default values to the call.
# In general they would never differ, and indeed, this inconsistency is not possible
# when using the plugin and plugin_function decorators.
plugin_code = """
def print_defaults(one=1, two="python default", three={"option1": "python option1 default"}, four={"option2": "python option2 default"}):
    print(one, two, three, four)

obj = shell.create_extension_object()
shell.add_extension_object_member(obj, "printDefaults", print_defaults, {
  "brief": "Testing default values on plugins.",
  "cli": True,
  "parameters":[
      {
          "name": "one",
          "brief": "int parameter",
          "type": "integer",
          "default": 2,
          "required": False
      },
      {
          "name": "two",
          "brief": "string parameter",
          "type": "string",
          "default": "plugin default",
          "required": False
      },
      {
          "name": "three",
          "brief": "optional dictionary parameter",
          "type": "dictionary",
          "options": [
            {
                "name": "option1",
                "type": "string",
            }
          ],
          "default": {"option1": "plugin option1 default"},
          "required": False
      },
      {
          "name": "four",
          "brief": "optional dictionary parameter",
          "type": "dictionary",
          "required": False,
          "options": [
            {
                "name": "option2",
                "type": "string",
            }
          ],
          "default": {"option2": "plugin option2 default"},
      }
  ]
})

shell.register_global('cli_tester', obj, {"brief": "CLI Integration Testing Plugin"})
"""

testutil.rmfile(plugin_path)
testutil.create_file(plugin_path, plugin_code)

#@<> Using all default values
rc = call_mysqlsh(["--", "cli_tester", "print-defaults"])
EXPECT_STDOUT_CONTAINS("2 plugin default {\"option1\": \"plugin option1 default\"} {\"option2\": \"plugin option2 default\"}")
WIPE_OUTPUT()

#@<> Using explicit value for 'three', CLI mapper handles defaults for 'one' and 'two'
rc = call_mysqlsh(["--", "cli_tester", "print-defaults", "--option1", "explicit value for option1"])
EXPECT_STDOUT_CONTAINS("2 plugin default {\"option1\": \"explicit value for option1\"} {\"option2\": \"plugin option2 default\"}")
WIPE_OUTPUT()

#@<> CLI mapper handles defaults for 'one', 'two' and 'three'
rc = call_mysqlsh(["--", "cli_tester", "print-defaults", "--option2", "explicit value for option2"])
EXPECT_STDOUT_CONTAINS("2 plugin default {\"option1\": \"plugin option1 default\"} {\"option2\": \"explicit value for option2\"}")
WIPE_OUTPUT()


#@<> Plugin code to demonstrate missing mandatory arguments generate an error
plugin_code = """
def print_element(key, elements):
    print(f"{elements[key]} ({key})")

obj = shell.create_extension_object()
shell.add_extension_object_member(obj, "printElement", print_element, {
  "brief": "Testing mandatory arguments in a plugin.",
  "cli": True,
  "parameters":[
      {
          "name": "key",
          "brief": "identifies the element to be printed",
          "type": "string",
      },
      {
          "name": "elements",
          "brief": "dictionary of printable elements",
          "type": "dictionary",
          "default": {"one": "default value for one", "two": "default value for two"},
          "required": False
      },
  ]
})

shell.register_global('custom_plugin', obj, {"brief": "CLI Integration Testing Plugin"})
"""

testutil.rmfile(plugin_path)
testutil.create_file(plugin_path, plugin_code)

#@<> Ensures documented default elements are used (the exposed function has no defaults)
rc = call_mysqlsh(["--", "custom_plugin", "print-element", "one"])
EXPECT_STDOUT_CONTAINS("default value for one (one)")
WIPE_OUTPUT()

#@<> Attempts printing unexisting default element
rc = call_mysqlsh(["--", "custom_plugin", "print-element", "item"])
EXPECT_STDOUT_CONTAINS("KeyError: 'item'")
WIPE_OUTPUT()

#@<> Prints custom element
rc = call_mysqlsh(["--", "custom_plugin", "print-element", "item", "--item", "this is a custom element"])
EXPECT_STDOUT_CONTAINS("this is a custom element (item)")
WIPE_OUTPUT()

#@<> Testing default elements are overriden by the new defaults
rc = call_mysqlsh(["--", "custom_plugin", "print-element", "one", "--item", "this is a custom element"])
EXPECT_STDOUT_CONTAINS("KeyError: 'one'")
WIPE_OUTPUT()


#@<> Tests missing mandatory argument
# NOTE: this is possible in CLI calls when a dictionary or list comes after other parameters
rc = call_mysqlsh(["--", "custom_plugin", "print-element", "--item='this is a custom element'"])
EXPECT_STDOUT_CONTAINS("ERROR: Missing value for required 'key' parameter.")
WIPE_OUTPUT()


#@<> Tests default values are used even in API calls
rc = call_mysqlsh(["-e", "custom_plugin.printElement('two')"])
EXPECT_STDOUT_CONTAINS("default value for two (two)")
WIPE_OUTPUT()


#@<> Testing integer like parameter with leading 0's is interpreted verbatim
rc = call_mysqlsh(["--", "custom_plugin", "print-element", "00456", "--00456", "this is a custom digits element"])
EXPECT_STDOUT_CONTAINS("this is a custom digits element (00456)")
WIPE_OUTPUT()


#@<> Testing float like parameter with leading 0's is interpreted verbatim
rc = call_mysqlsh(["--", "custom_plugin", "print-element", "00.456", "--00.456", "this is a custom digits.digits element"])
EXPECT_STDOUT_CONTAINS("this is a custom digits.digits element (00.456)")
WIPE_OUTPUT()

#@<> Finalization
testutil.rmdir(plugins_path, True)
