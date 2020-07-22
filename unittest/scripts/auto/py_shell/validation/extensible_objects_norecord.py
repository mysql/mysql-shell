#@<OUT> Registration from C++, the parent class
NAME
      testutil - Gives access to general testing functions and properties.

DESCRIPTION
      Gives access to general testing functions and properties.

PROPERTIES
      sample_module_j_s
            Sample module exported from C++

      sample_module_p_y
            Sample module exported from C++

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      register_function(parent, name, function[, definition])
            Registers a test utility function.

      register_module(parent, name[, options])
            Registers a test module.

#@<OUT> Registration from C++, a test module
NAME
      sample_module_p_y - Sample module exported from C++

SYNTAX
      testutil.sample_module_p_y

DESCRIPTION
      Exploring the posibility to dynamically create objects fron C++

FUNCTIONS
      help([member])
            Provides help about this object and it's members

#@<OUT> Registration from C++, register_module function
NAME
      register_module - Registers a test module.

SYNTAX
      testutil.register_module(parent, name[, options])

WHERE
      parent: The module that will contain the new module.
      name: The name of the new module.
      options: Options with help information for the module.

DESCRIPTION
      Registers a new test module into an existing test module.

      The parent module should be an existing module, in the format of:
      testutil[.<name>]*]

      The name should be a valid identifier.

      The object will be registered using the same name in both JavaScript and
      Python.

      The options parameter accepts the following options:

      - brief string. Short description of the new module.
      - details array. Detailed description of the module.

      Each entry in the details array will be turned into a paragraph on the
      module help.

      Only strings are allowed in the details array.

#@<OUT> Registration from C++, register_function function
NAME
      register_function - Registers a test utility function.

SYNTAX
      testutil.register_function(parent, name, function[, definition])

WHERE
      parent: The module that will contain the new function.
      name: The name of the new function in camelCase format.
      function: The function callback to be executed when the new function is
                called.
      definition: Options containing additional function definition details.

DESCRIPTION
      Registers a new function into an existing test module.

      The name should be a valid identifier.

      It should be an existing module, in the format of: testutil[.<name>]*]

      The function will be registered following the respective naming
      convention for JavaScript and Python.

      The definition parameter accepts the following options:

      - brief string. Short description of the new function.
      - details array. Detailed description of the new function.
      - parameters array. List of parameters the new function receives.

      Each entry in the details array will be turned into a paragraph on the
      module help.

      Only strings are allowed in the details array.

      Each parameter is defined as a dictionary, the following properties are
      allowed:

      - name: defines the name of the parameter
      - brief: a short description of the parameter
      - details: an array with the detailed description of the parameter
      - type: the data type of the parameter
      - required: a boolean indicating if the parameter is required or not

      The supported parameter types are:

      - string
      - integer
      - float
      - array
      - object
      - dictionary

      Parameters of type 'string' may contain a 'values' property with the list
      of values allowed for the parameter.

      Parameters of type 'object' may contain either:

      - a 'class' property with the type of object the parameter must be.
      - a 'classes' property with a list of types of objects that the parameter
        can be.

      Parameters of type 'dictionary' may contain an 'options' list defining
      the options that are allowed for the parameter.

      Each option on the 'options' list follows the same structure as the
      parameter definition.

#@ Register, function(string)
||

#@<OUT> Module help, function(string)
NAME
      sample_module_p_y - Sample module exported from C++

SYNTAX
      testutil.sample_module_p_y

DESCRIPTION
      Exploring the posibility to dynamically create objects fron C++

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      string_function(data)
            Brief description for stringFunction.

#@<OUT> Help, function(string)
NAME
      string_function - Brief description for stringFunction.

SYNTAX
      testutil.sample_module_p_y.string_function(data)

WHERE
      data: String. Brief description for string parameter.

DESCRIPTION
      Detailed description for stringFunction

      Detailed description for string parameter.

      The data parameter accepts the following values: one, two, three.

#@ Usage, function(string)
||SystemError: ArgumentError: sampleModulePY.string_function: Argument #1 is expected to be a string
||SystemError: ArgumentError: sampleModulePY.string_function: Argument #1 only accepts the following values: one, two, three.
|Python Function Definition:  one|

#@ Register, function(dictionary)
||

#@<OUT> Module help, function(dictionary)
NAME
      sample_module_p_y - Sample module exported from C++

SYNTAX
      testutil.sample_module_p_y

DESCRIPTION
      Exploring the posibility to dynamically create objects fron C++

FUNCTIONS
      dict_function([data])
            Brief definition for dictFunction.

      help([member])
            Provides help about this object and it's members

      string_function(data)
            Brief description for stringFunction.

#@<OUT> Help, function(dictionary)
NAME
      dict_function - Brief definition for dictFunction.

SYNTAX
      testutil.sample_module_p_y.dict_function([data])

WHERE
      data: Dictionary. Short description for dictionary parameter.

DESCRIPTION
      Detailed description for dictFunction

      Detailed description for dictionary parameter.

      The data parameter accepts the following options:

      - myOption (required) String. A sample option

      Details for the sample option

      The myOption option accepts the following values: test, value.


#@Usage, function(dictionary)
||SystemError: ArgumentError: sampleModulePY.dict_function: Missing required options at Argument #1: myOption
||SystemError: ArgumentError: sampleModulePY.dict_function: Invalid and missing options at Argument #1 (invalid: someOption), (missing: myOption)
||SystemError: ArgumentError: sampleModulePY.dict_function: Argument #1, option 'myOption' is expected to be a string
||SystemError: ArgumentError: sampleModulePY.dict_function: Argument #1, option 'myOption' only accepts the following values: test, value.
|No function data available|
|Function data:  test|

