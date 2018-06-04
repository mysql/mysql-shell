#@ __global__
||

#@<OUT> schema
NAME
      Schema - Represents a Schema as retrived from a session created using the
               X Protocol.

DESCRIPTION
      View Support

      MySQL Views are stored queries that when executed produce a result set.

      MySQL supports the concept of Updatable Views: in specific conditions are
      met, Views can be used not only to retrieve data from them but also to
      update, add and delete records.

      For the purpose of this API, Views behave similar to a Table, and so they
      are treated as Tables.

PROPERTIES
      name
            The name of this database object.

      schema
            The Schema object of this database object.

      session
            The Session object of this database object.

FUNCTIONS
      create_collection(name)
            Creates in the current schema a new collection with the specified
            name and retrieves an object representing the new collection
            created.

      drop_collection()
            Drops the specified collection.

      exists_in_database()
            Verifies if this object exists in the database.

      get_collection(name)
            Returns the Collection of the given name for this schema.

      get_collection_as_table(name)
            Returns a Table object representing a Collection on the database.

      get_collections()
            Returns a list of Collections for this Schema.

      get_name()
            Returns the name of this database object.

      get_schema()
            Returns the Schema object of this database object.

      get_session()
            Returns the Session object of this database object.

      get_table(name)
            Returns the Table of the given name for this schema.

      get_tables()
            Returns a list of Tables for this Schema.

      help()
            Provides help about this class and it's members

#@<OUT> schema.create_collection
NAME
      create_collection - Creates in the current schema a new collection with
                          the specified name and retrieves an object
                          representing the new collection created.

SYNTAX
      <Schema>.create_collection(name)

WHERE
      name: the name of the collection.

RETURNS
       the new created collection.

DESCRIPTION
      To specify a name for a collection, follow the naming conventions in
      MySQL.

#@<OUT> schema.drop_collection
NAME
      drop_collection - Drops the specified collection.

SYNTAX
      <Schema>.drop_collection()

RETURNS
       Nothing.

#@<OUT> schema.exists_in_database
NAME
      exists_in_database - Verifies if this object exists in the database.

SYNTAX
      <Schema>.exists_in_database()

RETURNS
       A boolean indicating if the object still exists on the database.

#@<OUT> schema.get_collection
NAME
      get_collection - Returns the Collection of the given name for this
                       schema.

SYNTAX
      <Schema>.get_collection(name)

WHERE
      name: the name of the Collection to look for.

RETURNS
       the Collection object matching the name.

DESCRIPTION
      Verifies if the requested Collection exist on the database, if exists,
      returns the corresponding Collection object.

      Updates the Collections cache.

#@<OUT> schema.get_collection_as_table
NAME
      get_collection_as_table - Returns a Table object representing a
                                Collection on the database.

SYNTAX
      <Schema>.get_collection_as_table(name)

WHERE
      name: the name of the collection to be retrieved as a table.

RETURNS
       the Table object representing the collection or undefined.

#@<OUT> schema.get_collections
NAME
      get_collections - Returns a list of Collections for this Schema.

SYNTAX
      <Schema>.get_collections()

RETURNS
       A List containing the Collection objects available for the Schema.

DESCRIPTION
      Pulls from the database the available Tables, Views and Collections.

      Does a full refresh of the Tables, Views and Collections cache.

      Returns a List of available Collection objects.

#@<OUT> schema.get_name
NAME
      get_name - Returns the name of this database object.

SYNTAX
      <Schema>.get_name()

#@<OUT> schema.get_schema
NAME
      get_schema - Returns the Schema object of this database object.

SYNTAX
      <Schema>.get_schema()

RETURNS
       The Schema object used to get to this object.

DESCRIPTION
      Note that the returned object can be any of:

      - Schema: if the object was created/retrieved using a Schema instance.
      - ClassicSchema: if the object was created/retrieved using an
        ClassicSchema.
      - Null: if this database object is a Schema or ClassicSchema.

#@<OUT> schema.get_session
NAME
      get_session - Returns the Session object of this database object.

SYNTAX
      <Schema>.get_session()

RETURNS
       The Session object used to get to this object.

DESCRIPTION
      Note that the returned object can be any of:

      - Session: if the object was created/retrieved using an Session instance.
      - ClassicSession: if the object was created/retrieved using an
        ClassicSession.

#@<OUT> schema.get_table
NAME
      get_table - Returns the Table of the given name for this schema.

SYNTAX
      <Schema>.get_table(name)

WHERE
      name: the name of the Table to look for.

RETURNS
       the Table object matching the name.

DESCRIPTION
      Verifies if the requested Table exist on the database, if exists, returns
      the corresponding Table object.

      Updates the Tables cache.

#@<OUT> schema.get_tables
NAME
      get_tables - Returns a list of Tables for this Schema.

SYNTAX
      <Schema>.get_tables()

RETURNS
       A List containing the Table objects available for the Schema.

DESCRIPTION
      Pulls from the database the available Tables, Views and Collections.

      Does a full refresh of the Tables, Views and Collections cache.

      Returns a List of available Table objects.

#@<OUT> schema.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Schema>.help()

#@<OUT> schema.name
NAME
      name - The name of this database object.

SYNTAX
      <Schema>.name

#@<OUT> schema.schema
NAME
      schema - The Schema object of this database object.

SYNTAX
      <Schema>.schema

#@<OUT> schema.session
NAME
      session - The Session object of this database object.

SYNTAX
      <Schema>.session
