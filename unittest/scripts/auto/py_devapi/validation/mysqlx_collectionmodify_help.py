#@ __global__
||

#@<OUT> collmodify
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
                       the unset() function instead.

      array_insert(docPath, value)
            Inserts a value into a specific position in an array attribute in
            documents of a collection.

      bind(name, value)
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
                       the patch() function instead.

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

#@<OUT> collfind.array_append
NAME
      array_append - Appends a value into an array attribute in documents of a
                     collection.

SYNTAX
      <CollectionModify>.array_append(docPath, value)

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

#@<OUT> collfind.array_delete
NAME
      array_delete - Deletes the value at a specific position in an array
                     attribute in documents of a collection.

SYNTAX
      <CollectionModify>.array_delete(docPath)

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
      the execute() method is called.

      ATTENTION: This function will be removed in a future release, use the
                 unset() function instead.

#@<OUT> collfind.array_insert
NAME
      array_insert - Inserts a value into a specific position in an array
                     attribute in documents of a collection.

SYNTAX
      <CollectionModify>.array_insert(docPath, value)

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
      once the execute() method is called.

#@<OUT> collfind.help
NAME
      help - Provides help about this class and it's members

SYNTAX
      <CollectionModify>.help([member])

WHERE
      member: If specified, provides detailed information on the given member.

#@<OUT> collfind.merge
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
      the execute() method is called.

      ATTENTION: This function will be removed in a future release, use the
                 patch() function instead.

#@<OUT> collfind.modify
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

#@<OUT> collfind.patch
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
      execute() method is called.

#@<OUT> collfind.set
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

#@<OUT> collfind.unset
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
      execute() method is called.

      For each attribute on the attributes list, adds an operation into the
      modify handler

      to remove the attribute on the documents that were included on the
      selection filter and limit.
