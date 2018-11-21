//@<OUT> Registration from C++, the parent class
NAME
      testutil - Gives access to general testing functions and properties.

DESCRIPTION
      Gives access to general testing functions and properties.

OBJECTS
 - sampleModuleJS Sample module exported from C++
 - sampleModulePY Sample module exported from C++

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
      sampleModuleJS.stringFunction(data)

WHERE
      data: string. Brief description for string parameter.

DESCRIPTION
      Detailed description for stringFunction

      Detailed description for string parameter.

      The data parameter accepts the following values:

      - one
      - two
      - three

//@ Usage, function(string)
||sampleModuleJS.stringFunction: Argument #1 is expected to be a string (ArgumentError)
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
      sampleModuleJS.dictFunction([data])

WHERE
      data: Dictionary. Short description for dictionary parameter.

DESCRIPTION
      Detailed description for dictFunction

      Detailed description for dictionary parameter.

      The data parameter accepts the following options:

      - myOption string. A sample option

      Details for the sample option

      The myOption option accepts the following values:

      - test
      - value

//@ Usage, function(dictionary)
||sampleModuleJS.dictFunction: Missing required options at Argument #1: myOption (ArgumentError)
||sampleModuleJS.dictFunction: Invalid and missing options at Argument #1 (invalid: someOption), (missing: myOption) (ArgumentError)
||sampleModuleJS.dictFunction: option myOption at Argument #1 is expected to be a string (ArgumentError)
||sampleModuleJS.dictFunction: option myOption at Argument #1 only accepts the following values: test, value. (ArgumentError)
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
      sampleModuleJS.objectFunction1(session)

WHERE
      session: object. Short description for object parameter.

DESCRIPTION
      Detailed description for objectFunction

      Detailed description for object parameter.

      The session parameter must be a Session object.

//@ Usage, function(Session)
||sampleModuleJS.objectFunction1: Argument #1 is expected to be a 'Session' object. (ArgumentError)
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
      sampleModuleJS.objectFunction2(session)

WHERE
      session: object. Short description for object parameter.

DESCRIPTION
      Detailed description for objectFunction

      Detailed description for object parameter.

      The session parameter must be any of Session, ClassicSession.

//@ Usage, function(Session and ClassicSession)
|Active Session: <ClassicSession:<<<__mysql_uri>>>>|
|Active Session: <Session:<<<__uri>>>>|

//@ Registration errors, function definition
||Testutils.registerFunction: Target object was not found. (ArgumentError)
||Testutils.registerFunction: Invalid options at function definition: extra (ArgumentError)
||Testutils.registerFunction: Option 'brief' is expected to be of type String, but is Integer (TypeError)
||Testutils.registerFunction: Option 'details' is expected to be of type Array, but is Integer (TypeError)
||Testutils.registerFunction: Option 'details' String expected, but value is Integer (TypeError)
||Testutils.registerFunction: Option 'parameters' is expected to be of type Array, but is Integer (TypeError)
||Testutils.registerFunction: Invalid definition at parameter #1 (ArgumentError)

//@ Registration errors, parameters
||Testutils.registerFunction: Missing required options at parameter #1: name, type (ArgumentError)
||Testutils.registerFunction: Option 'type' is expected to be of type String, but is Integer (TypeError)

//@ Registration errors, integer parameters
||Testutils.registerFunction: Invalid options at parameter 'sample': class, classes, options, values (ArgumentError)

//@ Registration errors, float parameters
||Testutils.registerFunction: Invalid options at parameter 'sample': class, classes, options, values (ArgumentError)

//@ Registration errors, bool parameters
||Testutils.registerFunction: Invalid options at parameter 'sample': class, classes, options, values (ArgumentError)

//@ Registration errors, string parameters
||Testutils.registerFunction: Invalid options at parameter 'sample': class, classes, options (ArgumentError)
||Testutils.registerFunction: Option 'values' is expected to be of type Array, but is Integer (TypeError)
||Testutils.registerFunction: Option 'values' String expected, but value is Integer (TypeError)

//@ Registration errors, object parameters
||Testutils.registerFunction: Invalid options at parameter 'sample': options, values (ArgumentError)
||Testutils.registerFunction: Option 'class' is expected to be of type String, but is Integer (TypeError)
||Testutils.registerFunction: Option 'classes' String expected, but value is Integer (TypeError)

//@ Registration errors, dictionary parameters
||Testutils.registerFunction: Invalid and missing options at parameter 'sample' (invalid: class, classes, values), (missing: options) (ArgumentError)
||Testutils.registerFunction: Missing 'options' at parameter 'sample'. (ArgumentError)
||Testutils.registerFunction: Invalid definition at parameter 'sample' option #1 (ArgumentError)
||Testutils.registerFunction: Missing required options at parameter 'sample' option #1: name, type (ArgumentError)

//@ Registration errors, invalid identifiers
||Testutils.registerModule: The module name must be a valid identifier. (ArgumentError)
||Testutils.registerFunction: The function name 'my function'is not a valid identifier. (ArgumentError)
||Testutils.registerFunction: parameter #1 is not a valid identifier: 'a sample'. (ArgumentError)
||Testutils.registerFunction: parameter 'sample' option #1 is not a valid identifier: 'an invalid name'. (ArgumentError)

//@ Function with custom names, registration error
||Testutils.registerFunction: Invalid function name. When using custom names only two names should be provided. (ArgumentError)

//@ Function with custom names, registration ok
||

//@<OUT> Function with custom names, help on module
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

      js_function(data)
            Brief description for stringFunction.

      objectFunction1(session)
            Brief definition for objectFunction.

      objectFunction2(session)
            Brief definition for objectFunction.

      stringFunction(data)
            Brief description for stringFunction.

//@<OUT> Function with custom names, help on function
NAME
      js_function - Brief description for stringFunction.

SYNTAX
      sampleModuleJS.js_function(data)

WHERE
      data: string. Brief description for string parameter.

DESCRIPTION
      Detailed description for stringFunction

      Detailed description for string parameter.


//@ Function with custom names, usage
|Some random data: some data|
