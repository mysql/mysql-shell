#@ TableUpdate: valid operations after update
|All expected functions are available|
|No additional functions are available|

#@ TableUpdate: valid operations after set
|All expected functions are available|
|No additional functions are available|

#@ TableUpdate: valid operations after where
|All expected functions are available|
|No additional functions are available|

#@ TableUpdate: valid operations after order_by
|All expected functions are available|
|No additional functions are available|

#@ TableUpdate: valid operations after limit
|All expected functions are available|
|No additional functions are available|

#@ TableUpdate: valid operations after bind
|All expected functions are available|
|No additional functions are available|

#@ TableUpdate: valid operations after execute
|All expected functions are available|
|No additional functions are available|

#@ Reusing CRUD with binding
|Updated Angel: 1|
|Updated Carol: 1|

#@# TableUpdate: Error conditions on update
||Invalid number of arguments in TableUpdate.update, expected 0 but got 1

#@# TableUpdate: Error conditions on set
||Invalid number of arguments in TableUpdate.set, expected 2 but got 0
||TableUpdate.set: Argument #1 is expected to be a string
||TableUpdate.set: Unsupported value received for table update operation on field "name", received: <NodeSession:


#@# TableUpdate: Error conditions on where
||Invalid number of arguments in TableUpdate.where, expected 1 but got 0
||TableUpdate.where: Argument #1 is expected to be a string
||TableUpdate.where: Unterminated quoted string starting at position 8


#@# TableUpdate: Error conditions on order_by
||Invalid number of arguments in TableUpdate.order_by, expected 1 but got 0
||TableUpdate.order_by: Argument #1 is expected to be an array
||TableUpdate.order_by: Order criteria can not be empty
||TableUpdate.order_by: Element #2 is expected to be a string

#@# TableUpdate: Error conditions on limit
||Invalid number of arguments in TableUpdate.limit, expected 1 but got 0
||TableUpdate.limit: Argument #1 is expected to be an unsigned int

#@# TableUpdate: Error conditions on bind
||Invalid number of arguments in TableUpdate.bind, expected 2 but got 0
||TableUpdate.bind: Argument #1 is expected to be a string
||TableUpdate.bind: Unable to bind value for unexisting placeholder: another

#@# TableUpdate: Error conditions on execute
||TableUpdate.execute: Missing value bindings for the next placeholders: data, years
||TableUpdate.execute: Missing value bindings for the next placeholders: data

#@# TableUpdate: simple test
|Affected Rows: 1|
|Updated Record: aline 13|

#@ TableUpdate: test using expression
|Affected Rows: 1|
|Updated Record: aline 23|

#@ TableUpdate: test using limits
|Affected Rows: 2|
|last_document_id: LogicError: Result.get_last_document_id: document id is not available.|
|get_last_document_id(): LogicError: Result.get_last_document_id: document id is not available.|
|last_document_ids: LogicError: Result.get_last_document_ids: document ids are not available.|
|get_last_document_ids(): LogicError: Result.get_last_document_ids: document ids are not available.|
|With 16 Years: 3|
|With 15 Years: 1|

#@ TableUpdate: test full update with view object
|Updated Females: 4|
|All Females: 7|
