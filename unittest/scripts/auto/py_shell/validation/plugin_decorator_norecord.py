#@<OUT> Lists help of plugin (js)
[[*]]> \? decorator
NAME
      decorator - Brief description of a decorated plugin.

DESCRIPTION
      This plugin will test that the object and functions can be properly
      registered using python decorators removing the burden of having to call
      the shell APIs for this purpose.

      On this case, a global object will be registered using as name this class
      name.

PROPERTIES
      inner
            Brief description of a child plugin object.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      testOptionalParameters(aString, anInt, aBool, aDict[, aList][, anUndefined])
            Tests documentation for optional parameters.

      testRequiredDictParams(first[, dict_param])
            Tests documentation for dictionary required parameters.

      testSimpleTypes(aString, anInt, aBool, aDict, aList, anUndefined)
            Tests documentation for simple types, no options defined.

#@<OUT> Lists help of plugin child object (js)
[[*]]> \? decorator.inner
NAME
      inner - Brief description of a child plugin object.

SYNTAX
      decorator.inner

DESCRIPTION
      This is a plugin object registered as a child of the decorator plugin, it
      is used to verify that child classes are good to define a composed plugin
      structure.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      testMoreOptions(dictOne, dictTwo, dictThree, dictFour)
            Tests some parameter documentation.

      testOptions(stritem[, options])
            Tests some parameter documentation.

#@<OUT> Lists help of plugin function for simple type parameters (js)
[[*]]> \? decorator.testSimpleTypes
NAME
      testSimpleTypes - Tests documentation for simple types, no options
                        defined.

SYNTAX
      decorator.testSimpleTypes(aString, anInt, aBool, aDict, aList,
      anUndefined)

WHERE
      aString: String - A string parameter.
      anInt: Integer - An integer parameter.
      aBool: Bool - A boolean parameter.
      aDict: Dictionary - A dictionary parameter.
      aList: Array - A list parameter.
      anUndefined: A parameter with no type.

DESCRIPTION
      The simple types function is only used to test how the simple parameters
      are properly documented using the decorator plugins.

      The aDict parameter accepts any key/value pair.


#@<OUT> Lists help of plugin function with optional parameters (js)
[[*]]> \? decorator.testOptionalParameters
NAME
      testOptionalParameters - Tests documentation for optional parameters.

SYNTAX
      decorator.testOptionalParameters(aString, anInt, aBool, aDict[, aList][,
      anUndefined])

WHERE
      aString: String - A string parameter.
      anInt: Integer - An integer parameter.
      aBool: Bool - A boolean parameter.
      aDict: Dictionary - A dictionary parameter.
      aList: Array - A list parameter.
      anUndefined: A parameter with no type.

DESCRIPTION
      The optional parameters function is only used to test how the parameters
      with default values are documented as optional params.

      The aDict parameter accepts any key/value pair.


#@<OUT> Lists help of plugin function with required dictionary parameters (js)
[[*]]> \? decorator.testRequiredDictParams
NAME
      testRequiredDictParams - Tests documentation for dictionary required
                               parameters.

SYNTAX
      decorator.testRequiredDictParams(first[, dict_param])

WHERE
      first: String - First required parameter.
      dict_param: Dictionary - Options.

DESCRIPTION
      The dict_param parameter accepts the following options:

      - param1: String (required) - First parameter.
      - param2: String (required) - Second parameter.
      - description: String - Description.
[[*]]> 

#@<OUT> Lists help of plugin function with options (js)
[[*]]> \? decorator.inner.testOptions
NAME
      testOptions - Tests some parameter documentation.

SYNTAX
      decorator.inner.testOptions(stritem[, options])

WHERE
      stritem: String - String parameter.
      options: Dictionary - Options to whatever.

DESCRIPTION
      The options function is only used to test how the options for a
      dictionary are properly documented.

      The options parameter accepts the following options:

      - strOption: String - String parameter.
      - intOption: Integer - String List parameter.
      - anyTypeOption: Any type option.


