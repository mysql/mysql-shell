//@ Initialization
||

//@<OUT> Collection.help
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

//@<OUT> Help on name
NAME
      name - The name of this database object.

SYNTAX
      <Collection>.name

//@<OUT> Help on schema
NAME
      schema - The Schema object of this database object.

SYNTAX
      <Collection>.schema

//@<OUT> Help on session
NAME
      session - The Session object of this database object.

SYNTAX
      <Collection>.session

//@<OUT> Help on add
NAME
      add - Inserts one or more documents into a collection.

SYNTAX
      Collection.add(...)
                [.execute()]

      add(...)
            This function has the following overloads:

            - add(documentList)
            - add(document[, document, ...])
            - add(mysqlx.expr(...))

            This function receives one or more document definitions to be added
            into a collection.

            A document definition may be provided in two ways:

            - Using a dictionary containing the document fields.
            - Using A JSON string as a document expression.

            There are three ways to add multiple documents:

            - Passing several parameters to the function, each parameter should
              be a document definition.
            - Passing a list of document definitions.
            - Calling this function several times before calling execute().

            To be added, every document must have a string property named '_id'
            ideally with a universal unique identifier (UUID) as value. If the
            '_id' property is missing, it is automatically set with an
            internally generated UUID.

            JSON as Document Expressions

            A document can be represented as a JSON expression as follows:

            mysqlx.expr(<JSON String>)

      execute()
            Executes the add operation, the documents are added to the target
            collection.

//@<OUT> Help on addOrReplaceOne
NAME
      addOrReplaceOne - Replaces or adds a document in a collection.

SYNTAX
      <Collection>.addOrReplaceOne(id, doc)

WHERE
      id: the identifier of the document to be replaced.
      doc: the new document.

RETURNS
       A Result object containing the number of affected rows.

DESCRIPTION
      Replaces the document identified with the given id. If no document is
      found matching the given id the given document will be added to the
      collection.

      Only one document will be affected by this operation.

      The id of the document remains inmutable, if the new document contains a
      different id, it will be ignored.

      Any constraint (unique key) defined on the collection is applicable on
      both the replace and add operations:

      - The replace operation will fail if the new document contains a unique
        key which is already defined for any document in the collection except
        the one being replaced.
      - The add operation will fail if the new document contains a unique key
        which is already defined for any document in the collection.

//@<OUT> Help on createIndex
NAME
      createIndex - Creates an index on a collection.

SYNTAX
      <Collection>.createIndex(name, indexDefinition)

WHERE
      name: the name of the index to be created.
      indexDefinition: a JSON document with the index information.

RETURNS
       a Result object.

DESCRIPTION
      This function will create an index on the collection using the
      information provided in indexDefinition.

      The indexDefinition is a JSON document with the next information:
      {
        fields : [<index_field>, ...],
        type   : <type>
      }

      - fields array of index_field objects, each describing a single document
        member to be included in the index.
      - type string, (optional) the type of index. One of INDEX or SPATIAL.
        Default is INDEX and may be omitted.

      A single index_field description consists of the following fields:
      {
        field    : <field>,
        type     : <type>,
        required : <boolean>
        options  : <uint>,
        srid     : <uint>
      }

      - field: string, the full document path to the document member or field
        to be indexed.
      - type: string, one of the supported SQL column types to map the field
        into. For numeric types, the optional UNSIGNED keyword may follow. For
        the TEXT type, the length to consider for indexing may be added.
      - required: bool, (optional) true if the field is required to exist in
        the document. defaults to false, except for GEOJSON where it defaults
        to true.
      - options: uint, (optional) special option flags for use when decoding
        GEOJSON data
      - srid: uint, (optional) srid value for use when decoding GEOJSON data.

      The 'options' and 'srid' fields in IndexField can and must be present
      only if 'type' is set to 'GEOJSON'.

//@<OUT> Help on dropIndex
NAME
      dropIndex - Drops an index from a collection.

