//@ CollectionRemove: valid operations after remove
|All expected functions are available|
|No additional functions are available|

//@ CollectionRemove: valid operations after sort
|All expected functions are available|
|No additional functions are available|

//@ CollectionRemove: valid operations after limit
|All expected functions are available|
|No additional functions are available|

//@ CollectionRemove: valid operations after bind
|All expected functions are available|
|No additional functions are available|

//@ CollectionRemove: valid operations after execute
|All expected functions are available|
|No additional functions are available|

//@ Reusing CRUD with binding
|Deleted donna: 1|
|Deleted alma: 1|

//@# CollectionRemove: Error conditions on remove
||Invalid number of arguments in CollectionRemove.remove, expected 1 but got 0
||CollectionRemove.remove: Requires a search condition.
||CollectionRemove.remove: Argument #1 is expected to be a string
||CollectionRemove.remove: Unterminated quoted string starting at position 8

//@# CollectionRemove: Error conditions sort
||Invalid number of arguments in CollectionRemove.sort, expected at least 1 but got 0
||CollectionRemove.sort: Argument #1 is expected to be a string or an array of strings
||CollectionRemove.sort: Sort criteria can not be empty
||CollectionRemove.sort: Element #2 is expected to be a string
||CollectionRemove.sort: Argument #2 is expected to be a string

//@# CollectionRemove: Error conditions on limit
||Invalid number of arguments in CollectionRemove.limit, expected 1 but got 0
||CollectionRemove.limit: Argument #1 is expected to be an unsigned int

//@# CollectionRemove: Error conditions on bind
||Invalid number of arguments in CollectionRemove.bind, expected 2 but got 0
||CollectionRemove.bind: Argument #1 is expected to be a string
||CollectionRemove.bind: Unable to bind value for unexisting placeholder: another

//@# CollectionRemove: Error conditions on execute
||CollectionRemove.execute: Missing value bindings for the next placeholders: data, years
||CollectionRemove.execute: Missing value bindings for the next placeholders: data

//@ CollectionRemove: remove under condition
|Affected Rows: 1|
|lastDocumentId: Result.getLastDocumentId: document id is not available.|
|getLastDocumentId(): Result.getLastDocumentId: document id is not available.|
|lastDocumentIds: Result.getLastDocumentIds: document ids are not available.|
|getLastDocumentIds(): Result.getLastDocumentIds: document ids are not available.|
|Records Left: 4|

//@ CollectionRemove: remove with binding
|Affected Rows: 2|
|Records Left: 2|


//@ CollectionRemove: full remove
|Affected Rows: 2|
|Records Left: 0|

//@# CollectionRemove: Set Execution
|Set Affected Rows: 1|
|name|
|alias|
|last_name|
|age|

//@ CollectionRemove: Simple Unset Execution
|Unset Affected Rows: 1|
|name|
|alias|
|~last_name|
|age|

//@ CollectionRemove: List Unset Execution
|Unset Affected Rows: 1|
|name|
|~alias|
|~last_name|
|~age|


//@ CollectionRemove: Merge Execution
|Merge Affected Rows: 1|

|Brian's last_name: black|
|Brian's age: 15|
|Brian's alias: bri|
|Brian's first girlfriend: martha|
|Brian's second girlfriend: karen|

//@ CollectionRemove: arrayAppend Execution
|Array Append Affected Rows: 1|
|Brian's girlfriends: 3|
|Brian's last: cloe|

//@ CollectionRemove: arrayInsert Execution
|Array Insert Affected Rows: 1|
|Brian's girlfriends: 4|
|Brian's second: samantha|

//@ CollectionRemove: arrayDelete Execution
|Array Delete Affected Rows: 1|
|Brian's girlfriends: 3|
|Brian's third: cloe|

//@ CollectionRemove: sorting and limit Execution
|Affected Rows: 2|

//@ CollectionRemove: sorting and limit Execution - 1
|name|
|age|
|gender|
|sample|

//@ CollectionRemove: sorting and limit Execution - 2
|name|
|age|
|gender|
|sample|

//@ CollectionRemove: sorting and limit Execution - 3
|name|
|age|
|gender|
|~sample|

//@ CollectionRemove: sorting and limit Execution - 4
|name|
|age|
|gender|
|~sample|

//@<OUT> CollectionRemove: help
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
