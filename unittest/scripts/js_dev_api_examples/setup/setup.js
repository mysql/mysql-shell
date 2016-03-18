mysqlx = require('mysqlx').mysqlx;

// Variables assummed to exist on the script must be declared in global scope
var testSession = null;
var db;
var myTable;
var myColl;
var nodeSession;
var session;

function ensure_session(){
  if (type(testSession) == "Null")
  {
    print("Creating testSession...\n");
    var  uri = os.getenv('MYSQL_URI');
    testSession = mysqlx.getNodeSession(uri);

    // Ensures the user on dev-api exists
    try {
      testSession.sql("create user mike@'%' identified by 's3cr3t!'").execute();
      testSession.sql("grant all on *.* to mike@'%' with grant option").execute();
    }
    catch(err)
    {
      // This means the user existed already
    }  
  }
  else
    print("Session exists...\n");

  session = testSession;
}

function ensure_not_a_schema(schema){
  ensure_session()

  try{
      var s = testSession.dropSchema(schema);
      print (schema + " schema has been deleted...\n");
  }
  catch(err)
  {
    print (schema + " schema does not exist...\n");
  }
}

function ensure_test_schema() {
  ensure_session();

  try {
      var s = testSession.getSchema('test');
      print("Test schema exists...\n");
  }
  catch(err){
    print("Creating test schema...\n");
    testSession.createSchema('test');
  }
	
	testSession.setCurrentSchema('test');
}

function ensure_test_schema_on_db() {
  ensure_test_schema();
  print("Assigning test schema to db...\n");
  db = testSession.getSchema('test');
}

function ensure_employee_table() {
  ensure_test_schema_on_db();

  try{
    var table = testSession.getSchema('test').getTable('employee');

    print("Employee table exists...\n");
  }
  catch(err){
    print("Creating employee table...\n");
    testSession.sql('create table test.employee (name varchar(50), age integer, gender varchar(20))').execute();
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
}

function ensure_relatives_collection() {
  ensure_test_schema_on_db();

  try{
    var test_coll = testSession.getSchema('test').getCollection('relatives');

    print("Relatives collection exists...\n");
  }
  catch(err){
    print("Creating relatives collection...\n");
    var test_coll = db.createCollection('relatives');
    
    var result = test_coll.add({name: 'jack', age: 17, alias: 'jack'}).execute();
    var result = test_coll.add({name: 'adam', age: 15, alias: 'jr'}).execute();
    var result = test_coll.add({name: 'brian', age: 14, alias: 'brian'}).execute();
    var result = test_coll.add({name: 'charles', age: 13, alias: 'jr'}).execute();
    var result = test_coll.add({name: 'clare', age: 14, alias: 'cla'}).execute();
    var result = test_coll.add({name: 'donna', age: 16, alias: 'donna'}).execute();
  }
}

function ensure_employee_table_on_mytable() {
  ensure_employee_table();
  
  print("Assigning employee table to myTable...\n");
  
  myTable = testSession.getSchema('test').getTable('employee');
}

function ensure_empty_my_table_table() {
  ensure_test_schema();
  
  print("Creating my_table table...\n");
  testSession.sql('drop table if exists test.my_table').execute();
  testSession.sql('create table test.my_table (id integer, name varchar(50))').execute();
}

function ensure_my_table_table() {
  ensure_test_schema();
  
  print("Creating my_table table...\n");
  testSession.sql('drop table if exists test.my_table').execute();
  testSession.sql('create table test.my_table (_id integer NOT NULL AUTO_INCREMENT PRIMARY KEY, name varchar(50), birthday DATE, age INTEGER)').execute();
}

function ensure_my_collection_collection() {
  ensure_test_schema_on_db();

  try{
    var test_coll = testSession.getSchema('test').getCollection('my_collection');

    print("my_collection collection exists...\n");
  }
  catch(err){
    print("Creating my_collection collection...\n");
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
}

function ensure_customers_collection() {
  ensure_test_schema_on_db();

  try{
    var test_coll = testSession.getSchema('test').getCollection('customers');

    print("customers collection exists...\n");
  }
  catch(err){
    print("Creating customers collection...\n");
    var test_coll = db.createCollection('customers');
  }
}

function ensure_my_coll_collection() {
  ensure_test_schema_on_db();

  try{
    var test_coll = testSession.getSchema('test').getCollection('my_coll');

    print("my_coll collection exists...\n");
  }
  catch(err){
    print("Creating my_coll collection...\n");
    var test_coll = db.createCollection('my_coll');
  }
}


function ensure_not_collection(name) {
  ensure_test_schema();

  try{
    var test_coll = testSession.getSchema('test').getCollection(name);

    print ("Dropping " + name + "...\n");
    testSession.dropCollection('test', name);
  }
  catch(err){
    print(name + " does not exist...\n");
  }
}


function ensure_custom_id_unique(){
	ensure_my_collection_collection();
	myColl = db.getCollection('my_collection');
	
	myColl.remove('_id = "custom_id"').execute();
}

function ensure_table_users_exists(){
	ensure_test_schema();

	try{
		var test_coll = testSession.getSchema('test').getTable('users');

		print('users table exists...');
	}
	catch(err){
		print('Creating users table...');
		testSession.sql('create table users (name varchar(50), age int)').execute();
  }
  
  testSession.sql('delete from test.users').execute();
  testSession.sql('insert into test.users values ("Jack", 17)').execute();
  testSession.sql('insert into test.users values ("John", 40)').execute();
  testSession.sql('insert into test.users values ("Alfred", 39)').execute();
  testSession.sql('insert into test.users values ("Mike", 40)').execute();
  testSession.sql('insert into test.users values ("Armand", 50)').execute();
  testSession.sql('insert into test.users values ("Rafa", 38)').execute();

	nodeSession = testSession;
}

function ensure_my_proc_procedure_exists(){
	ensure_table_users_exists();
	
	var procedure = "create procedure my_proc() " + 
                  "begin " + 
                  "select * from test.users where age > 40; " + 
                  "select * from test.users where age < 40; " + 
                  "delete from test.users where age = 40; " + 
                  "end";
                  
	testSession.sql("drop procedure if exists my_proc").execute();
	testSession.sql(procedure).execute();
	
	nodeSession = testSession;
}

// Executes the functions associated to every assumption defined on the test case
for(index in __assumptions__)
{
	print ('Assumption:', __assumptions__[index], '\n');
	
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
      ensure_not_collection("my_collection");
      ensure_my_collection_collection();
      break;
    case "customers collection exists":
      ensure_not_collection("customers");
      ensure_customers_collection();
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
    case "my_table table exists":
      ensure_my_table_table();
      break;
    case "my_collection collection not exists":
      ensure_not_collection("my_collection");
			break;
    case "my_coll collection is empty":
      ensure_not_collection("my_coll");
      ensure_my_coll_collection();
			break;
		case "custom_id is unique":
			ensure_custom_id_unique();
			break;
		case "users table exists":
			ensure_table_users_exists();
			break;
		case "my_proc procedure exists":
			ensure_my_proc_procedure_exists();
			break;
    case "relatives collection exists":
      ensure_not_collection("relatives");
      ensure_relatives_collection();
      break;
    case "schema unexisting does not exists":
      ensure_not_a_schema('unexisting')
      break;
    default:
      break;
  }
}
