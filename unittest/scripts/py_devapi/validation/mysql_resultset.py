
#@ Resultset has_data() False
|has_data(): False|

#@ Resultset has_data() True
|has_data(): True|

#@ Resultset get_columns()
|Field Number: 3|
|First Field: name|
|Second Field: age|
|Third Field: gender|

#@ Resultset columns
|Field Number: 3|
|First Field: name|
|Second Field: age|
|Third Field: gender|

#@ Resultset row index access
|Name with index: jack|
|Age with index: 17|
|Length with index: 17|
|Gender with index: male|

#@ Resultset row get_field access
|Name with get_field: jack|
|Age with get_field: 17|
|Length with get_field: 17|
|Unable to get gender from alias: jack|

#@ Resultset property access
|Name with property: jack|
|Age with property: 17|
|Unable to get length with property: 4|

#@<OUT> Resultset row as object
Name with property: jack
Age with property: 17
{"age": 17, "alias": "jack", "length": 17}

