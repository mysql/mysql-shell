#@Shell create object errors
||ValueError: Shell.create_extension_object: Invalid number of arguments, expected 0 but got 1


#@<OUT> Shell create object ok
<ExtensionObject>

#@object parameter should be an object
||TypeError: Shell.add_extension_object_member: Argument #1 is expected to be an object


#@but not any object, an extension object
||TypeError: Shell.add_extension_object_member: Argument #1 is expected to be an extension object


#@name parameters must be a string
||TypeError: Shell.add_extension_object_member: Argument #2 is expected to be a string


#@name parameters must be a valid identifier (member)
||ValueError: Shell.add_extension_object_member: The member name 'my name' is not a valid identifier.


#@name parameters must be a valid identifier (function)
||ValueError: Shell.add_extension_object_member: The function name 'my name' is not a valid identifier.


#@member definition must be a dictionary
||TypeError: Shell.add_extension_object_member: Argument #4 is expected to be a map


#@member definition 'brief' must be a string
||TypeError: Shell.add_extension_object_member: Option 'brief' is expected to be of type String, but is Integer


#@member definition 'details' must be an array
||TypeError: Shell.add_extension_object_member: Option 'details' is expected to be of type Array, but is Integer


#@member definition 'details' must be an array of strings
||TypeError: Shell.add_extension_object_member: Option 'details' String expected, but value is Integer


#@function definition does not accept other attributes
||ValueError: Shell.add_extension_object_member: Invalid options at function definition: extra


#@member definition does not accept other attributes
||ValueError: Shell.add_extension_object_member: Invalid options at member definition: extra


#@member definition does not accept 'parameters' if member is not a function
||ValueError: Shell.add_extension_object_member: Invalid options at member definition: extra


#@member definition 'parameters' must be an array
||TypeError: Shell.add_extension_object_member: Option 'parameters' is expected to be of type Array, but is Integer


#@member definition 'parameters' must be an array of dictionaries
||ValueError: Shell.add_extension_object_member: Invalid definition at parameter #1


#@A parameter definition requires name
||ValueError: Shell.add_extension_object_member: Missing required options at parameter #1: name


#@A parameter definition requires string on name
||TypeError: Shell.add_extension_object_member: Option 'name' is expected to be of type String, but is Integer


#@A parameter definition requires valid identifier on name
||ValueError: Shell.add_extension_object_member: Unsupported type used at parameter 'my sample' #1. Allowed types: array, bool, dictionary, float, integer, object, string


#@A parameter definition requires a string on type
||TypeError: Shell.add_extension_object_member: Option 'type' is expected to be of type String, but is Integer


#@A parameter definition requires a valid type on type
||ValueError: Shell.add_extension_object_member: Unsupported type used at parameter 'sample'. Allowed types: array, bool, dictionary, float, integer, object, string


#@A parameter definition requires a boolean on required
||TypeError: Shell.add_extension_object_member: Option 'required' Bool expected, but value is String


#@On parameters, duplicate definitions are not allowed
||ValueError: Shell.add_extension_object_member: parameter 'myname' is already defined.


#@On parameters, required ones can't come after optional ones
||ValueError: Shell.add_extension_object_member: parameter 'required' can not be required after optional parameter 'optional'


#@A parameter definition requires a string on brief
||TypeError: Shell.add_extension_object_member: Option 'brief' is expected to be of type String, but is Integer


#@A parameter definition requires a string on details
||TypeError: Shell.add_extension_object_member: Option 'details' is expected to be of type Array, but is Integer


#@A parameter definition requires just strings on details
||TypeError: Shell.add_extension_object_member: Option 'details' String expected, but value is Integer


#@<OUT> Non object parameters do not accept 'class' or 'classes' attributes
Shell.add_extension_object_member: Invalid options at string parameter 'sample': class

Shell.add_extension_object_member: Invalid options at integer parameter 'sample': class

