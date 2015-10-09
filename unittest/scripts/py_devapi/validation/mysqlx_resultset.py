#@ Resultset hasData() False
|hasData(): False|

#@ Resultset hasData() True
|hasData(): True|

#@ Resultset getColumns()
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
