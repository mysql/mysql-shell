//@<OUT> Collection.help()
A Document is a set of key and value pairs, as represented by a JSON object.

A Document is represented internally using the MySQL binary JSON object,
through the JSON MySQL datatype.

The values of fields can contain other documents, arrays, and lists of
documents.

The following properties are currently supported.

 - name    The name of this database object.
 - session The Session object of this database object.
 - schema  The Schema object of this database object.


The following functions are currently supported.

 - add              Inserts one or more documents into a collection.
 - addOrReplaceOne  Replaces or adds a document in a collection.
 - createIndex      Creates a non unique/unique index on a collection.
 - dropIndex        Drops an index from a collection.
 - existsInDatabase Verifies if this object exists in the database.
 - find             Retrieves documents from a collection, matching a specified
                    criteria.
 - getName          Returns the name of this database object.
 - getOne           Fetches the document with the given _id from the
                    collection.
 - getSchema        Returns the Schema object of this database object.
 - getSession       Returns the Session object of this database object.
 - help             Provides help about this class and it's members
 - modify           Creates a collection update handler.
 - remove           Creates a document deletion handler.
 - removeOne        Removes document with the given _id value.
 - replaceOne       Replaces an existing document with a new document.

//@<OUT> Collection.help('add')
Inserts one or more documents into a collection.

SYNTAX

  <Collection>.add(...)
              [.execute()]

DESCRIPTION

Inserts one or more documents into a collection.

  .add(...)

    Variations

      add(document[, document, ...])[.add(...)]
      add(documentList)[.add(...)]

    This function receives one or more document definitions to be added into a
    collection.

    A document definition may be provided in two ways:

     - Using a dictionary containing the document fields.
     - Using A JSON string as a document expression.

    There are three ways to add multiple documents:

     - Passing several parameters to the function, each parameter should be a
       document definition.
     - Passing a list of document definitions.
     - Calling this function several times before calling execute().

    To be added, every document must have a string property named '_id' ideally
    with a universal unique identifier (UUID) as value. If the '_id' property
    is missing, it is automatically set with an internally generated UUID.


    ** JSON as Document Expressions **

    A document can be represented as a JSON expression as follows:

    mysqlx.expr(<JSON String>)

  .execute()

    Executes the add operation, the documents are added to the target
    collection.

//@<OUT> Collection.help('addOrReplaceOne')
Replaces or adds a document in a collection.

SYNTAX

  <Collection>.addOrReplaceOne(id, doc)

WHERE

  id: the identifier of the document to be replaced.
  doc: the new document.

RETURNS

 A Result object containing the number of affected rows.

DESCRIPTION

Replaces the document identified with the given id. If no document is found
matching the given id the given document will be added to the collection.

Only one document will be affected by this operation.

The id of the document remains inmutable, if the new document contains a
different id, it will be ignored.

Any constraint (unique key) defined on the collection is applicable on both the
replace and add operations:

 - The replace operation will fail if the new document contains a unique key
   which is already defined for any document in the collection except the one
   being replaced.
 - The add operation will fail if the new document contains a unique key which
   is already defined for any document in the collection.

//@<OUT> Collection.help('createIndex')
Creates a non unique/unique index on a collection.

SYNTAX

  <Collection>.createIndex(...)
              [.field(...)]
              [.execute(...)]

DESCRIPTION

Creates a non unique/unique index on a collection.

  .createIndex(...)

    Variations

      createIndex(String indexName)
      createIndex(String indexName, IndexType type)

    Sets the name for the creation of a non unique index on the collection.

    Sets the name for the creation of a unique index on the collection.

  .field(...)

    Adds column to be part of the collection index being created.

  .execute(...)

    Executes the document addition for the documents cached on this object.


//@<OUT> Collection.help('dropIndex')
Drops an index from a collection.

SYNTAX

  <Collection>.dropIndex()

//@<OUT> Collection.help('existsInDatabase')
Verifies if this object exists in the database.

SYNTAX

  <Collection>.existsInDatabase()

RETURNS

 A boolean indicating if the object still exists on the database.

//@<OUT> Collection.help('find')
Retrieves documents from a collection, matching a specified criteria.

