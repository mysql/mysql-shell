#@ Testing collection name retrieving
|get_name(): collection1|
|name: collection1|

#@ Testing session retrieving
|get_session(): <Session:|
|session: <Session:|

#@ Testing collection schema retrieving
|get_schema(): <Schema:py_shell_test>|
|schema: <Schema:py_shell_test>|

#@ Testing dropping index
|None|
|None|
|None|

#@ Testing dropping index using execute
||AttributeError: 'NoneType' object has no attribute 'execute'

#@ Testing existence
|Valid: True|
|Invalid: False|

#================= add_or_replace_one ======================
#@ add_or_replace_one parameter error conditions
||Collection.add_or_replace_one: Invalid number of arguments, expected 2 but got 0
||Collection.add_or_replace_one: Argument #1 is expected to be a string
||Collection.add_or_replace_one: Argument #2 is expected to be a map

#@ add_or_replace_one: adding new document 1
|Query OK, 1 item affected|

#@ add_or_replace_one: adding new document 2
|Query OK, 1 item affected|

#@<OUT> Verify added documents
{
    "_id": "document_001",
    "name": "basic"
}
{
    "_id": "document_002",
    "name": "basic"
}

#@ add_or_replace_one: replacing an existing document
|Query OK, 2 items affected|

#@<OUT> add_or_replace_one: Verify replaced document
{
    "_id": "document_001",
    "name": "complex",
    "state": "updated"
}
{
    "_id": "document_002",
    "name": "basic"
}

#@ add_or_replace_one: replacing an existing document, ignoring new _id
|Query OK, 2 items affected|

#@<OUT> add_or_replace_one: Verify replaced document with ignored _id
{
    "_id": "document_001",
    "name": "medium"
}
{
    "_id": "document_002",
    "name": "basic"
}

#@ add_or_replace_one: adding with key
|Query OK, 1 item affected|

#@ add_or_replace_one: error adding with key (BUG#27013165) {VER(>=8.0.3) and VER(<8.0.20)}
||MySQL Error (5018): Unable upsert data in document collection 'add_or_replace_one'

#@ add_or_replace_one: error adding with key (BUG#27013165) {VER(>=8.0.20)}
||MySQL Error (5116): Document contains a field value that is not unique but required to be

#@ add_or_replace_one: error replacing with key
||Document contains a field value that is not unique but required to be

#@ add_or_replace_one: replacing document matching id and key
|Query OK, 2 items affected|

#@ add_or_replace_one: attempt on dropped collection
||Table 'py_shell_test.add_or_replace_one' doesn't exist


#================= get_one ======================
#@ get_one: parameter error conditions
||Collection.get_one: Invalid number of arguments, expected 1 but got 0
||Collection.get_one: Argument #1 is expected to be a string

#@<OUT> get_one: returns expected document
{
    "_id": "document_001",
    "name": "test"
}

#@ get_one: returns NULL if no match found
|None|

#@ get_one: attempt on dropped collection
||Table 'py_shell_test.get_one' doesn't exist

#================= remove_one ======================
#@<OUT> remove_one: initialization
{
    "_id": "document_001",
    "name": "test"
}
{
    "_id": "document_002",
    "name": "test"
}


#@ remove_one: parameter error conditions
||Collection.remove_one: Invalid number of arguments, expected 1 but got 0
||Collection.remove_one: Argument #1 is expected to be a string

#@ remove_one: removes the expected document
|Query OK, 1 item affected|

#@ remove_one: suceeds with 0 affected rows
|Query OK, 0 items affected|

#@<OUT> remove_one: final verification
{
    "_id": "document_002",
    "name": "test"
}


#@ remove_one: attempt on dropped collection
||Table 'py_shell_test.remove_one' doesn't exist

#================= replace_one ======================
#@<OUT> replace_one: initialization
{
    "_id": "document_001",
    "name": "simple"
}
{
    "_id": "document_002",
    "name": "simple"
}

#@ replace_one parameter error conditions
||Collection.replace_one: Invalid number of arguments, expected 2 but got 0
||Collection.replace_one: Argument #1 is expected to be a string
||Collection.replace_one: Argument #2 is expected to be a map

#@ replace_one: replacing an existing document
|Query OK, 1 item affected|

#@<OUT> replace_one: Verify replaced document
{
    "_id": "document_001",
    "name": "complex",
    "state": "updated"
}
{
    "_id": "document_002",
    "name": "simple"
}

#@ replace_one: replacing unexisting document
|Query OK, 0 items affected|

#@ replace_one: replacing an existing document, ignoring new _id
|Query OK, 1 item affected|

#@<OUT> replace_one: Verify replaced document with ignored _id
{
    "_id": "document_001",
    "name": "medium"
}
{
    "_id": "document_002",
    "name": "simple"
}


#@ replace_one: error replacing with key {VER(< 8.0.19)}
||Duplicate entry 'simple' for key '_name'

#@ replace_one: error replacing with key {VER(>= 8.0.19)}
||Duplicate entry 'simple' for key 'replace_one._name'

#@ replace_one: replacing document matching id and key
|Query OK, 1 item affected|

#@<OUT> Verify replaced document with id and key
{
    "_id": "document_001",
    "name": "medium",
    "sample": true
}
{
    "_id": "document_002",
    "name": "simple"
}

#@ replace_one: attempt on dropped collection
||Table 'py_shell_test.replace_one' doesn't exist

#@ WL12412-TS1_1: Count takes no arguments
||ArgumentError: Collection.count: Invalid number of arguments, expected 0 but got 1

#@ WL12412-TS1_2: Count returns correct number of documents
|Initial Document Count: 0|
|Final Document Count: 3|

#@ WL12412-TS2_1: Count throws error on unexisting collection
||RuntimeError: Collection.count: Table 'py_shell_test.count_collection' doesn't exist
