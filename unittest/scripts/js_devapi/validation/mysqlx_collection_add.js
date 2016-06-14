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
||Invalid number of arguments in CollectionAdd.add, expected 1 but got 0
||CollectionAdd.add: Argument is expected to be either a document or a list of documents
||CollectionAdd.add: Element #1 is expected to be a document or a JSON expression
||CollectionAdd.add: Element #1 is expected to be a JSON expression
||CollectionAdd.add: Invalid data type for _id field, should be a string


//@ Collection.add execution
|Affected Rows Single: 1|
|lastDocumentId Single:|
|getLastDocumentId Single:|
|#lastDocumentIds Single: 1|
|#getLastDocumentIds Single: 1|

|Affected Rows Single Known ID: 1|
|lastDocumentId Single Known ID: sample_document|
|getLastDocumentId Single Known ID: sample_document|
|#lastDocumentIds Single Known ID: 1|
|#getLastDocumentIds Single Known ID: 1|
|#lastDocumentIds Single Known ID: sample_document|
|#getLastDocumentIds Single Known ID: sample_document|


|Affected Rows Multi: 2|
|lastDocumentId Multi: Result.getLastDocumentId(): document id is not available.|
|getLastDocumentId Multi: Result.getLastDocumentId(): document id is not available.|
|#lastDocumentIds Multi: 2|
|#getLastDocumentIds Multi: 2|


|Affected Rows Multi Known IDs: 2|
|lastDocumentId Multi Known IDs: Result.getLastDocumentId(): document id is not available.|
|getLastDocumentId Multi Known IDs: Result.getLastDocumentId(): document id is not available.|

|#lastDocumentIds Multi Known IDs: 2|
|#getLastDocumentIds Multi Known IDs: 2|
|First lastDocumentIds Multi Known IDs: known_00|
|First getLastDocumentIds Multi Known IDs: known_00|
|Second lastDocumentIds Multi Known IDs: known_01|
|Second getLastDocumentIds Multi Known IDs: known_01|

|Affected Rows Empty List: -1|
|lastDocumentId Empty List: Result.getLastDocumentId(): document id is not available.|
|getLastDocumentId Empty List: Result.getLastDocumentId(): document id is not available.|

|#lastDocumentIds Empty List: 0|
|#getLastDocumentIds Empty List: 0|

|Affected Rows Chained: 2|
|Affected Rows Single Expression: 1|
|Affected Rows Mixed List: 2|