SYNTAX

  <Collection>.find(...)
              [.fields(...)]
              [.groupBy(...)[.having(searchCondition)]]
              [.sort(...)]
              [.limit(numberOfRows)[.skip(offset)]]
              [.bind(placeHolder, value)[.bind(...)]]
              [.execute()]

DESCRIPTION

Retrieves documents from a collection, matching a specified criteria.

  .find(...)

    Variations

      find()
      find(searchCondition)

    Sets the search condition to identify the Documents to be retrieved from
    the owner Collection. If the search condition is not specified the find
    operation will be executed over all the documents in the collection.

    The search condition supports parameter binding.

  .fields(...)

    Variations

      fields(field[, field, ...])
      fields(fieldList)
      fields(projectionExpression)

    This function sets the fields to be retrieved from each document matching
    the criteria on this find operation.

    A field is defined as a string value containing an expression defining the
    field to be retrieved.

    The fields to be retrieved can be set using any of the next methods:

     - Passing each field definition as an individual string parameter.
     - Passing a list of strings containing the field definitions.
     - Passing a JSON expression representing a document projection to be
       generated.

  .groupBy(...)

    Variations

      groupBy(fieldList)
      groupBy(field[, field, ...])

    Sets a grouping criteria for the resultset.

  .having(searchCondition)

    Sets a condition for records to be considered in agregate function
    operations.

  .sort(...)

    Variations

      sort(sortCriterion[, sortCriterion, ...])
      sort(sortCritera)

    If used the CollectionFind operation will return the records sorted with
    the defined criteria.

    Every defined sort criterion sollows the next format:

    name [ ASC | DESC ]

    ASC is used by default if the sort order is not specified.

  .limit(numberOfRows)

    If used, the CollectionFind operation will return at most numberOfRows
    documents.

  .skip(offset)

    If used, the first 'offset' records will not be included on the result.

  .bind(placeHolder, value)[.bind(...)]

    Binds a value to a specific placeholder used on this CollectionFind object.

    An error will be raised if the placeholder indicated by name does not
    exist.

    This function must be called once for each used placeohlder or an error
    will be raised when the execute method is called.

  .execute()

    Executes the find operation with all the configured options.

//@<OUT> Collection.help('getName')
Returns the name of this database object.

SYNTAX

  <Collection>.getName()

//@<OUT> Collection.help('getOne')
Fetches the document with the given _id from the collection.

SYNTAX

  <Collection>.getOne(id)

WHERE

  id: The identifier of the document to be retrieved.

RETURNS

 The Document object matching the given id or NULL if no match is found.

//@<OUT> Collection.help('getSchema')
Returns the Schema object of this database object.

SYNTAX

  <Collection>.getSchema()

RETURNS

 The Schema object used to get to this object.

DESCRIPTION

Note that the returned object can be any of:

 - Schema: if the object was created/retrieved using a Schema instance.
 - ClassicSchema: if the object was created/retrieved using an ClassicSchema.
 - Null: if this database object is a Schema or ClassicSchema.


//@<OUT> Collection.help('getSession')
Returns the Session object of this database object.

SYNTAX

  <Collection>.getSession()

RETURNS

 The Session object used to get to this object.

DESCRIPTION

Note that the returned object can be any of:

 - Session: if the object was created/retrieved using an Session instance.
 - ClassicSession: if the object was created/retrieved using an ClassicSession.

//@<OUT> Collection.help('modify')
Creates a collection update handler.

SYNTAX

  <Collection>.modify(...)
              [.set(...)]
              [.unset(...)]
              [.merge(...)]
              [.arrayInsert(...)]
              [.arrayAppend(...)]
              [.arrayDelete(...)]
              [.sort(...)]
              [.limit(...)]
              [.bind(...)]
              [.execute(...)]

DESCRIPTION

