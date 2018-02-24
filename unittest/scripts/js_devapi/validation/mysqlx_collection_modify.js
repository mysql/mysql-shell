//@ CollectionModify: valid operations after modify and set
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after modify and unset empty
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after modify and unset list
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after modify and unset multiple params
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after modify and merge
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after modify and patch
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after modify and arrayInsert
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after modify and arrayAppend
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after modify and arrayDelete
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after sort
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after limit
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after bind
|All expected functions are available|
|No additional functions are available|

//@ CollectionModify: valid operations after execute
|All expected functions are available|
|No additional functions are available|

//@ Reusing CRUD with binding
|Updated Angel: 1|
|Updated Carol: 1|

//@# CollectionModify: Error conditions on modify
||Invalid number of arguments in CollectionModify.modify, expected 1 but got 0
||CollectionModify.modify: Requires a search condition.
||CollectionModify.modify: Argument #1 is expected to be a string
||CollectionModify.modify: Unterminated quoted string starting at position 8

//@# CollectionModify: Error conditions on set
||Invalid number of arguments in CollectionModify.set, expected 2 but got 0
||CollectionModify.set: Argument #1 is expected to be a string

//@# CollectionModify: Error conditions on unset
||Invalid number of arguments in CollectionModify.unset, expected at least 1 but got 0
||CollectionModify.unset: Argument #1 is expected to be either string or list of strings
||CollectionModify.unset: Argument #1 is expected to be a string
||CollectionModify.unset: Element #2 is expected to be a string
||CollectionModify.unset: Invalid document path

//@# CollectionModify: Error conditions on merge
||Invalid number of arguments in CollectionModify.merge, expected 1 but got 0
||CollectionModify.merge: Argument expected to be a JSON object

//@# CollectionModify: Error conditions on patch
||Invalid number of arguments in CollectionModify.patch, expected 1 but got 0
||CollectionModify.patch: Argument expected to be a JSON object

//@# CollectionModify: Error conditions on arrayInsert
||Invalid number of arguments in CollectionModify.arrayInsert, expected 2 but got 0
||CollectionModify.arrayInsert: Argument #1 is expected to be a string
||CollectionModify.arrayInsert: Invalid document path
||CollectionModify.arrayInsert: An array document path must be specified

//@# CollectionModify: Error conditions on arrayAppend
||Invalid number of arguments in CollectionModify.arrayAppend, expected 2 but got 0
||CollectionModify.arrayAppend: Argument #1 is expected to be a string
||CollectionModify.arrayAppend: Invalid document path
||CollectionModify.arrayAppend: Unsupported value received: <Session:

//@# CollectionModify: Error conditions on arrayDelete
||Invalid number of arguments in CollectionModify.arrayDelete, expected 1 but got
||CollectionModify.arrayDelete: Argument #1 is expected to be a string
||CollectionModify.arrayDelete: Invalid document path
||CollectionModify.arrayDelete: An array document path must be specified

//@# CollectionModify: Error conditions on sort
||Invalid number of arguments in CollectionModify.sort, expected at least 1 but got 0
||CollectionModify.sort: Argument #1 is expected to be a string or an array of strings
||CollectionModify.sort: Sort criteria can not be empty
||CollectionModify.sort: Element #2 is expected to be a string
||CollectionModify.sort: Argument #2 is expected to be a string

//@# CollectionModify: Error conditions on limit
||Invalid number of arguments in CollectionModify.limit, expected 1 but got 0
||CollectionModify.limit: Argument #1 is expected to be an unsigned int

//@# CollectionModify: Error conditions on bind
||Invalid number of arguments in CollectionModify.bind, expected 2 but got 0
||CollectionModify.bind: Argument #1 is expected to be a string
||CollectionModify.bind: Unable to bind value for unexisting placeholder: another

//@# CollectionModify: Error conditions on execute
||CollectionModify.execute: Missing value bindings for the following placeholders: data, years
||CollectionModify.execute: Missing value bindings for the following placeholders: data

//@# CollectionModify: Set Execution
|Set Affected Rows: 1|

|age|
|alias|
|last_name|
|name|

//@# CollectionModify: Set Execution Binding Array
|Set Affected Rows: 1|
|age|
|alias|
|hobbies|
|last_name|
|name|
|soccer|
|dance|
|read|

//@ CollectionModify: Simple Unset Execution
|~last_name|
|Unset Affected Rows: 1|
|age|
|alias|
|name|

