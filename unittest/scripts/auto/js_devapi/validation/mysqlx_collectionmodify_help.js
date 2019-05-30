//@ Initialization
||

//@<OUT> CollectionModify help
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

//@<OUT> Help on arrayAppend
NAME
      arrayAppend - Appends a value into an array attribute in documents of a
                    collection.

SYNTAX
      <CollectionModify>.arrayAppend(docPath, value)

WHERE
      docPath: A document path that identifies the array attribute where the
               value will be appended.
      value: The value to be appended.

RETURNS
       This CollectionModify object.

DESCRIPTION
      Adds an operation into the modify handler to append a value into an array
      attribute on the documents that were included on the selection filter and
      limit.

//@<OUT> Help on arrayDelete
NAME
      arrayDelete - Deletes the value at a specific position in an array
                    attribute in documents of a collection.

SYNTAX
      <CollectionModify>.arrayDelete(docPath)

WHERE
      docPath: A document path that identifies the array attribute and position
               of the value to be deleted.

RETURNS
       This CollectionModify object.

DESCRIPTION
      Adds an operation into the modify handler to delete a value from an array
      attribute on the documents that were included on the selection filter and
      limit.

      The attribute deletion will be done on the collection's documents once
      the execute method is called.

      ATTENTION: This function will be removed in a future release, use the
                 unset function instead.

//@<OUT> Help on arrayInsert
NAME
      arrayInsert - Inserts a value into a specific position in an array
                    attribute in documents of a collection.

SYNTAX
      <CollectionModify>.arrayInsert(docPath, value)

WHERE
      docPath: A document path that identifies the array attribute and position
               where the value will be inserted.
      value: The value to be inserted.

RETURNS
       This CollectionModify object.

DESCRIPTION
      Adds an operation into the modify handler to insert a value into an array
      attribute on the documents that were included on the selection filter and
      limit.

      The insertion of the value will be done on the collection's documents
      once the execute method is called.

//@<OUT> Help on bind
NAME
      bind - Binds a value to a specific placeholder used on this
             CollectionModify object.

SYNTAX
      <CollectionModify>.bind(name:, value:)

WHERE
      name:: The name of the placeholder to which the value will be bound.
      value:: The value to be bound on the placeholder.

RETURNS
       This CollectionModify object.

//@<OUT> Help on execute
NAME
      execute - Executes the update operations added to the handler with the
                configured filter and limit.

SYNTAX
      <CollectionModify>.execute()

RETURNS
       CollectionResultset A Result object that can be used to retrieve the
      results of the update operation.

//@<OUT> Help on help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <CollectionModify>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

//@<OUT> Help on limit
NAME
      limit - Sets a limit for the documents to be updated by the operations
              added to the handler.

SYNTAX
      <CollectionModify>.limit(numberOfDocs)

WHERE
      numberOfDocs: the number of documents to affect on the update operations.

RETURNS
       This CollectionModify object.

DESCRIPTION
      This method is usually used in combination with sort to fix the amount of
      documents to be updated.

//@<OUT> Help on merge
NAME
      merge - Adds attributes taken from a document into the documents in a
              collection.

SYNTAX
      <CollectionModify>.merge(document)

WHERE
      document: The document from which the attributes will be merged.

RETURNS
       This CollectionModify object.

DESCRIPTION
      This function adds an operation to add into the documents of a
      collection, all the attributes defined in document that do not exist on
      the collection's documents.

      The attribute addition will be done on the collection's documents once
      the execute method is called.

      ATTENTION: This function will be removed in a future release, use the
                 patch function instead.

//@<OUT> Help on modify
NAME
      modify - Sets the search condition to identify the Documents to be
               updated on the owner Collection.

SYNTAX
      <CollectionModify>.modify(searchCondition)

WHERE
      searchCondition: An expression to identify the documents to be updated.

RETURNS
       This CollectionModify object.

DESCRIPTION
      Creates a handler to update documents in the collection.

      A condition must be provided to this function, all the documents matching
      the condition will be updated.

      To update all the documents, set a condition that always evaluates to
      true, for example '1'.

//@<OUT> Help on patch
NAME
      patch - Performs modifications on a document based on a patch JSON
              object.

SYNTAX
      <CollectionModify>.patch(document)

WHERE
      document: The JSON object to be used on the patch process.

RETURNS
       This CollectionModify object.

DESCRIPTION
      This function adds an operation to update the documents of a collection,
      the patch operation follows the algorithm described on the JSON Merge
      Patch RFC7386.

      The patch JSON object will be used to either add, update or remove fields
      from documents in the collection that match the filter specified on the
      call to the modify() function.

      The operation to be performed depends on the attributes defined at the
      patch JSON object:

      - Any attribute with value equal to null will be removed if exists.
      - Any attribute with value different than null will be updated if exists.
      - Any attribute with value different than null will be added if does not
        exists.

      Special considerations:

      - The _id of the documents is immutable, so it will not be affected by
        the patch operation even if it is included on the patch JSON object.
      - The patch JSON object accepts expression objects as values. If used
        they will be evaluated at the server side.

      The patch operations will be done on the collection's documents once the
      execute method is called.

//@<OUT> Help on set
NAME
      set - Sets or updates attributes on documents in a collection.

SYNTAX
      <CollectionModify>.set(attribute, value)

WHERE
      attribute: A string with the document path of the item to be set.
      value: The value to be set on the specified attribute.

RETURNS
       This CollectionModify object.

DESCRIPTION
      Adds an operation into the modify handler to set an attribute on the
      documents that were included on the selection filter and limit.

      - If the attribute is not present on the document, it will be added with
        the given value.
      - If the attribute already exists on the document, it will be updated
        with the given value.

      Using Expressions for Values

      The received values are set into the document in a literal way unless an
      expression is used.

      When an expression is used, it is evaluated on the server and the
      resulting value is set into the document.

//@<OUT> Help on sort
NAME
      sort - Sets the document order in which the update operations added to
             the handler should be done.

SYNTAX
      <CollectionModify>.sort(sortDataList)
      <CollectionModify>.sort(sortData[, sortData, ...])

RETURNS
       This CollectionModify object.

DESCRIPTION
      The elements of sortExprStr list are usually strings defining the
      attribute name on which the collection sorting will be based. Each
      criterion could be followed by asc or desc to indicate ascending

      or descending order respectively. If no order is specified, ascending
      will be used by default.

      This method is usually used in combination with limit to fix the amount
      of documents to be updated.

//@<OUT> Help on unset
NAME
      unset - Removes attributes from documents in a collection.

SYNTAX
      <CollectionModify>.unset(attributeList)
      <CollectionModify>.unset(attribute[, attribute, ...])

WHERE
      attribute: A string with the document path of the attribute to be
                 removed.
      attributes: A list with the document paths of the attributes to be
                  removed.

RETURNS
       This CollectionModify object.

DESCRIPTION
      The attribute removal will be done on the collection's documents once the
      execute method is called.

      For each attribute on the attributes list, adds an operation into the
      modify handler

      to remove the attribute on the documents that were included on the
      selection filter and limit.

//@ Finalization
||
