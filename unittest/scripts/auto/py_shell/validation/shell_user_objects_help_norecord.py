#@<OUT> Original globals in help
GLOBAL OBJECTS

The following modules and objects are ready for use when the shell starts:

 - dba      Used for InnoDB cluster administration.
 - mysql    Support for connecting to MySQL servers using the classic MySQL
            protocol.
 - mysqlx   Used to work with X Protocol sessions using the MySQL X DevAPI.
 - shell    Gives access to general purpose functions and properties.
 - testutil
 - util     Global object that groups miscellaneous tools like upgrade checker
            and JSON import.

For additional information on these global objects use: <object>.help()


#@<OUT> Registering a new global object
GLOBAL OBJECTS

The following modules and objects are ready for use when the shell starts:

 - dba      Used for InnoDB cluster administration.
 - myGlobal User defined global object.
 - mysql    Support for connecting to MySQL servers using the classic MySQL
            protocol.
 - mysqlx   Used to work with X Protocol sessions using the MySQL X DevAPI.
 - shell    Gives access to general purpose functions and properties.
 - testutil
 - util     Global object that groups miscellaneous tools like upgrade checker
            and JSON import.

For additional information on these global objects use: <object>.help()


#@<OUT> Now we can retrieve the object's help
NAME
      myGlobal - User defined global object.

DESCRIPTION
      This object is used to verify how each element help is registered on the
      go.

      The reason for this is because this object was registered as global right
      after it's creation

FUNCTIONS
      help([member])
            Provides help about this object and it's members

#@<OUT> Now we can ask about member help
NAME
      my_integer - Simple description for integer member.

SYNTAX
      myGlobal.my_integer

DESCRIPTION
      The member myInteger should only be used for obscure purposes.

#@<OUT> The member data is added to the object help
NAME
      myGlobal - User defined global object.

DESCRIPTION
      This object is used to verify how each element help is registered on the
      go.

      The reason for this is because this object was registered as global right
      after it's creation

PROPERTIES
      my_integer
            Simple description for integer member.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

#@<OUT> The member data is updated with all the members
NAME
      myGlobal - User defined global object.

DESCRIPTION
      This object is used to verify how each element help is registered on the
      go.

      The reason for this is because this object was registered as global right
      after it's creation

PROPERTIES
      my_array
            Simple description for array member.

      my_boolean
            Simple description for boolean member.

      my_child_object
            Child object with it's own definitions.

      my_dictionary
            Simple description for dictionary member.

      my_float
            Simple description for float member.

      my_integer
            Simple description for integer member.

      my_null
            Simple description for none member.

      my_string
            Simple description for string member.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

#@<OUT> The child object is available as well
NAME
      my_child_object - Child object with it's own definitions.

SYNTAX
      myGlobal.my_child_object

DESCRIPTION
      Child object with it's own definitions.

PROPERTIES
      my_child_array
            Simple description for array member.

      my_child_boolean
            Simple description for boolean member.

      my_child_dictionary
            Simple description for dictionary member.

      my_child_float
            Simple description for float member.

      my_child_null
            Simple description for none member.

      my_child_string
            Simple description for string member.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

#@<OUT> The function help is available
NAME
      my_function - Simple description for a function.

SYNTAX
      myGlobal.my_function(one, two[, three][, four])

WHERE
      one: String - Exersices the string parameter usage.
      two: Integer - Exersices the integer parameter usage.
      three: Float - Exersices the float parameter usage.
      four: Bool - Exersices the boolean parameter usage.

DESCRIPTION
      This function is to test how the help gets properly registered.

      This parameter must be a string.

      With one of the supported values.

      The one parameter accepts the following values: 1, 2, 3.

      Will not provide additional details about this parameter.

#@<OUT> The function is added to the object help
NAME
      myGlobal - User defined global object.

DESCRIPTION
      This object is used to verify how each element help is registered on the
      go.

      The reason for this is because this object was registered as global right
      after it's creation

PROPERTIES
      my_array
            Simple description for array member.

      my_boolean
            Simple description for boolean member.

      my_child_object
            Child object with it's own definitions.

      my_dictionary
            Simple description for dictionary member.

      my_float
            Simple description for float member.

      my_integer
            Simple description for integer member.

      my_null
            Simple description for none member.

      my_string
            Simple description for string member.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      my_function(one, two[, three][, four])
            Simple description for a function.

#@<OUT> The function help is available on the help system
NAME
      my_second_function - Simple description for a function.

SYNTAX
      myGlobal.my_second_function(one, two[, data])

WHERE
      one: Array - Exersices the array parameter usage.
      two: Object - A session object for DB work execution.
      data: Dictionary - A sample dictionary with options for the function.

DESCRIPTION
      This function is to test how the help gets properly registered.

      This parameter must be an array.

      The two parameter must be a Session object.

      The data parameter accepts the following options:

      - myOption: Bool - This option will be used show how options look like at
        the function help.

#@<OUT> The function is added to the object help again
NAME
      myGlobal - User defined global object.

DESCRIPTION
      This object is used to verify how each element help is registered on the
      go.

      The reason for this is because this object was registered as global right
      after it's creation

PROPERTIES
      my_array
            Simple description for array member.

      my_boolean
            Simple description for boolean member.

      my_child_object
            Child object with it's own definitions.

      my_dictionary
            Simple description for dictionary member.

      my_float
            Simple description for float member.

      my_integer
            Simple description for integer member.

      my_null
            Simple description for none member.

      my_string
            Simple description for string member.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      my_function(one, two[, three][, four])
            Simple description for a function.

      my_second_function(one, two[, data])
            Simple description for a function.

#@<OUT> Request the second object help
NAME
      mySecondGlobal - User defined global object.

DESCRIPTION
      This object is used to verify how each element help is registered on the
      go.

      The reason for this is because this object was registered as global right
      after it's creation

PROPERTIES
      my_array
            Simple description for array member.

      my_boolean
            Simple description for boolean member.

      my_child_object
            Child object with it's own definitions.

      my_dictionary
            Simple description for dictionary member.

      my_float
            Simple description for float member.

      my_integer
            Simple description for integer member.

      my_null
            Simple description for none member.

      my_string
            Simple description for string member.

FUNCTIONS
      help([member])
            Provides help about this object and it's members

      my_function(one, two[, three][, four])
            Simple description for a function.

      my_second_function(one, two[, data])
            Simple description for a function.

#@<OUT> Finally the last global help
GLOBAL OBJECTS

The following modules and objects are ready for use when the shell starts:

 - dba            Used for InnoDB cluster administration.
 - myGlobal       User defined global object.
 - mySecondGlobal User defined global object.
 - mysql          Support for connecting to MySQL servers using the classic
                  MySQL protocol.
 - mysqlx         Used to work with X Protocol sessions using the MySQL X
                  DevAPI.
 - shell          Gives access to general purpose functions and properties.
 - testutil
 - util           Global object that groups miscellaneous tools like upgrade
                  checker and JSON import.

For additional information on these global objects use: <object>.help()
