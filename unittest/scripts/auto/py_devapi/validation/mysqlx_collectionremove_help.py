#@ __global__
||

#@<OUT> collremove
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

#@<OUT> collremove.bind
NAME
      bind - Binds a value to a specific placeholder used on this
             CollectionRemove object.

SYNTAX
      <CollectionRemove>.bind(name, value)

WHERE
      name: The name of the placeholder to which the value will be bound.
      value: The value to be bound on the placeholder.

RETURNS
       This CollectionRemove object.

DESCRIPTION
      An error will be raised if the placeholder indicated by name does not
      exist.

      This function must be called once for each used placeholder or an error
      will be raised when the execute method is called.

#@<OUT> collremove.execute
NAME
      execute - Executes the document deletion with the configured filter and
                limit.

SYNTAX
      <CollectionRemove>.execute()

RETURNS
       Result A Result object that can be used to retrieve the results of the
      deletion operation.

#@<OUT> collremove.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <CollectionRemove>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> collremove.limit
NAME
      limit - Sets a limit for the documents to be deleted.

SYNTAX
      <CollectionRemove>.limit(numberOfDocs)

WHERE
      numberOfDocs: the number of documents to affect in the remove execution.

RETURNS
       This CollectionRemove object.

DESCRIPTION
      This method is usually used in combination with sort to fix the amount of
      documents to be deleted.

#@<OUT> collremove.remove
NAME
      remove - Sets the search condition to filter the documents to be deleted
               from the owner Collection.

SYNTAX
      <CollectionRemove>.remove(searchCondition)

WHERE
      searchCondition: An expression to filter the documents to be deleted.

RETURNS
       This CollectionRemove object.

DESCRIPTION
      Creates a handler for the deletion of documents on the collection.

      A condition must be provided to this function, all the documents matching
      the condition will be removed from the collection.

      To delete all the documents, set a condition that always evaluates to
      true, for example '1'.

      The searchCondition supports parameter binding.

      This function is called automatically when
      Collection.remove(searchCondition) is called.

      The actual deletion of the documents will occur only when the execute
      method is called.

#@<OUT> collremove.sort
NAME
      sort - Sets the order in which the deletion should be done.

SYNTAX
      <CollectionRemove>.sort(sortExprList)
      <CollectionRemove>.sort(sortExpr[, sortExpr, ...])

RETURNS
       This CollectionRemove object.

DESCRIPTION
      The elements of sortExprStr list are strings defining the column name on
      which the sorting will be based in the form of 'columnIdentifier [ ASC |
      DESC ]'.

      If no order criteria is specified, ascending will be used by default.

      This method is usually used in combination with limit to fix the amount
      of documents to be deleted.
