#@<OUT> Sanity test - UC show all schemas on reservedKeywords check
   ucfilterschema_test1.System - Table name
   ucfilterschema_test2.System - Table name
   ucfilterschema_test3.System - Table name
   ucfilterschema_test1.NTile.JSON_TABLE - Column name
   ucfilterschema_test1.NTile.cube - Column name
   ucfilterschema_test1.System.JSON_TABLE - Column name
   ucfilterschema_test1.System.cube - Column name
   ucfilterschema_test2.NTile.JSON_TABLE - Column name
   ucfilterschema_test2.NTile.cube - Column name
   ucfilterschema_test2.System.JSON_TABLE - Column name
   ucfilterschema_test2.System.cube - Column name
   ucfilterschema_test3.NTile.JSON_TABLE - Column name
   ucfilterschema_test3.NTile.cube - Column name
   ucfilterschema_test3.System.JSON_TABLE - Column name
   ucfilterschema_test3.System.cube - Column name
   ucfilterschema_test1.System.first_value - Trigger name
   ucfilterschema_test1.System.last_value - Trigger name
   ucfilterschema_test2.System.first_value - Trigger name
   ucfilterschema_test2.System.last_value - Trigger name
   ucfilterschema_test3.System.first_value - Trigger name
   ucfilterschema_test3.System.last_value - Trigger name
   ucfilterschema_test1.LATERAL - View name
   ucfilterschema_test1.NTile - View name
   ucfilterschema_test2.LATERAL - View name
   ucfilterschema_test2.NTile - View name
   ucfilterschema_test3.LATERAL - View name
   ucfilterschema_test3.NTile - View name
   ucfilterschema_test1.Array - Routine name
   ucfilterschema_test1.full - Routine name
   ucfilterschema_test1.inteRsect - Routine name
   ucfilterschema_test1.rows - Routine name
   ucfilterschema_test2.Array - Routine name
   ucfilterschema_test2.full - Routine name
   ucfilterschema_test2.inteRsect - Routine name
   ucfilterschema_test2.rows - Routine name
   ucfilterschema_test3.Array - Routine name
   ucfilterschema_test3.full - Routine name
   ucfilterschema_test3.inteRsect - Routine name
   ucfilterschema_test3.rows - Routine name


#@<OUT> includeSchemas - filter in only test_schema1
   ucfilterschema_test1.System - Table name
   ucfilterschema_test1.NTile.JSON_TABLE - Column name
   ucfilterschema_test1.NTile.cube - Column name
   ucfilterschema_test1.System.JSON_TABLE - Column name
   ucfilterschema_test1.System.cube - Column name
   ucfilterschema_test1.System.first_value - Trigger name
   ucfilterschema_test1.System.last_value - Trigger name
   ucfilterschema_test1.LATERAL - View name
   ucfilterschema_test1.NTile - View name
   ucfilterschema_test1.Array - Routine name
   ucfilterschema_test1.full - Routine name
   ucfilterschema_test1.inteRsect - Routine name
   ucfilterschema_test1.rows - Routine name


#@<OUT> includeSchemas - filter in only test_schema1, test_schema3 with tick quotes
   ucfilterschema_test1.System - Table name
   ucfilterschema_test3.System - Table name
   ucfilterschema_test1.NTile.JSON_TABLE - Column name
   ucfilterschema_test1.NTile.cube - Column name
   ucfilterschema_test1.System.JSON_TABLE - Column name
   ucfilterschema_test1.System.cube - Column name
   ucfilterschema_test3.NTile.JSON_TABLE - Column name
   ucfilterschema_test3.NTile.cube - Column name
   ucfilterschema_test3.System.JSON_TABLE - Column name
   ucfilterschema_test3.System.cube - Column name
   ucfilterschema_test1.System.first_value - Trigger name
   ucfilterschema_test1.System.last_value - Trigger name
   ucfilterschema_test3.System.first_value - Trigger name
   ucfilterschema_test3.System.last_value - Trigger name
   ucfilterschema_test1.LATERAL - View name
   ucfilterschema_test1.NTile - View name
   ucfilterschema_test3.LATERAL - View name
   ucfilterschema_test3.NTile - View name
   ucfilterschema_test1.Array - Routine name
   ucfilterschema_test1.full - Routine name
   ucfilterschema_test1.inteRsect - Routine name
   ucfilterschema_test1.rows - Routine name
   ucfilterschema_test3.Array - Routine name
   ucfilterschema_test3.full - Routine name
   ucfilterschema_test3.inteRsect - Routine name
   ucfilterschema_test3.rows - Routine name