#@<OUT> Test calling simple types function (js)
[[*]]> decorator.testSimpleTypes('one', 2, false, {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')
aString value: one
anInt value: 2
aBool value: False
aDict value: {"whateverOption": "whateverValue"}
aList value: [1, 2, 3]
anUndefined value: Some Value

#@<OUT> Test calling function with optionals 1 (js)
[[*]]> decorator.testOptionalParameters('two', 1, true, {whateverOption:'whateverValue'})
aString value: two
anInt value: 1
aBool value: True
aDict value: {"whateverOption": "whateverValue"}
aList value: [1, 2, 3]

#@<OUT> Test calling function with optionals 2 (js)
[[*]]> decorator.testOptionalParameters('two', 1, true, {whateverOption:'whateverValue'}, [4,5,6])
aString value: two
anInt value: 1
aBool value: True
aDict value: {"whateverOption": "whateverValue"}
aList value: [4, 5, 6]

#@<OUT> Test calling function with optionals 3 (js)
[[*]]> decorator.testOptionalParameters('two', 1, true, {whateverOption:'whateverValue'}, [4,5,6], {anykey:'anyValue'})
aString value: two
anInt value: 1
aBool value: True
aDict value: {"whateverOption": "whateverValue"}
aList value: [4, 5, 6]
anUndefined value: {"anykey": "anyValue"}

#@<OUT> Test calling function with required dictionary parameters 1 (js)
[[*]]> decorator.testRequiredDictParams('one')
first value: one
dict: {}
[[*]]> 

#@<OUT> Test calling function with required dictionary parameters 2 (js)
[[*]]> decorator.testRequiredDictParams('one', {'param1': 'value1'})
decorator.testRequiredDictParams: Missing required options at Argument #2: param2 (ArgumentError)
[[*]]> 

#@<OUT> Test calling function with required dictionary parameters 3 (js)
[[*]]> decorator.testRequiredDictParams('one', {'param1': 'value1', 'param2': 'value2'})
first value: one
dict: {"param1": "value1", "param2": "value2"}
[[*]]> 


#@<OUT> Test calling function with options 1 (js)
[[*]]> decorator.inner.testOptions('Passing No Options')
stritem value: Passing No Options

#@<OUT> Test calling function with options 2 (js)
[[*]]> decorator.inner.testOptions('Passing Options', {strOption:'String Option Value', intOption: 45, anyTypeOption:[1,'one',true]})
stritem value: Passing Options
options: {"anyTypeOption": [1, "one", true], "intOption": 45, "strOption": "String Option Value"}

#@ Function Call Errors (js)
|[[*]]> decorator.testSimpleTypes(1, 2, false, {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')|
|decorator.testSimpleTypes: Argument #1 is expected to be a string (TypeError)|
|[[*]]> decorator.testSimpleTypes('one', 'two', false, {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')|
|decorator.testSimpleTypes: Argument #2 is expected to be an integer (TypeError)|
|[[*]]> decorator.testSimpleTypes('one', 2, 'error', {whateverOption:'whateverValue'}, [1,2,3], 'Some Value')|
|decorator.testSimpleTypes: Argument #3 is expected to be a bool (TypeError)|
|[[*]]> decorator.testSimpleTypes('one', 2, false, [1,2], [1,2,3], 'Some Value')|
|decorator.testSimpleTypes: Argument #4 is expected to be a map (TypeError)|
|[[*]]> decorator.testSimpleTypes('one', 2, false, {whateverOption:'whateverValue'}, 'error', 'Some Value')|
|decorator.testSimpleTypes: Argument #5 is expected to be an array (TypeError)|
|[[*]]> decorator.testSimpleTypes('one', 2, false, {whateverOption:'whateverValue'}, [1,2,3])|
|decorator.testSimpleTypes: Invalid number of arguments, expected 6 but got 5 (ArgumentError)|
|[[*]]> decorator.inner.testOptions('Passing Options', {invalidOption:'String Option Value'})|
|inner.testOptions: Invalid options at Argument #2: invalidOption (ArgumentError)|

#@<OUT> Lists help of plugin (py)
[[*]]> \? decorator
NAME
      decorator - Brief description of a decorated plugin.

DESCRIPTION
      This plugin will test that the object and functions can be properly
      registered using python decorators removing the burden of having to call
      the shell APIs for this purpose.

      On this case, a global object will be registered using as name this class
      name.

PROPERTIES
      inner
            Brief description of a child plugin object.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      test_optional_parameters(aString, anInt, aBool, aDict[, aList][, anUndefined])
            Tests documentation for optional parameters.

      test_required_dict_params(first[, dict_param])
            Tests documentation for dictionary required parameters.

      test_simple_types(aString, anInt, aBool, aDict, aList, anUndefined)
            Tests documentation for simple types, no options defined.

#@<OUT> Lists help of plugin child object (py)
[[*]]> \? decorator.inner
NAME
      inner - Brief description of a child plugin object.

SYNTAX
      decorator.inner

DESCRIPTION
      This is a plugin object registered as a child of the decorator plugin, it
      is used to verify that child classes are good to define a composed plugin
      structure.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      test_more_options(dictOne, dictTwo, dictThree, dictFour)
            Tests some parameter documentation.

      test_options(stritem[, options])
            Tests some parameter documentation.

#@<OUT> Lists help of plugin function for simple type parameters (py)
[[*]]> \? decorator.test_simple_types
NAME
      test_simple_types - Tests documentation for simple types, no options
                          defined.

SYNTAX
      decorator.test_simple_types(aString, anInt, aBool, aDict, aList,
      anUndefined)

WHERE
      aString: String - A string parameter.
      anInt: Integer - An integer parameter.
      aBool: Bool - A boolean parameter.
      aDict: Dictionary - A dictionary parameter.
      aList: Array - A list parameter.
      anUndefined: A parameter with no type.

DESCRIPTION
      The simple types function is only used to test how the simple parameters
      are properly documented using the decorator plugins.

      The aDict parameter accepts any key/value pair.


#@<OUT> Lists help of plugin function with optional parameters (py)
[[*]]> \? decorator.test_optional_parameters
NAME
      test_optional_parameters - Tests documentation for optional parameters.

SYNTAX
      decorator.test_optional_parameters(aString, anInt, aBool, aDict[,
      aList][, anUndefined])

WHERE
      aString: String - A string parameter.
      anInt: Integer - An integer parameter.
      aBool: Bool - A boolean parameter.
      aDict: Dictionary - A dictionary parameter.
      aList: Array - A list parameter.
      anUndefined: A parameter with no type.

DESCRIPTION
      The optional parameters function is only used to test how the parameters
      with default values are documented as optional params.

      The aDict parameter accepts any key/value pair.

#@<OUT> Lists help of plugin function with required dictionary parameters (py)
[[*]]> \? decorator.test_required_dict_params
NAME
      test_required_dict_params - Tests documentation for dictionary required
                                  parameters.

SYNTAX
      decorator.test_required_dict_params(first[, dict_param])

WHERE
      first: String - First required parameter.
      dict_param: Dictionary - Options.

DESCRIPTION
      The dict_param parameter accepts the following options:

      - param1: String (required) - First parameter.
      - param2: String (required) - Second parameter.
      - description: String - Description.
[[*]]> 


#@<OUT> Lists help of plugin function with options (py)
[[*]]> \? decorator.inner.test_options
NAME
      test_options - Tests some parameter documentation.

SYNTAX
      decorator.inner.test_options(stritem[, options])

WHERE
      stritem: String - String parameter.
      options: Dictionary - Options to whatever.

DESCRIPTION
      The options function is only used to test how the options for a
      dictionary are properly documented.

      The options parameter accepts the following options:

      - strOption: String - String parameter.
      - intOption: Integer - String List parameter.
      - anyTypeOption: Any type option.


#@<OUT> Lists help of function with open and fixed options in params and options (py)
[[*]]> \? decorator.inner.test_more_options
NAME
      test_more_options - Tests some parameter documentation.

SYNTAX
      decorator.inner.test_more_options(dictOne, dictTwo, dictThree, dictFour)

WHERE
      dictOne: Dictionary - Dictionary with specific options.
      dictTwo: Dictionary - Dictionary with specific options.
      dictThree: Dictionary - Open dictionary.
      dictFour: Dictionary - Open dictionary.

DESCRIPTION
      This test ensures the dictionaries and options are properly parsed based
      on the documentation.

      The dictOne parameter accepts the following options:

      - optOne: Dictionary - Dictionary option.
      - optTwo: Dictionary - Dictionary option.

      The optOne option accepts the following options:

      - strSample: String - sample option.

      The optTwo option accepts any key/value pair.

      The dictTwo parameter accepts the following options:

      - optThree: Dictionary - Dictionary option.
      - optFour: Dictionary - Dictionary option.

      The optThree option accepts the following options:

      - strLast: String - sample option.

      The optFour option accepts any key/value pair.

      The dictThree parameter accepts any key/value pair.

      The dictFour parameter accepts any key/value pair.


#@<OUT> Test calling simple types function (py)
[[*]]> decorator.test_simple_types('one', 2, False, {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')
aString value: one
anInt value: 2
aBool value: False
aDict value: {"whateverOption": "whateverValue"}
aList value: [1, 2, 3]
anUndefined value: Some Value

#@<OUT> Test calling function with optionals 1 (py)
[[*]]> decorator.test_optional_parameters('two', 1, True, {'whateverOption':'whateverValue'})
aString value: two
anInt value: 1
aBool value: True
aDict value: {"whateverOption": "whateverValue"}
aList value: [1, 2, 3]

#@<OUT> Test calling function with optionals 2 (py)
[[*]]> decorator.test_optional_parameters('two', 1, True, {'whateverOption':'whateverValue'}, [4,5,6])
aString value: two
anInt value: 1
aBool value: True
aDict value: {"whateverOption": "whateverValue"}
aList value: [4, 5, 6]

#@<OUT> Test calling function with optionals 3 (py)
[[*]]> decorator.test_optional_parameters('two', 1, True, {'whateverOption':'whateverValue'}, [4,5,6], {'anykey':'anyValue'})
aString value: two
anInt value: 1
aBool value: True
aDict value: {"whateverOption": "whateverValue"}
aList value: [4, 5, 6]
anUndefined value: {"anykey": "anyValue"}

#@ Test calling function with required dictionary parameters 1 (py)
|[[*]]> decorator.test_required_dict_params('one')|
|ValueError: decorator.test_required_dict_params: Missing required options at Argument #2: param1, param2|

#@ Test calling function with required dictionary parameters 2 (py)
|[[*]]> decorator.test_required_dict_params('one', {'param1': 'value1'})|
|ValueError: decorator.test_required_dict_params: Missing required options at Argument #2: param2|

#@<OUT> Test calling function with required dictionary parameters 3 (py)
[[*]]> decorator.test_required_dict_params('one', {'param1': 'value1', 'param2': 'value2'})
first value: one
dict: {"param1": "value1", "param2": "value2"}
[[*]]> 

#@<OUT> Test calling function with options 1 (py)
[[*]]> decorator.inner.test_options('Passing No Options')
stritem value: Passing No Options

#@<OUT> Test calling function with options 2 (py)
[[*]]> decorator.inner.test_options('Passing Options', {'strOption':'String Option Value', 'intOption': 45, 'anyTypeOption':[1,'one',True]})
stritem value: Passing Options
options: {"anyTypeOption": [1, "one", true], "intOption": 45, "strOption": "String Option Value"}

#@ Function Call Errors (py)
|[[*]]> decorator.test_simple_types(1, 2, False, {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')|
|TypeError: decorator.test_simple_types: Argument #1 is expected to be a string|
|[[*]]> decorator.test_simple_types('one', 'two', False, {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')|
|TypeError: decorator.test_simple_types: Argument #2 is expected to be an integer|
|[[*]]> decorator.test_simple_types('one', 2, 'error', {'whateverOption':'whateverValue'}, [1,2,3], 'Some Value')|
|TypeError: decorator.test_simple_types: Argument #3 is expected to be a bool|
|[[*]]> decorator.test_simple_types('one', 2, False, 'error', [1,2,3], 'Some Value')|
|TypeError: decorator.test_simple_types: Argument #4 is expected to be a map|
|[[*]]> decorator.test_simple_types('one', 2, False, {'whateverOption':'whateverValue'}, 'error', 'Some Value')|
|TypeError: decorator.test_simple_types: Argument #5 is expected to be an array|
|[[*]]> decorator.test_simple_types('one', 2, False, {'whateverOption':'whateverValue'}, [1,2,3])|
|ValueError: decorator.test_simple_types: Invalid number of arguments, expected 6 but got 5|
|[[*]]> decorator.inner.test_options('Passing Options', {'invalidOption':'String Option Value'})|

#@<OUT> Lists help of plugin (cli)
The following object provides command line operations at 'decorator':

   inner
      Brief description of a child plugin object.

The following operations are available at 'decorator':

   test-optional-parameters
      Tests documentation for optional parameters.

   test-required-dict-params
      Tests documentation for dictionary required parameters.

   test-simple-types
      Tests documentation for simple types, no options defined.

#@<OUT> Lists help of plugin child object (cli)
The following operations are available at 'decorator inner':

   test-more-options
      Tests some parameter documentation.

   test-options
      Tests some parameter documentation.

#@<OUT> Lists help of plugin function for simple type parameters (cli)
NAME
      test-simple-types - Tests documentation for simple types, no options
                          defined.

SYNTAX
      decorator test-simple-types <aString> <anInt> <aBool> <aDict> <aList>
      --anUndefined[:<type>]=<value>

WHERE
      aString: String - A string parameter.
      anInt: Integer - An integer parameter.
      aBool: Bool - A boolean parameter.
      aList: Array - A list parameter.

OPTIONS
--anUndefined[:<type>]=<value>
            A parameter with no type.

#@<OUT> Lists help of plugin function with options (cli)
NAME
      test-options - Tests some parameter documentation.

SYNTAX
      decorator inner test-options <stritem> [<options>]

WHERE
      stritem: String - String parameter.

OPTIONS
--strOption=<str>
            String parameter.

--intOption=<int>
            String List parameter.

--anyTypeOption[:<type>]=<value>
            Any type option.

#@<OUT> Test calling simple types function (cli)
aString value: one
anInt value: 2
aBool value: False
aDict value: {"whateverOption": "whateverValue"}
aList value: [1, 2, 3]
anUndefined value: 'Some Value'

#@<OUT> Test calling function with optionals 1 (cli)
aString value: two
anInt value: 1
aBool value: True
aDict value: {"whateverOption": "whateverValue"}
aList value: [1, 2, 3]

#@<OUT> Test calling function with optionals 2 (cli)
aString value: two
anInt value: 1
aBool value: True
aDict value: {"whateverOption": "whateverValue"}
aList value: [4, 5, 6]

#@<OUT> Test calling function with optionals 3 (cli)
aString value: two
anInt value: 1
aBool value: True
aDict value: {"anykey": "anyValue", "whateverOption": "whateverValue"}
aList value: [4, 5, 6]

#@<OUT> Test calling function with options 1 (cli)
stritem value: Passing No Options

#@<OUT> Help for aaa
[[*]]> \? aaa
NAME
      aaa - Brief description of the aaa plugin.

DESCRIPTION
      Brief description of the aaa plugin.

PROPERTIES
      bbb
            Brief description of aaa.bbb plugin object.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

SEE ALSO

Additional entries were found matching aaa

The following topics were found at the bbb category:

- bbb.aaa

For help on a specific topic use: \? <topic>

e.g.: \? bbb.aaa

#@<OUT> Help for nested aaa
[[*]]> \? bbb.aaa
NAME
      aaa - Brief description of aaa.bbb plugin object.

SYNTAX
      bbb.aaa

DESCRIPTION
      Brief description of aaa.bbb plugin object.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

#@<OUT> Help for bbb
[[*]]> \? bbb
NAME
      bbb - Brief description of the bbb plugin.

DESCRIPTION
      Brief description of the bbb plugin.

PROPERTIES
      aaa
            Brief description of aaa.bbb plugin object.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

SEE ALSO

Additional entries were found matching bbb

The following topics were found at the aaa category:

- aaa.bbb

For help on a specific topic use: \? <topic>

e.g.: \? aaa.bbb

#@<OUT> Help for nested bbb
[[*]]> \? aaa.bbb
NAME
      bbb - Brief description of aaa.bbb plugin object.

SYNTAX
      aaa.bbb

DESCRIPTION
      Brief description of aaa.bbb plugin object.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

