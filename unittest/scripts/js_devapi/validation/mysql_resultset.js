//@ Result member validation
|affectedItemsCount: OK|
|executionTime: OK|
|warningCount: OK|
|warnings: OK|
|warningsCount: OK|
|getAffectedItemsCount: OK|
|getExecutionTime: OK|
|getWarningCount: OK|
|getWarnings: OK|
|getWarningsCount: OK|
|columnCount: OK|
|columnNames: OK|
|columns: OK|
|info: OK|
|getColumnCount: OK|
|getColumnNames: OK|
|getColumns: OK|
|getInfo: OK|
|fetchOne: OK|
|fetchAll: OK|
|hasData: OK|
|nextDataSet: OK|
|nextResult: OK|
|affectedRowCount: OK|
|autoIncrementValue: OK|
|getAffectedRowCount: OK|
|getAutoIncrementValue: OK|

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

//@ Resultset row members
|Member Count: 6|
|length: OK|
|getField: OK|
|getLength: OK|
|alias: OK|
|age: OK|

// Resultset row index access
|Name with index: jack|
|Age with index: 17|
|Length with index: 17|
|Gender with index: male|

// Resultset row index access
|Name with getField: jack|
|Age with getField: 17|
|Length with getField: 17|
|Unable to get gender from alias: jack|

// Resultset property access
|Name with property: jack|
|Age with property: 17|
|Unable to get length with property: 4|
