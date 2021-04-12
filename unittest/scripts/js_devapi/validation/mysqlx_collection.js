
//@ Testing collection name retrieving
|getName(): collection1|
|name: collection1|

//@ Testing session retrieving
|getSession(): <Session:|
|session: <Session:|

//@ Testing collection schema retrieving
|getSchema(): <Schema:js_shell_test>|
|schema: <Schema:js_shell_test>|

//@ Testing dropping index
|undefined|
|undefined|
|undefined|

//@ Testing dropping index using execute
||TypeError: Cannot read property 'execute' of undefined

//@ Testing existence
|Valid: true|
|Invalid: false|

//================= addOrReplaceOne ======================
//@ addOrReplaceOne parameter error conditions
||Collection.addOrReplaceOne: Invalid number of arguments, expected 2 but got 0
||Collection.addOrReplaceOne: Argument #1 is expected to be a string
||Collection.addOrReplaceOne: Argument #2 is expected to be a map

//@ addOrReplaceOne: adding new document 1
|Query OK, 1 item affected|

//@ addOrReplaceOne: adding new document 2
|Query OK, 1 item affected|

//@<OUT> Verify added documents
{
    "_id": "document_001",
    "name": "basic"
}
{
    "_id": "document_002",
    "name": "basic"
}

//@ addOrReplaceOne: replacing an existing document
|Query OK, 2 items affected|

//@<OUT> addOrReplaceOne: Verify replaced document
{
    "_id": "document_001",
    "name": "complex",
    "state": "updated"
}
{
    "_id": "document_002",
    "name": "basic"
}

//@ addOrreplaceOne: replacing an existing document, wrong _id
||Collection.addOrReplaceOne: Replacement document has an _id that is different than the matched document

//@ addOrReplaceOne: adding with key
|Query OK, 1 item affected|

//@ addOrReplaceOne: error adding with key (BUG#27013165) {VER(>=8.0.3) && VER(<8.0.20)}
||Unable upsert data in document collection 'add_or_replace_one' (MySQL Error 5018)

//@ addOrReplaceOne: error adding with key (BUG#27013165) {VER(>=8.0.20)}
||Document contains a field value that is not unique but required to be (MySQL Error 5116)

//@ addOrReplaceOne: error replacing with key
||Document contains a field value that is not unique but required to be

//@ addOrReplaceOne: replacing document matching id and key
|Query OK, 2 items affected|

//@ addOrReplaceOne: attempt on dropped collection
||Table 'js_shell_test.add_or_replace_one' doesn't exist


//================= getOne ======================
//@ getOne: parameter error conditions
||Collection.getOne: Invalid number of arguments, expected 1 but got 0
||Collection.getOne: Argument #1 is expected to be a string

//@<OUT> getOne: returns expected document
{
    "_id": "document_001",
    "name": "test"
}

//@ getOne: returns NULL if no match found
|null|

//@ getOne: attempt on dropped collection
||Table 'js_shell_test.get_one' doesn't exist

//================= removeOne ======================
//@<OUT> removeOne: initialization
{
    "_id": "document_001",
    "name": "test"
}
{
    "_id": "document_002",
    "name": "test"
}


//@ removeOne: parameter error conditions
||Collection.removeOne: Invalid number of arguments, expected 1 but got 0
||Collection.removeOne: Argument #1 is expected to be a string

//@ removeOne: removes the expected document
|Query OK, 1 item affected|

//@ removeOne: suceeds with 0 affected rows
|Query OK, 0 items affected|

//@<OUT> removeOne: final verification
{
    "_id": "document_002",
    "name": "test"
}


//@ removeOne: attempt on dropped collection
||Table 'js_shell_test.remove_one' doesn't exist

//================= replaceOne ======================
//@<OUT> replaceOne: initialization
{
    "_id": "document_001",
    "name": "simple"
}
{
    "_id": "document_002",
    "name": "simple"
}

//@ replaceOne parameter error conditions
||Collection.replaceOne: Invalid number of arguments, expected 2 but got 0
||Collection.replaceOne: Argument #1 is expected to be a string
||Collection.replaceOne: Argument #2 is expected to be a map

//@ replaceOne: replacing an existing document
|Query OK, 1 item affected|

//@<OUT> replaceOne: Verify replaced document
{
    "_id": "document_001",
    "name": "complex",
    "state": "updated"
}
{
    "_id": "document_002",
    "name": "simple"
}

//@ replaceOne: replacing unexisting document
|Query OK, 0 items affected|

//@ replaceOne: replacing an existing document, wrong _id
||Collection.replaceOne: Replacement document has an _id that is different than the matched document

//@ replaceOne: error replacing with key {VER(< 8.0.19)}
||Duplicate entry 'simple' for key '_name'

//@ replaceOne: error replacing with key {VER(>= 8.0.19)}
||Duplicate entry 'simple' for key 'replace_one._name'

//@ replaceOne: replacing document matching id and key
|Query OK, 1 item affected|

//@<OUT> Verify replaced document with id and key
{
    "_id": "document_001",
    "name": "medium",
    "sample": true
}
{
    "_id": "document_002",
    "name": "simple"
}

//@ replaceOne: attempt on dropped collection
||Table 'js_shell_test.replace_one' doesn't exist

//@ WL12412-TS1_1: Count takes no arguments
||Collection.count: Invalid number of arguments, expected 0 but got 1 (ArgumentError)

//@ WL12412-TS1_2: Count returns correct number of documents
|Initial Document Count: 0|
|Final Document Count: 3|

//@ WL12412-TS2_1: Count throws error on unexisting collection
||Collection.count: Table 'js_shell_test.count_collection' doesn't exist (MYSQLSH 1146)

//@<OUT> BUG32377134 Add empty list of document crash MySQL Shell
Records: 2  Duplicates: 0  Warnings: 0
Query OK, 1 item affected ([[*]])
Query OK, 1 item affected ([[*]])
{
    "_id": "001"
}
{
    "_id": "002"
}
{
    "_id": "003"
}
{
    "_id": "004"
}
4 documents in set ([[*]])
