//@<OUT> mysqlx help
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
      dateValue(year, month, day[, hour, day, minute[, milliseconds]])
            Creates a Date object which represents a date time.

      expr(expressionStr)
            Creates an Expression object based on a string.

      getSession(connectionData[, password])
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

//@<OUT> Help on LockContention
NAME
      LockContention - Row locking mode constants.

SYNTAX
      mysqlx.LockContention

DESCRIPTION
      These constants are used to indicate the locking mode to be used at the
      lockShared and lockExclusive functions of the TableSelect and
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
            Provides help about this object and it's members

//@<OUT> Help on Type
NAME
      Type - Data type constants.

SYNTAX
      mysqlx.Type

DESCRIPTION
      The data type constants assigned to a Column object retrieved through
      RowResult.getColumns().

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
            Provides help about this object and it's members

//@<OUT> Help on BaseResult
NAME
      BaseResult - Base class for the different types of results returned by
                   the server.

DESCRIPTION
      Base class for the different types of results returned by the server.

PROPERTIES
      affectedItemsCount
            Same as getAffectedItemsCount

      executionTime
            Same as getExecutionTime

      warningCount
            Same as getWarningCount

            ATTENTION: This property will be removed in a future release, use
                       the warningsCount property instead.

      warnings
            Same as getWarnings

      warningsCount
            Same as getWarningsCount

FUNCTIONS
      getAffectedItemsCount()
            The the number of affected items for the last operation.

      getExecutionTime()
            Retrieves a string value indicating the execution time of the
            executed operation.

      getWarningCount()
            The number of warnings produced by the last statement execution.

            ATTENTION: This function will be removed in a future release, use
                       the getWarningsCount function instead.

      getWarnings()
            Retrieves the warnings generated by the executed operation.

      getWarningsCount()
            The number of warnings produced by the last statement execution.

      help([member])
            Provides help about this class and it's members

//@<OUT> Help on Collection
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

      addOrReplaceOne(id, doc)
            Replaces or adds a document in a collection.

      createIndex(name, indexDefinition)
            Creates an index on a collection.

      dropIndex()
            Drops an index from a collection.

      existsInDatabase()
            Verifies if this object exists in the database.

      find([searchCondition])
            Retrieves documents from a collection, matching a specified
            criteria.

      getName()
            Returns the name of this database object.

      getOne(id)
            Fetches the document with the given _id from the collection.

      getSchema()
            Returns the Schema object of this database object.

      getSession()
            Returns the Session object of this database object.

      help([member])
            Provides help about this class and it's members

      modify(searchCondition)
            Creates a collection update handler.

      remove(searchCondition)
            Creates a document deletion handler.

      removeOne(id)
            Removes document with the given _id value.

      replaceOne(id, doc)
            Replaces an existing document with a new document.

//@<OUT> Help on CollectionAdd
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

//@<OUT> Help on CollectionFind
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

      groupBy(...)
            Sets a grouping criteria for the resultset.

      having(condition)
            Sets a condition for records to be considered in agregate function
            operations.

      help([member])
            Provides help about this class and it's members

      limit(numberOfDocs)
            Sets the maximum number of documents to be returned by the
            operation.

      lockExclusive([lockContention])
            Instructs the server to acquire an exclusive lock on documents
            matched by this find operation.

      lockShared([lockContention])
            Instructs the server to acquire shared row locks in documents
            matched by this find operation.

      skip(numberOfDocs)
            Sets number of documents to skip on the resultset when a limit has
            been defined.

            ATTENTION: This function will be removed in a future release, use
                       the offset function instead.

      sort(...)
            Sets the sorting criteria to be used on the DocResult.

//@<OUT> Help on CollectionModify
NAME
      CollectionModify - Operation to update documents on a Collection.

DESCRIPTION
      A CollectionModify object represents an operation to update documents on
      a Collection, it is created through the modify function on the Collection
      class.

FUNCTIONS
      arrayAppend(docPath, value)
            Appends a value into an array attribute in documents of a
            collection.

      arrayDelete(docPath)
            Deletes the value at a specific position in an array attribute in
            documents of a collection.

            ATTENTION: This function will be removed in a future release, use
                       the unset function instead.

      arrayInsert(docPath, value)
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

