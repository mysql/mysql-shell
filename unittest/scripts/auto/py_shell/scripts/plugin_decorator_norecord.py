#@<> Initialization
import os

plugin_code = '''
from mysqlsh.plugin_manager import plugin, plugin_function

@plugin
class decorator:
    """
    Brief description of a decorated plugin.

    This plugin will test that the object and functions can be properly
    registered using python decorators removing the burden of having to call
    the shell APIs for this purpose.

    On this case, a global object will be registered using as name this class
    name.
    """

    class inner:
        """
        Brief description of a child plugin object.

        This is a plugin object registered as a child of the decorator plugin,
        it is used to verify that child classes are good to define a composed
        plugin structure.
        """
        pass

    pass

@plugin_function("decorator.testSimpleTypes", cli=True)
def test_simple_types(aString, anInt, aBool, aDict, aList, anUndefined):
    """
    Tests documentation for simple types, no options defined.

    Args:
        aString (str): A string parameter.
        anInt (int): An integer parameter.
        aBool (bool): A boolean parameter.
        aDict (dict): A dictionary parameter.
        aList (list): A list parameter.
        anUndefined: A parameter with no type.

    The simple types function is only used to test how the simple parameters
    are properly documented using the decorator plugins.
    """
    print("aString value:", aString)
    print("anInt value:", anInt)
    print("aBool value:", aBool)
    print("aDict value:", aDict)
    print("aList value:", aList)
    print("anUndefined value:", anUndefined)

@plugin_function("decorator.testOptionalParameters", cli=True)
def test_optional_parameters(aString, anInt, aBool, aDict, aList=[1,2,3], anUndefined=None):
    """
    Tests documentation for optional parameters.

    Args:
        aString (str): A string parameter.
        anInt (int): An integer parameter.
        aBool (bool): A boolean parameter.
        aDict (dict): A dictionary parameter.
        aList (list): A list parameter.
        anUndefined: A parameter with no type.

    The optional parameters function is only used to test how the parameters
    with default values are documented as optional params.
    """
    print("aString value:", aString)
    print("anInt value:", anInt)
    print("aBool value:", aBool)
    print("aDict value:", aDict)
    print("aList value:", aList)
    if anUndefined is not None:
        print("anUndefined value:", anUndefined)


@plugin_function("decorator.inner.testOptions", cli=True)
def test_options(stritem, options=None):
    """
    Tests some parameter documentation.

    Args:
        stritem (str): String parameter.
        options (dictionary): Options to whatever.

    Allowed options for options:
        strOption (str): String parameter.
        intOption (int): String List parameter.
        anyTypeOption: Any type option.

    The options function is only used to test how the options for a
    dictionary are properly documented.

    """
    print("stritem value:", stritem)
    if options is not None:
        print("options:", options)
'''

user_path = testutil.get_user_config_path()
plugins_path = os.path.join(user_path, "plugins")
plugin_folder_path = os.path.join(plugins_path, "cli_tester")
plugin_path =  os.path.join(plugin_folder_path, "init.py")
testutil.mkdir(plugin_folder_path, True)
testutil.create_file(plugin_path, plugin_code)

