//@ Initialization
||

//@<OUT> Row help
NAME
      Row - Represents the a Row in a Result.

DESCRIPTION
      When a row object is created, its fields are exposed as properties of the
      Row object if two conditions are met:

      - Its name must be a valid identifier: [_a-zA-Z][_a-zA-Z0-9]*
      - Its name must be different from names of the members of this object.

      In the case a field does not met these conditions, it must be retrieved
      through <Row>.getField(<fieldName>).

PROPERTIES
      length
            Returns number of fields in the Row.

FUNCTIONS
      getField(name)
            Returns the value of the field named name.

      getLength()
            Returns number of fields in the Row.

      help()
            Provides help about this class and it's members

//@<OUT> Help on length
NAME
      length - Returns number of fields in the Row.

SYNTAX
      <Row>.length

//@<OUT> Help on getField
NAME
      getField - Returns the value of the field named name.

SYNTAX
      <Row>.getField(name)

WHERE
      name: The name of the field to be retrieved.

//@<OUT> Help on getLength
NAME
      getLength - Returns number of fields in the Row.

SYNTAX
      <Row>.getLength()

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Row>.help()

//@ Finalization
||
