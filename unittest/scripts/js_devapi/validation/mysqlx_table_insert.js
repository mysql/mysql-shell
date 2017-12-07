//@ TableInsert: valid operations after empty insert
|All expected functions are available|
|No additional functions are available|

//@ TableInsert: valid operations after empty insert and values
|All expected functions are available|
|No additional functions are available|

//@ TableInsert: valid operations after empty insert and values 2
|All expected functions are available|
|No additional functions are available|

//@ TableInsert: valid operations after insert with field list
|All expected functions are available|
|No additional functions are available|

//@ TableInsert: valid operations after insert with field list and values
|All expected functions are available|
|No additional functions are available|

//@ TableInsert: valid operations after insert with field list and values 2
|All expected functions are available|
|No additional functions are available|

//@ TableInsert: valid operations after insert with fields and values
|All expected functions are available|
|No additional functions are available|

//@ TableInsert: valid operations after execute
|All expected functions are available|
|No additional functions are available|

//@# TableInsert: Error conditions on insert
||TableInsert.insert: Argument #1 is expected to be either string, a list of strings or a map with fields and values
||TableInsert.insert: Argument #2 is expected to be a string
||TableInsert.insert: Element #2 is expected to be a string
||ERROR: 5014: Wrong number of fields in row being inserted
||Unsupported value received: <Session
||Unknown column 'id' in 'field list'

//@ Table.insert execution
|Affected Rows No Columns: 1|
|Affected Rows Columns: 1|
|Affected Rows Multiple Values: 2|
|Affected Rows Document: 1|
|lastDocumentId: Result.getLastDocumentId: document id is not available.|
|getLastDocumentId(): Result.getLastDocumentId: document id is not available.|
|lastDocumentIds: Result.getLastDocumentIds: document ids are not available.|
|getLastDocumentIds(): Result.getLastDocumentIds: document ids are not available.|

//@ Table.insert execution on a View
|Affected Rows Through View: 1|