SYNTAX
      <Collection>.dropIndex()

//@<OUT> Help on existsInDatabase
NAME
      existsInDatabase - Verifies if this object exists in the database.

SYNTAX
      <Collection>.existsInDatabase()

RETURNS
       A boolean indicating if the object still exists on the database.

//@<OUT> Help on find
NAME
      find - Retrieves documents from a collection, matching a specified
             criteria.

SYNTAX
      Collection.find([searchCondition])
                [.fields(...)]
                [.groupBy(...)[.having(condition)]]
                [.sort(...)]
                [.limit(numberOfDocs)[.skip(numberOfDocs)]]
                [.lockShared([lockContention])]
                [.lockExclusive([lockContention])]
                [.bind(name, value)]
                [.execute()]

      find([searchCondition])
            Sets the search condition to identify the Documents to be retrieved
            from the owner Collection. If the search condition is not specified
            the find operation will be executed over all the documents in the
            collection.

            The search condition supports parameter binding.

      fields(...)
            This function has the following overloads:

            - fields(fieldList)
            - fields(field[, field, ...])
            - fields(mysqlx.expr(...))

            This function sets the fields to be retrieved from each document
            matching the criteria on this find operation.

            A field is defined as a string value containing an expression
            defining the field to be retrieved.

            The fields to be retrieved can be set using any of the next
            methods:

            - Passing each field definition as an individual string parameter.
            - Passing a list of strings containing the field definitions.
            - Passing a JSON expression representing a document projection to
              be generated.

      groupBy(...)
            This function has the following overloads:

            - groupBy(fieldList)
            - groupBy(field[, field, ...])

            Sets a grouping criteria for the resultset.

      having(condition)
            Sets a condition for records to be considered in agregate function
            operations.

      sort(...)
            This function has the following overloads:

            - sort(sortCriteriaList)
            - sort(sortCriteria[, sortCriteria, ...])

            If used the CollectionFind operation will return the records sorted
            with the defined criteria.

            Every defined sort criterion sollows the next format:

            name [ ASC | DESC ]

            ASC is used by default if the sort order is not specified.

      limit(numberOfDocs)
            If used, the operation will return at most numberOfDocs documents.

            This function can be called every time the statement is executed.

      skip(numberOfDocs)
            If used, the first numberOfDocs' records will not be included on
            the result.

            ATTENTION: This function will be removed in a future release, use
                       the offset function instead.

      lockShared([lockContention])
            When this function is called, the selected documents will belocked
            for write operations, they may be retrieved on a different session,
            but no updates will be allowed.

            The acquired locks will be released when the current transaction is
            commited or rolled back.

            The lockContention parameter defines the behavior of the operation
            if another session contains an exlusive lock to matching documents.

            The lockContention can be specified using the following constants:

            - mysqlx.LockContention.DEFAULT
            - mysqlx.LockContention.NOWAIT
            - mysqlx.LockContention.SKIP_LOCKED

            The lockContention can also be specified using the following string
            literals (no case sensitive):

            - 'DEFAULT'
            - 'NOWAIT'
            - 'SKIP_LOCKED'

            If no lockContention or the default is specified, the operation
            will block if another session already holds an exclusive lock on
            matching documents until the lock is released.

            If lockContention is set to NOWAIT and another session already
            holds an exclusive lock on matching documents, the operation will
            not block and an error will be generated.

            If lockContention is set to SKIP_LOCKED and another session already
            holds an exclusive lock on matching documents, the operation will
            not block and will return only those documents not having an
            exclusive lock.

            This operation only makes sense within a transaction.

      lockExclusive([lockContention])
            When this function is called, the selected documents will be locked
            for read operations, they will not be retrievable by other session.

            The acquired locks will be released when the current transaction is
            commited or rolled back.

            The lockContention parameter defines the behavior of the operation
            if another session contains a lock to matching documents.

            The lockContention can be specified using the following constants:

            - mysqlx.LockContention.DEFAULT
            - mysqlx.LockContention.NOWAIT
            - mysqlx.LockContention.SKIP_LOCKED

            The lockContention can also be specified using the following string
            literals (no case sensitive):

            - 'DEFAULT'
            - 'NOWAIT'
            - 'SKIP_LOCKED'

            If no lockContention or the default is specified, the operation
            will block if another session already holds a lock on matching
            documents.

            If lockContention is set to NOWAIT and another session already
            holds a lock on matching documents, the operation will not block
            and an error will be generated.

            If lockContention is set to SKIP_LOCKED and  another session
            already holds a lock on matching documents, the operation will not
            block and will return only those documents not having a lock.

            This operation only makes sense within a transaction.

      bind(name, value)
            Binds a value to a specific placeholder used on this CollectionFind
            object.

            An error will be raised if the placeholder indicated by name does
            not exist.

            This function must be called once for each used placeholder or an
            error will be raised when the execute method is called.

      execute()
            Executes the find operation with all the configured options.