Creates a collection update handler.

  .modify(...)

    Creates a handler to update documents in the collection.

    A condition must be provided to this function, all the documents matching
    the condition will be updated.

    To update all the documents, set a condition that always evaluates to true,
    for example '1'.

  .set(...)

    Adds an opertion into the modify handler to set an attribute on the
    documents that were included on the selection filter and limit.

     - If the attribute is not present on the document, it will be added with
       the given value.
     - If the attribute already exists on the document, it will be updated with
       the given value.


    **  Using Expressions for Values  **

    The received values are set into the document in a literal way unless an
    expression is used.

    When an expression is used, it is evaluated on the server and the resulting
    value is set into the document.

  .unset(...)

    Variations

      unset(String attribute)
      unset(List attributes)

    The attribute removal will be done on the collection's documents once the
    execute method is called.

    For each attribute on the attributes list, adds an opertion into the modify
    handler

    to remove the attribute on the documents that were included on the
    selection filter and limit.

  .merge(...)

    This function adds an operation to add into the documents of a collection,
    all the attribues defined in document that do not exist on the collection's
    documents.

    The attribute addition will be done on the collection's documents once the
    execute method is called.

  .arrayInsert(...)

    Adds an opertion into the modify handler to insert a value into an array
    attribute on the documents that were included on the selection filter and
    limit.

    The insertion of the value will be done on the collection's documents once
    the execute method is called.

  .arrayAppend(...)

    Adds an opertion into the modify handler to append a value into an array
    attribute on the documents that were included on the selection filter and
    limit.

  .arrayDelete(...)

    Adds an opertion into the modify handler to delete a value from an array
    attribute on the documents that were included on the selection filter and
    limit.

    The attribute deletion will be done on the collection's documents once the
    execute method is called.

  .sort(...)

    The elements of sortExprStr list are usually strings defining the attribute
    name on which the collection sorting will be based. Each criterion could be
    followed by asc or desc to indicate ascending

    or descending order respectivelly. If no order is specified, ascending will
    be used by default.

    This method is usually used in combination with limit to fix the amount of
    documents to be updated.

  .limit(...)

    This method is usually used in combination with sort to fix the amount of
    documents to be updated.

  .bind(...)

    Binds a value to a specific placeholder used on this CollectionModify
    object.

  .execute(...)

    Executes the update operations added to the handler with the configured
    filter and limit.

//@<OUT> Collection.help('remove')
Creates a document deletion handler.

SYNTAX

  <Collection>.remove(...)
              [.sort(...)]
              [.limit(...)]
              [.bind(...)]
              [.execute(...)]

DESCRIPTION

Creates a document deletion handler.

  .remove(...)

    Creates a handler for the deletion of documents on the collection.

    A condition must be provided to this function, all the documents matching
    the condition will be removed from the collection.

    To delete all the documents, set a condition that always evaluates to true,
    for example '1'.

    The searchCondition supports parameter binding.

    This function is called automatically when
    Collection.remove(searchCondition) is called.

    The actual deletion of the documents will occur only when the execute
    method is called.

  .sort(...)

    The elements of sortExprStr list are strings defining the column name on
    which the sorting will be based in the form of 'columnIdentifier [ ASC |
    DESC ]'.

    If no order criteria is specified, ascending will be used by default.

    This method is usually used in combination with limit to fix the amount of
    documents to be deleted.

  .limit(...)

    This method is usually used in combination with sort to fix the amount of
    documents to be deleted.

  .bind(...)

    An error will be raised if the placeholder indicated by name does not
    exist.

    This function must be called once for each used placeohlder or an error
    will be raised when the execute method is called.

  .execute(...)

    Executes the document deletion with the configured filter and limit.


//@<OUT> Collection.help('removeOne')
Removes document with the given _id value.

SYNTAX

  <Collection>.removeOne(id)

WHERE

  id: The id of the document to be removed.

RETURNS

 A Result object containing the number of affected rows.

DESCRIPTION

If no document is found matching the given id, the Result object will indicate
0 as the number of affected rows.

//@<OUT> Collection.help('replaceOne')
Replaces an existing document with a new document.

SYNTAX

  <Collection>.replaceOne(id, doc)

WHERE

  id: identifier of the document to be replaced.
  doc: the new document.

RETURNS

 A Result object containing the number of affected rows.

DESCRIPTION

Replaces the document identified with the given id. If no document is found
matching the given id the returned Result will indicate 0 affected items.

Only one document will be affected by this operation.

The id of the document remain inmutable, if the new document contains a
different id, it will be ignored.

Any constraint (unique key) defined on the collection is applicable:

The operation will fail if the new document contains a unique key which is
already defined for any document in the collection except the one being
replaced.

//@ Finalization
||
