
//@ Resultset hasData false
|hasData: false|

//@ Resultset hasData true
|hasData: true|

//@ Resultset getColumns()
|Field Number: 3|
|First Field: name|
|Second Field: age|
|Third Field: gender|

//@ Resultset columns
|Field Number: 3|
|First Field: name|
|Second Field: age|
|Third Field: gender|

//@ Resultset row index access
|Name with index: jack|
|Age with index: 17|
|Length with index: 17|
|Gender with index: male|

//@ Resultset row getField access
|Name with getField: jack|
|Age with getField: 17|
|Length with getField: 17|
|Unable to get gender from alias: jack|

//@ Resultset property access
|Name with property: jack|
|Age with property: 17|
|Unable to get length with property: 4|

//@<OUT> Resultset row as objects
Alias with property: jack
Age as item: 17

{
    "age": 17, 
    "alias": "jack", 
    "length": 17
}