Shell.add_extension_object_member: Invalid options at float parameter 'sample': class

Shell.add_extension_object_member: Invalid options at bool parameter 'sample': class

Shell.add_extension_object_member: Invalid options at array parameter 'sample': class

Shell.add_extension_object_member: Invalid options at dictionary parameter 'sample': class

Shell.add_extension_object_member: Invalid options at boolean parameter 'sample': class

Shell.add_extension_object_member: Invalid options at string parameter 'sample': classes

Shell.add_extension_object_member: Invalid options at integer parameter 'sample': classes

Shell.add_extension_object_member: Invalid options at float parameter 'sample': classes

Shell.add_extension_object_member: Invalid options at bool parameter 'sample': classes

Shell.add_extension_object_member: Invalid options at array parameter 'sample': classes

Shell.add_extension_object_member: Invalid options at dictionary parameter 'sample': classes

Shell.add_extension_object_member: Invalid options at boolean parameter 'sample': classes


#@<OUT> Non dictionary parameters do not accept 'options' attribute
Shell.add_extension_object_member: Invalid options at string parameter 'sample': options

Shell.add_extension_object_member: Invalid options at integer parameter 'sample': options

Shell.add_extension_object_member: Invalid options at float parameter 'sample': options

Shell.add_extension_object_member: Invalid options at bool parameter 'sample': options

Shell.add_extension_object_member: Invalid options at array parameter 'sample': options

Shell.add_extension_object_member: Invalid options at object parameter 'sample': options

Shell.add_extension_object_member: Invalid options at boolean parameter 'sample': options


#@<OUT> Non string parameters do not accept 'values' attribute
Shell.add_extension_object_member: Invalid options at dictionary parameter 'sample': values

Shell.add_extension_object_member: Invalid options at integer parameter 'sample': values

Shell.add_extension_object_member: Invalid options at float parameter 'sample': values

Shell.add_extension_object_member: Invalid options at bool parameter 'sample': values

Shell.add_extension_object_member: Invalid options at array parameter 'sample': values

Shell.add_extension_object_member: Invalid options at object parameter 'sample': values

Shell.add_extension_object_member: Invalid options at boolean parameter 'sample': values


#@String parameters 'values' must be an array
||TypeError: Shell.add_extension_object_member: Option 'values' is expected to be of type Array, but is Integer


#@String parameter 'values' must be an array of strings
||TypeError: Shell.add_extension_object_member: Option 'values' String expected, but value is Integer

#@ Object parameter 'class' must be a string
||TypeError: Shell.add_extension_object_member: Option 'class' is expected to be of type String, but is Integer

#@ Object parameter 'class' can not be empty
||ValueError: Shell.add_extension_object_member: The following class is not recognized, it can not be used on the validation of an object parameter: ''.

#@ Object parameter 'classes' must be an array
||TypeError: Shell.add_extension_object_member: Option 'classes' is expected to be of type Array, but is Integer

#@ Object parameter 'classes' must be an array of strings
||TypeError: Shell.add_extension_object_member: Option 'classes' String expected, but value is Integer

#@ Object parameter 'classes' can not be an empty array
||ValueError: Shell.add_extension_object_member: An empty array is not valid for the classes option.

#@ Object parameter 'class' must hold a valid class name
||ValueError: Shell.add_extension_object_member: The following class is not recognized, it can not be used on the validation of an object parameter: unknown.

#@ Object parameter 'classes' must hold valid class names (singular)
||ValueError: Shell.add_extension_object_member: The following class is not recognized, it can not be used on the validation of an object parameter: Weirdie.

#@ Object parameter 'classes' must hold valid class names (plural)
||ValueError: Shell.add_extension_object_member: The following classes are not recognized, they can not be used on the validation of an object parameter: Unexisting, Weirdie.

#@Dictionary parameter 'options' should be an array
||TypeError: Shell.add_extension_object_member: Option 'options' is expected to be of type Array, but is Integer


