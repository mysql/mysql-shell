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

    Unordered lists are supported using sphinx syntax:

    * Item 1
    * Item 2
    * Item 3
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

    Unordered lists are supported using sphinx syntax:

    * Item 1
    * Item 2
    * Item 3

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

shell_env = ["MYSQLSH_TERM_COLOR_MODE=nocolor", "MYSQLSH_USER_CONFIG_HOME=" + user_path]

def __call_mysqlsh(cmdline_args):
    return testutil.call_mysqlsh(["--quiet-start=2"] + cmdline_args, "", shell_env)

def call_mysqlsh_e(e_arg, py=False):
    return __call_mysqlsh((["--py"] if py else ["--js"]) + ["-ifull", "-e"] + [e_arg])

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


#@<> BUG#33451028 - Unable to register plugin named as pre-exiting plugin nested object
aaa_plugin_code = '''
from mysqlsh.plugin_manager import plugin, plugin_function

@plugin
class aaa:
    """
    Brief description of the aaa plugin.
    """

    class bbb:
        """
        Brief description of aaa.bbb plugin object.
        """
        pass

    pass
'''

bbb_plugin_code = '''
from mysqlsh.plugin_manager import plugin, plugin_function

@plugin
class bbb:
    """
    Brief description of the bbb plugin.
    """

    class aaa:
        """
        Brief description of aaa.bbb plugin object.
        """
        pass

    pass
'''

aaa_folder_path = os.path.join(plugins_path, "aaa")
bbb_folder_path = os.path.join(plugins_path, "bbb")
aaa_path =  os.path.join(aaa_folder_path, "init.py")
bbb_path =  os.path.join(bbb_folder_path, "init.py")
testutil.mkdir(aaa_folder_path, True)
testutil.mkdir(bbb_folder_path, True)
testutil.create_file(aaa_path, aaa_plugin_code)
testutil.create_file(bbb_path, bbb_plugin_code)


#@<OUT> Help for aaa
rc = call_mysqlsh_e("\\? aaa")

#@<OUT> aaa.help()
rc = call_mysqlsh_e("aaa.help()")

#@<OUT> Help for nested aaa
rc = call_mysqlsh_e("\\? bbb.aaa")

#@<OUT> bbb.aaa.help()
rc = call_mysqlsh_e("bbb.aaa.help()")

#@<OUT> Help for bbb
rc = call_mysqlsh_e("\\? bbb")

#@<OUT> bbb.help()
rc = call_mysqlsh_e("bbb.help()")

#@<OUT> Help for nested bbb
rc = call_mysqlsh_e("\\? aaa.bbb")

#@<OUT> aaa.bbb.help()
rc = call_mysqlsh_e("aaa.bbb.help()")

#@<OUT> Help from thread
test_thread_code = """
import threading

def test_help():
    print(aaa.help())

t = threading.Thread(target=test_help)
t.start()
t.join()
"""

testutil.create_file("thread_test.py", test_thread_code)
rc = __call_mysqlsh(["--py", "-f", "thread_test.py"])
testutil.rmfile("thread_test.py")

#@<> Bug#33462107 - plugin_function: unable to attach function to existing object
sample_plugin_code = '''
from mysqlsh.plugin_manager import plugin, plugin_function

@plugin
class sample():
    """
    A sample plugin
    """

    class myObject():
        """
        A nested object...
        """

@plugin_function("sample.myObject.testFunction")
def test():
    print("My test function")
'''

sample_folder_path = os.path.join(plugins_path, "sample")
sample_path =  os.path.join(sample_folder_path, "init.py")
testutil.mkdir(sample_folder_path, True)
testutil.create_file(sample_path, sample_plugin_code)

WIPE_OUTPUT()
call_mysqlsh_e("sample.myObject.testFunction()")
EXPECT_STDOUT_CONTAINS("My test function")

WIPE_OUTPUT()
call_mysqlsh_py_e("sample.my_object.test_function()")
EXPECT_STDOUT_CONTAINS("My test function")

#@<> Plugin shell incompatible, shell version out of valid range
version = shell.version.split()[1].split('-')[0].split('.')
base_version = f"{version[0]}.{version[1]}."
v_plus_1 = base_version + str(int(version[2])+1)
v_minus_1 = base_version + str(int(version[2])-1)
v_minus_2 = base_version + str(int(version[2])-2)
base_version = base_version + version[2]

plugin_code = f'''
from mysqlsh.plugin_manager import plugin, plugin_function

@plugin(shell_version_min="{v_minus_2}", shell_version_max="{v_minus_1}")
class sample():
    """
    A sample plugin
    """

@plugin_function("sample.testFunction")
def test():
    print("My test function")
'''
testutil.rmfile(sample_path)
testutil.create_file(sample_path, plugin_code)

testutil.call_mysqlsh(["-e", "shell.version"], "", shell_env)

