#@Shell create object errors
||ValueError: Invalid number of arguments, expected 0 but got 1


#@<OUT> Shell create object ok
<ExtensionObject>

#@object parameter should be an object
||TypeError: Argument #1 is expected to be an object


#@but not any object, an extension object
||TypeError: Argument #1 is expected to be an extension object


#@name parameters must be a string
||TypeError: Argument #2 is expected to be a string


#@name parameters must be a valid identifier (member)
||ValueError: The member name 'my name' is not a valid identifier.


#@name parameters must be a valid identifier (function)
||ValueError: The function name 'my name' is not a valid identifier.


#@member definition must be a dictionary
||TypeError: Argument #4 is expected to be a map


#@member definition 'brief' must be a string
||TypeError: Option 'brief' is expected to be of type String, but is Integer


#@member definition 'details' must be an array
||TypeError: Option 'details' is expected to be of type Array, but is Integer


#@member definition 'details' must be an array of strings
||TypeError: Option 'details' is expected to be an array of strings


#@function definition does not accept other attributes
||ValueError: Invalid options at function definition: extra


#@member definition does not accept other attributes
||ValueError: Invalid options at member definition: extra


#@member definition does not accept 'parameters' if member is not a function
||ValueError: Invalid options at member definition: extra


#@member definition 'parameters' must be an array
||TypeError: Option 'parameters' is expected to be of type Array, but is Integer


#@member definition 'parameters' must be an array of dictionaries
||ValueError: Invalid definition at parameter #1


#@A parameter definition requires name
||ValueError: Missing required options at parameter #1: name


#@A parameter definition requires string on name
||TypeError: Option 'name' is expected to be of type String, but is Integer


#@A parameter definition requires valid identifier on name
||ValueError: Unsupported type used at parameter 'my sample' #1. Allowed types: array, bool, dictionary, float, integer, object, string


#@A parameter definition requires a string on type
||TypeError: Option 'type' is expected to be of type String, but is Integer


#@A parameter definition requires a valid type on type
||ValueError: Unsupported type used at parameter 'sample'. Allowed types: array, bool, dictionary, float, integer, object, string


#@A parameter definition requires a boolean on required
||TypeError: Option 'required' Bool expected, but value is String


#@On parameters, duplicate definitions are not allowed
||ValueError: parameter 'myname' is already defined.


#@On parameters, required ones can't come after optional ones
||ValueError: parameter 'required' can not be required after optional parameter 'optional'


#@A parameter definition requires a string on brief
||TypeError: Option 'brief' is expected to be of type String, but is Integer


#@A parameter definition requires a string on details
||TypeError: Option 'details' is expected to be of type Array, but is Integer


#@A parameter definition requires just strings on details
||TypeError: Option 'details' is expected to be an array of strings


#@<OUT> Non object parameters do not accept 'class' or 'classes' attributes
Invalid options at string parameter 'sample': class

Invalid options at integer parameter 'sample': class

Invalid options at float parameter 'sample': class

Invalid options at bool parameter 'sample': class

Invalid options at array parameter 'sample': class

Invalid options at dictionary parameter 'sample': class

Invalid options at boolean parameter 'sample': class

Invalid options at string parameter 'sample': classes

Invalid options at integer parameter 'sample': classes

Invalid options at float parameter 'sample': classes

Invalid options at bool parameter 'sample': classes

Invalid options at array parameter 'sample': classes

Invalid options at dictionary parameter 'sample': classes

Invalid options at boolean parameter 'sample': classes


#@<OUT> Non dictionary parameters do not accept 'options' attribute
Invalid options at string parameter 'sample': options

Invalid options at integer parameter 'sample': options

Invalid options at float parameter 'sample': options

Invalid options at bool parameter 'sample': options

Invalid options at array parameter 'sample': options

Invalid options at object parameter 'sample': options

Invalid options at boolean parameter 'sample': options


#@<OUT> Non string parameters do not accept 'values' attribute
Invalid options at dictionary parameter 'sample': values

Invalid options at integer parameter 'sample': values

Invalid options at float parameter 'sample': values

Invalid options at bool parameter 'sample': values

Invalid options at array parameter 'sample': values

Invalid options at object parameter 'sample': values

Invalid options at boolean parameter 'sample': values


#@String parameters 'values' must be an array
||TypeError: Option 'values' is expected to be of type Array, but is Integer


