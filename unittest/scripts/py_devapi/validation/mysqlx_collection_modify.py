#@ CollectionModify: valid operations after modify and set
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and unset empty
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and unset list
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and unset multiple params
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and merge
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and patch
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and array_insert
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and array_append
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after modify and array_delete
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after sort
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after limit
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after bind
|All expected functions are available|
|No additional functions are available|

#@ CollectionModify: valid operations after execute
|All expected functions are available|
|No additional functions are available|

#@ Reusing CRUD with binding
|Updated Angel: 1|
|Updated Carol: 1|

#@# CollectionModify: Error conditions on modify
||Collection.modify: Invalid number of arguments, expected 1 but got 0
||Collection.modify: Requires a search condition.
||Collection.modify: Argument #1 is expected to be a string
||Collection.modify: Unterminated quoted string starting at position 8

#@# CollectionModify: Error conditions on set
||CollectionModify.set: Invalid number of arguments, expected 2 but got 0
||CollectionModify.set: Argument #1 is expected to be a string
||CollectionModify.set: Invalid document path
||CollectionModify.set: Invalid document path
||CollectionModify.set: Invalid document path

#@# CollectionModify: Error conditions on unset
||CollectionModify.unset: Invalid number of arguments, expected at least 1 but got 0
||CollectionModify.unset: Argument #1 is expected to be either string or list of strings
||CollectionModify.unset: Argument #1 is expected to be a string
||CollectionModify.unset: Element #2 is expected to be a string
||CollectionModify.unset: Invalid document path
||CollectionModify.unset: Invalid document path
||CollectionModify.unset: Invalid document path

#@# CollectionModify: Error conditions on merge
||CollectionModify.merge: Invalid number of arguments, expected 1 but got 0
||CollectionModify.merge: Argument expected to be a JSON object

#@# CollectionModify: Error conditions on patch
||CollectionModify.patch: Invalid number of arguments, expected 1 but got 0
||CollectionModify.patch: Argument expected to be a JSON object

#@# CollectionModify: Error conditions on array_insert
||CollectionModify.array_insert: Invalid number of arguments, expected 2 but got 0
||CollectionModify.array_insert: Argument #1 is expected to be a string
||CollectionModify.array_insert: Invalid document path
||CollectionModify.array_insert: Invalid document path
||CollectionModify.array_insert: Invalid document path
||CollectionModify.array_insert: An array document path must be specified

#@# CollectionModify: Error conditions on array_append
||CollectionModify.array_append: Invalid number of arguments, expected 2 but got 0
||CollectionModify.array_append: Argument #1 is expected to be a string
||CollectionModify.array_append: Invalid document path
||CollectionModify.array_append: Invalid document path
||CollectionModify.array_append: Invalid document path
||CollectionModify.array_append: Unsupported value received: <Session:

#@# CollectionModify: Error conditions on array_delete
||CollectionModify.array_delete: Invalid number of arguments, expected 1 but got
||CollectionModify.array_delete: Argument #1 is expected to be a string
||CollectionModify.array_delete: Invalid document path
||CollectionModify.array_delete: Invalid document path
||CollectionModify.array_delete: Invalid document path
||CollectionModify.array_delete: An array document path must be specified

#@# CollectionModify: Error conditions on sort
||CollectionModify.sort: Invalid number of arguments, expected at least 1 but got 0
||CollectionModify.sort: Argument #1 is expected to be a string or an array of strings
||CollectionModify.sort: Sort criteria can not be empty
||CollectionModify.sort: Element #2 is expected to be a string
||CollectionModify.sort: Argument #2 is expected to be a string

#@# CollectionModify: Error conditions on limit
||CollectionModify.limit: Invalid number of arguments, expected 1 but got 0
||CollectionModify.limit: Argument #1 is expected to be an unsigned int

#@# CollectionModify: Error conditions on bind
||CollectionModify.bind: Invalid number of arguments, expected 2 but got 0
||CollectionModify.bind: Argument #1 is expected to be a string
||CollectionModify.bind: Unable to bind value for unexisting placeholder: another

#@# CollectionModify: Error conditions on execute
||CollectionModify.execute: Missing value bindings for the following placeholders: data, years
||CollectionModify.execute: Missing value bindings for the following placeholders: data