EXPECT_STDOUT_CONTAINS("Could not register plugin object 'sample'.")
EXPECT_STDOUT_CONTAINS(f"This plugin requires Shell between versions {v_minus_2} and {v_minus_1}.")


#@<> Plugin shell incompatible, shell version above max version
plugin_code = f'''
from mysqlsh.plugin_manager import plugin, plugin_function

@plugin(shell_version_max="{v_minus_1}")
class sample():
    """
    A sample plugin
    """

@plugin_function("sample.testFunction")
def test():
    print("My test function")
'''
testutil.rmfile(sample_path)
testutil.create_file(sample_path, plugin_code)

testutil.call_mysqlsh(["-e", "shell.version"], "", shell_env)

EXPECT_STDOUT_CONTAINS("Could not register plugin object 'sample'.")
EXPECT_STDOUT_CONTAINS(f"This plugin does not work on Shell versions newer than {v_minus_1}.")


#@<> Plugin shell incompatible, shell version below min version
plugin_code = f'''
from mysqlsh.plugin_manager import plugin, plugin_function

@plugin(shell_version_min="{v_plus_1}")
class sample():
    """
    A sample plugin
    """

@plugin_function("sample.testFunction")
def test():
    print("My test function")
'''
testutil.rmfile(sample_path)
testutil.create_file(sample_path, plugin_code)

testutil.call_mysqlsh(["-e", "shell.version"], "", shell_env)

EXPECT_STDOUT_CONTAINS("Could not register plugin object 'sample'.")
EXPECT_STDOUT_CONTAINS(f"This plugin requires at least Shell version {v_plus_1}.")


#@<> Extending utils object
plugin_code = f'''
from mysqlsh.plugin_manager import plugin, plugin_function

@plugin(parent="util")
class sample():
    """
    A sample plugin
    """

@plugin_function("util.utilTest", cli=True)
def util_test():
    print("My custom function at util.")

@plugin_function("util.sample.testFunction", cli=True)
def test():
    print("My inner function at util.sample.")
'''
testutil.rmfile(sample_path)
testutil.create_file(sample_path, plugin_code)

testutil.call_mysqlsh(["--", "util", "util-test"], "", shell_env)
testutil.call_mysqlsh(["--", "util", "sample", "test-function"], "", shell_env)

EXPECT_STDOUT_CONTAINS("My custom function at util.")
EXPECT_STDOUT_CONTAINS(f"My inner function at util.sample.")


#@<> Plugin call with python object as parameter
plugin_code = f'''
from mysqlsh.plugin_manager import plugin, plugin_function

@plugin
class sample():
    """
    A sample plugin
    """

@plugin_function("sample.test")
def test(session, sql):
    session.run_sql(sql)
'''

script_code = f'''
class DrySession():
    def run_sql(self, sql):
        print(f"EXECUTED SQL: {{sql}}")

session = DrySession()

sample.test(session, "SELECT * FROM SAKILA.ACTOR")
'''


testutil.rmfile(sample_path)
testutil.create_file(sample_path, plugin_code)
testutil.create_file("my-script.py", script_code)

testutil.call_mysqlsh(["--py", "-f", "my-script.py"], "", shell_env)

EXPECT_STDOUT_CONTAINS("EXECUTED SQL: SELECT * FROM SAKILA.ACTOR")


#@<> Help on different plugins with different implementations for same object.function
plugin_code = f'''
from mysqlsh.plugin_manager import plugin, plugin_function


@plugin
class plugin1():
    """Plugin to manage the MySQL REST Data Service (MRS)."""

    class list():
        """Used to list MRS objects."""


@plugin
class plugin2():
    """Plugin to manage the MySQL Database Service on OCI."""

    class list():
        """Used to list OCI objects."""


@plugin_function('plugin1.list.data', shell=True, cli=True, web=True)
def listdata1(one, two, three):
    """Get users configured within a service and/or auth_app

    Args:
        one (str): Use this service_id to search for all users within this service.
        two (str): Use this auth_app_id to list all the users for this auth_app.
        three (object): The database session to use.

    Returns:
        None
    """
    pass


@plugin_function('plugin2.list.data')
def listdata2(four, five, six):
    """Lists users

    Lists all users of a given compartment.

    Args:
        four (object): An OCI config object or None.
        five (bool): If set to false exceptions are raised
        six (bool): If set to true, a list object is returned.

    Returns:
        A list of users
    """
    pass
'''


testutil.rmfile(sample_path)
testutil.create_file(sample_path, plugin_code)

#@ plugin1.list.data
rc = call_mysqlsh_py_e("\\? plugin1.list.data")

#@ plugin2.list.data
rc = call_mysqlsh_py_e("\\? plugin2.list.data")

#@<> Finalization
testutil.rmdir(plugins_path, True)
testutil.rmfile("my-script.py")
