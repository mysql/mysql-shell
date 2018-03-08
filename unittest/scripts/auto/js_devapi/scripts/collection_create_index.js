//@ {VER(>=8.0.5)}
shell.connect(__uripwd);
session.dropSchema('my_schema');
var schema = session.createSchema('my_schema');
var coll = schema.createCollection('my_coll');
shell.options.outputFormat = 'vertical';

// This function will retrieve the key columns
// for my_coll and create global variables with them
// So they can be evaluated as dynamic variables on the expected
// results
function createIndexColumnVars() {
  var table = schema.getCollectionAsTable('my_coll');
  var res = table.select().execute();
  var columns = res.getColumns();
  var col_count = 0;
  for(index in columns) {
    var c = columns[index];
    if (c.columnName != 'doc' && c.columnName != '_id') {
      col_count += 1;
      var code = "idx_col_" + col_count + " = '" + c.columnName + "'";
      eval(code);
    }
  }
}

//@ Create an index on a single field. 1 (WL10858-FR1_1)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TEXT(10)'}]});
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index on a single field. 2 (WL10858-FR1_1)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ Create an index on a single field with all the possibles options. 1 (WL10858-FR1_2)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TEXT(10)', required: true}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index on a single field with all the possibles options. 2 (WL10858-FR1_2)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ Create an index on multiple fields 1 (WL10858-FR1_3)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TEXT(10)'}, {field: '$.myField2', type: 'TEXT(10)'}, {field: '$.myField3', type: 'INT'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index on multiple fields 2 (WL10858-FR1_3)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');


//@ Create an index on multiple fields with all the possibles options. 1 (WL10858-FR1_4)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TEXT(10)'}, {field: '$.myField2', type: 'TEXT(10)', required: true}, {field: '$.myField3', type: 'INT', required: false}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index on multiple fields with all the possibles options. 2 (WL10858-FR1_4)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ Create an index using a geojson datatype field. 1 (WL10858-FR1_5)
coll.createIndex('myIndex', {fields: [{field: '$.myGeoJsonField', type: 'GEOJSON', required: true}], type:'SPATIAL'})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index using a geojson datatype field. 2 (WL10858-FR1_5)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ Create an index using a geojson datatype field without specifying the required flag it should be set to true by default. 1 (WL10858-FR1_6)
coll.createIndex('myIndex', {fields: [{field: '$.myGeoJsonField', type: 'GEOJSON'}], type:'SPATIAL'})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index using a geojson datatype field without specifying the required flag it should be set to true by default. 2 (WL10858-FR1_6)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ Create an index using a geojson datatype field with all the possibles options. 1 (WL10858-FR1_7)
coll.createIndex('myIndex', {fields: [{field: '$.myGeoJsonField', type: 'GEOJSON', required: true, options: 2, srid: 4400}], type:'SPATIAL'})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index using a geojson datatype field with all the possibles options. 2 (WL10858-FR1_7)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ Create an index using a datetime field. 1 (WL10858-FR1_8)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'DATETIME'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index using a datetime field. 2 (WL10858-FR1_8)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ Create an index using a timestamp field. 1 (WL10858-FR1_9)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TIMESTAMP'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index using a timestamp field. 2 (WL10858-FR1_9)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ Create an index using a time field. 1 (WL10858-FR1_10)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TIME'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index using a time field. 2 (WL10858-FR1_10)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ Create an index using a date field. 1 (WL10858-FR1_11)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'DATE'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index using a date field. 2 (WL10858-FR1_11)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ Create an index using a numeric field. 1 (WL10858-FR1_12)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'NUMERIC UNSIGNED'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Create an index using a numeric field. 2 (WL10858-FR1_12)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@ FR1_13	Create an index using a decimal field. 1 (WL10858-FR1_13)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'DECIMAL'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ FR1_13	Create an index using a decimal field. 2 (WL10858-FR1_13)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@	Create an index using a double field. 1 (WL10858-FR1_14)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'DOUBLE'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@	Create an index using a double field. 2 (WL10858-FR1_14)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@	Create an index using a float field. 1 (WL10858-FR1_15)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'FLOAT UNSIGNED'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@	Create an index using a float field. 2 (WL10858-FR1_15)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@	Create an index using a real field. 1 (WL10858-FR1_16)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'REAL UNSIGNED'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@	Create an index using a real field. 2 (WL10858-FR1_16)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@	Create an index using a bigint field. 1 (WL10858-FR1_17)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'BIGINT'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@	Create an index using a bigint field. 2 (WL10858-FR1_17)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@	Create an index using a integer field. 1 (WL10858-FR1_18)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'INTEGER UNSIGNED'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@	Create an index using a integer field. 2 (WL10858-FR1_18)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@	Create an index using a mediumint field. 1 (WL10858-FR1_19)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'MEDIUMINT UNSIGNED'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@	Create an index using a mediumint field. 2 (WL10858-FR1_19)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@	Create an index using a smallint field. 1 (WL10858-FR1_20)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'SMALLINT'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@	Create an index using a smallint field. 2 (WL10858-FR1_20)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');

//@	Create an index using a tinyint field. 1 (WL10858-FR1_21)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TINYINT UNSIGNED'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@	Create an index using a tinyint field. 2 (WL10858-FR1_21)
session.sql('show create table my_schema.my_coll').execute();
coll.dropIndex('myIndex');


//@ Verify that the dropIndex function removes the index entry from the table schema of a collection. 1 (WL10858-FR4_1)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TEXT(10)'}]})
createIndexColumnVars();
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Verify that the dropIndex function removes the index entry from the table schema of a collection. 2 (WL10858-FR4_1)
session.sql('show create table my_schema.my_coll').execute();

//@ Verify that the dropIndex function removes the index entry from the table schema of a collection. 3 (WL10858-FR4_1)
coll.dropIndex('myIndex');
session.sql('show index from my_schema.my_coll where key_Name = "myIndex"').execute();
//@ Verify that the dropIndex function removes the index entry from the table schema of a collection. 4 (WL10858-FR4_1)
session.sql('show create table my_schema.my_coll').execute();

//@ Verify that the dropIndex silently succeeds if the index does not exist. (WL10858-FR4_2)
coll.dropIndex('DoesNotExists')

//@ Create an index with the name of an index that already exists. (WL10858-FR5_2)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TEXT(10)'}]})
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TEXT(10)'}]})
coll.dropIndex('myIndex');

//@ Create an index with a not valid JSON document definition. (WL10858-FR5_3)
coll.createIndex('myIndex', {fields: [{field = '$.myField', type = 'TEXT(10)'}]})
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TEXT(10)']})
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TEXT(10)'}})

//@ Create an index where its definition is a JSON document but its structure is not valid. (WL10858-FR5_4)
coll.createIndex('myIndex', {fields: [{myfield: '$.myField', type: 'TEXT(10)'}]})

//@ Create an index with the index type not "INDEX" or "SPATIAL" (case insensitive). (WL10858-FR5_5)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'TEXT(10)'}], type:'IDX'})
coll.createIndex('myIndex', {fields: [{field: '$.myGeoJsonField', type: 'GEOJSON', required: true}], type:'SPATIAL_'})
coll.createIndex('myIndex', {fields: [{field: '$.myGeoJsonField', type: 'GEOJSON', required: true}], type:'INVALID'})

//@ Create a 'SPATIAL' index with "required" flag set to false. (WL10858-FR5_6)
coll.createIndex('myField', {fields: [{field: '$.myField', type: 'GEOJSON', required: false, options: 2, srid: 4326}], type:'SPATIAL'})

//@ Create an index with an invalid "type" specified (type names are case insensitive). (WL10858-FR5_7)
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: '_Text(10)'}]})
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'Invalid'}]})
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'Timestamps'}]})
coll.createIndex('myIndex', {fields: [{field: '$.myField', type: 'Dates'}]})

//@ Create an index specifiying geojson options for non geojson data type. (WL10858-FR5_8)
coll.createIndex('myField', {fields: [{field: '$.myField', type: 'TEXT(10)', options: 2, srid: 4326}]})

//@ Create an index with mismatched data types (WL10858-ET_1)
coll.add({numeric:10})
coll.createIndex('myIntField', {fields: [{field: '$.numeric', type: 'DATETIME'}]})

//@ Create an index specifiying SPATIAL as the index type for a non spatial data type (WL10858-ET_2)
coll.createIndex('myField', {fields: [{field: '$.myField', type: 'TEXT(10)'}], type:'SPATIAL'})

//@ Create an index specifiying INDEX as the index type for a spatial data type (WL10858-ET_3)
coll.createIndex('myField', {fields: [{field: '$.myField', type: 'GEOJSON', required: true, options: 2, srid: 4326}], type:'INDEX'})
