#@ CollectionModify: valid operations after modify and set
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and unset empty
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and unset list
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and unset multiple params
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and merge
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and array_insert
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and array_append
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and array_delete
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after sort
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after limit
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after bind
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after execute
|All expected functions are available|
|No additional functions are available|

#@ Reusing CRUD with binding
|Updated Angel: 1|
|Updated Carol: 1|

#@# CollectionModify: Error conditions on modify
||Invalid number of arguments in CollectionModify.modify, expected 1 but got 0
||CollectionModify.modify: Requires a search condition.
||CollectionModify.modify: Argument #1 is expected to be a string
||CollectionModify.modify: Unterminated quoted string starting at position 8

#@# CollectionModify: Error conditions on set
||Invalid number of arguments in CollectionModify.set, expected 2 but got 0
||CollectionModify.set: Argument #1 is expected to be a string
||CollectionModify.set: Invalid document path

#@# CollectionModify: Error conditions on unset
||Invalid number of arguments in CollectionModify.unset, expected at least 1 but got 0
||CollectionModify.unset: Argument #1 is expected to be either string or list of strings
||CollectionModify.unset: Argument #1 is expected to be a string
||CollectionModify.unset: Element #2 is expected to be a string
||CollectionModify.unset: Invalid document path

#@# CollectionModify: Error conditions on merge
||Invalid number of arguments in CollectionModify.merge, expected 1 but got 0
||CollectionModify.merge: Argument expected to be a JSON object

#@# CollectionModify: Error conditions on array_insert
||Invalid number of arguments in CollectionModify.array_insert, expected 2 but got 0
||CollectionModify.array_insert: Argument #1 is expected to be a string
||CollectionModify.array_insert: Invalid document path
||CollectionModify.array_insert: An array document path must be specified

#@# CollectionModify: Error conditions on array_append
||Invalid number of arguments in CollectionModify.array_append, expected 2 but got 0
||CollectionModify.array_append: Argument #1 is expected to be a string
||CollectionModify.array_append: Invalid document path
||CollectionModify.array_append: Unsupported value received: <NodeSession:

#@# CollectionModify: Error conditions on array_delete
||Invalid number of arguments in CollectionModify.array_delete, expected 1 but got
||CollectionModify.array_delete: Argument #1 is expected to be a string
||CollectionModify.array_delete: Invalid document path
||CollectionModify.array_delete: An array document path must be specified

#@# CollectionModify: Error conditions on sort
||Invalid number of arguments in CollectionModify.sort, expected at least 1 but got 0
||CollectionModify.sort: Argument #1 is expected to be a string or an array of strings
||CollectionModify.sort: Sort criteria can not be empty
||CollectionModify.sort: Element #2 is expected to be a string
||CollectionModify.sort: Argument #2 is expected to be a string

#@# CollectionModify: Error conditions on limit
||Invalid number of arguments in CollectionModify.limit, expected 1 but got 0
||CollectionModify.limit: Argument #1 is expected to be an unsigned int

#@# CollectionModify: Error conditions on bind
||Invalid number of arguments in CollectionModify.bind, expected 2 but got 0
||CollectionModify.bind: Argument #1 is expected to be a string
||CollectionModify.bind: Unable to bind value for unexisting placeholder: another

#@# CollectionModify: Error conditions on execute
||CollectionModify.execute: Missing value bindings for the following placeholders: data, years
||CollectionModify.execute: Missing value bindings for the following placeholders: data

#@# CollectionModify: Set Execution
|Set Affected Rows: 1|
|name|
|alias|
|last_name|
|age|

#@# CollectionModify: Set Execution Binding Array
|Set Affected Rows: 1|
|name|
|alias|
|last_name|
|age|
|soccer|
|dance|
|reading|

#@ CollectionModify: Simple Unset Execution
|Unset Affected Rows: 1|
|name|
|alias|
|~last_name|
|age|

#@ CollectionModify: List Unset Execution
|Unset Affected Rows: 1|
|name|
|~alias|
|~last_name|
|~age|


#@ CollectionModify: Merge Execution
|Merge Affected Rows: 1|

|Brian's last_name: black|
|Brian's age: 15|
|Brian's alias: bri|
|Brian's first girlfriend: martha|
|Brian's second girlfriend: karen|

#@ CollectionModify: array_append Execution
|last_document_id: LogicError: Result.get_last_document_id: document id is not available.|
|get_last_document_id(): LogicError: Result.get_last_document_id: document id is not available.|
|last_document_ids: LogicError: Result.get_last_document_ids: document ids are not available.|
|get_last_document_ids(): LogicError: Result.get_last_document_ids: document ids are not available.|

|Array Append Affected Rows: 1|
|Brian's girlfriends: 3|
|Brian's last: cloe|

#@ CollectionModify: array_insert Execution
|Array Insert Affected Rows: 1|
|Brian's girlfriends: 4|
|Brian's second: samantha|

#@ CollectionModify: array_delete Execution
|Array Delete Affected Rows: 1|
|Brian's girlfriends: 3|
|Brian's third: cloe|

#@ CollectionModify: sorting and limit Execution
|Affected Rows: 2|

#@ CollectionModify: sorting and limit Execution - 1
|name|
|age|
|gender|
|sample|

#@ CollectionModify: sorting and limit Execution - 2
|name|
|age|
|gender|
|sample|

#@ CollectionModify: sorting and limit Execution - 3
|name|
|age|
|gender|
|~sample|

#@ CollectionModify: sorting and limit Execution - 4
|name|
|age|
|gender|
|~sample|

#@<OUT> CollectionModify: help
Creates a collection update handler.

SYNTAX

  <Collection>.modify(...)
              [.set(...)]
              [.unset(...)]
              [.merge(...)]
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
