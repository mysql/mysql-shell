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

#@ Resultset buffering on SQL
|Result 1 Field 1: name|
|Result 1 Field 2: age|

|Result 2 Field 1: name|
|Result 2 Field 2: gender|

|Result 1 Record 1: adam|
|Result 2 Record 1: alma|

|Result 1 Record 2: angel|
|Result 2 Record 2: angel|

|Result 1 Record 3: brian|
|Result 2 Record 3: brian|

|Result 1 Record 4: jack|
|Result 2 Record 4: carol|

#@<OUT> Row as object on SQL
{"age": 15, "name": "adam"}
{"gender": "female", "name": "alma"}

#@ Resultset buffering on CRUD
|Result 1 Field 1: name|
|Result 1 Field 2: age|

|Result 2 Field 1: name|
|Result 2 Field 2: gender|

|Result 1 Record 1: adam|
|Result 2 Record 1: alma|

|Result 1 Record 2: angel|
|Result 2 Record 2: angel|

|Result 1 Record 3: brian|
|Result 2 Record 3: brian|

|Result 1 Record 4: jack|
|Result 2 Record 4: carol|

#@<OUT> Row as object on CRUD
{"age": 15, "name": "adam"}
{"gender": "female", "name": "alma"}

#@ Resultset table
|7|

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
