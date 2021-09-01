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

@plugin_function("decorator.testRequiredDictParams", cli=True)
def test_required_dict_params(first, dict_param=dict()):
    """
    Tests documentation for dictionary required parameters.

    Args:
        first (str): First required parameter.
        dict_param (dict): Options.

    Allowed options for dict_param:
        param1 (str,required): First parameter
        param2 (str,required): Second parameter
        description (str): Description

    """
    print("first value:", first)
    print("dict:", dict_param)

@plugin_function("decorator.inner.testOptions", cli=True)
def test_options(stritem, options=None):
    """
    Tests some parameter documentation.

    Args:
        stritem (str): String parameter.
        options (dict): Options to whatever.

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

# This function is to test that option registration is done correctly in the
# following cases:
# - Dict parameter with options, documented as "dict"
# - Dict parameter with options, documented as "dictionary"
# - Dict parameter with no options, documented as "dict"
# - Dict parameter with no options, documented as "dictionary"
#
# In addition does the same testing for nested dictionary options
# - Dict option with options, documented as "dict"
# - Dict option with options, documented as "dictionary"
# - Dict option with no options, documented as "dict"
# - Dict option with no options, documented as "dictionary"
@plugin_function("decorator.inner.testMoreOptions", cli=True)
def test_more_options(dictOne, dictTwo, dictThree, dictFour):
    """
    Tests some parameter documentation.

    Args:
        dictOne (dict): Dictionary with specific options.
        dictTwo (dict): Dictionary with specific options.
        dictThree (dict): Open dictionary.
        dictFour (dict): Open dictionary.

    Allowed options for dictOne:
        optOne (dict): Dictionary option.
        optTwo (dict): Dictionary option.

    Allowed options for dictTwo:
        optThree (dict): Dictionary option.
        optFour (dict): Dictionary option.

    This test ensures the dictionaries and options are properly parsed
    based on the documentation.

    Allowed options for optOne:
        strSample (str): sample option

    Allowed options for optThree:
        strLast (str): sample option
    """
    pass
'''

user_path = testutil.get_user_config_path()
plugins_path = os.path.join(user_path, "plugins")
plugin_folder_path = os.path.join(plugins_path, "cli_tester")
plugin_path =  os.path.join(plugin_folder_path, "init.py")
testutil.mkdir(plugin_folder_path, True)
testutil.create_file(plugin_path, plugin_code)

def __call_mysqlsh(cmdline_args):
    return testutil.call_mysqlsh(["--quiet-start=2"] + cmdline_args, "", ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path])

def call_mysqlsh_e(e_arg, py=False):
    return __call_mysqlsh((["--py"] if py else []) + ["-ifull", "-e"] + [e_arg])

def call_mysqlsh_py_e(e_arg):
    return call_mysqlsh_e(e_arg, True)

def call_mysqlsh_cli(*cmdline_args):
    return __call_mysqlsh(["--"] + [arg for arg in cmdline_args])

# Using the Plugin In JavaScript
# ==============================
#@<> Lists help of plugin (js)
rc = call_mysqlsh_e("\\? decorator")

#@<> Lists help of plugin child object (js)
rc = call_mysqlsh_e("\\? decorator.inner")

#@<> Lists help of plugin function for simple type parameters (js)
rc = call_mysqlsh_e("\\? decorator.testSimpleTypes")

#@<> Lists help of plugin function with optional parameters (js)
rc = call_mysqlsh_e("\\? decorator.testOptionalParameters")

#@<> Lists help of plugin function with required dictionary parameters (js)
rc = call_mysqlsh_e("\\? decorator.testRequiredDictParams")

#@<> Lists help of plugin function with options (js)
rc = call_mysqlsh_e("\\? decorator.inner.testOptions")

#@<> Test calling simple types function (js)
rc = call_mysqlsh_e("decorator.testSimpleTypes('one', 2, false, {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')")

#@<> Test calling function with optionals 1 (js)
rc = call_mysqlsh_e("decorator.testOptionalParameters('two', 1, true, {whateverOption:'whateverValue'})")

#@<> Test calling function with optionals 2 (js)
rc = call_mysqlsh_e("decorator.testOptionalParameters('two', 1, true, {whateverOption:'whateverValue'}, [4,5,6])")

#@<> Test calling function with optionals 3 (js)
rc = call_mysqlsh_e("decorator.testOptionalParameters('two', 1, true, {whateverOption:'whateverValue'}, [4,5,6], {anykey:'anyValue'})")

#@<> Test calling function with required dictionary parameters 1 (js)
rc = call_mysqlsh_e("decorator.testRequiredDictParams('one')")

#@<> Test calling function with required dictionary parameters 2 (js)
rc = call_mysqlsh_e("decorator.testRequiredDictParams('one', {'param1': 'value1'})")

#@<> Test calling function with required dictionary parameters 3 (js)
rc = call_mysqlsh_e("decorator.testRequiredDictParams('one', {'param1': 'value1', 'param2': 'value2'})")

#@<> Test calling function with options 1 (js)
rc = call_mysqlsh_e("decorator.inner.testOptions('Passing No Options')")

#@<> Test calling function with options 2 (js)
rc = call_mysqlsh_e("decorator.inner.testOptions('Passing Options', {strOption:'String Option Value', intOption: 45, anyTypeOption:[1,'one',true]})")

#@<> Function Call Errors (js)
rc = call_mysqlsh_e("decorator.testSimpleTypes(1, 2, false, {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')")
rc = call_mysqlsh_e("decorator.testSimpleTypes('one', 'two', false, {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')")
rc = call_mysqlsh_e("decorator.testSimpleTypes('one', 2, 'error', {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')")
rc = call_mysqlsh_e("decorator.testSimpleTypes('one', 2, false, [1,2], [1,2,3], 'Some Value')")
rc = call_mysqlsh_e("decorator.testSimpleTypes('one', 2, false, {whateverOption:'whateverValue'}, 'error', 'Some Value')")
rc = call_mysqlsh_e("decorator.testSimpleTypes('one', 2, false, {whateverOption:'whateverValue'}, [1,2,3])")
rc = call_mysqlsh_e("decorator.inner.testOptions('Passing Options', {invalidOption:'String Option Value'})")

# Using the Plugin In Python
# ==========================
#@<> Lists help of plugin (py)
rc = call_mysqlsh_py_e("\\? decorator")

#@<> Lists help of plugin child object (py)
rc = call_mysqlsh_py_e("\\? decorator.inner")

#@<> Lists help of plugin function for simple type parameters (py)
rc = call_mysqlsh_py_e("\\? decorator.test_simple_types")

#@<> Lists help of plugin function with optional parameters (py)
rc = call_mysqlsh_py_e("\\? decorator.test_optional_parameters")

#@<> Lists help of plugin function with required dictionary parameters (py)
rc = call_mysqlsh_py_e("\\? decorator.test_required_dict_params")

#@<> Lists help of plugin function with options (py)
rc = call_mysqlsh_py_e("\\? decorator.inner.test_options")

#@<> Lists help of function with open and fixed options in params and options (py)
rc = call_mysqlsh_py_e("\\? decorator.inner.test_more_options")

#@<> Test calling simple types function (py)
rc = call_mysqlsh_py_e("decorator.test_simple_types('one', 2, False, {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')")

#@<> Test calling function with optionals 1 (py)
rc = call_mysqlsh_py_e("decorator.test_optional_parameters('two', 1, True, {'whateverOption':'whateverValue'})")

#@<> Test calling function with optionals 2 (py)
rc = call_mysqlsh_py_e("decorator.test_optional_parameters('two', 1, True, {'whateverOption':'whateverValue'}, [4,5,6])")

#@<> Test calling function with optionals 3 (py)
rc = call_mysqlsh_py_e("decorator.test_optional_parameters('two', 1, True, {'whateverOption':'whateverValue'}, [4,5,6], {'anykey':'anyValue'})")

#@<> Test calling function with required dictionary parameters 1 (py)
rc = call_mysqlsh_py_e("decorator.test_required_dict_params('one')")

#@<> Test calling function with required dictionary parameters 2 (py)
rc = call_mysqlsh_py_e("decorator.test_required_dict_params('one', {'param1': 'value1'})")

#@<> Test calling function with required dictionary parameters 3 (py)
rc = call_mysqlsh_py_e("decorator.test_required_dict_params('one', {'param1': 'value1', 'param2': 'value2'})")

#@<> Test calling function with options 1 (py)
rc = call_mysqlsh_py_e("decorator.inner.test_options('Passing No Options')")

#@<> Test calling function with options 2 (py)
rc = call_mysqlsh_py_e("decorator.inner.test_options('Passing Options', {'strOption':'String Option Value', 'intOption': 45, 'anyTypeOption':[1,'one',True]})")

#@<> Function Call Errors (py)
rc = call_mysqlsh_py_e("decorator.test_simple_types(1, 2, False, {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')")
rc = call_mysqlsh_py_e("decorator.test_simple_types('one', 'two', False, {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')")
rc = call_mysqlsh_py_e("decorator.test_simple_types('one', 2, 'error', {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')")
rc = call_mysqlsh_py_e("decorator.test_simple_types('one', 2, False, 'error', [1,2,3], 'Some Value')")
rc = call_mysqlsh_py_e("decorator.test_simple_types('one', 2, False, {'whateverOption':'whateverValue'}, 'error', 'Some Value')")
rc = call_mysqlsh_py_e("decorator.test_simple_types('one', 2, False, {'whateverOption':'whateverValue'}, [1,2,3])")
rc = call_mysqlsh_py_e("decorator.inner.test_options('Passing Options', {'invalidOption':'String Option Value'})")


###########################
# Using the Plugin As CLI
# ==========================
#@<> Lists help of plugin (cli)
rc = call_mysqlsh_cli("decorator", "--help")

#@<> Lists help of plugin child object (cli)
rc = call_mysqlsh_cli("decorator", "inner", "--help")

#@<> Lists help of plugin function for simple type parameters (cli)
rc = call_mysqlsh_cli("decorator", "test_simple_types", "--help")

#@<> Lists help of plugin function with options (cli)
rc = call_mysqlsh_cli("decorator", "inner", "test_options", "--help")

#@<> Test calling simple types function (cli)
rc = call_mysqlsh_cli("decorator", "test_simple_types", 'one', "2", "false", '--whateverOption=whateverValue', "1,2,3", "--an-undefined=\'Some Value\'")

#@<> Test calling function with optionals 1 (cli)
rc = call_mysqlsh_cli("decorator", "test_optional_parameters", 'two', "1", "true", '--whateverOption=whateverValue')

#@<> Test calling function with optionals 2 (cli)
rc = call_mysqlsh_cli("decorator", "test_optional_parameters", 'two', "1", "True", "--whateverOption=whateverValue", "4,5,6")

#@<> Test calling function with optionals 3 (cli)
rc = call_mysqlsh_cli("decorator", "test_optional_parameters", 'two', "1", "True", '--whateverOption=whateverValue', "4,5,6", '--anykey=anyValue')

#@<> Test calling function with options 1 (cli)
rc = call_mysqlsh_cli("decorator", "inner", "test_options", 'Passing No Options')

#@<> Finalization
testutil.rmdir(plugins_path, True)