#@Dictionary parameter 'options' must be an array of dictionaries
||ValueError: Shell.add_extension_object_member: Invalid definition at parameter 'sample', option #1


#@Parameter option definition requires type and name, missing both
||ValueError: Shell.add_extension_object_member: Missing required options at parameter 'sample', option #1: name


#@Parameter option definition 'required' must be boolean
||TypeError: Shell.add_extension_object_member: Option 'required' Bool expected, but value is String


#@Parameter option definition 'brief' must be string
||TypeError: Shell.add_extension_object_member: Option 'brief' is expected to be of type String, but is Integer


#@Parameter option definition 'details' must be array
||TypeError: Shell.add_extension_object_member: Option 'details' is expected to be of type Array, but is Integer


#@Parameter option definition 'details' must be array of strings
||TypeError: Shell.add_extension_object_member: Option 'details' String expected, but value is Integer


#@Parameter option definition, duplicates are not allowed
||ValueError: Shell.add_extension_object_member: option 'myoption' is already defined.


#@Parameter option definition, unknown attributes are not allowed
||ValueError: Shell.add_extension_object_member: Invalid options at parameter 'sample', string option 'myoption': other


#@Adding duplicate member
||ValueError: Shell.add_extension_object_member: The object already has a 'sample' member.

#@ Adding self as member
||ValueError: Shell.add_extension_object_member: An extension object can not be member of itself.

#@ Adding object that was already added
||ValueError: Shell.add_extension_object_member: The provided extension object is already registered as part of another extension object.


#@Registering global, missing arguments
||ValueError: Shell.register_global: Invalid number of arguments, expected 2 to 3 but got 0
||ValueError: Shell.register_global: Invalid number of arguments, expected 2 to 3 but got 1


#@Registering global, invalid data for name parameter
||TypeError: Shell.register_global: Argument #1 is expected to be a string


#@Registering global, invalid name parameter
||ValueError: Shell.register_global: The name 'bad name' is not a valid identifier.


#@Registering global, invalid data for object parameter, not an object
||TypeError: Shell.register_global: Argument #2 is expected to be an object


#@Registering global, invalid data for object parameter, not an extension object
||TypeError: Shell.register_global: Argument #2 is expected to be an extension object


#@Registering global, invalid data definition
||TypeError: Shell.register_global: Argument #3 is expected to be a map


#@Registering global, invalid definition, brief should be string
||TypeError: Shell.register_global: Option 'brief' is expected to be of type String, but is Integer


#@Registering global, invalid definition, details should be array
||TypeError: Shell.register_global: Option 'details' is expected to be of type Array, but is Integer


#@Registering global, invalid definition, details should be array of strings
||TypeError: Shell.register_global: Option 'details' String expected, but value is Integer


#@Registering global, invalid definition, other attributes not accepted
||ValueError: Shell.register_global: Invalid options at object definition.: other


#@<OUT> Registering global, no definition
The Shell Help is organized in categories and topics. To get help for a
specific category or topic use: \? <pattern>

The <pattern> argument should be the name of a category or a topic.

The pattern is a filter to identify topics for which help is required, it can
use the following wildcards:

- ? matches any single character.
- * matches any character sequence.

The following are the main help categories:

 - AdminAPI       The AdminAPI is an API that enables configuring and managing
                  InnoDB clusters and replicasets, among other things.
 - Shell Commands Provides details about the available built-in shell commands.
 - ShellAPI       Contains information about the shell and util global objects
                  as well as the mysql module that enables executing SQL on
                  MySQL Servers.
 - SQL Syntax     Entry point to retrieve syntax help on SQL statements.
 - X DevAPI       Details the mysqlx module as well as the capabilities of the
                  X DevAPI which enable working with MySQL as a Document Store

The available topics include:

- The dba global object and the classes available at the AdminAPI.
- The mysqlx module and the classes available at the X DevAPI.
- The mysql module and the global objects and classes available at the
  ShellAPI.
- The functions and properties of the classes exposed by the APIs.
- The available shell commands.
- Any word that is part of an SQL statement.
- Command Line - invoking built-in shell functions without entering interactive
  mode.

SHELL COMMANDS

The shell commands allow executing specific operations including updating the
shell configuration.

The following shell commands are available:

 - \                   Start multi-line input when in SQL mode.
 - \connect    (\c)    Connects the shell to a MySQL server and assigns the
                       global session.
 - \disconnect         Disconnects the global session.
 - \edit       (\e)    Launch a system editor to edit a command to be executed.
 - \exit               Exits the MySQL Shell, same as \quit.
 - \help       (\?,\h) Prints help information about a specific topic.
 - \history            View and edit command line history.
 - \js                 Switches to JavaScript processing mode.
 - \nopager            Disables the current pager.
 - \nowarnings (\w)    Don't show warnings after every statement.
 - \option             Allows working with the available shell options.
 - \pager      (\P)    Sets the current pager.
 - \py                 Switches to Python processing mode.
 - \quit       (\q)    Exits the MySQL Shell.
 - \reconnect          Reconnects the global session.
 - \rehash             Refresh the autocompletion cache.
 - \show               Executes the given report with provided options and
                       arguments.
 - \source     (\.)    Loads and executes a script from a file.
 - \sql                Executes SQL statement or switches to SQL processing
                       mode when no statement is given.
 - \status     (\s)    Print information about the current global session.
 - \system     (\!)    Execute a system shell command.
 - \use        (\u)    Sets the active schema.
 - \warnings   (\W)    Show warnings after every statement.
 - \watch              Executes the given report with provided options and
                       arguments in a loop.

GLOBAL OBJECTS

The following modules and objects are ready for use when the shell starts:

 - dba      Used for InnoDB cluster administration.
 - goodName
 - mysql    Support for connecting to MySQL servers using the classic MySQL
            protocol.
 - mysqlx   Used to work with X Protocol sessions using the MySQL X DevAPI.
 - shell    Gives access to general purpose functions and properties.
 - testutil
 - util     Global object that groups miscellaneous tools like upgrade checker
            and JSON import.

For additional information on these global objects use: <object>.help()

EXAMPLES
\? AdminAPI
      Displays information about the AdminAPI.

\? \connect
      Displays usage details for the \connect command.

\? check_instance_configuration
      Displays usage details for the dba.check_instance_configuration function.

\? sql syntax
      Displays the main SQL help categories.

#@<OUT> Registering global using existing global names
Shell.register_global: A global named 'shell' already exists.

Shell.register_global: A global named 'dba' already exists.

Shell.register_global: A global named 'util' already exists.

Shell.register_global: The name 'mysql' is reserved.

Shell.register_global: The name 'mysqlx' is reserved.

Shell.register_global: A global named 'session' already exists.

Shell.register_global: A global named 'db' already exists.

Shell.register_global: A global named 'sys' already exists.

Shell.register_global: A global named 'os' already exists.

Shell.register_global: A global named 'goodName' already exists.


#@ Adding object that was already registered
||ValueError: Shell.add_extension_object_member: The provided extension object is already registered as: sampleObject.

#@ Registering object that was already registered
||ValueError: Shell.register_global: The provided extension object is already registered as: sampleObject


#@ Attempt to get property of unregistered
||RuntimeError: Unable to access members in an unregistered extension object.

#@ Attempt to set property of unregistered
||RuntimeError: Unable to modify members in an unregistered extension object.

#@ Attempt to call function of unregistered
||RuntimeError: Unable to call functions in an unregistered extension object.

#@ Attempt to get property of registered
|5|

#@ Attempt to set property of registered
|7|

#@ Attempt to call function of registered
|Hello World!|
