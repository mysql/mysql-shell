#@ Result member validation
|affected_items_count: OK|
|execution_time: OK|
|warning_count: OK|
|warnings: OK|
|warnings_count: OK|
|get_affected_items_count: OK|
|get_execution_time: OK|
|get_warning_count: OK|
|get_warnings: OK|
|get_warnings_count: OK|
|column_count: OK|
|column_names: OK|
|columns: OK|
|info: OK|
|get_column_count: OK|
|get_column_names: OK|
|get_columns: OK|
|get_info: OK|
|fetch_one: OK|
|fetch_all: OK|
|has_data: OK|
|next_data_set: OK|
|next_result: OK|
|affected_row_count: OK|
|auto_increment_value: OK|
|get_affected_row_count: OK|
|get_auto_increment_value: OK|

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

#@<OUT> Resultset row as object
Name with property: jack
Age with property: 17
{"age": 17, "alias": "jack", "length": 17}

