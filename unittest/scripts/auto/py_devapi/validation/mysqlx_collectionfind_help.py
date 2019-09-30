#@ __global__
||

#@<OUT> collfind
NAME
      CollectionFind - Operation to retrieve documents from a Collection.

DESCRIPTION
      A CollectionFind object represents an operation to retrieve documents
      from a Collection, it is created through the find function on the
      Collection class.

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
            Sets a condition for records to be considered in aggregate function
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

#@<OUT> collfind.bind
NAME
      bind - Binds a value to a specific placeholder used on this
             CollectionFind object.

SYNTAX
      <CollectionFind>.bind(name, value)

WHERE
      name: The name of the placeholder to which the value will be bound.
      value: The value to be bound on the placeholder.

RETURNS
      This CollectionFind object.

DESCRIPTION
      Binds a value to a specific placeholder used on this CollectionFind
      object.

      An error will be raised if the placeholder indicated by name does not
      exist.

      This function must be called once for each used placeholder or an error
      will be raised when the execute method is called.

#@<OUT> collfind.execute
NAME
      execute - Executes the find operation with all the configured options.

SYNTAX
      <CollectionFind>.execute()

RETURNS
      A DocResult object that can be used to traverse the documents returned by
      this operation.

#@<OUT> collfind.fields
NAME
      fields - Sets the fields to be retrieved from each document matching the
               criteria on this find operation.

SYNTAX
      <CollectionFind>.fields(fieldList)
      <CollectionFind>.fields(field[, field, ...])
      <CollectionFind>.fields(mysqlx.expr(...))

WHERE
      fieldDefinition: Definition of the fields to be retrieved.

RETURNS
      This CollectionFind object.

DESCRIPTION
      This function sets the fields to be retrieved from each document matching
      the criteria on this find operation.

      A field is defined as a string value containing an expression defining
      the field to be retrieved.

      The fields to be retrieved can be set using any of the next methods:

      - Passing each field definition as an individual string parameter.
      - Passing a list of strings containing the field definitions.
      - Passing a JSON expression representing a document projection to be
        generated.

#@<OUT> collfind.find
NAME
      find - Sets the search condition to identify the Documents to be
             retrieved from the owner Collection.

SYNTAX
      <CollectionFind>.find([searchCondition])

WHERE
      searchCondition: String expression defining the condition to be used on
                       the selection.

RETURNS
      This CollectionFind object.

DESCRIPTION
      Sets the search condition to identify the Documents to be retrieved from
      the owner Collection. If the search condition is not specified the find
      operation will be executed over all the documents in the collection.

      The search condition supports parameter binding.

#@<OUT> collfind.group_by
NAME
      group_by - Sets a grouping criteria for the resultset.

SYNTAX
      <CollectionFind>.group_by(fieldList)
      <CollectionFind>.group_by(field[, field, ...])

RETURNS
      This CollectionFind object.

DESCRIPTION
      Sets a grouping criteria for the resultset.

#@<OUT> collfind.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <CollectionFind>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> collfind.limit
NAME
      limit - Sets the maximum number of documents to be returned by the
              operation.

SYNTAX
      <CollectionFind>.limit(numberOfDocs)

WHERE
      numberOfDocs: The maximum number of documents to be retrieved.

RETURNS
      This CollectionFind object.

DESCRIPTION
      If used, the operation will return at most numberOfDocs documents.

      This function can be called every time the statement is executed.

#@<OUT> collfind.lock_exclusive
NAME
      lock_exclusive - Instructs the server to acquire an exclusive lock on
                       documents matched by this find operation.

SYNTAX
      <CollectionFind>.lock_exclusive([lockContention])

WHERE
      lockContention: Parameter to indicate how to handle documents that are
                      already locked.

RETURNS
      This CollectionFind object.

DESCRIPTION
      When this function is called, the selected documents will be locked for
      read operations, they will not be retrievable by other session.

      The acquired locks will be released when the current transaction is
      committed or rolled back.

      The lockContention parameter defines the behavior of the operation if
      another session contains a lock to matching documents.

      The lockContention can be specified using the following constants:

      - mysqlx.LockContention.DEFAULT
      - mysqlx.LockContention.NOWAIT
      - mysqlx.LockContention.SKIP_LOCKED

      The lockContention can also be specified using the following string
      literals (no case sensitive):

      - 'DEFAULT'
      - 'NOWAIT'
      - 'SKIP_LOCKED'

      If no lockContention or the default is specified, the operation will
      block if another session already holds a lock on matching documents.

      If lockContention is set to NOWAIT and another session already holds a
      lock on matching documents, the operation will not block and an error
      will be generated.

      If lockContention is set to SKIP_LOCKED and  another session already
      holds a lock on matching documents, the operation will not block and will
      return only those documents not having a lock.

      This operation only makes sense within a transaction.

#@<OUT> collfind.lock_shared
NAME
      lock_shared - Instructs the server to acquire shared row locks in
                    documents matched by this find operation.

SYNTAX
      <CollectionFind>.lock_shared([lockContention])

WHERE
      lockContention: Parameter to indicate how to handle documents that are
                      already locked.

RETURNS
      This CollectionFind object.

DESCRIPTION
      When this function is called, the selected documents will be locked for
      write operations, they may be retrieved on a different session, but no
      updates will be allowed.

      The acquired locks will be released when the current transaction is
      committed or rolled back.

      The lockContention parameter defines the behavior of the operation if
      another session contains an exclusive lock to matching documents.

      The lockContention can be specified using the following constants:

      - mysqlx.LockContention.DEFAULT
      - mysqlx.LockContention.NOWAIT
      - mysqlx.LockContention.SKIP_LOCKED

      The lockContention can also be specified using the following string
      literals (no case sensitive):

      - 'DEFAULT'
      - 'NOWAIT'
      - 'SKIP_LOCKED'

      If no lockContention or the default is specified, the operation will
      block if another session already holds an exclusive lock on matching
      documents until the lock is released.

      If lockContention is set to NOWAIT and another session already holds an
      exclusive lock on matching documents, the operation will not block and an
      error will be generated.

      If lockContention is set to SKIP_LOCKED and another session already holds
      an exclusive lock on matching documents, the operation will not block and
      will return only those documents not having an exclusive lock.

      This operation only makes sense within a transaction.

#@<OUT> collfind.sort
NAME
      sort - Sets the sorting criteria to be used on the DocResult.

SYNTAX
      <CollectionFind>.sort(sortCriteriaList)
      <CollectionFind>.sort(sortCriteria[, sortCriteria, ...])

WHERE
      sortCriteria: The sort criteria for the returned documents.

RETURNS
      This CollectionFind object.

DESCRIPTION
      If used the CollectionFind operation will return the records sorted with
      the defined criteria.

      Every defined sort criterion follows the next format:

      name [ ASC | DESC ]

      ASC is used by default if the sort order is not specified.