#@# CollectionModify: Set Execution
|Set Affected Rows: 1|
|age|
|alias|
|last_name|
|name|

#@# CollectionModify: Set Execution Binding Array
|Set Affected Rows: 1|
|age|
|alias|
|last_name|
|name|
|soccer|
|dance|
|reading|

#@ CollectionModify: Simple Unset Execution
|~last_name|
|Unset Affected Rows: 1|
|age|
|alias|
|name|

#@ CollectionModify: List Unset Execution
|~alias|
|~last_name|
|~age|
|Unset Affected Rows: 1|
|name|

#@ CollectionModify: Patch Execution
|Patch Affected Rows: 1|

|Brian's last_name: white|
|Brian's age: 14|
|Brian's alias: bw|
|Brian's first girlfriend: lois|
|Brian's second girlfriend: jane|

#@ CollectionModify: unset for merge
|Unset Affected Rows: 1|

#@ CollectionModify: Merge Execution
|Merge Affected Rows: 1|

|Brian's last_name: black|
|Brian's age: 15|
|Brian's alias: bri|
|Brian's first girlfriend: martha|
|Brian's second girlfriend: karen|

#@ CollectionModify: array_append Execution
|Array Append Affected Rows: 1|
|Brian's girlfriends: 3|
|Brian's last: cloe|

#@ CollectionModify: array_insert Execution
|Array Insert Affected Rows: 1|
|Brian's girlfriends: 4|
|Brian's second: samantha|

#@ CollectionModify: array_delete Execution
|Array Delete Affected Rows: 1|
|Brian's girlfriends: 3|
|Brian's third: cloe|

#@ CollectionModify: sorting and limit Execution
|Affected Rows: 2|

#@ CollectionModify: sorting and limit Execution - 1
|age|
|gender|
|name|
|sample|

#@ CollectionModify: sorting and limit Execution - 2
|age|
|gender|
|name|
|sample|

#@ CollectionModify: sorting and limit Execution - 3
|~sample|
|age|
|gender|
|name|

#@ CollectionModify: sorting and limit Execution - 4
|~sample|
|age|
|gender|
|name|

#@<OUT> CollectionModify: Patch initial documents
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female"
}


#@<OUT> CollectionModify: Patch adding fields to multiple documents (WL10856-FR1_1)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "address": "TBD",
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "address": "TBD",
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "address": "TBD",
    "hobbies": []
}

#@<OUT> CollectionModify: Patch updating field on multiple documents (WL10856-FR1_4)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "address": {
        "street": "TBD"
    },
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "address": {
        "street": "TBD"
    },
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "address": {
        "street": "TBD"
    },
    "hobbies": []
}

#@<OUT> CollectionModify: Patch updating field on multiple nested documents (WL10856-FR1_5)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "address": {
        "street": "Main"
    },
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "address": {
        "street": "Main"
    },
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "address": {
        "street": "Main"
    },
    "hobbies": []
}

#@<OUT> CollectionModify: Patch adding field on multiple nested documents (WL10856-FR1_2)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "address": {
        "number": 0,
        "street": "Main"
    },
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "address": {
        "number": 0,
        "street": "Main"
    },
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "address": {
        "number": 0,
        "street": "Main"
    },
    "hobbies": []
}

#@<OUT> CollectionModify: Patch removing field on multiple nested documents (WL10856-FR1_8)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "address": {
        "street": "Main"
    },
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "address": {
        "street": "Main"
    },
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "address": {
        "street": "Main"
    },
    "hobbies": []
}

#@<OUT> CollectionModify: Patch removing field on multiple documents (WL10856-FR1_7)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "hobbies": []
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "hobbies": []
}


#@<OUT> CollectionModify: Patch adding field with multiple calls to patch (WL10856-FR2.1_1)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "address": {},
    "hobbies": [],
    "last_name": "doe"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "address": {},
    "hobbies": [],
    "last_name": "doe"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "address": {},
    "hobbies": [],
    "last_name": "doe"
}