//@ CollectionModify: List Unset Execution
|~alias|
|~last_name|
|~age|
|Unset Affected Rows: 1|
|name|


//@ CollectionModify: Merge Execution
|Merge Affected Rows: 1|

|Brian's last_name: black|
|Brian's age: 15|
|Brian's alias: bri|
|Brian's first girlfriend: martha|
|Brian's second girlfriend: karen|

//@ CollectionModify: arrayAppend Execution
|Array Append Affected Rows: 1|
|Brian's girlfriends: 3|
|Brian's last: cloe|

//@ CollectionModify: arrayInsert Execution
|Array Insert Affected Rows: 1|
|Brian's girlfriends: 4|
|Brian's second: samantha|

//@ CollectionModify: arrayDelete Execution
|Array Delete Affected Rows: 1|
|Brian's girlfriends: 3|
|Brian's third: cloe|

//@ CollectionModify: sorting and limit Execution
|Affected Rows: 2|

//@ CollectionModify: sorting and limit Execution - 1
|age|
|gender|
|name|
|sample|

//@ CollectionModify: sorting and limit Execution - 2
|age|
|gender|
|name|
|sample|

//@ CollectionModify: sorting and limit Execution - 3
|~sample|
|age|
|gender|
|name|

//@ CollectionModify: sorting and limit Execution - 4
|~sample|
|age|
|gender|
|name|

//@<OUT> CollectionModify: Patch initial documents
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "age": 13,
        "gender": "female",
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "age": 15,
        "gender": "female",
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "age": 16,
        "gender": "female",
        "name": "donna"
    }
]


//@<OUT> CollectionModify: Patch adding fields to multiple documents (WL10856-FR1_1)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": "TBD",
        "age": 13,
        "gender": "female",
        "hobbies": [],
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": "TBD",
        "age": 15,
        "gender": "female",
        "hobbies": [],
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": "TBD",
        "age": 16,
        "gender": "female",
        "hobbies": [],
        "name": "donna"
    }
]

//@<OUT> CollectionModify: Patch updating field on multiple documents (WL10856-FR1_4)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "street": "TBD"
        },
        "age": 13,
        "gender": "female",
        "hobbies": [],
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "street": "TBD"
        },
        "age": 15,
        "gender": "female",
        "hobbies": [],
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "street": "TBD"
        },
        "age": 16,
        "gender": "female",
        "hobbies": [],
        "name": "donna"
    }
]


//@<OUT> CollectionModify: Patch updating field on multiple nested documents (WL10856-FR1_5)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "street": "Main"
        },
        "age": 13,
        "gender": "female",
        "hobbies": [],
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "street": "Main"
        },
        "age": 15,
        "gender": "female",
        "hobbies": [],
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "street": "Main"
        },
        "age": 16,
        "gender": "female",
        "hobbies": [],
        "name": "donna"
    }
]

//@<OUT> CollectionModify: Patch adding field on multiple nested documents (WL10856-FR1_2)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "number": 0,
            "street": "Main"
        },
        "age": 13,
        "gender": "female",
        "hobbies": [],
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "number": 0,
            "street": "Main"
        },
        "age": 15,
        "gender": "female",
        "hobbies": [],
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "number": 0,
            "street": "Main"
        },
        "age": 16,
        "gender": "female",
        "hobbies": [],
        "name": "donna"
    }
]


//@<OUT> CollectionModify: Patch removing field on multiple nested documents (WL10856-FR1_8)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "street": "Main"
        },
        "age": 13,
        "gender": "female",
        "hobbies": [],
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "street": "Main"
        },
        "age": 15,
        "gender": "female",
        "hobbies": [],
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "street": "Main"
        },
        "age": 16,
        "gender": "female",
        "hobbies": [],
        "name": "donna"
    }
]

//@<OUT> CollectionModify: Patch removing field on multiple documents (WL10856-FR1_7)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "age": 13,
        "gender": "female",
        "hobbies": [],
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "age": 15,
        "gender": "female",
        "hobbies": [],
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "age": 16,
        "gender": "female",
        "hobbies": [],
        "name": "donna"
    }
]

//@<OUT> CollectionModify: Patch adding field with multiple calls to patch (WL10856-FR2.1_1)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {},
        "age": 13,
        "gender": "female",
        "hobbies": [],
        "last_name": "doe",
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {},
        "age": 15,
        "gender": "female",
        "hobbies": [],
        "last_name": "doe",
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {},
        "age": 16,
        "gender": "female",
        "hobbies": [],
        "last_name": "doe",
        "name": "donna"
    }
]

