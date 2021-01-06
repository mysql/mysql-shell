//@<OUT> Registration from C++, the parent class
NAME
      testutil - Gives access to general testing functions and properties.

DESCRIPTION
      Gives access to general testing functions and properties.

PROPERTIES
      sampleModuleJS
            Sample module exported from C++

      sampleModulePY
            Sample module exported from C++

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      registerFunction(parent, name, function[, definition])
            Registers a test utility function.

      registerModule(parent, name[, options])
            Registers a test module.

//@<OUT> Registration from C++, a test module
NAME
      sampleModuleJS - Sample module exported from C++

SYNTAX
      testutil.sampleModuleJS

DESCRIPTION
      Exploring the posibility to dynamically create objects fron C++

FUNCTIONS
      help([member])
            Provides help about this object and it's members

//@<OUT> Registration from C++, registerModule function
NAME
      registerModule - Registers a test module.

SYNTAX
      testutil.registerModule(parent, name[, options])

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

//@<OUT> Registration from C++, registerFunction function
NAME
      registerFunction - Registers a test utility function.

SYNTAX
      testutil.registerFunction(parent, name, function[, definition])

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

//@ Register, function(string)
||

//@<OUT> Module help, function(string)
NAME
      sampleModuleJS - Sample module exported from C++

SYNTAX
      testutil.sampleModuleJS

DESCRIPTION
      Exploring the posibility to dynamically create objects fron C++

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      stringFunction(data)
            Brief description for stringFunction.

//@<OUT> Help, function(string)
NAME
      stringFunction - Brief description for stringFunction.

SYNTAX
      testutil.sampleModuleJS.stringFunction(data)

WHERE
      data: String - Brief description for string parameter.

DESCRIPTION
      Detailed description for stringFunction

      Detailed description for string parameter.

      The data parameter accepts the following values: one, two, three.

//@ Usage, function(string)
||sampleModuleJS.stringFunction: Argument #1 is expected to be a string (TypeError)
||sampleModuleJS.stringFunction: Argument #1 only accepts the following values: one, two, three. (ArgumentError)
|JavaScript Function Definition: one|

//@ Register, function(dictionary)
||

//@<OUT> Module help, function(dictionary)
NAME
      sampleModuleJS - Sample module exported from C++

SYNTAX
      testutil.sampleModuleJS

DESCRIPTION
      Exploring the posibility to dynamically create objects fron C++

FUNCTIONS
      dictFunction([data])
            Brief definition for dictFunction.

      help([member])
            Provides help about this object and it's members

      stringFunction(data)
            Brief description for stringFunction.

//@<OUT> Help, function(dictionary)
NAME
      dictFunction - Brief definition for dictFunction.

SYNTAX
      testutil.sampleModuleJS.dictFunction([data])

WHERE
      data: Dictionary - Short description for dictionary parameter.

DESCRIPTION
      Detailed description for dictFunction

      Detailed description for dictionary parameter.

      The data parameter accepts the following options:

      - myOption: String (required) - A sample option.

      Details for the sample option

      The myOption option accepts the following values: test, value.


//@ Usage, function(dictionary)
||sampleModuleJS.dictFunction: Missing required options at Argument #1: myOption (ArgumentError)
||sampleModuleJS.dictFunction: Invalid and missing options at Argument #1 (invalid: someOption), (missing: myOption) (ArgumentError)
||sampleModuleJS.dictFunction: Argument #1, option 'myOption' is expected to be a string (TypeError)
||sampleModuleJS.dictFunction: Argument #1, option 'myOption' only accepts the following values: test, value. (ArgumentError)
|No function data available|
|Function data: test|

//@ Register, function(dictionary), no option validation
||

//@ Usage, function(dictionary), no option validation
|Full data:[object Object]|
|Full data:[object Object]|
|Function data: 5|
|Function data: whatever|
|No function data available|
|Function data: test|

//@ Register, function(Session)
||

//@<OUT> Module help, function(Session)
NAME
      sampleModuleJS - Sample module exported from C++

SYNTAX
      testutil.sampleModuleJS

DESCRIPTION
      Exploring the posibility to dynamically create objects fron C++

FUNCTIONS
      dictFunction([data])
            Brief definition for dictFunction.

      freeDictFunction([data])
            Brief definition for dictFunction.

      help([member])
            Provides help about this object and it's members

      objectFunction1(session)
            Brief definition for objectFunction.

      stringFunction(data)
            Brief description for stringFunction.

//@<OUT> Help, function(Session)
NAME
      objectFunction1 - Brief definition for objectFunction.

SYNTAX
      testutil.sampleModuleJS.objectFunction1(session)

WHERE
      session: Object - Short description for object parameter.