//@<OUT> Help on getName
NAME
      getName - Returns the name of this database object.

SYNTAX
      <Collection>.getName()

//@<OUT> Help on getOne
NAME
      getOne - Fetches the document with the given _id from the collection.

SYNTAX
      <Collection>.getOne(id)

WHERE
      id: The identifier of the document to be retrieved.

RETURNS
       The Document object matching the given id or NULL if no match is found.

//@<OUT> Help on getSchema
NAME
      getSchema - Returns the Schema object of this database object.

SYNTAX
      <Collection>.getSchema()

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
      <Collection>.getSession()

RETURNS
       The Session object used to get to this object.

DESCRIPTION
      Note that the returned object can be any of:

      - Session: if the object was created/retrieved using an Session instance.
      - ClassicSession: if the object was created/retrieved using an
        ClassicSession.

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <Collection>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on modify
NAME
      modify - Creates a collection update handler.

SYNTAX
      Collection.modify(searchCondition)
                [.set(attribute, value)]
                [.unset(...)]
                [.merge(document)]
                [.patch(document)]
                [.arrayInsert(docPath, value)]
                [.arrayAppend(docPath, value)]
                [.arrayDelete(docPath)]
                [.sort(...)]
                [.limit(numberOfDocs)]
                [.bind(name:, value:)]
                [.execute()]

      modify(searchCondition)
            Creates a handler to update documents in the collection.

            A condition must be provided to this function, all the documents
            matching the condition will be updated.

            To update all the documents, set a condition that always evaluates
            to true, for example '1'.

      set(attribute, value)
            Adds an opertion into the modify handler to set an attribute on the
            documents that were included on the selection filter and limit.

            - If the attribute is not present on the document, it will be added
              with the given value.
            - If the attribute already exists on the document, it will be
              updated with the given value.

            Using Expressions for Values

            The received values are set into the document in a literal way
            unless an expression is used.

            When an expression is used, it is evaluated on the server and the
            resulting value is set into the document.

      unset(...)
            This function has the following overloads:

            - unset(attributeList)
            - unset(attribute[, attribute, ...])

            The attribute removal will be done on the collection's documents
            once the execute method is called.

            For each attribute on the attributes list, adds an opertion into
            the modify handler

            to remove the attribute on the documents that were included on the
            selection filter and limit.

      merge(document)
            This function adds an operation to add into the documents of a
            collection, all the attribues defined in document that do not exist
            on the collection's documents.

            The attribute addition will be done on the collection's documents
            once the execute method is called.

            ATTENTION: This function will be removed in a future release, use
                       the patch function instead.

      patch(document)
            This function adds an operation to update the documents of a
            collection, the patch operation follows the algorithm described on
            the JSON Merge Patch RFC7386.

            The patch JSON object will be used to either add, update or remove
            fields from documents in the collection that match the filter
            specified on the call to the modify() function.

            The operation to be performed depends on the attributes defined at
            the patch JSON object:

            - Any attribute with value equal to null will be removed if exists.
            - Any attribute with value different than null will be updated if
              exists.
            - Any attribute with value different than null will be added if
              does not exists.

            Special considerations:

            - The _id of the documents is inmutable, so it will not be affected
              by the patch operation even if it is included on the patch JSON
              object.
            - The patch JSON object accepts expression objects as values. If
              used they will be evaluated at the server side.

            The patch operations will be done on the collection's documents
            once the execute method is called.

      arrayInsert(docPath, value)
            Adds an opertion into the modify handler to insert a value into an
            array attribute on the documents that were included on the
            selection filter and limit.

            The insertion of the value will be done on the collection's
            documents once the execute method is called.

      arrayAppend(docPath, value)
            Adds an opertion into the modify handler to append a value into an
            array attribute on the documents that were included on the
            selection filter and limit.

      arrayDelete(docPath)
            Adds an opertion into the modify handler to delete a value from an
            array attribute on the documents that were included on the
            selection filter and limit.

            The attribute deletion will be done on the collection's documents
            once the execute method is called.

            ATTENTION: This function will be removed in a future release, use
                       the unset function instead.

      sort(...)
            This function has the following overloads:

            - sort(sortDataList)
            - sort(sortData[, sortData, ...])

            The elements of sortExprStr list are usually strings defining the
            attribute name on which the collection sorting will be based. Each
            criterion could be followed by asc or desc to indicate ascending

            or descending order respectivelly. If no order is specified,
            ascending will be used by default.

            This method is usually used in combination with limit to fix the
            amount of documents to be updated.

      limit(numberOfDocs)
            This method is usually used in combination with sort to fix the
            amount of documents to be updated.

            This function can be called every time the statement is executed.

      bind(name:, value:)
            Binds a value to a specific placeholder used on this
            CollectionModify object.

      execute()
            Executes the update operations added to the handler with the
            configured filter and limit.