#@<OUT> includeSchemas - filters in test_schema1, test_schema2, ignores invalid one
   ucfilterschema_test1.System - Table name
   ucfilterschema_test2.System - Table name
   ucfilterschema_test1.NTile.JSON_TABLE - Column name
   ucfilterschema_test1.NTile.cube - Column name
   ucfilterschema_test1.System.JSON_TABLE - Column name
   ucfilterschema_test1.System.cube - Column name
   ucfilterschema_test2.NTile.JSON_TABLE - Column name
   ucfilterschema_test2.NTile.cube - Column name
   ucfilterschema_test2.System.JSON_TABLE - Column name
   ucfilterschema_test2.System.cube - Column name
   ucfilterschema_test1.System.first_value - Trigger name
   ucfilterschema_test1.System.last_value - Trigger name
   ucfilterschema_test2.System.first_value - Trigger name
   ucfilterschema_test2.System.last_value - Trigger name
   ucfilterschema_test1.LATERAL - View name
   ucfilterschema_test1.NTile - View name
   ucfilterschema_test2.LATERAL - View name
   ucfilterschema_test2.NTile - View name
   ucfilterschema_test1.Array - Routine name
   ucfilterschema_test1.full - Routine name
   ucfilterschema_test1.inteRsect - Routine name
   ucfilterschema_test1.rows - Routine name
   ucfilterschema_test2.Array - Routine name
   ucfilterschema_test2.full - Routine name
   ucfilterschema_test2.inteRsect - Routine name
   ucfilterschema_test2.rows - Routine name


#@<OUT> excludeSchemas - filter out only test_schema1
   ucfilterschema_test2.System - Table name
   ucfilterschema_test3.System - Table name
   ucfilterschema_test2.NTile.JSON_TABLE - Column name
   ucfilterschema_test2.NTile.cube - Column name
   ucfilterschema_test2.System.JSON_TABLE - Column name
   ucfilterschema_test2.System.cube - Column name
   ucfilterschema_test3.NTile.JSON_TABLE - Column name
   ucfilterschema_test3.NTile.cube - Column name
   ucfilterschema_test3.System.JSON_TABLE - Column name
   ucfilterschema_test3.System.cube - Column name
   ucfilterschema_test2.System.first_value - Trigger name
   ucfilterschema_test2.System.last_value - Trigger name
   ucfilterschema_test3.System.first_value - Trigger name
   ucfilterschema_test3.System.last_value - Trigger name
   ucfilterschema_test2.LATERAL - View name
   ucfilterschema_test2.NTile - View name
   ucfilterschema_test3.LATERAL - View name
   ucfilterschema_test3.NTile - View name
   ucfilterschema_test2.Array - Routine name
   ucfilterschema_test2.full - Routine name
   ucfilterschema_test2.inteRsect - Routine name
   ucfilterschema_test2.rows - Routine name
   ucfilterschema_test3.Array - Routine name
   ucfilterschema_test3.full - Routine name
   ucfilterschema_test3.inteRsect - Routine name
   ucfilterschema_test3.rows - Routine name


#@<OUT> excludeSchemas - filter out only test_schema1, test_schema3 with tick quotes
   ucfilterschema_test2.System - Table name
   ucfilterschema_test2.NTile.JSON_TABLE - Column name
   ucfilterschema_test2.NTile.cube - Column name
   ucfilterschema_test2.System.JSON_TABLE - Column name
   ucfilterschema_test2.System.cube - Column name
   ucfilterschema_test2.System.first_value - Trigger name
   ucfilterschema_test2.System.last_value - Trigger name
   ucfilterschema_test2.LATERAL - View name
   ucfilterschema_test2.NTile - View name
   ucfilterschema_test2.Array - Routine name
   ucfilterschema_test2.full - Routine name
   ucfilterschema_test2.inteRsect - Routine name
   ucfilterschema_test2.rows - Routine name

#@<OUT> excludeSchemas - filters out test_schema1, test_schema2, ignores invalid one
   ucfilterschema_test3.System - Table name
   ucfilterschema_test3.NTile.JSON_TABLE - Column name
   ucfilterschema_test3.NTile.cube - Column name
   ucfilterschema_test3.System.JSON_TABLE - Column name
   ucfilterschema_test3.System.cube - Column name
   ucfilterschema_test3.System.first_value - Trigger name
   ucfilterschema_test3.System.last_value - Trigger name
   ucfilterschema_test3.LATERAL - View name
   ucfilterschema_test3.NTile - View name
   ucfilterschema_test3.Array - Routine name
   ucfilterschema_test3.full - Routine name
   ucfilterschema_test3.inteRsect - Routine name
   ucfilterschema_test3.rows - Routine name

