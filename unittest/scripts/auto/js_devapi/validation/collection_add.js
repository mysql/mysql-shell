//@ CollectionAdd: valid operations after add with no documents
|All expected functions are available|
|No additional functions are available|

//@ CollectionAdd: valid operations after add
|All expected functions are available|
|No additional functions are available|

//@ CollectionAdd: valid operations after execute
|All expected functions are available|
|No additional functions are available|

//@# CollectionAdd: Error conditions on add
||Invalid number of arguments in CollectionAdd.add, expected at least 1 but got 0
||CollectionAdd.add: Argument #1 expected to be a document, JSON expression or a list of documents
||CollectionAdd.add: Element #1 expected to be a document, JSON expression or a list of documents
||CollectionAdd.add: Argument #1 expected to be a document, JSON expression or a list of documents (ArgumentError)
||CollectionAdd.add: Element #2 expected to be a document, JSON expression or a list of documents
||CollectionAdd.add: Argument #2 expected to be a document, JSON expression or a list of documents

//@ WL11435_ET1_1 Collection.add error no id
||Document is missing a required field

//@ Collection.add execution, Variations >=8.0.5
|Affected Rows Chained: 2|
|Affected Rows Single Expression: 1|
|Affected Rows Mixed List: 2|
|Affected Rows Multiple Params: 2|