//@<OUT> Help on remove
NAME
      remove - Creates a document deletion handler.

SYNTAX
      Collection.remove(searchCondition)
                [.sort(...)]
                [.limit(numberOfDocs)]
                [.bind(name, value)]
                [.execute()]

      remove(searchCondition)
            Creates a handler for the deletion of documents on the collection.

            A condition must be provided to this function, all the documents
            matching the condition will be removed from the collection.

            To delete all the documents, set a condition that always evaluates
            to true, for example '1'.

            The searchCondition supports parameter binding.

            This function is called automatically when
            Collection.remove(searchCondition) is called.

            The actual deletion of the documents will occur only when the
            execute method is called.

      sort(...)
            This function has the following overloads:

            - sort(sortExprList)
            - sort(sortExpr[, sortExpr, ...])

            The elements of sortExprStr list are strings defining the column
            name on which the sorting will be based in the form of
            'columnIdentifier [ ASC | DESC ]'.

            If no order criteria is specified, ascending will be used by
            default.

            This method is usually used in combination with limit to fix the
            amount of documents to be deleted.

      limit(numberOfDocs)
            This method is usually used in combination with sort to fix the
            amount of documents to be deleted.

            This function can be called every time the statement is executed.

      bind(name, value)
            An error will be raised if the placeholder indicated by name does
            not exist.

            This function must be called once for each used placeholder or an
            error will be raised when the execute method is called.

      execute()
            Executes the document deletion with the configured filter and
            limit.

//@<OUT> Help on removeOne
NAME
      removeOne - Removes document with the given _id value.

SYNTAX
      <Collection>.removeOne(id)

WHERE
      id: The id of the document to be removed.

RETURNS
       A Result object containing the number of affected rows.

DESCRIPTION
      If no document is found matching the given id, the Result object will
      indicate 0 as the number of affected rows.

//@ Finalization
||