//@<OUT> Help on CollectionRemove
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

//@<OUT> Help on DatabaseObject
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
      existsInDatabase()
            Verifies if this object exists in the database.

      getName()
            Returns the name of this database object.

      getSchema()
            Returns the Schema object of this database object.

      getSession()
            Returns the Session object of this database object.

      help([member])
            Provides help about this class and it's members

//@<OUT> Help on DocResult
NAME
      DocResult - Allows traversing the DbDoc objects returned by a
                  Collection.find operation.

DESCRIPTION
      Allows traversing the DbDoc objects returned by a Collection.find
      operation.

PROPERTIES
      affectedItemsCount
            Same as getAffectedItemsCount

      executionTime
            Same as getExecutionTime

      warningCount
            Same as getWarningCount

            ATTENTION: This property will be removed in a future release, use
                       the warningsCount property instead.

      warnings
            Same as getWarnings

      warningsCount
            Same as getWarningsCount

FUNCTIONS
      fetchAll()
            Returns a list of DbDoc objects which contains an element for every
            unread document.

      fetchOne()
            Retrieves the next DbDoc on the DocResult.

      getAffectedItemsCount()
            The the number of affected items for the last operation.

      getExecutionTime()
            Retrieves a string value indicating the execution time of the
            executed operation.

      getWarningCount()
            The number of warnings produced by the last statement execution.

            ATTENTION: This function will be removed in a future release, use
                       the getWarningsCount function instead.

      getWarnings()
            Retrieves the warnings generated by the executed operation.

      getWarningsCount()
            The number of warnings produced by the last statement execution.

      help([member])
            Provides help about this class and it's members

//@<OUT> Help on Result
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
      affectedItemCount
            Same as getAffectedItemCount

            ATTENTION: This property will be removed in a future release, use
                       the affectedItemsCount property instead.

      affectedItemsCount
            Same as getAffectedItemsCount

      autoIncrementValue
            Same as getAutoIncrementValue

      executionTime
            Same as getExecutionTime

      generatedIds
            Same as getGeneratedIds.

      warningCount
            Same as getWarningCount

            ATTENTION: This property will be removed in a future release, use
                       the warningsCount property instead.

      warnings
            Same as getWarnings

      warningsCount
            Same as getWarningsCount

FUNCTIONS
      getAffectedItemCount()
            The the number of affected items for the last operation.

            ATTENTION: This function will be removed in a future release, use
                       the getAffectedItemsCount function instead.

      getAffectedItemsCount()
            The the number of affected items for the last operation.

      getAutoIncrementValue()
            The last insert id auto generated (from an insert operation)

      getExecutionTime()
            Retrieves a string value indicating the execution time of the
            executed operation.

      getGeneratedIds()
            Returns the list of document ids generated on the server.

      getWarningCount()
            The number of warnings produced by the last statement execution.

            ATTENTION: This function will be removed in a future release, use
                       the getWarningsCount function instead.

      getWarnings()
            Retrieves the warnings generated by the executed operation.

      getWarningsCount()
            The number of warnings produced by the last statement execution.

      help([member])
            Provides help about this class and it's members

//@<OUT> Help on RowResult
NAME
      RowResult - Allows traversing the Row objects returned by a Table.select
                  operation.

DESCRIPTION
      Allows traversing the Row objects returned by a Table.select operation.

PROPERTIES
      affectedItemsCount
            Same as getAffectedItemsCount

      columnCount
            Same as getColumnCount

      columnNames
            Same as getColumnNames

      columns
            Same as getColumns

      executionTime
            Same as getExecutionTime

      warningCount
            Same as getWarningCount

            ATTENTION: This property will be removed in a future release, use
                       the warningsCount property instead.

      warnings
            Same as getWarnings

      warningsCount
            Same as getWarningsCount

