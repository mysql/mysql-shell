//@ CollectionAdd: valid operations after add
|All expected functions are available|
|No additional functions are available|

//@ CollectionAdd: valid operations after execute
|All expected functions are available|
|No additional functions are available|

//@# CollectionAdd: Error conditions on add
||Invalid number of arguments in CollectionAdd.add, expected 1 but got 0
||CollectionAdd.add: Argument is expected to be either a document or a list of documents
||CollectionAdd.add: Element #1 is expected to be a document

//@ Collection.add single document
|Affected Rows Single: 1|
|Affected Rows List: 2|
|Affected Rows Chained: 2|
