#@<OUT> mysqlx.Type
NAME
      Type - Data type constants.

SYNTAX
      mysqlx.Type

DESCRIPTION
      The data type constants assigned to a Column object retrieved through
      RowResult.get_columns().

PROPERTIES
      BIGINT
            A large integer.

      BIT
            A bit-value type.

      BYTES
            A binary string.

      DATE
            A date.

      DATETIME
            A date and time combination.

      DECIMAL
            A packed "exact" fixed-point number.

      ENUM
            An enumeration.

      FLOAT
            A floating-point number.

      GEOMETRY
            A geometry type.

      INT
            A normal-size integer.

      JSON
            A JSON-format string.

      MEDIUMINT
            A medium-sized integer.

      SET
            A set.

      SMALLINT
            A small integer.

      STRING
            A character string.

      TIME
            A time.

      TINYINT
            A very small integer.

FUNCTIONS
      help([member])
            Provides help about this class and it's members

#@<OUT> mysqlx.date_value
NAME
      date_value - Creates a Date object which represents a date time.

SYNTAX
      mysqlx.date_value(year, month, day[, hour, day, minute[, milliseconds]])

WHERE
      year: The year to be used in the new Date object.
      month: The month to be used in the new Date object.
      day: The month to be used in the new Date object.
      hour: Hour to be used in the new Date object.
      minutes: Minutes to be used in the new Date object.
      seconds: Seconds to be used in the new Date object.
      milliseconds: Milliseconds to be used in the new Date object.

DESCRIPTION
      This function creates a Date object containing:

      - A date value.
      - A date and time value.
      - A date and time value with milliseconds.

#@<OUT> mysqlx.expr
NAME
      expr - Creates an Expression object based on a string.

SYNTAX
      mysqlx.expr(expressionStr)

WHERE
      expressionStr: The expression to be represented by the object

DESCRIPTION
      An expression object is required in many operations on the X DevAPI.

      Some applications of the expression objects include:

      - Creation of documents based on a JSON string
      - Defining calculated fields when inserting data on the database
      - Defining calculated fields when pulling data from the database

#@<OUT> mysqlx.get_session
NAME
      get_session - Creates a Session instance using the provided connection
                    data.

SYNTAX
      mysqlx.get_session(connectionData[, password])

WHERE
      connectionData: The connection data for the session
      password: Password for the session

RETURNS
       A Session