#@String parameter 'values' must be an array of strings
||TypeError: Option 'values' is expected to be an array of strings

#@ Object parameter 'class' must be a string
||TypeError: Option 'class' is expected to be of type String, but is Integer

#@ Object parameter 'class' can not be empty
||ValueError: The following class is not recognized, it can not be used on the validation of an object parameter: ''.

#@ Object parameter 'classes' must be an array
||TypeError: Option 'classes' is expected to be of type Array, but is Integer

#@ Object parameter 'classes' must be an array of strings
||TypeError: Option 'classes' is expected to be an array of strings

#@ Object parameter 'classes' can not be an empty array
||ValueError: An empty array is not valid for the classes option.

#@ Object parameter 'class' must hold a valid class name
||ValueError: The following class is not recognized, it can not be used on the validation of an object parameter: unknown.

#@ Object parameter 'classes' must hold valid class names (singular)
||ValueError: The following class is not recognized, it can not be used on the validation of an object parameter: Weirdie.

#@ Object parameter 'classes' must hold valid class names (plural)
||ValueError: The following classes are not recognized, they can not be used on the validation of an object parameter: Unexisting, Weirdie.

#@Dictionary parameter 'options' should be an array
||TypeError: Option 'options' is expected to be of type Array, but is Integer


#@Dictionary parameter 'options' must be an array of dictionaries
||ValueError: Invalid definition at parameter 'sample', option #1


#@Parameter option definition requires type and name, missing both
||ValueError: Missing required options at parameter 'sample', option #1: name


#@Parameter option definition 'required' must be boolean
||TypeError: Option 'required' Bool expected, but value is String


#@Parameter option definition 'brief' must be string
||TypeError: Option 'brief' is expected to be of type String, but is Integer


#@Parameter option definition 'details' must be array
||TypeError: Option 'details' is expected to be of type Array, but is Integer


#@Parameter option definition 'details' must be array of strings
||TypeError: Option 'details' is expected to be an array of strings


#@Parameter option definition, duplicates are not allowed
||ValueError: option 'myoption' is already defined.


#@Parameter option definition, unknown attributes are not allowed
||ValueError: Invalid options at parameter 'sample', string option 'myoption': other


#@Adding duplicate member
||ValueError: The object already has a 'sample' member.

#@ Adding self as member
||ValueError: An extension object can not be member of itself.

#@ Adding object that was already added
||ValueError: The provided extension object is already registered as part of another extension object.


#@Registering global, missing arguments
||ValueError: Invalid number of arguments, expected 2 to 3 but got 0
||ValueError: Invalid number of arguments, expected 2 to 3 but got 1


#@Registering global, invalid data for name parameter
||TypeError: Argument #1 is expected to be a string


#@Registering global, invalid name parameter
||ValueError: The name 'bad name' is not a valid identifier.


#@Registering global, invalid data for object parameter, not an object
||TypeError: Argument #2 is expected to be an object


#@Registering global, invalid data for object parameter, not an extension object
||TypeError: Argument #2 is expected to be an extension object


#@Registering global, invalid data definition
||TypeError: Argument #3 is expected to be a map


#@Registering global, invalid definition, brief should be string
||TypeError: Option 'brief' is expected to be of type String, but is Integer


#@Registering global, invalid definition, details should be array
||TypeError: Option 'details' is expected to be of type Array, but is Integer


#@Registering global, invalid definition, details should be array of strings
||TypeError: Option 'details' is expected to be an array of strings


#@Registering global, invalid definition, other attributes not accepted
||ValueError: Invalid options at object definition.: other


#@<OUT> Registering global, no definition
GLOBAL OBJECTS

The following modules and objects are ready for use when the shell starts:

${*}
 - goodName
${*}

For additional information on these global objects use: <object>.help()

#@<OUT> Registering global using existing global names
A global named 'shell' already exists.

A global named 'dba' already exists.

A global named 'util' already exists.

The name 'mysql' is reserved.

The name 'mysqlx' is reserved.

A global named 'session' already exists.

A global named 'db' already exists.

A global named 'sys' already exists.

A global named 'os' already exists.

A global named 'goodName' already exists.


#@ Adding object that was already registered
||ValueError: The provided extension object is already registered as: sampleObject.

#@ Registering object that was already registered
||ValueError: The provided extension object is already registered as: sampleObject


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
