mysqlx = require('mysqlx').mysqlx;

// Variables assummed to exist on the script must be declared in global scope
var session;
var db;
var myTable;

function ensure_session(){
  if (type(session) == "Undefined")
  {
    print("Creating session...");
    var  uri = os.getenv('MYSQL_URI');
    session = mysqlx.getNodeSession(uri);

    // Ensures the user on dev-api exists
    try {
      session.sql("create user mike@localhost identified by 's3cr3t!'").execute();
      session.sql("grant all on *.* to mike@localhost with grant option").execute();
    }
    catch(err)
    {
      // This means the user existed already
    }  
  }
  else
    print("Session exists...");
}

function ensure_test_schema() {
  ensure_session();
  
  var s = session.getSchema('test');
  
  try {
      var schema = session.createSchema('test'); 
      print("Creating test schema...");
    }
  catch(err)
  {
    // This means the schema existed already
    print("Test schema exists...");
  }  
}

function ensure_test_schema_on_db() {
  ensure_test_schema();
  print("Assigning test schema to db...");
  db = session.getSchema('test');
}

function ensure_employee_table() {
  ensure_test_schema();
  
  var table = session.getSchema('test').getTable('employee');
  if (type(table) == "Undefined")
  {
    print("Creating employee table...");
    session.sql('create table test.employee (name varchar(50), age integer, gender varchar(20))').execute();
    var table = db.getTable('employee');
    
    var result = table.insert({'name': 'jack', 'age': 17, 'gender': 'male'}).execute();
    var result = table.insert({'name': 'adam', 'age': 15, 'gender': 'male'}).execute();
    var result = table.insert({'name': 'brian', 'age': 14, 'gender': 'male'}).execute();
    var result = table.insert({'name': 'alma', 'age': 13, 'gender': 'female'}).execute();
    var result = table.insert({'name': 'clare', 'age': 14, 'gender': 'female'}).execute();
    var result = table.insert({'name': 'donna', 'age': 16, 'gender': 'female'}).execute();
    var result = table.insert({'name': 'angel', 'age': 14, 'gender': 'male'}).execute();
    var result = table.insert({'name': 'mike', 'age': 39, 'gender': 'male'}).execute();
    var result = table.insert({'name': 'John', 'age': 20, 'gender': 'male'}).execute();
    var result = table.insert({'name': 'johannes', 'age': 28, 'gender': 'male'}).execute();
    var result = table.insert({'name': 'Sally', 'age': 19, 'gender': 'female'}).execute();
    var result = table.insert({'name': 'Molly', 'age': 25, 'gender': 'female'}).execute();
  }
  else{
    print("Employee table exists...");
  }
}

function ensure_employee_table_on_mytable() {
  ensure_employee_table();
  
  print("Assigning employee table to myTable...");
  
  myTable = session.getSchema('test').getTable('employee');
}

function ensure_empty_my_table_table() {
  ensure_test_schema();
  
  print("Creating my_table table...");
  session.sql('drop table if exists test.my_table').execute();
  session.sql('create table test.my_table (id integer, name varchar(50))').execute();
}

function ensure_my_collection_collection() {
  ensure_test_schema();
  
  var test_coll = session.getSchema('test').getCollection('my_collection');
  if (type(test_coll) == "Undefined")
  {
    print("Creating my_collection collection...");
    var test_coll = db.createCollection('my_collection');

    var result = test_coll.add({name: 'jack', age: 17, gender: 'male'}).execute();
    var result = test_coll.add({name: 'adam', age: 15, gender: 'male'}).execute();
    var result = test_coll.add({name: 'brian', age: 14, gender: 'male'}).execute();
    var result = test_coll.add({name: 'alma', age: 13, gender: 'female'}).execute();
    var result = test_coll.add({name: 'clare', age: 14, gender: 'female'}).execute();
    var result = test_coll.add({name: 'donna', age: 16, gender: 'female'}).execute();
    var result = test_coll.add({name: 'angel', age: 14, gender: 'male'}).execute();
    var result = test_coll.add({name: 'mike', age: 39, gender: 'male'}).execute();
    var result = test_coll.add({name: 'John', age: 20, gender: 'male'}).execute();
    var result = test_coll.add({name: 'johannes', age: 28, gender: 'male'}).execute();
    var result = test_coll.add({name: 'Sally', age: 19, gender: 'female'}).execute();
    var result = test_coll.add({name: 'Molly', age: 25, gender: 'female'}).execute();
  }
  else{
    print("my_collection collection exists...");
  }
}


function ensure_not_my_collection_collection() {
  ensure_test_schema();
  
  var test_coll = session.getSchema('test').getCollection('my_collection');
  if (type(test_coll) != "Undefined")
  {
    test_coll.drop();
  }
}

// Executes the functions associated to every assumption defined on the test case
for(index in __assumptions__)
{
  switch(__assumptions__[index]){
    case "connected session":
      ensure_session();
      break;
    case "test schema exists":
      ensure_test_schema();
      break;
    case "test schema assigned to db":
      ensure_test_schema_on_db();
      break;
    case "my_collection collection exists":
      ensure_my_collection_collection();
      break;
    case "employee table exists":
      ensure_employee_table();
      break;
    case "employee table exist on myTable":
      ensure_employee_table_on_mytable();
      break;
    case "empty my_table table exists":
      ensure_empty_my_table_table();
      break;
    case "my_collection collection not exists":
      ensure_not_my_collection_collection();
    default:
      break;
  }
}
