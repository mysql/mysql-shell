# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>
from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

mySession.drop_schema('js_shell_test')
schema = mySession.create_schema('js_shell_test')

# Creates a test collection and inserts data into it
collection = schema.create_collection('collection1')

# ---------------------------------------------
# Collection.add Unit Testing: Dynamic Behavior
# ---------------------------------------------
#@ CollectionAdd: valid operations after add with no documents
crud = collection.add([])
validate_crud_functions(crud, ['add', 'execute'])

#@ CollectionAdd: valid operations after add
crud = collection.add({"_id":"sample", "name":"john", "age":17, "account": None})
validate_crud_functions(crud, ['add', 'execute'])

#@ CollectionAdd: valid operations after execute
result = crud.execute()
validate_crud_functions(crud, ['add', 'execute'])


# ---------------------------------------------
# Collection.add Unit Testing: Error Conditions
# ---------------------------------------------

#@# CollectionAdd: Error conditions on add
crud = collection.add()
crud = collection.add(45)
crud = collection.add(['invalid data'])
crud = collection.add(mysqlx.expr('5+1'))
crud = collection.add([{'name': 'sample'}, 'error']);
crud = collection.add({'name': 'sample'}, 'error');


# ---------------------------------------
# Collection.Add Unit Testing: Execution
# ---------------------------------------

#@<> Collection.add execution  {VER(>=8.0.11)}
result = collection.add({ "name": 'document01', "Passed": 'document', "count": 1 }).execute()
EXPECT_EQ(1, result.affected_item_count)
EXPECT_EQ(1, result.affected_items_count)
EXPECT_EQ(1, len(result.generated_ids))
EXPECT_EQ(1, len(result.get_generated_ids()))
# WL11435_FR3_1
EXPECT_EQ(result.generated_ids[0], collection.find('name = "document01"').execute().fetch_one()._id);
id_prefix = result.generated_ids[0][:8]

#@<> WL11435_FR3_2 Collection.add execution, Single Known ID
result = collection.add({ "_id": "sample_document", "name": 'document02', "passed": 'document', "count": 1 }).execute()
EXPECT_EQ(1, result.affected_item_count)
EXPECT_EQ(1, result.affected_items_count)
EXPECT_EQ(0, len(result.generated_ids))
EXPECT_EQ(0, len(result.get_generated_ids()))
EXPECT_EQ('sample_document', collection.find('name = "document02"').execute().fetch_one()._id);

#@ WL11435_ET1_1 Collection.add error no id {VER(<8.0.11)}
result = collection.add({'name': 'document03', 'Passed': 'document', 'count': 1 }).execute();

#@<> Collection.add execution, Multiple {VER(>=8.0.11)}
result = collection.add([{ "name": 'document03', "passed": 'again', "count": 2 }, { "name": 'document04', "passed": 'once again', "count": 3 }]).execute()
EXPECT_EQ(2, result.affected_item_count)
EXPECT_EQ(2, result.affected_items_count)

# WL11435_ET2_6
EXPECT_EQ(2, len(result.generated_ids))
EXPECT_EQ(2, len(result.get_generated_ids()))

# Verifies IDs have the same prefix
EXPECT_EQ(id_prefix, result.generated_ids[0][:8]);
EXPECT_EQ(id_prefix, result.generated_ids[1][:8]);

# WL11435_FR3_1 Verifies IDs are assigned in the expected order
EXPECT_EQ(result.generated_ids[0], collection.find('name = "document03"').execute().fetch_one()._id);
EXPECT_EQ(result.generated_ids[1], collection.find('name = "document04"').execute().fetch_one()._id);

# WL11435_ET2_2 Verifies IDs are sequential
EXPECT_TRUE(result.generated_ids[0] < result.generated_ids[1]);