#@<OUT> CollectionModify: Patch adding field with multiple calls to patch on nested documents (WL10856-FR2.1_2)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "address": {
        "number": 0,
        "street": "main"
    },
    "hobbies": [],
    "last_name": "doe"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "address": {
        "number": 0,
        "street": "main"
    },
    "hobbies": [],
    "last_name": "doe"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "address": {
        "number": 0,
        "street": "main"
    },
    "hobbies": [],
    "last_name": "doe"
}

#@<OUT> CollectionModify: Patch updating fields with multiple calls to patch (WL10856-FR2.1_3, WL10856-FR2.1_4)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "address": {
        "number": 0,
        "street": "riverside"
    },
    "hobbies": [],
    "last_name": "houston"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "address": {
        "number": 0,
        "street": "riverside"
    },
    "hobbies": [],
    "last_name": "houston"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "address": {
        "number": 0,
        "street": "riverside"
    },
    "hobbies": [],
    "last_name": "houston"
}

#@<OUT> CollectionModify: Patch removing fields with multiple calls to patch (WL10856-FR2.1_5, WL10856-FR2.1_6)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "address": {
        "street": "riverside"
    },
    "last_name": "houston"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "address": {
        "street": "riverside"
    },
    "last_name": "houston"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "address": {
        "street": "riverside"
    },
    "last_name": "houston"
}

#@<OUT> CollectionModify: Patch adding field to multiple documents using expression (WL10856-ET_13)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "street": "riverside"
        },
        "age": 13,
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "street": "riverside"
        },
        "age": 15,
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "street": "riverside"
        },
        "age": 16,
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "donna"
    }
]

#@<OUT> CollectionModify: Patch adding field to multiple nested documents using expression (WL10856-ET_14)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "alma",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "ide"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "carol",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "ide"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "donna",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "ide"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}

#@<OUT> CollectionModify: Patch updating field to multiple documents using expression (WL10856-ET_15)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "Alma",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "ide"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "Carol",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "ide"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "Donna",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "ide"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}

#@<OUT> CollectionModify: Patch updating field to multiple nested documents using expression (WL10856-ET_16)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "name": "Alma",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "riv"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "name": "Carol",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "riv"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "name": "Donna",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "riv"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}

#@<OUT> CollectionModify: Patch including _id, ignores _id applies the rest (WL10856-ET_17)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "city": "Washington",
    "name": "Alma",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "riv"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "city": "Washington",
    "name": "Carol",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "riv"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "city": "Washington",
    "name": "Donna",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "riv"
    },
    "last_name": "houston",
    "last_update": "<<<last_update>>>"
}


#@ CollectionModify: Patch adding field with null value coming from an expression (WL10856-ET_19)
|Query OK, 0 items affected|

#@<OUT> CollectionModify: Patch updating field with null value coming from an expression (WL10856-ET_20)
{
    "_id": "5C514FF38144957BE71111C04E0D1249",
    "age": 13,
    "city": "Washington",
    "name": "Alma",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "riv"
    },
    "last_name": "houston"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1250",
    "age": 15,
    "city": "Washington",
    "name": "Carol",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "riv"
    },
    "last_name": "houston"
}
{
    "_id": "5C514FF38144957BE71111C04E0D1251",
    "age": 16,
    "city": "Washington",
    "name": "Donna",
    "gender": "female",
    "address": {
        "street": "riverside",
        "street_short": "riv"
    },
    "last_name": "houston"
}

#@ CollectionModify: Patch removing the _id field (WL10856-ET_25)
|Query OK, 0 items affected|

#@<OUT> BUG#33795149 - set() + backtick quotes
{
    "_id": "5C514FF38144957BE71111C04E0D1253",
    "name": "jack",
    "name surname": "jack doe"
}

#@<OUT> BUG#33795149 - set() + doc path
{
    "_id": "5C514FF38144957BE71111C04E0D1253",
    "name": "jack",
    "home address": "NY",
    "name surname": "jack doe"
}

#@<OUT> BUG#33795149 - unset() + backtick quotes
{
    "_id": "5C514FF38144957BE71111C04E0D1253",
    "name": "jack",
    "home address": "NY"
}

#@<OUT> BUG#33795149 - unset() + doc path
{
    "_id": "5C514FF38144957BE71111C04E0D1253",
    "name": "jack"
}