FUNCTIONS
      fetchAll()
            Returns a list of DbDoc objects which contains an element for every
            unread document.

      fetchOne()
            Retrieves the next Row on the RowResult.

      getAffectedItemsCount()
            The the number of affected items for the last operation.

      getColumnCount()
            Retrieves the number of columns on the current result.

      getColumnNames()
            Gets the columns on the current result.

      getColumns()
            Gets the column metadata for the columns on the active result.

      getExecutionTime()
            Retrieves a string value indicating the execution time of the
            executed operation.

      getWarningCount()
            The number of warnings produced by the last statement execution.

            ATTENTION: This function will be removed in a future release, use
                       the getWarningsCount function instead.

      getWarnings()
            Retrieves the warnings generated by the executed operation.

      getWarningsCount()
            The number of warnings produced by the last statement execution.

      help([member])
            Provides help about this class and it's members

//@<OUT> Help on Schema
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
      createCollection(name)
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

//@<OUT> Help on Session
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
      currentSchema
            Retrieves the active schema on the session.

      defaultSchema
            Retrieves the Schema configured as default for the session.

      uri
            Retrieves the URI for the current session.

FUNCTIONS
      close()
            Closes the session.

      commit()
            Commits all the operations executed after a call to
            startTransaction().

      createSchema(name)
            Creates a schema on the database and returns the corresponding
            object.

      dropSchema(name)
            Drops the schema with the specified name.

      getCurrentSchema()
            Retrieves the active schema on the session.

      getDefaultSchema()
            Retrieves the Schema configured as default for the session.

      getSchema(name)
            Retrieves a Schema object from the current session through it's
            name.

      getSchemas()
            Retrieves the Schemas available on the session.

      getUri()
            Retrieves the URI for the current session.

      help([member])
            Provides help about this class and it's members

      isOpen()
            Returns true if session is known to be open.

      quoteName(id)
            Escapes the passed identifier.

      releaseSavepoint(name)
            Removes a savepoint defined on a transaction.

      rollback()
            Discards all the operations executed after a call to
            startTransaction().

      rollbackTo(name)
            Rolls back the transaction to the named savepoint without
            terminating the transaction.

      setCurrentSchema(name)
            Sets the current schema for this session, and returns the schema
            object for it.

      setFetchWarnings(enable)
            Enables or disables warning generation.

      setSavepoint([name])
            Creates or replaces a transaction savepoint with the given name.

      sql(statement)
            Creates a SqlExecute object to allow running the received SQL
            statement on the target MySQL Server.

      startTransaction()
            Starts a transaction context on the server.

//@<OUT> Help on SqlExecute
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

//@<OUT> Help on Table
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
      delete()
            Creates a record deletion handler.

      existsInDatabase()
            Verifies if this object exists in the database.

      getName()
            Returns the name of this database object.

      getSchema()
            Returns the Schema object of this database object.

      getSession()
            Returns the Session object of this database object.

      help([member])
            Provides help about this class and it's members

      insert(...)
            Creates TableInsert object to insert new records into the table.

      isView()
            Indicates whether this Table object represents a View on the
            database.

      select(...)
            Creates a TableSelect object to retrieve rows from the table.

      update()
            Creates a record update handler.

//@<OUT> Help on TableDelete
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

      orderBy(...)
            Sets the order in which the records will be deleted.

      where([expression])
            Sets the search condition to filter the records to be deleted from
            the Table.

//@<OUT> Help on TableInsert
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

//@<OUT> Help on TableSelect
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

      groupBy(...)
            Sets a grouping criteria for the retrieved rows.

      having(condition)
            Sets a condition for records to be considered in agregate function
            operations.

      help([member])
            Provides help about this class and it's members

      limit(numberOfRows)
            Sets the maximum number of rows to be returned on the select
            operation.

      lockExclusive([lockContention])
            Instructs the server to acquire an exclusive lock on rows matched
            by this find operation.

      lockShared([lockContention])
            Instructs the server to acquire shared row locks in documents
            matched by this find operation.

      offset(numberOfRows)
            Sets number of rows to skip on the resultset when a limit has been
            defined.

      orderBy(...)
            Sets the order in which the records will be retrieved.

      select(...)
            Defines the columns to be retrieved from the table.

      where([expression])
            Sets the search condition to filter the records to be retrieved
            from the Table.

//@<OUT> Help on TableUpdate
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

      orderBy(...)
            Sets the order in which the records will be updated.

      set(attribute, value)
            Adds an update operation.

      update()
            Initializes the update operation.

      where([expression])
            Sets the search condition to filter the records to be updated.

