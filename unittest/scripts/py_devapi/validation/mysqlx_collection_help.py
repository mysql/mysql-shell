#@<OUT> Collection.help()
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

 - add                Inserts one or more documents into a collection.
 - add_or_replace_one Replaces or adds a document in a collection.
 - create_index       Creates an index on a collection.
 - drop_index         Drops an index from a collection.
 - exists_in_database Verifies if this object exists in the database.
 - find               Retrieves documents from a collection, matching a
                      specified criteria.
 - get_name           Returns the name of this database object.
 - get_one            Fetches the document with the given _id from the
                      collection.
 - get_schema         Returns the Schema object of this database object.
 - get_session        Returns the Session object of this database object.
 - help               Provides help about this class and it's members
 - modify             Creates a collection update handler.
 - remove             Creates a document deletion handler.
 - remove_one         Removes document with the given _id value.
 - replace_one        Replaces an existing document with a new document.

#@<OUT> Collection.help('add')
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

#@<OUT> Collection.help('add_or_replace_one')
Replaces or adds a document in a collection.

SYNTAX

  <Collection>.add_or_replace_one(id, doc)

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

#@<OUT> Collection.help('create_index')
Creates an index on a collection.

SYNTAX

  <Collection>.create_index(name, indexDefinition)

WHERE

  name: the name of the index to be created.
  indexDefinition: a JSON document with the index information.

RETURNS

 a Result object.

DESCRIPTION

This function will create an index on the collection using the information
provided in indexDefinition.

The indexDefinition is a JSON document with the next information:
{
  fields : [<index_field>, ...],
  type   : <type>
}

 - fields array of index_field objects, each describing a single document
   member to be included in the index.
 - type string, (optional) the type of index. One of INDEX or SPATIAL. Default
   is INDEX and may be omitted.

A single index_field description consists of the following fields:
{
  field    : <field>,
  type     : <type>,
  required : <boolean>
  options  : <uint>,
  srid     : <uint>
}

 - field: string, the full document path to the document member or field to be
   indexed.
 - type: string, one of the supported SQL column types to map the field into.
   For numeric types, the optional UNSIGNED keyword may follow. For the TEXT
   type, the length to consider for indexing may be added.
 - required: bool, (optional) true if the field is required to exist in the
   document. defaults to false, except for GEOJSON where it defaults to true.
 - options: uint, (optional) special option flags for use when decoding GEOJSON
   data
 - srid: uint, (optional) srid value for use when decoding GEOJSON data.

The 'options' and 'srid' fields in IndexField can and must be present only if
'type' is set to 'GEOJSON'.




#@<OUT> Collection.help('drop_index')
Drops an index from a collection.

SYNTAX

  <Collection>.drop_index()

#@<OUT> Collection.help('exists_in_database')
Verifies if this object exists in the database.

SYNTAX

  <Collection>.exists_in_database()

RETURNS

 A boolean indicating if the object still exists on the database.

#@<OUT> Collection.help('find')
Retrieves documents from a collection, matching a specified criteria.