#@ Register, function(dictionary), no option validation
||

#@Usage, function(dictionary), no option validation
|No function data available|
|Full data: {"someOption": 5}|
|Function data:  5|
|Function data:  whatever|
|No function data available|
|Function data:  test|


#@ Register, function(Session)
||

#@<OUT> Module help, function(Session)
NAME
      sample_module_p_y - Sample module exported from C++

SYNTAX
      testutil.sample_module_p_y

DESCRIPTION
      Exploring the posibility to dynamically create objects fron C++

FUNCTIONS
      dict_function([data])
            Brief definition for dictFunction.

      free_dict_function([data])
            Brief definition for dictFunction.

      help([member])
            Provides help about this object and it's members

      object_function1(session)
            Brief definition for objectFunction.

      string_function(data)
            Brief description for stringFunction.

#@<OUT> Help, function(Session)
NAME
      object_function1 - Brief definition for objectFunction.

SYNTAX
      testutil.sample_module_p_y.object_function1(session)

WHERE
      session: Object. Short description for object parameter.

DESCRIPTION
      Detailed description for objectFunction

      Detailed description for object parameter.

      The session parameter must be a Session object.

#@ Usage, function(Session)
||SystemError: ArgumentError: sampleModulePY.object_function1: Argument #1 is expected to be a 'Session' object.
|Active Session: <Session:<<<__uri>>>>|

#@ Register, function(Session and ClassicSession)
||

#@<OUT> Module help, function(Session and ClassicSession)
NAME
      sample_module_p_y - Sample module exported from C++

SYNTAX
      testutil.sample_module_p_y

DESCRIPTION
      Exploring the posibility to dynamically create objects fron C++

FUNCTIONS
      dict_function([data])
            Brief definition for dictFunction.

      free_dict_function([data])
            Brief definition for dictFunction.

      help([member])
            Provides help about this object and it's members

      object_function1(session)
            Brief definition for objectFunction.

      object_function2(session)
            Brief definition for objectFunction.

      string_function(data)
            Brief description for stringFunction.

#@<OUT> Help, function(Session and ClassicSession)
NAME
      object_function2 - Brief definition for objectFunction.

SYNTAX
      testutil.sample_module_p_y.object_function2(session)

WHERE
      session: Object. Short description for object parameter.

DESCRIPTION
      Detailed description for objectFunction

      Detailed description for object parameter.

      The session parameter must be any of Session, ClassicSession.

#@ Usage, function(Session and ClassicSession)
|Active Session: <ClassicSession:<<<__mysql_uri>>>>|
|Active Session: <Session:<<<__uri>>>>|


#@ Registration errors, function definition
||SystemError: ArgumentError: Shell.add_extension_object_member: Argument #1 is expected to be an object
||SystemError: ArgumentError: Shell.add_extension_object_member: Argument #1 is expected to be an extension object.
||SystemError: ArgumentError: Shell.add_extension_object_member: Argument #2 is expected to be a string
||SystemError: ArgumentError: Shell.add_extension_object_member: Invalid options at function definition: extra
||SystemError: TypeError: Shell.add_extension_object_member: Option 'brief' is expected to be of type String, but is Integer
||SystemError: TypeError: Shell.add_extension_object_member: Option 'details' is expected to be of type Array, but is Integer
||SystemError: TypeError: Shell.add_extension_object_member: Option 'details' String expected, but value is Integer
||SystemError: TypeError: Shell.add_extension_object_member: Option 'parameters' is expected to be of type Array, but is Integer
||SystemError: ArgumentError: Shell.add_extension_object_member: Invalid definition at parameter #1


#@ Registration errors, parameters
||SystemError: ArgumentError: Shell.add_extension_object_member: Missing required options at parameter #1: name
||SystemError: TypeError: Shell.add_extension_object_member: Option 'type' is expected to be of type String, but is Integer

#@ Registration errors, integer parameters
||SystemError: ArgumentError: Shell.add_extension_object_member: Invalid options at integer parameter 'sample': class, classes, options, values

#@ Registration errors, float parameters
||SystemError: ArgumentError: Shell.add_extension_object_member: Invalid options at float parameter 'sample': class, classes, options, values


#@ Registration errors, bool parameters
||SystemError: ArgumentError: Shell.add_extension_object_member: Invalid options at bool parameter 'sample': class, classes, options, values

#@ Registration errors, string parameters
||SystemError: ArgumentError: Shell.add_extension_object_member: Invalid options at string parameter 'sample': class, classes, options
||SystemError: TypeError: Shell.add_extension_object_member: Option 'values' is expected to be of type Array, but is Integer
||SystemError: TypeError: Shell.add_extension_object_member: Option 'values' String expected, but value is Integer


#@ Registration errors, dictionary parameters
||SystemError: ArgumentError: Shell.add_extension_object_member: Invalid options at dictionary parameter 'sample': class, classes, values
||SystemError: ArgumentError: Shell.add_extension_object_member: Invalid definition at parameter 'sample', option #1
||SystemError: ArgumentError: Shell.add_extension_object_member: Missing required options at parameter 'sample', option #1: name

#@ Registration errors, invalid identifiers
||SystemError: ArgumentError: Shell.add_extension_object_member: The function name 'my function' is not a valid identifier.
||SystemError: ArgumentError: Shell.add_extension_object_member: parameter #1 is not a valid identifier: 'a sample'.
||SystemError: ArgumentError: Shell.add_extension_object_member: parameter 'sample', option #1 is not a valid identifier: 'an invalid name'.