# Using the Plugin In JavaScript
# ==============================
#@<> Lists help of plugin (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "\\? decorator"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin child object (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "\\? decorator.inner"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin function for simple type parameters (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "\\? decorator.testSimpleTypes"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin function with optional parameters (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "\\? decorator.testOptionalParameters"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin function with options (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "\\? decorator.inner.testOptions"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling simple types function (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.testSimpleTypes('one', 2, false, {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with optionals 1 (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.testOptionalParameters('two', 1, true, {whateverOption:'whateverValue'})"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with optionals 2 (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.testOptionalParameters('two', 1, true, {whateverOption:'whateverValue'}, [4,5,6])"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with optionals 3 (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.testOptionalParameters('two', 1, true, {whateverOption:'whateverValue'}, [4,5,6], {anykey:'anyValue'})"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with options 1 (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.inner.testOptions('Passing No Options')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with options 2 (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.inner.testOptions('Passing Options', {strOption:'String Option Value', intOption: 45, anyTypeOption:[1,'one',true]})"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Function Call Errors (js)
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.testSimpleTypes(1, 2, false, {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.testSimpleTypes('one', 'two', false, {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.testSimpleTypes('one', 2, 'error', {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.testSimpleTypes('one', 2, false, [1,2], [1,2,3], 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.testSimpleTypes('one', 2, false, {whateverOption:'whateverValue'}, 'error', 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.testSimpleTypes('one', 2, false, {whateverOption:'whateverValue'}, [1,2,3])"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["-ifull", "-e", "decorator.inner.testOptions('Passing Options', {invalidOption:'String Option Value'})"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

# Using the Plugin In Python
# ==========================
#@<> Lists help of plugin (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "\\? decorator"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin child object (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "\\? decorator.inner"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin function for simple type parameters (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "\\? decorator.test_simple_types"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin function with optional parameters (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "\\? decorator.test_optional_parameters"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin function with options (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "\\? decorator.inner.test_options"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling simple types function (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.test_simple_types('one', 2, False, {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with optionals 1 (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.test_optional_parameters('two', 1, True, {'whateverOption':'whateverValue'})"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with optionals 2 (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.test_optional_parameters('two', 1, True, {'whateverOption':'whateverValue'}, [4,5,6])"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with optionals 3 (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.test_optional_parameters('two', 1, True, {'whateverOption':'whateverValue'}, [4,5,6], {'anykey':'anyValue'})"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with options 1 (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.inner.test_options('Passing No Options')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with options 2 (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.inner.test_options('Passing Options', {'strOption':'String Option Value', 'intOption': 45, 'anyTypeOption':[1,'one',True]})"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Function Call Errors (py)
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.test_simple_types(1, 2, False, {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.test_simple_types('one', 'two', False, {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.test_simple_types('one', 2, 'error', {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.test_simple_types('one', 2, False, 'error', [1,2,3], 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.test_simple_types('one', 2, False, {'whateverOption':'whateverValue'}, 'error', 'Some Value')"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.test_simple_types('one', 2, False, {'whateverOption':'whateverValue'}, [1,2,3])"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])
rc = testutil.call_mysqlsh(["--py", "-ifull", "-e", "decorator.inner.test_options('Passing Options', {'invalidOption':'String Option Value'})"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])


###########################
# Using the Plugin As CLI
# ==========================
#@<> Lists help of plugin (cli)
rc = testutil.call_mysqlsh(["--quiet-start=2", "--", "decorator", "--help"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin child object (cli)
rc = testutil.call_mysqlsh(["--quiet-start=2", "--", "decorator", "inner", "--help"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin function for simple type parameters (cli)
rc = testutil.call_mysqlsh(["--quiet-start=2", "--", "decorator", "test_simple_types", "--help"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Lists help of plugin function with options (cli)
rc = testutil.call_mysqlsh(["--quiet-start=2", "--", "decorator", "inner", "test_options", "--help"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling simple types function (cli)
rc = testutil.call_mysqlsh(["--quiet-start=2", "--", "decorator", "test_simple_types", 'one', "2", "false", '--whateverOption=whateverValue', "1,2,3", "--an-undefined=\'Some Value\'"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with optionals 1 (cli)
rc = testutil.call_mysqlsh(["--quiet-start=2", "--", "decorator", "test_optional_parameters", 'two', "1", "true", '--whateverOption=whateverValue'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with optionals 2 (cli)
rc = testutil.call_mysqlsh(["--quiet-start=2", "--", "decorator", "test_optional_parameters", 'two', "1", "True", "--whateverOption=whateverValue", "4,5,6"], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with optionals 3 (cli)
rc = testutil.call_mysqlsh(["--quiet-start=2", "--", "decorator", "test_optional_parameters", 'two', "1", "True", '--whateverOption=whateverValue', "4,5,6", '--anykey=anyValue'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Test calling function with options 1 (cli)
rc = testutil.call_mysqlsh(["--quiet-start=2", "--", "decorator", "inner", "test_options", 'Passing No Options'], "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

#@<> Finalization
testutil.rmdir(plugins_path, True)