SYNTAX

  <Collection>.find(...)
              [.fields(...)]
              [.group_by(...)[.having(searchCondition)]]
              [.sort(...)]
              [.limit(numberOfRows)[.skip(offset)]]
              [.lock_shared(...)]
              [.lock_exclusive(...)]
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

  .group_by(...)

    Variations

      group_by(fieldList)
      group_by(field[, field, ...])

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

  .lock_shared(...)

    When this function is called, the selected documents will belocked for
    write operations, they may be retrieved on a different session, but no
    updates will be allowed.

    The acquired locks will be released when the current transaction is
    commited or rolled back.

    The lockContention parameter defines the behavior of the operation if
    another session contains an exlusive lock to matching documents.

    The lockContention can be specified using the following constants:

     - mysqlx.LockContention.DEFAULT
     - mysqlx.LockContention.NOWAIT
     - mysqlx.LockContention.SKIP_LOCK

    The lockContention can also be specified using the following string
    literals (no case sensitive):

     - 'DEFAULT'
     - 'NOWAIT'
     - 'SKIP_LOCK'

    If no lockContention or the default is specified, the operation will block
    if another session already holds an exclusive lock on matching documents
    until the lock is released.

    If lockContention is set to NOWAIT and another session already holds an
    exclusive lock on matching documents, the operation will not block and an
    error will be generated.

    If lockContention is set to SKIP_LOCK and another session already holds an
    exclusive lock on matching documents, the operation will not block and will
    return only those documents not having an exclusive lock.

    This operation only makes sense within a transaction.

  .lock_exclusive(...)

    When this function is called, the selected documents will be locked for
    read operations, they will not be retrievable by other session.

    The acquired locks will be released when the current transaction is
    commited or rolled back.

    The lockContention parameter defines the behavior of the operation if
    another session contains a lock to matching documents.

    The lockContention can be specified using the following constants:

     - mysqlx.LockContention.DEFAULT
     - mysqlx.LockContention.NOWAIT
     - mysqlx.LockContention.SKIP_LOCK

    The lockContention can also be specified using the following string
    literals (no case sensitive):

     - 'DEFAULT'
     - 'NOWAIT'
     - 'SKIP_LOCK'

    If no lockContention or the default is specified, the operation will block
    if another session already holds a lock on matching documents.

    If lockContention is set to NOWAIT and another session already holds a lock
    on matching documents, the operation will not block and an error will be
    generated.

    If lockContention is set to SKIP_LOCK and  another session already holds a
    lock on matching documents, the operation will not block and will return
    only those documents not having a lock.

    This operation only makes sense within a transaction.

  .bind(placeHolder, value)[.bind(...)]

    Binds a value to a specific placeholder used on this CollectionFind object.

    An error will be raised if the placeholder indicated by name does not
    exist.

    This function must be called once for each used placeohlder or an error
    will be raised when the execute method is called.

  .execute()

    Executes the find operation with all the configured options.



#@<OUT> Collection.help('get_name')
Returns the name of this database object.

SYNTAX

  <Collection>.get_name()

#@<OUT> Collection.help('get_one')
Fetches the document with the given _id from the collection.

SYNTAX

  <Collection>.get_one(id)

WHERE

  id: The identifier of the document to be retrieved.

RETURNS

 The Document object matching the given id or NULL if no match is found.


#@<OUT> Collection.help('get_schema')
Returns the Schema object of this database object.

SYNTAX

  <Collection>.get_schema()

RETURNS

 The Schema object used to get to this object.

DESCRIPTION

Note that the returned object can be any of:

 - Schema: if the object was created/retrieved using a Schema instance.
 - ClassicSchema: if the object was created/retrieved using an ClassicSchema.
 - Null: if this database object is a Schema or ClassicSchema.


#@<OUT> Collection.help('get_session')
Returns the Session object of this database object.

SYNTAX

  <Collection>.get_session()

RETURNS

 The Session object used to get to this object.

DESCRIPTION

Note that the returned object can be any of:

 - Session: if the object was created/retrieved using an Session instance.
 - ClassicSession: if the object was created/retrieved using an ClassicSession.

#@<OUT> Collection.help('modify')
Creates a collection update handler.

SYNTAX

  <Collection>.modify(...)
              [.set(...)]
              [.unset(...)]
              [.merge(...)]
              [.patch(...)]
              [.array_insert(...)]
              [.array_append(...)]
              [.array_delete(...)]
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

  .patch(...)

    This function adds an operation to update the documents of a collection,
    the patch operation follows the algorithm described on the JSON Merge Patch
    RFC7386.

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

     - The _id of the documents is inmutable, so it will not be affected by the
       patch operation even if it is included on the patch JSON object.
     - The patch JSON object accepts expression objects as values. If used they
       will be evaluated at the server side.

    The patch operations will be done on the collection's documents once the
    execute method is called.

  .array_insert(...)

    Adds an opertion into the modify handler to insert a value into an array
    attribute on the documents that were included on the selection filter and
    limit.

    The insertion of the value will be done on the collection's documents once
    the execute method is called.

  .array_append(...)

    Adds an opertion into the modify handler to append a value into an array
    attribute on the documents that were included on the selection filter and
    limit.

  .array_delete(...)

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

#@<OUT> Collection.help('remove')
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


#@<OUT> Collection.help('remove_one')
Removes document with the given _id value.

SYNTAX

  <Collection>.remove_one(id)

WHERE

  id: The id of the document to be removed.

RETURNS

 A Result object containing the number of affected rows.

DESCRIPTION

If no document is found matching the given id, the Result object will indicate
0 as the number of affected rows.


#@<OUT> Collection.help('replace_one')
Replaces an existing document with a new document.

SYNTAX

  <Collection>.replace_one(id, doc)

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

#@ Finalization
||
