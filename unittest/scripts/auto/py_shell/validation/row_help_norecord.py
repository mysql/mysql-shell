#@ __global__
||

#@<OUT> row
NAME
      Row - Represents the a Row in a Result.

DESCRIPTION
      When a row object is created, its fields are exposed as properties of the
      Row object if two conditions are met:

      - Its name must be a valid identifier: [_a-zA-Z][_a-zA-Z0-9]*
      - Its name must be different from names of the members of this object.

      In the case a field does not met these conditions, it must be retrieved
      through the this function.

PROPERTIES
      length
            Returns number of fields in the Row.

FUNCTIONS
      get_field(name)
            Returns the value of the field named name.

      get_length()
            Returns number of fields in the Row.

      help([member])
            Provides help about this class and it's members

#@<OUT> row.get_field
NAME
      get_field - Returns the value of the field named name.

SYNTAX
      <Row>.get_field(name)

WHERE
      name: The name of the field to be retrieved.

#@<OUT> row.get_length
NAME
      get_length - Returns number of fields in the Row.

SYNTAX
      <Row>.get_length()

#@<OUT> row.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Row>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> row.length
NAME
      length - Returns number of fields in the Row.

SYNTAX
      <Row>.length

