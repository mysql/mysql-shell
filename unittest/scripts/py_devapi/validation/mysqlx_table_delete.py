#@ TableDelete: valid operations after delete
|All expected functions are available|
|No additional functions are available|

#@ TableDelete: valid operations after where
|All expected functions are available|
|No additional functions are available|

#@ TableDelete: valid operations after order_by
|All expected functions are available|
|No additional functions are available|

#@ TableDelete: valid operations after limit
|All expected functions are available|
|No additional functions are available|

#@ TableDelete: valid operations after bind
|All expected functions are available|
|No additional functions are available|

#@ TableDelete: valid operations after execute
|All expected functions are available|
|No additional functions are available|

#@ Reusing CRUD with binding
|Deleted donna: 1|
|Deleted alma: 1|


#@# TableDelete: Error conditions on delete
||Invalid number of arguments in TableDelete.delete, expected 0 but got 1

#@# TableDelete: Error conditions on where
||Invalid number of arguments in TableDelete.where, expected 1 but got 0
||TableDelete.where: Argument #1 is expected to be a string
||TableDelete.where: Unterminated quoted string starting at position 8

#@# TableDelete: Error conditions on order_by
||Invalid number of arguments in TableDelete.order_by, expected at least 1 but got 0
||TableDelete.order_by: Argument #1 is expected to be a string or an array of strings
||TableDelete.order_by: Order criteria can not be empty
||TableDelete.order_by: Element #2 is expected to be a string
||TableDelete.order_by: Argument #2 is expected to be a string

#@# TableDelete: Error conditions on limit
||Invalid number of arguments in TableDelete.limit, expected 1 but got 0
||TableDelete.limit: Argument #1 is expected to be an unsigned int

#@# TableDelete: Error conditions on bind
||Invalid number of arguments in TableDelete.bind, expected 2 but got 0
||TableDelete.bind: Argument #1 is expected to be a string
||TableDelete.bind: Unable to bind value for unexisting placeholder: another

#@# TableDelete: Error conditions on execute
||TableDelete.execute: Missing value bindings for the next placeholders: data, years
||TableDelete.execute: Missing value bindings for the next placeholders: data

#@ TableDelete: delete under condition
|Affected Rows: 1|
|Records Left: 4|

#@ TableDelete: delete with binding
|Affected Rows: 2|
|Records Left: 2|


#@ TableDelete: full delete with a view object
|Affected Rows: 2|
|Records Left: 0|

#@ TableDelete: with limit 0
|Records Left: 5|

#@ TableDelete: with limit 1
|Affected Rows: 2|
|Records Left: 3|

#@ TableDelete: with limit 2
|Affected Rows: 2|
|Records Left: 1|

#@ TableDelete: with limit 3
|Affected Rows: 1|
|last_document_id: LogicError: Result.get_last_document_id: document id is not available.|
|get_last_document_id(): LogicError: Result.get_last_document_id: document id is not available.|
|last_document_ids: LogicError: Result.get_last_document_ids: document ids are not available.|
|get_last_document_ids(): LogicError: Result.get_last_document_ids: document ids are not available.|
|Records Left: 0|