DESCRIPTION
      A Session object uses the X Protocol to allow executing operations on the
      connected MySQL Server.

      The connection data may be specified in the following formats:

      - A URI string
      - A dictionary with the connection options

      A basic URI string has the following format:

      [scheme://][user[:password]@]<host[:port]|socket>[/schema][?option=value&option=value...]

      SSL Connection Options

      The following options are valid for use either in a URI or in a
      dictionary:

      - ssl-mode: the SSL mode to be used in the connection.
      - ssl-ca: the path to the X509 certificate authority in PEM format.
      - ssl-capath: the path to the directory that contains the X509
        certificates authorities in PEM format.
      - ssl-cert: The path to the X509 certificate in PEM format.
      - ssl-key: The path to the X509 key in PEM format.
      - ssl-crl: The path to file that contains certificate revocation lists.
      - ssl-crlpath: The path of directory that contains certificate revocation
        list files.
      - ssl-cipher: SSL Cipher to use.
      - tls-version: List of protocols permitted for secure connections
      - auth-method: Authentication method
      - get-server-public-key: Request public key from the server required for
        RSA key pair-based password exchange. Use when connecting to MySQL 8.0
        servers with classic MySQL sessions with SSL mode DISABLED.
      - server-public-key-path: The path name to a file containing a
        client-side copy of the public key required by the server for RSA key
        pair-based password exchange. Use when connecting to MySQL 8.0 servers
        with classic MySQL sessions with SSL mode DISABLED.
      - connect-timeout: The connection timeout in milliseconds. If not
        provided a default timeout of 10 seconds will be used. Specifying a
        value of 0 disables the connection timeout.
      - compression: Enable/disable compression in client/server protocol,
        valid values: "true", "false", "1", and "0".

      When these options are defined in a URI, their values must be URL
      encoded.

      The following options are also valid when a dictionary is used:

      Base Connection Options

      - scheme: the protocol to be used on the connection.
      - user: the MySQL user name to be used on the connection.
      - dbUser: alias for user.
      - password: the password to be used on the connection.
      - dbPassword: same as password.
      - host: the hostname or IP address to be used on a TCP connection.
      - port: the port to be used in a TCP connection.
      - socket: the socket file name to be used on a connection through unix
        sockets.
      - schema: the schema to be selected once the connection is done.

      ATTENTION: The dbUser and dbPassword options are will be removed in a
                 future release.

      The connection options are case insensitive and can only be defined once.

      If an option is defined more than once, an error will be generated.

      For additional information on connection data use \? connection.

#@<OUT> mysqlx.help
NAME
      help - Provides help about this module and it's members

SYNTAX
      mysqlx.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> mysqlx help
NAME
      mysqlx - Encloses the functions and classes available to interact with an
               X Protocol enabled MySQL Product.

DESCRIPTION
      The objects contained on this module provide a full API to interact with
      the different MySQL Products implementing the X Protocol.

      In the case of a MySQL Server the API will enable doing operations on the
      different database objects such as schema management operations and both
      table and collection management and CRUD operations. (CRUD: Create, Read,
      Update, Delete).

      Intention of the module is to provide a full API for development through
      scripting languages such as JavaScript and Python, this would be normally
      achieved through a normal session.

      To use the properties and functions available on this module you first
      need to import it.

      When running the shell in interactive mode, this module is automatically
      imported.

CONSTANTS
 - LockContention Row locking mode constants.
 - Type           Data type constants.

FUNCTIONS
      date_value(year, month, day[, hour, day, minute[, milliseconds]])
            Creates a Date object which represents a date time.

      expr(expressionStr)
            Creates an Expression object based on a string.

      get_session(connectionData[, password])
            Creates a Session instance using the provided connection data.

      help([member])
            Provides help about this module and it's members

CLASSES
 - BaseResult       Base class for the different types of results returned by
                    the server.
 - Collection       A Collection is a container that may be used to store
                    Documents in a MySQL database.
 - CollectionAdd    Operation to insert documents into a Collection.
 - CollectionFind   Operation to retrieve documents from a Collection.
 - CollectionModify Operation to update documents on a Collection.
 - CollectionRemove Operation to delete documents on a Collection.
 - DatabaseObject   Provides base functionality for database objects.
 - DocResult        Allows traversing the DbDoc objects returned by a
                    Collection.find operation.
 - Result           Allows retrieving information about non query operations
                    performed on the database.
 - RowResult        Allows traversing the Row objects returned by a
                    Table.select operation.
 - Schema           Represents a Schema as retrived from a session created
                    using the X Protocol.
 - Session          Enables interaction with a MySQL Server using the X
                    Protocol.
 - SqlExecute       Handler for execution SQL statements, supports parameter
                    binding.
 - SqlResult        Allows browsing through the result information after
                    performing an operation on the database done through
                    Session.sql
 - Table            Represents a Table on an Schema, retrieved with a session
                    created using mysqlx module.
 - TableDelete      Operation to delete data from a table.
 - TableInsert      Operation to insert data into a table.
 - TableSelect      Operation to retrieve rows from a table.
 - TableUpdate      Operation to add update records in a Table.

#@<OUT> Help on LockContention
NAME
      LockContention - Row locking mode constants.

SYNTAX
      mysqlx.LockContention

DESCRIPTION
      These constants are used to indicate the locking mode to be used at the
      lock_shared and lock_exclusive functions of the TableSelect and
      CollectionFind objects.

PROPERTIES
      DEFAULT
            A default locking mode.

      NOWAIT
            A locking read never waits to acquire a row lock. The query
            executes immediately, failing with an error if a requested row is
            locked.

      SKIP_LOCKED
            A locking read never waits to acquire a row lock. The query
            executes immediately, removing locked rows from the result set.

FUNCTIONS
      help([member])
            Provides help about this class and it's members

#@<OUT> Help on BaseResult
NAME
      BaseResult - Base class for the different types of results returned by
                   the server.

DESCRIPTION
      Base class for the different types of results returned by the server.

PROPERTIES
      affected_items_count
            Same as get_affected_items_count

      execution_time
            Same as get_execution_time

      warning_count
            Same as get_warning_count

            ATTENTION: This property will be removed in a future release, use
                       the warnings_count property instead.

      warnings
            Same as get_warnings

      warnings_count
            Same as get_warnings_count

FUNCTIONS
      get_affected_items_count()
            The the number of affected items for the last operation.

      get_execution_time()
            Retrieves a string value indicating the execution time of the
            executed operation.

      get_warning_count()
            The number of warnings produced by the last statement execution.

            ATTENTION: This function will be removed in a future release, use
                       the get_warnings_count function instead.

      get_warnings()
            Retrieves the warnings generated by the executed operation.

      get_warnings_count()
            The number of warnings produced by the last statement execution.

      help([member])
            Provides help about this class and it's members

#@<OUT> Help on Collection
NAME
      Collection - A Collection is a container that may be used to store
                   Documents in a MySQL database.

DESCRIPTION
      A Document is a set of key and value pairs, as represented by a JSON
      object.

      A Document is represented internally using the MySQL binary JSON object,
      through the JSON MySQL datatype.

      The values of fields can contain other documents, arrays, and lists of
      documents.

PROPERTIES
      name
            The name of this database object.

      schema
            The Schema object of this database object.

      session
            The Session object of this database object.

FUNCTIONS
      add(...)
            Inserts one or more documents into a collection.

      add_or_replace_one(id, doc)
            Replaces or adds a document in a collection.

      count()
            Returns the number of documents in the collection.

      create_index(name, indexDefinition)
            Creates an index on a collection.

      drop_index()
            Drops an index from a collection.

      exists_in_database()
            Verifies if this object exists in the database.

      find([searchCondition])
            Retrieves documents from a collection, matching a specified
            criteria.

      get_name()
            Returns the name of this database object.

      get_one(id)
            Fetches the document with the given _id from the collection.

      get_schema()
            Returns the Schema object of this database object.

      get_session()
            Returns the Session object of this database object.

      help([member])
            Provides help about this class and it's members

      modify(searchCondition)
            Creates a collection update handler.

      remove(searchCondition)
            Creates a document deletion handler.

      remove_one(id)
            Removes document with the given _id value.

      replace_one(id, doc)
            Replaces an existing document with a new document.

#@<OUT> Help on CollectionAdd
NAME
      CollectionAdd - Operation to insert documents into a Collection.

DESCRIPTION
      A CollectionAdd object represents an operation to add documents into a
      Collection, it is created through the add function on the Collection
      class.

FUNCTIONS
      add(...)
            Adds documents into a collection.

      execute()
            Executes the add operation, the documents are added to the target
            collection.

      help([member])
            Provides help about this class and it's members

#@<OUT> Help on CollectionFind
NAME
      CollectionFind - Operation to retrieve documents from a Collection.

DESCRIPTION
      A CollectionFind object represents an operation to retreve documents from
      a Collection, it is created through the find function on the Collection
      class.

FUNCTIONS
      bind(name, value)
            Binds a value to a specific placeholder used on this CollectionFind
            object.

      execute()
            Executes the find operation with all the configured options.

      fields(...)
            Sets the fields to be retrieved from each document matching the
            criteria on this find operation.

      find([searchCondition])
            Sets the search condition to identify the Documents to be retrieved
            from the owner Collection.

      group_by(...)
            Sets a grouping criteria for the resultset.

      having(condition)
            Sets a condition for records to be considered in agregate function
            operations.

      help([member])
            Provides help about this class and it's members

      limit(numberOfDocs)
            Sets the maximum number of documents to be returned by the
            operation.

      lock_exclusive([lockContention])
            Instructs the server to acquire an exclusive lock on documents
            matched by this find operation.

      lock_shared([lockContention])
            Instructs the server to acquire shared row locks in documents
            matched by this find operation.

      skip(numberOfDocs)
            Sets number of documents to skip on the resultset when a limit has
            been defined.

            ATTENTION: This function will be removed in a future release, use
                       the offset function instead.

      sort(...)
            Sets the sorting criteria to be used on the DocResult.

#@<OUT> Help on CollectionModify
NAME
      CollectionModify - Operation to update documents on a Collection.

DESCRIPTION
      A CollectionModify object represents an operation to update documents on
      a Collection, it is created through the modify function on the Collection
      class.

FUNCTIONS
      array_append(docPath, value)
            Appends a value into an array attribute in documents of a
            collection.

      array_delete(docPath)
            Deletes the value at a specific position in an array attribute in
            documents of a collection.

            ATTENTION: This function will be removed in a future release, use
                       the unset function instead.

      array_insert(docPath, value)
            Inserts a value into a specific position in an array attribute in
            documents of a collection.

      bind(name:, value:)
            Binds a value to a specific placeholder used on this
            CollectionModify object.

      execute()
            Executes the update operations added to the handler with the
            configured filter and limit.

      help([member])
            Provides help about this class and it's members

      limit(numberOfDocs)
            Sets a limit for the documents to be updated by the operations
            added to the handler.

      merge(document)
            Adds attributes taken from a document into the documents in a
            collection.

            ATTENTION: This function will be removed in a future release, use
                       the patch function instead.

      modify(searchCondition)
            Sets the search condition to identify the Documents to be updated
            on the owner Collection.

      patch(document)
            Performs modifications on a document based on a patch JSON object.

      set(attribute, value)
            Sets or updates attributes on documents in a collection.

      sort(...)
            Sets the document order in which the update operations added to the
            handler should be done.

      unset(...)
            Removes attributes from documents in a collection.

#@<OUT> Help on CollectionRemove
NAME
      CollectionRemove - Operation to delete documents on a Collection.

DESCRIPTION
      A CollectionRemove object represents an operation to remove documents on
      a Collection, it is created through the remove function on the Collection
      class.

FUNCTIONS
      bind(name, value)
            Binds a value to a specific placeholder used on this
            CollectionRemove object.

      execute()
            Executes the document deletion with the configured filter and
            limit.

      help([member])
            Provides help about this class and it's members

      limit(numberOfDocs)
            Sets a limit for the documents to be deleted.

      remove(searchCondition)
            Sets the search condition to filter the documents to be deleted
            from the owner Collection.

      sort(...)
            Sets the order in which the deletion should be done.

#@<OUT> Help on DatabaseObject
NAME
      DatabaseObject - Provides base functionality for database objects.

DESCRIPTION
      Provides base functionality for database objects.

PROPERTIES
      name
            The name of this database object.

      schema
            The Schema object of this database object.

      session
            The Session object of this database object.

FUNCTIONS
      exists_in_database()
            Verifies if this object exists in the database.

      get_name()
            Returns the name of this database object.

      get_schema()
            Returns the Schema object of this database object.

      get_session()
            Returns the Session object of this database object.

      help([member])
            Provides help about this class and it's members

#@<OUT> Help on DocResult
NAME
      DocResult - Allows traversing the DbDoc objects returned by a
                  Collection.find operation.

DESCRIPTION
      Allows traversing the DbDoc objects returned by a Collection.find
      operation.

PROPERTIES
      affected_items_count
            Same as get_affected_items_count

      execution_time
            Same as get_execution_time

      warning_count
            Same as get_warning_count

            ATTENTION: This property will be removed in a future release, use
                       the warnings_count property instead.

      warnings
            Same as get_warnings

      warnings_count
            Same as get_warnings_count

FUNCTIONS
      fetch_all()
            Returns a list of DbDoc objects which contains an element for every
            unread document.

      fetch_one()
            Retrieves the next DbDoc on the DocResult.

      get_affected_items_count()
            The the number of affected items for the last operation.

      get_execution_time()
            Retrieves a string value indicating the execution time of the
            executed operation.

      get_warning_count()
            The number of warnings produced by the last statement execution.

            ATTENTION: This function will be removed in a future release, use
                       the get_warnings_count function instead.

      get_warnings()
            Retrieves the warnings generated by the executed operation.

      get_warnings_count()
            The number of warnings produced by the last statement execution.

      help([member])
            Provides help about this class and it's members

#@<OUT> Help on Result
NAME
      Result - Allows retrieving information about non query operations
               performed on the database.

DESCRIPTION
      An instance of this class will be returned on the CRUD operations that
      change the content of the database:

      - On Table: insert, update and delete
      - On Collection: add, modify and remove

      Other functions on the Session class also return an instance of this
      class:

      - Transaction handling functions
      - Transaction handling functions

PROPERTIES
      affected_item_count
            Same as get_affected_item_count

            ATTENTION: This property will be removed in a future release, use
                       the affected_items_count property instead.

      affected_items_count
            Same as get_affected_items_count

      auto_increment_value
            Same as get_auto_increment_value

      execution_time
            Same as get_execution_time

      generated_ids
            Same as get_generated_ids.

      warning_count
            Same as get_warning_count

            ATTENTION: This property will be removed in a future release, use
                       the warnings_count property instead.

      warnings
            Same as get_warnings

      warnings_count
            Same as get_warnings_count

FUNCTIONS
      get_affected_item_count()
            The the number of affected items for the last operation.

            ATTENTION: This function will be removed in a future release, use
                       the get_affected_items_count function instead.

      get_affected_items_count()
            The the number of affected items for the last operation.

      get_auto_increment_value()
            The last insert id auto generated (from an insert operation)

      get_execution_time()
            Retrieves a string value indicating the execution time of the
            executed operation.

      get_generated_ids()
            Returns the list of document ids generated on the server.

      get_warning_count()
            The number of warnings produced by the last statement execution.

            ATTENTION: This function will be removed in a future release, use
                       the get_warnings_count function instead.

      get_warnings()
            Retrieves the warnings generated by the executed operation.

      get_warnings_count()
            The number of warnings produced by the last statement execution.

      help([member])
            Provides help about this class and it's members

#@<OUT> Help on RowResult
NAME
      RowResult - Allows traversing the Row objects returned by a Table.select
                  operation.

DESCRIPTION
      Allows traversing the Row objects returned by a Table.select operation.

PROPERTIES
      affected_items_count
            Same as get_affected_items_count

      column_count
            Same as get_column_count

      column_names
            Same as get_column_names

      columns
            Same as get_columns

      execution_time
            Same as get_execution_time

      warning_count
            Same as get_warning_count

            ATTENTION: This property will be removed in a future release, use
                       the warnings_count property instead.

      warnings
            Same as get_warnings

      warnings_count
            Same as get_warnings_count

FUNCTIONS
      fetch_all()
            Returns a list of DbDoc objects which contains an element for every
            unread document.

      fetch_one()
            Retrieves the next Row on the RowResult.

      get_affected_items_count()
            The the number of affected items for the last operation.

      get_column_count()
            Retrieves the number of columns on the current result.

      get_column_names()
            Gets the columns on the current result.

      get_columns()
            Gets the column metadata for the columns on the active result.

      get_execution_time()
            Retrieves a string value indicating the execution time of the
            executed operation.

      get_warning_count()
            The number of warnings produced by the last statement execution.

            ATTENTION: This function will be removed in a future release, use
                       the get_warnings_count function instead.

      get_warnings()
            Retrieves the warnings generated by the executed operation.

      get_warnings_count()
            The number of warnings produced by the last statement execution.

      help([member])
            Provides help about this class and it's members

#@<OUT> Help on Schema
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

      help([member])
            Provides help about this class and it's members

#@<OUT> Help on Session
NAME
      Session - Enables interaction with a MySQL Server using the X Protocol.

DESCRIPTION
      Document Store functionality can be used through this object, in addition
      to SQL.

      This class allows performing database operations such as:

      - Schema management operations.
      - Access to relational tables.
      - Access to Document Store collections.
      - Enabling/disabling warning generation.
      - Retrieval of connection information.

PROPERTIES
      current_schema
            Retrieves the active schema on the session.

      default_schema
            Retrieves the Schema configured as default for the session.

      uri
            Retrieves the URI for the current session.

FUNCTIONS
      close()
            Closes the session.

      commit()
            Commits all the operations executed after a call to
            startTransaction().

      create_schema(name)
            Creates a schema on the database and returns the corresponding
            object.

      drop_schema(name)
            Drops the schema with the specified name.

      get_current_schema()
            Retrieves the active schema on the session.

      get_default_schema()
            Retrieves the Schema configured as default for the session.

      get_schema(name)
            Retrieves a Schema object from the current session through it's
            name.

      get_schemas()
            Retrieves the Schemas available on the session.

      get_uri()
            Retrieves the URI for the current session.

      help([member])
            Provides help about this class and it's members

      is_open()
            Returns true if session is known to be open.

      quote_name(id)
            Escapes the passed identifier.

      release_savepoint(name)
            Removes a savepoint defined on a transaction.

      rollback()
            Discards all the operations executed after a call to
            startTransaction().

      rollback_to(name)
            Rolls back the transaction to the named savepoint without
            terminating the transaction.

      set_current_schema(name)
            Sets the current schema for this session, and returns the schema
            object for it.

      set_fetch_warnings(enable)
            Enables or disables warning generation.

      set_savepoint([name])
            Creates or replaces a transaction savepoint with the given name.

      sql(statement)
            Creates a SqlExecute object to allow running the received SQL
            statement on the target MySQL Server.

      start_transaction()
            Starts a transaction context on the server.

#@<OUT> Help on SqlExecute
NAME
      SqlExecute - Handler for execution SQL statements, supports parameter
                   binding.

DESCRIPTION
      This object should only be created by calling the sql function at a
      Session instance.

FUNCTIONS
      bind(value, values)
            Registers a parameter to be bound on the execution of the SQL
            statement.

            Registers a list of parameter to be bound on the execution of the
            SQL statement.

      execute()
            Executes the sql statement.

      help([member])
            Provides help about this class and it's members

      sql(statement)
            Sets the sql statement to be executed by this handler.

#@<OUT> Help on Table
NAME
      Table - Represents a Table on an Schema, retrieved with a session created
              using mysqlx module.

DESCRIPTION
      Represents a Table on an Schema, retrieved with a session created using
      mysqlx module.

PROPERTIES
      name
            The name of this database object.

      schema
            The Schema object of this database object.

      session
            The Session object of this database object.

FUNCTIONS
      count()
            Returns the number of records in the table.

      delete()
            Creates a record deletion handler.

      exists_in_database()
            Verifies if this object exists in the database.

      get_name()
            Returns the name of this database object.

      get_schema()
            Returns the Schema object of this database object.

      get_session()
            Returns the Session object of this database object.

      help([member])
            Provides help about this class and it's members

      insert(...)
            Creates TableInsert object to insert new records into the table.

      is_view()
            Indicates whether this Table object represents a View on the
            database.

      select(...)
            Creates a TableSelect object to retrieve rows from the table.

      update()
            Creates a record update handler.

#@<OUT> Help on TableDelete
NAME
      TableDelete - Operation to delete data from a table.

DESCRIPTION
      A TableDelete represents an operation to remove records from a Table, it
      is created through the delete function on the Table class.

FUNCTIONS
      bind(name, value)
            Binds a value to a specific placeholder used on this operation.

      delete()
            Initializes the deletion operation.

      execute()
            Executes the delete operation with all the configured options.

      help([member])
            Provides help about this class and it's members

      limit(numberOfRows)
            Sets the maximum number of rows to be deleted by the operation.

      order_by(...)
            Sets the order in which the records will be deleted.

      where([expression])
            Sets the search condition to filter the records to be deleted from
            the Table.

#@<OUT> Help on TableInsert
NAME
      TableInsert - Operation to insert data into a table.

DESCRIPTION
      A TableInsert object is created through the insert function on the Table
      class.

FUNCTIONS
      execute()
            Executes the insert operation.

      help([member])
            Provides help about this class and it's members

      insert(...)
            Initializes the insertion operation.

      values(value[, value, ...])
            Adds a new row to the insert operation with the given values.

#@<OUT> Help on TableSelect
NAME
      TableSelect - Operation to retrieve rows from a table.

DESCRIPTION
      A TableSelect represents a query to retrieve rows from a Table. It is is
      created through the select function on the Table class.

FUNCTIONS
      bind(name, value)
            Binds a value to a specific placeholder used on this operation.

      execute()
            Executes the select operation with all the configured options.

      group_by(...)
            Sets a grouping criteria for the retrieved rows.

      having(condition)
            Sets a condition for records to be considered in agregate function
            operations.

      help([member])
            Provides help about this class and it's members

      limit(numberOfRows)
            Sets the maximum number of rows to be returned on the select
            operation.

      lock_exclusive([lockContention])
            Instructs the server to acquire an exclusive lock on rows matched
            by this find operation.

      lock_shared([lockContention])
            Instructs the server to acquire shared row locks in documents
            matched by this find operation.

      offset(numberOfRows)
            Sets number of rows to skip on the resultset when a limit has been
            defined.

      order_by(...)
            Sets the order in which the records will be retrieved.

      select(...)
            Defines the columns to be retrieved from the table.

      where([expression])
            Sets the search condition to filter the records to be retrieved
            from the Table.

#@<OUT> Help on TableUpdate
NAME
      TableUpdate - Operation to add update records in a Table.

DESCRIPTION
      A TableUpdate object is used to update rows in a Table, is created
      through the update function on the Table class.

FUNCTIONS
      bind(name, value)
            Binds a value to a specific placeholder used on this operation.

      execute()
            Executes the update operation with all the configured options.

      help([member])
            Provides help about this class and it's members

      limit(numberOfRows)
            Sets the maximum number of rows to be updated by the operation.

      order_by(...)
            Sets the order in which the records will be updated.

      set(attribute, value)
            Adds an update operation.

      update()
            Initializes the update operation.

      where([expression])
            Sets the search condition to filter the records to be updated.