#@<OUT> includeSchemas - test_schema1, excludeSchemas - test_schema3, includeSchemas should take priority
   ucfilterschema_test1.System - Table name
   ucfilterschema_test1.NTile.JSON_TABLE - Column name
   ucfilterschema_test1.NTile.cube - Column name
   ucfilterschema_test1.System.JSON_TABLE - Column name
   ucfilterschema_test1.System.cube - Column name
   ucfilterschema_test1.System.first_value - Trigger name
   ucfilterschema_test1.System.last_value - Trigger name
   ucfilterschema_test1.LATERAL - View name
   ucfilterschema_test1.NTile - View name
   ucfilterschema_test1.Array - Routine name
   ucfilterschema_test1.full - Routine name
   ucfilterschema_test1.inteRsect - Routine name
   ucfilterschema_test1.rows - Routine name


#@<OUT> includeTables - filters in System and LATERAL from test_schema1
   ucfilterschema_test1.System - Table name
   ucfilterschema_test1.System.JSON_TABLE - Column name
   ucfilterschema_test1.System.cube - Column name
   ucfilterschema_test1.LATERAL - View name


#@<OUT> excludeTables - filters out System and LATERAL from test_schema1
   ucfilterschema_test2.System - Table name
   ucfilterschema_test3.System - Table name
   ucfilterschema_test1.NTile.JSON_TABLE - Column name
   ucfilterschema_test1.NTile.cube - Column name
   ucfilterschema_test2.NTile.JSON_TABLE - Column name
   ucfilterschema_test2.NTile.cube - Column name
   ucfilterschema_test2.System.JSON_TABLE - Column name
   ucfilterschema_test2.System.cube - Column name
   ucfilterschema_test3.NTile.JSON_TABLE - Column name
   ucfilterschema_test3.NTile.cube - Column name
   ucfilterschema_test3.System.JSON_TABLE - Column name
   ucfilterschema_test3.System.cube - Column name
   ucfilterschema_test1.NTile - View name
   ucfilterschema_test2.LATERAL - View name
   ucfilterschema_test2.NTile - View name
   ucfilterschema_test3.LATERAL - View name
   ucfilterschema_test3.NTile - View name


#@<OUT> includeTriggers - no filter, so list all triggers in selected table
   ucfilterschema_test1.System - Table name
   ucfilterschema_test1.System.JSON_TABLE - Column name
   ucfilterschema_test1.System.cube - Column name
   ucfilterschema_test1.System.first_value - Trigger name
   ucfilterschema_test1.System.last_value - Trigger name


#@<OUT> includeTriggers - filters in first_value trigger from test_schema1.System
   ucfilterschema_test1.System - Table name
   ucfilterschema_test1.System.JSON_TABLE - Column name
   ucfilterschema_test1.System.cube - Column name
   ucfilterschema_test1.System.first_value - Trigger name


#@<OUT> excludeTriggers - filters out first_value trigger from test_schema1.System
   ucfilterschema_test1.System - Table name
   ucfilterschema_test1.System.JSON_TABLE - Column name
   ucfilterschema_test1.System.cube - Column name
   ucfilterschema_test1.System.last_value - Trigger name


#@<OUT> routines with no filters
   ucfilterschema_test1.Array - Routine name
   ucfilterschema_test1.full - Routine name
   ucfilterschema_test1.inteRsect - Routine name
   ucfilterschema_test1.rows - Routine name
   ucfilterschema_test2.Array - Routine name
   ucfilterschema_test2.full - Routine name
   ucfilterschema_test2.inteRsect - Routine name
   ucfilterschema_test2.rows - Routine name
   ucfilterschema_test3.Array - Routine name
   ucfilterschema_test3.full - Routine name
   ucfilterschema_test3.inteRsect - Routine name
   ucfilterschema_test3.rows - Routine name


#@<OUT> includeRoutines - includes Array and rows routines in test_schema2
   ucfilterschema_test2.Array - Routine name
   ucfilterschema_test2.rows - Routine name


#@<OUT> excludeRoutines - includes Array and rows routines in test_schema2
   ucfilterschema_test1.Array - Routine name
   ucfilterschema_test1.full - Routine name
   ucfilterschema_test1.inteRsect - Routine name
   ucfilterschema_test1.rows - Routine name
   ucfilterschema_test2.full - Routine name
   ucfilterschema_test2.inteRsect - Routine name
   ucfilterschema_test3.Array - Routine name
   ucfilterschema_test3.full - Routine name
   ucfilterschema_test3.inteRsect - Routine name
   ucfilterschema_test3.rows - Routine name