DESCRIPTION
      Detailed description for objectFunction

      Detailed description for object parameter.

      The session parameter must be a Session object.

//@ Usage, function(Session)
||sampleModuleJS.objectFunction1: Argument #1 is expected to be a 'Session' object (TypeError)
|Active Session: <Session:<<<__uri>>>>|

//@ Register, function(Session and ClassicSession)
||

//@<OUT> Module help, function(Session and ClassicSession)
NAME
      sampleModuleJS - Sample module exported from C++

SYNTAX
      testutil.sampleModuleJS

DESCRIPTION
      Exploring the posibility to dynamically create objects fron C++

FUNCTIONS
      dictFunction([data])
            Brief definition for dictFunction.

      freeDictFunction([data])
            Brief definition for dictFunction.

      help([member])
            Provides help about this object and it's members

      objectFunction1(session)
            Brief definition for objectFunction.

      objectFunction2(session)
            Brief definition for objectFunction.

      stringFunction(data)
            Brief description for stringFunction.

//@<OUT> Help, function(Session and ClassicSession)
NAME
      objectFunction2 - Brief definition for objectFunction.

SYNTAX
      testutil.sampleModuleJS.objectFunction2(session)

WHERE
      session: Object - Short description for object parameter.

DESCRIPTION
      Detailed description for objectFunction

      Detailed description for object parameter.

      The session parameter must be any of Session, ClassicSession.

//@ Usage, function(Session and ClassicSession)
|Active Session: <ClassicSession:<<<__mysql_uri>>>>|
|Active Session: <Session:<<<__uri>>>>|

//@ Registration errors, function definition
||Shell.addExtensionObjectMember: Argument #1 is expected to be an object (TypeError)
||Shell.addExtensionObjectMember: Argument #1 is expected to be an extension object (TypeError)
||Shell.addExtensionObjectMember: Argument #2 is expected to be a string (TypeError)
||Shell.addExtensionObjectMember: Invalid options at function definition: extra (ArgumentError)
||Shell.addExtensionObjectMember: Option 'brief' is expected to be of type String, but is Integer (TypeError)
||Shell.addExtensionObjectMember: Option 'details' is expected to be of type Array, but is Integer (TypeError)
||Shell.addExtensionObjectMember: Option 'details' String expected, but value is Integer (TypeError)
||Shell.addExtensionObjectMember: Option 'parameters' is expected to be of type Array, but is Integer (TypeError)
||Shell.addExtensionObjectMember: Invalid definition at parameter #1 (ArgumentError)

//@ Registration errors, parameters
||Shell.addExtensionObjectMember: Missing required options at parameter #1: name (ArgumentError)
||Shell.addExtensionObjectMember: Option 'type' is expected to be of type String, but is Integer (TypeError)

//@ Registration errors, integer parameters
||Shell.addExtensionObjectMember: Invalid options at integer parameter 'sample': class, classes, options, values (ArgumentError)

//@ Registration errors, float parameters
||Shell.addExtensionObjectMember: Invalid options at float parameter 'sample': class, classes, options, values (ArgumentError)

//@ Registration errors, bool parameters
||Shell.addExtensionObjectMember: Invalid options at bool parameter 'sample': class, classes, options, values (ArgumentError)

//@ Registration errors, string parameters
||Shell.addExtensionObjectMember: Invalid options at string parameter 'sample': class, classes, options (ArgumentError)
||Shell.addExtensionObjectMember: Option 'values' is expected to be of type Array, but is Integer (TypeError)
||Shell.addExtensionObjectMember: Option 'values' String expected, but value is Integer (TypeError)

//@ Registration errors, object parameters
||Shell.addExtensionObjectMember: Invalid options at object parameter 'sample': options, values (ArgumentError)
||Shell.addExtensionObjectMember: Option 'class' is expected to be of type String, but is Integer (TypeError)
||Shell.addExtensionObjectMember: Option 'classes' String expected, but value is Integer (TypeError)

//@ Registration errors, dictionary parameters
||Shell.addExtensionObjectMember: Invalid options at dictionary parameter 'sample': class, classes, values (ArgumentError)
||Shell.addExtensionObjectMember: Invalid definition at parameter 'sample', option #1 (ArgumentError)
||Shell.addExtensionObjectMember: Missing required options at parameter 'sample', option #1: name (ArgumentError)

//@ Registration errors, invalid identifiers
||Shell.addExtensionObjectMember: The function name 'my function' is not a valid identifier. (ArgumentError)
||Shell.addExtensionObjectMember: parameter #1 is not a valid identifier: 'a sample'. (ArgumentError)
||Shell.addExtensionObjectMember: parameter 'sample', option #1 is not a valid identifier: 'an invalid name'. (ArgumentError)