//@<OUT> CollectionModify: Patch adding field with multiple calls to patch on nested documents (WL10856-FR2.1_2)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "number": 0,
            "street": "main"
        },
        "age": 13,
        "gender": "female",
        "hobbies": [],
        "last_name": "doe",
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "number": 0,
            "street": "main"
        },
        "age": 15,
        "gender": "female",
        "hobbies": [],
        "last_name": "doe",
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "number": 0,
            "street": "main"
        },
        "age": 16,
        "gender": "female",
        "hobbies": [],
        "last_name": "doe",
        "name": "donna"
    }
]

//@<OUT> CollectionModify: Patch updating fields with multiple calls to patch (WL10856-FR2.1_3, WL10856-FR2.1_4)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "number": 0,
            "street": "riverside"
        },
        "age": 13,
        "gender": "female",
        "hobbies": [],
        "last_name": "houston",
        "name": "alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "number": 0,
            "street": "riverside"
        },
        "age": 15,
        "gender": "female",
        "hobbies": [],
        "last_name": "houston",
        "name": "carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "number": 0,
            "street": "riverside"
        },
        "age": 16,
        "gender": "female",
        "hobbies": [],
        "last_name": "houston",
        "name": "donna"
    }
]

//@<OUT> CollectionModify: Patch removing fields with multiple calls to patch (WL10856-FR2.1_5, WL10856-FR2.1_6)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "street": "riverside"
        },
        "age": 13,
        "gender": "female",
        "last_name": "houston",
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
        "name": "donna"
    }
]

//@<OUT> CollectionModify: Patch adding field to multiple documents using expression (WL10856-ET_13)
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

//@<OUT> CollectionModify: Patch adding field to multiple nested documents using expression (WL10856-ET_14)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "street": "riverside",
            "street_short": "ide"
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
            "street": "riverside",
            "street_short": "ide"
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
            "street": "riverside",
            "street_short": "ide"
        },
        "age": 16,
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "donna"
    }
]

//@<OUT> CollectionModify: Patch updating field to multiple documents using expression (WL10856-ET_15)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "street": "riverside",
            "street_short": "ide"
        },
        "age": 13,
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "Alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "street": "riverside",
            "street_short": "ide"
        },
        "age": 15,
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "Carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "street": "riverside",
            "street_short": "ide"
        },
        "age": 16,
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "Donna"
    }
]

//@<OUT> CollectionModify: Patch updating field to multiple nested documents using expression (WL10856-ET_16)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "street": "riverside",
            "street_short": "riv"
        },
        "age": 13,
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "Alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "street": "riverside",
            "street_short": "riv"
        },
        "age": 15,
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "Carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "street": "riverside",
            "street_short": "riv"
        },
        "age": 16,
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "Donna"
    }
]

//@<OUT> CollectionModify: Patch including _id, ignores _id applies the rest (WL10856-ET_17)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "street": "riverside",
            "street_short": "riv"
        },
        "age": 13,
        "city": "Washington",
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "Alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "street": "riverside",
            "street_short": "riv"
        },
        "age": 15,
        "city": "Washington",
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "Carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "street": "riverside",
            "street_short": "riv"
        },
        "age": 16,
        "city": "Washington",
        "gender": "female",
        "last_name": "houston",
        "last_update": "<<<last_update>>>",
        "name": "Donna"
    }
]

//@ CollectionModify: Patch adding field with null value coming from an expression (WL10856-ET_19)
|Query OK, 0 items affected|

//@<OUT> CollectionModify: Patch updating field with null value coming from an expression (WL10856-ET_20)
[
    {
        "_id": "5C514FF38144957BE71111C04E0D1249",
        "address": {
            "street": "riverside",
            "street_short": "riv"
        },
        "age": 13,
        "city": "Washington",
        "gender": "female",
        "last_name": "houston",
        "name": "Alma"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1250",
        "address": {
            "street": "riverside",
            "street_short": "riv"
        },
        "age": 15,
        "city": "Washington",
        "gender": "female",
        "last_name": "houston",
        "name": "Carol"
    },
    {
        "_id": "5C514FF38144957BE71111C04E0D1251",
        "address": {
            "street": "riverside",
            "street_short": "riv"
        },
        "age": 16,
        "city": "Washington",
        "gender": "female",
        "last_name": "houston",
        "name": "Donna"
    }
]

//@ CollectionModify: Patch removing the _id field (WL10856-ET_25)
|Query OK, 0 items affected|
