//@ SqlResult member validation
|executionTime: OK|
|warningCount: OK|
|warnings: OK|
|getExecutionTime: OK|
|getWarningCount: OK|
|getWarnings: OK|
|columnCount: OK|
|columnNames: OK|
|columns: OK|
|getColumnCount: OK|
|getColumnNames: OK|
|getColumns: OK|
|fetchOne: OK|
|fetchAll: OK|
|hasData: OK|
|nextDataSet: OK|
|affectedRowCount: OK|
|autoIncrementValue: OK|
|getAffectedRowCount: OK|
|getAutoIncrementValue: OK|

//@ Result member validation
|executionTime: OK|
|warningCount: OK|
|warnings: OK|
|getExecutionTime: OK|
|getWarningCount: OK|
|getWarnings: OK|
|affectedItemCount: OK|
|autoIncrementValue: OK|
|lastDocumentId: OK|
|lastDocumentId: OK|
|getAffectedItemCount: OK|
|getAutoIncrementValue: OK|
|getLastDocumentId: OK|

//@ RowResult member validation
|executionTime: OK|
|warningCount: OK|
|warnings: OK|
|getExecutionTime: OK|
|getWarningCount: OK|
|getWarnings: OK|
|columnCount: OK|
|columnNames: OK|
|columns: OK|
|getColumnCount: OK|
|getColumnNames: OK|
|getColumns: OK|
|fetchOne: OK|
|fetchAll: OK|

//@ DocResult member validation
|executionTime: OK|
|warningCount: OK|
|warnings: OK|
|getExecutionTime: OK|
|getWarningCount: OK|
|getWarnings: OK|
|fetchOne: OK|
|fetchAll: OK|

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

//@ Resultset buffering on SQL
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

//@ Resultset buffering on CRUD
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

//@ Resultset table
|7|