#@ SqlResult member validation
|execution_time: OK|
|warning_count: OK|
|warnings: OK|
|get_execution_time: OK|
|get_warning_count: OK|
|get_warnings: OK|
|column_count: OK|
|column_names: OK|
|columns: OK|
|get_column_count: OK|
|get_column_names: OK|
|get_columns: OK|
|fetch_one: OK|
|fetch_all: OK|
|has_data: OK|
|next_data_set: OK|
|affected_row_count: OK|
|auto_increment_value: OK|
|get_affected_row_count: OK|
|get_auto_increment_value: OK|

#@ Result member validation
|execution_time: OK|
|warning_count: OK|
|warnings: OK|
|get_execution_time: OK|
|get_warning_count: OK|
|get_warnings: OK|
|affected_item_count: OK|
|auto_increment_value: OK|
|last_document_id: OK|
|last_document_id: OK|
|get_affected_item_count: OK|
|get_auto_increment_value: OK|
|get_last_document_id: OK|

#@ RowResult member validation
|execution_time: OK|
|warning_count: OK|
|warnings: OK|
|get_execution_time: OK|
|get_warning_count: OK|
|get_warnings: OK|
|column_count: OK|
|column_names: OK|
|columns: OK|
|get_column_count: OK|
|get_column_names: OK|
|get_columns: OK|
|fetch_one: OK|
|fetch_all: OK|

#@ DocResult member validation
|execution_time: OK|
|warning_count: OK|
|warnings: OK|
|get_execution_time: OK|
|get_warning_count: OK|
|get_warnings: OK|
|fetch_one: OK|
|fetch_all: OK|

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

#@ Resultset table
|7|

#@ Resultset row members
|Member Count: 6|
|length: OK|
|get_field: OK|
|get_length: OK|
|alias: OK|
|age: OK|

# Resultset row index access
|Name with index: jack|
|Age with index: 17|
|Length with index: 17|
|Gender with index: male|

# Resultset row index access
|Name with get_field: jack|
|Age with get_field: 17|
|Length with get_field: 17|
|Unable to get gender from alias: jack|

# Resultset property access
|Name with property: jack|
|Age with property: 17|
|Unable to get length with property: 4|