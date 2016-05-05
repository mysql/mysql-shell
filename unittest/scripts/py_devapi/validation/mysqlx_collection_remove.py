#@ CollectionRemove: valid operations after remove
|All expected functions are available|
|No additional functions are available|

#@ CollectionRemove: valid operations after sort
|All expected functions are available|
|No additional functions are available|

#@ CollectionRemove: valid operations after limit
|All expected functions are available|
|No additional functions are available|

#@ CollectionRemove: valid operations after bind
|All expected functions are available|
|No additional functions are available|

#@ CollectionRemove: valid operations after execute
|All expected functions are available|
|No additional functions are available|

#@ Reusing CRUD with binding
|Deleted donna: 1|
|Deleted alma: 1|

#@# CollectionRemove: Error conditions on remove
||CollectionRemove.remove: Argument #1 is expected to be a string
||CollectionRemove.remove: Unterminated quoted string starting at position 8

#@# CollectionRemove: Error conditions sort
||Invalid number of arguments in CollectionRemove.sort, expected 1 but got 0
||CollectionRemove.sort: Argument #1 is expected to be an array
||CollectionRemove.sort: Sort criteria can not be empty
||CollectionRemove.sort: Element #2 is expected to be a string

#@# CollectionRemove: Error conditions on limit
||Invalid number of arguments in CollectionRemove.limit, expected 1 but got 0
||CollectionRemove.limit: Argument #1 is expected to be an unsigned int

#@# CollectionRemove: Error conditions on bind
||Invalid number of arguments in CollectionRemove.bind, expected 2 but got 0
||CollectionRemove.bind: Argument #1 is expected to be a string
||CollectionRemove.bind: Unable to bind value for unexisting placeholder: another

#@# CollectionRemove: Error conditions on execute
||CollectionRemove.execute: Missing value bindings for the next placeholders: data, years
||CollectionRemove.execute: Missing value bindings for the next placeholders: data

#@ CollectionRemove: remove under condition
|Affected Rows: 1|
|lastDocumentId: LogicError: Result.getLastDocumentId(): document id is not available.|
|getLastDocumentId(): LogicError: Result.getLastDocumentId(): document id is not available.|
|lastDocumentIds: LogicError: Result.getLastDocumentIds(): document ids are not available.|
|getLastDocumentIds(): LogicError: Result.getLastDocumentIds(): document ids are not available.|
|Records Left: 4|

#@ CollectionRemove: remove with binding
|Affected Rows: 2|
|Records Left: 2|


#@ CollectionRemove: full remove
|Affected Rows: 2|
|Records Left: 0|

#@# CollectionRemove: Set Execution
|Set Affected Rows: 1|
|name|
|alias|
|last_name|
|age|

#@ CollectionRemove: Simple Unset Execution
|Unset Affected Rows: 1|
|name|
|alias|
|~last_name|
|age|

#@ CollectionRemove: List Unset Execution
|Unset Affected Rows: 1|
|name|
|~alias|
|~last_name|
|~age|


#@ CollectionRemove: Merge Execution
|Merge Affected Rows: 1|

|Brian's last_name: black|
|Brian's age: 15|
|Brian's alias: bri|
|Brian's first girlfriend: martha|
|Brian's second girlfriend: karen|

#@ CollectionRemove: arrayAppend Execution
|Array Append Affected Rows: 1|
|Brian's girlfriends: 3|
|Brian's last: cloe|

#@ CollectionRemove: arrayInsert Execution
|Array Insert Affected Rows: 1|
|Brian's girlfriends: 4|
|Brian's second: samantha|

#@ CollectionRemove: arrayDelete Execution
|Array Delete Affected Rows: 1|
|Brian's girlfriends: 3|
|Brian's third: cloe|

#@ CollectionRemove: sorting and limit Execution
|Affected Rows: 2|

#@ CollectionRemove: sorting and limit Execution - 1
|name|
|age|
|gender|
|sample|

#@ CollectionRemove: sorting and limit Execution - 2
|name|
|age|
|gender|
|sample|

#@ CollectionRemove: sorting and limit Execution - 3
|name|
|age|
|gender|
|~sample|

#@ CollectionRemove: sorting and limit Execution - 4
|name|
|age|
|gender|
|~sample|