#@<> WL11435_ET2_3 Collection.add execution, Multiple Known IDs
result = collection.add([{ "_id": "known_00", "name": 'document05', "passed": 'again', "count": 2 }, { "_id": "known_01", "name": 'document06', "passed": 'once again', "count": 3 }]).execute()
EXPECT_EQ(2, result.affected_item_count)
EXPECT_EQ(2, result.affected_items_count)

# WL11435_ET2_5
EXPECT_EQ(0, len(result.generated_ids))
EXPECT_EQ(0, len(result.get_generated_ids()))
EXPECT_EQ('known_00', collection.find('name = "document05"').execute().fetch_one()._id);
EXPECT_EQ('known_01', collection.find('name = "document06"').execute().fetch_one()._id);


result = collection.add([]).execute()
EXPECT_EQ(-1, result.affected_item_count)
EXPECT_EQ(-1, result.affected_items_count)
EXPECT_EQ(0, len(result.generated_ids))
EXPECT_EQ(0, len(result.get_generated_ids()))

#@ Collection.add execution, Variations >=8.0.11 {VER(>=8.0.11)}
//! [CollectionAdd: Chained Calls]
result = collection.add({ "name": 'my fourth', "passed": 'again', "count": 4 }).add({ "name": 'my fifth', "passed": 'once again', "count": 5 }).execute()
print "Affected Rows Chained:", result.affected_items_count, "\n"
//! [CollectionAdd: Chained Calls]

//! [CollectionAdd: Using an Expression]
result = collection.add(mysqlx.expr('{"name": "my fifth", "passed": "document", "count": 1}')).execute()
print "Affected Rows Single Expression:", result.affected_items_count, "\n"
//! [CollectionAdd: Using an Expression]

//! [CollectionAdd: Document List]
result = collection.add([{ "name": 'my sexth', "passed": 'again', "count": 5 }, mysqlx.expr('{"name": "my senevth", "passed": "yep again", "count": 5}')]).execute()
print "Affected Rows Mixed List:", result.affected_items_count, "\n"
//! [CollectionAdd: Document List]

//! [CollectionAdd: Multiple Parameters]
result = collection.add({ "name": 'my eigth', "passed": 'yep', "count": 6 }, mysqlx.expr('{"name": "my nineth", "passed": "yep again", "count": 6}')).execute()
print "Affected Rows Multiple Params:", result.affected_items_count, "\n"
//! [CollectionAdd: Multiple Parameters]

#@<> Collection.add execution, Variations <8.0.11 {VER(<8.0.11)}
result = collection.add({'_id': '1E9C92FDA74ED311944E00059A3C7A44', "name": 'my fourth', "passed": 'again', "count": 4 }).add({'_id': '1E9C92FDA74ED311944E00059A3C7A45', "name": 'my fifth', "passed": 'once again', "count": 5 }).execute()
EXPECT_EQ(2, result.affected_item_count)
EXPECT_EQ(2, result.affected_items_count)

result = collection.add(mysqlx.expr('{"_id": "1E9C92FDA74ED311944E00059A3C7A46", "name": "my fifth", "passed": "document", "count": 1}')).execute()
EXPECT_EQ(1, result.affected_item_count)
EXPECT_EQ(1, result.affected_items_count)

result = collection.add([{'_id': '1E9C92FDA74ED311944E00059A3C7A47', "name": 'my sexth', "passed": 'again', "count": 5 }, mysqlx.expr('{"_id": "1E9C92FDA74ED311944E00059A3C7A48", "name": "my senevth", "passed": "yep again", "count": 5}')]).execute()
EXPECT_EQ(2, result.affected_item_count)
EXPECT_EQ(2, result.affected_items_count)

result = collection.add({'_id': '1E9C92FDA74ED311944E00059A3C7A49', "name": 'my eigth', "passed": 'yep', "count": 6 }, mysqlx.expr('{"_id": "1E9C92FDA74ED311944E00059A3C7A4A", "name": "my nineth", "passed": "yep again", "count": 6}')).execute()
EXPECT_EQ(2, result.affected_item_count)
EXPECT_EQ(2, result.affected_items_count)

# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()
