//@ Initialization
||

//@<OUT> Schema.help
NAME
      Schema - Represents a Schema as retrieved from a session created using
               the X Protocol.

DESCRIPTION
      View Support

      MySQL Views are stored queries that when executed produce a result set.

      MySQL supports the concept of Updatable Views: in specific conditions are
      met, Views can be used not only to retrieve data from them but also to
      update, add and delete records.

      For the purpose of this API, Views behave similar to a Table, and so they
      are treated as Tables.

      Tables and Collections as Properties

      A Schema object may expose tables and collections as properties, this way
      they can be accessed as:

      - schema.<collection_name>
      - schema.<table_name>

      This handy way of accessing tables and collections is available if they
      met the following conditions:

      - They existed at the moment the Schema object was retrieved from the
        session.
      - The name is a valid identifier.
      - The name is different from any other property or function on the Schema
        object.

      If any of the conditions is not met, the way to access the table or
      collection is by using the standard DevAPI functions:

      - schema.getTable(<name>)
      - schema.getCollection(<name>)

PROPERTIES
      name
            The name of this database object.

      schema
            The Schema object of this database object.

      session
            The Session object of this database object.

      Some tables and collections are also exposed as properties of the Schema
      object. For details look at 'Tables and Collections as Properties' on the
      DETAILS section.

FUNCTIONS
      createCollection(name[, options])
            Creates in the current schema a new collection with the specified
            name and retrieves an object representing the new collection
            created.

      dropCollection()
            Drops the specified collection.

      existsInDatabase()
            Verifies if this object exists in the database.

      getCollection(name)
            Returns the Collection of the given name for this schema.

      getCollectionAsTable(name)
            Returns a Table object representing a Collection on the database.

      getCollections()
            Returns a list of Collections for this Schema.

      getName()
            Returns the name of this database object.

      getSchema()
            Returns the Schema object of this database object.

      getSession()
            Returns the Session object of this database object.

      getTable(name)
            Returns the Table of the given name for this schema.

      getTables()
            Returns a list of Tables for this Schema.

      help([member])
            Provides help about this class and it's members

      modifyCollection(name, options)
            Modifies the schema validation of a collection.

RELATED TOPICS
 - Dynamic Properties

//@<OUT> Help on dynamic properties
A Schema object may expose tables and collections as properties, this way they
can be accessed as:

- schema.<collection_name>
- schema.<table_name>

This handy way of accessing tables and collections is available if they met the
following conditions:

- They existed at the moment the Schema object was retrieved from the session.
- The name is a valid identifier.
- The name is different from any other property or function on the Schema
  object.

If any of the conditions is not met, the way to access the table or collection
is by using the standard DevAPI functions:

- schema.getTable(<name>)
- schema.getCollection(<name>)

//@<OUT> Help on name
NAME
      name - The name of this database object.

SYNTAX
      <Schema>.name

//@<OUT> Help on schema
NAME
      schema - The Schema object of this database object.

SYNTAX
      <Schema>.schema

//@<OUT> Help on session
NAME
      session - The Session object of this database object.

SYNTAX
      <Schema>.session

//@<OUT> Help on createCollection
NAME
      createCollection - Creates in the current schema a new collection with
                         the specified name and retrieves an object
                         representing the new collection created.

SYNTAX
      <Schema>.createCollection(name[, options])

WHERE
      name: the name of the collection.
      options: Dictionary with options.

RETURNS
      the new created collection.

DESCRIPTION
      To specify a name for a collection, follow the naming conventions in
      MySQL.

      The options dictionary may contain the following attributes:

      - reuseExistingObject: boolean, indicating if the call should succeed
        when collection with the same name already exists.
      - validation: object defining JSON schema validation for the collection.

      The validation object allows the following attributes:

      - level: which can be either 'strict' or 'off'.
      - schema: a JSON schema specification as described at json-schema.org.

//@<OUT> Help on dropCollection
NAME
      dropCollection - Drops the specified collection.

SYNTAX
      <Schema>.dropCollection()

RETURNS
      Nothing.

//@<OUT> Help on modifyCollection
NAME
      modifyCollection - Modifies the schema validation of a collection.

SYNTAX
      <Schema>.modifyCollection(name, options)

WHERE
      name: the name of the collection.
      options: dictionary with options.

DESCRIPTION
      The options dictionary may contain the following attributes:

      - validation: object defining JSON schema validation for the collection.

      The validation object allows the following attributes:

      - level: which can be either 'strict' or 'off'.
      - schema: a JSON schema specification as described at json-schema.org.

//@<OUT> Help on existsInDatabase
NAME
      existsInDatabase - Verifies if this object exists in the database.

SYNTAX
      <Schema>.existsInDatabase()

RETURNS
      A boolean indicating if the object still exists on the database.

//@<OUT> Help on getCollection
NAME
      getCollection - Returns the Collection of the given name for this schema.

SYNTAX
      <Schema>.getCollection(name)

WHERE
      name: the name of the Collection to look for.

RETURNS
      the Collection object matching the name.

DESCRIPTION
      Verifies if the requested Collection exist on the database, if exists,
      returns the corresponding Collection object.

      Updates the Collections cache.

//@<OUT> Help on getCollectionAsTable
NAME
      getCollectionAsTable - Returns a Table object representing a Collection
                             on the database.

SYNTAX
      <Schema>.getCollectionAsTable(name)

WHERE
      name: the name of the collection to be retrieved as a table.

RETURNS
      the Table object representing the collection or undefined.

//@<OUT> Help on getCollections
NAME
      getCollections - Returns a list of Collections for this Schema.

SYNTAX
      <Schema>.getCollections()

RETURNS
      A List containing the Collection objects available for the Schema.

DESCRIPTION
      Pulls from the database the available Tables, Views and Collections.

      Does a full refresh of the Tables, Views and Collections cache.

      Returns a List of available Collection objects.

//@<OUT> Help on getName
NAME
      getName - Returns the name of this database object.

SYNTAX
      <Schema>.getName()

//@<OUT> Help on getSchema
NAME
      getSchema - Returns the Schema object of this database object.

SYNTAX
      <Schema>.getSchema()

RETURNS
      The Schema object used to get to this object.

DESCRIPTION
      Note that the returned object can be any of:

      - Schema: if the object was created/retrieved using a Schema instance.
      - ClassicSchema: if the object was created/retrieved using an
        ClassicSchema.
      - Null: if this database object is a Schema or ClassicSchema.

//@<OUT> Help on getSession
NAME
      getSession - Returns the Session object of this database object.

SYNTAX
      <Schema>.getSession()

RETURNS
      The Session object used to get to this object.

DESCRIPTION
      Note that the returned object can be any of:

      - Session: if the object was created/retrieved using an Session instance.
      - ClassicSession: if the object was created/retrieved using an
        ClassicSession.

//@<OUT> Help on getTable
NAME
      getTable - Returns the Table of the given name for this schema.

SYNTAX
      <Schema>.getTable(name)

WHERE
      name: the name of the Table to look for.

RETURNS
      the Table object matching the name.

DESCRIPTION
      Verifies if the requested Table exist on the database, if exists, returns
      the corresponding Table object.

      Updates the Tables cache.

//@<OUT> Help on getTables
NAME
      getTables - Returns a list of Tables for this Schema.

SYNTAX
      <Schema>.getTables()

RETURNS
      A List containing the Table objects available for the Schema.

DESCRIPTION
      Pulls from the database the available Tables, Views and Collections.

      Does a full refresh of the Tables, Views and Collections cache.

      Returns a List of available Table objects.

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Schema>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.
