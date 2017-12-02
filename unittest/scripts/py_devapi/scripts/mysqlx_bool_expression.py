# Assumptions: validate_crud_functions available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<plugin_port>

from mysqlsh import mysqlx

mySession = mysqlx.get_session(__uripwd)

ensure_schema_does_not_exist(mySession, 'js_shell_test')
schema = mySession.create_schema('js_shell_test')

# A dummy collection with just 1 entry to let us run arbitrary queries on
coll = schema.create_collection("coll")
coll.add({"_id": "id1", "a":1, "n": None, "d":1.2345}).execute()
coll.add({"_id": "id2", "array":[5,6,7]}).execute()
coll.add({"_id": "id3", "object":{"a":8}}).execute()

#@ Expression evaluation (true)
true_expr = [
  "1 in (1,2,3)",
  "1 in [1,2,3]",
  "[1] in ([1], [2])",
  "2 in ((1+1))",
  "[1] in [[1], [2], [3]]",
  "[] in [[], [2], [3]]",
  "{'a':5} in [{'a':5}]",
  "{'a':5} in {'a':5, 'b':6}"
]

for i in true_expr:
  r = coll.find().fields([i]).execute().fetch_one()
  print i, " => ", r[i]

#@ Expression evaluation (false)
false_expr = [
  "4 in (1,2,3)",
  "4 in [1,2,3]",
  "[4] in [[1], [2], [3]]",
#unknown  "[] in [[1], [2], [3]]",
  "{'a':5} in [{'a':6}]",
  "{'a':5} in {'b':6}"
]

for i in false_expr:
  r = coll.find().fields([i]).execute().fetch_one()
  print i, " => ", r[i]

#@ Expression evaluation (filter)
filter_expr = [
  "6 in array",
  "null in array",
  "null in $.array",
  "null not in array",
  "null not in $.array"
]

for i in filter_expr:
  print i, " => ", coll.find(i).execute().fetch_all()

#-----------------------------------------------------------------------------

movies = schema.create_collection('movies')

test_data = [
   { "_id" : "a6f4b93e1a264a108393524f29546a8c", "title" : "AFRICAN EGG", "description" : "A Fast-Paced Documentary of a Pastry Chef And a Dentist who must Pursue a Forensic Psychologist in The Gulf of Mexico", "releaseyear" : 2006, "language" : "English", "duration" : 130, "rating" : "G", "genre" : "Science fiction",
            "actors" : [{ "name" : "MILLA PECK", "country" : "Mexico", "birthdate": "12 Jan 1984"},
                        { "name" : "VAL BOLGER", "country" : "Botswana", "birthdate": "26 Jul 1975" },
                        { "name" : "SCARLETT BENING", "country" : "Syria", "birthdate": "16 Mar 1978" }],
          "additionalinfo" : { "director" : "Sharice Legaspi", "writers" : ["Rusty Couturier", "Angelic Orduno", "Carin Postell"], "productioncompanies" : ["Qvodrill", "Indigoholdings"]}
   }
]

for i in test_data:
  movies.add(i).execute()

tabl = schema.get_collection_as_table("movies")

#WL10848 F1 - Expressions must accept use of the IN operator with any pair of operands that are valid for any other arithmetic or boolean operator
fr1_cases = [
  '(1>5) in (true, false)',
  '(1+5) in (1, 2, 3, 4, 5)',
  "('a'>'b') in (true, false)",
  "1 in (1,2,3)",
  'true IN [(1>5), not (false), (true or false), (false and true)]',
  'true IN ((1>5), not (false), (true or false), (false and true))',
  'actors in actors',
  '{ "name" : "MILLA PECK" } IN actors',
  '[1,2,3] in actors',
#XXX not supported yet '{"field":true} IN ("mystring", 124, myvar, othervar.jsonobj)',
  "actor.name IN ['a name', null, (1<5-4), myvar.jsonobj.name]",
  'true IN [1-5/2*2 > 3-2/1*2]',
]

fr1_cases_tbl = [
  '(1>5) in (true, false)',
  '(1+5) in (1, 2, 3, 4, 5)',
  "('a'>'b') in (true, false)",
  "1 in (1,2,3)",
  'true IN [(1>5), not (false), (true or false), (false and true)]',
  'true IN ((1>5), not (false), (true or false), (false and true))',
  "doc->'$.actors' in doc->'$.actors'",
  '{ "name" : "MILLA PECK" } IN doc->\'$.actors\'',
  '[1,2,3] in doc->\'$.actors\'',
#XXX not supported yet '{"field":true} IN ("mystring", 124, myvar, othervar.jsonobj)',
  "doc->'$.actor.name' IN ['a name', null, (1<5-4), doc->'$.myvar.jsonobj.name']",
  'true IN [1-5/2*2 > 3-2/1*2]'
]

# These test that boolean expressions work everywhere they should

#@ IN basic - collection find
for i in fr1_cases:
  print i
  movies.find(i).execute().fetch_all()
  movies.find().sort([i]).execute().fetch_all()
  #movies.find().fields([i]).groupBy([i]).having(i).execute().fetch_all()

#@ IN syntactically valid but unsupported
movies.find('(1+5) in [1, 2, 3, 4, 5]').execute().fetch_all()
movies.find('(1>5) in [true, false]').execute().fetch_all()
movies.find("('a'>'b') in [true, false]").execute().fetch_all()
movies.find('1-5/2*2 > 3-2/1*2 IN [true, false]').execute().fetch_all()

# TODO(rennox): This is actually returning a result
# movies.find('not false and true IN [true]').execute().fetch_all()


#@ IN basic - collection modify
for i in fr1_cases:
  print i
  mySession.start_transaction()
  movies.modify(i).unset("bogus").execute()
  mySession.rollback()

#@ IN basic - collection remove
for i in fr1_cases:
  print i
  mySession.start_transaction()
  movies.remove(i).execute()
  mySession.rollback()

#@ IN basic - table select
for i in fr1_cases_tbl:
  print i
  tabl.select().where(i).execute().fetch_all()
  tabl.select().order_by([i]).execute().fetch_all()
  # tabl.select([i]).groupBy([i]).having(i).execute().fetch_all()

#@ IN basic - table update
for i in fr1_cases_tbl:
  print i
  mySession.start_transaction()
  tabl.update().set("doc", mysqlx.expr("doc")).where(i).execute()
  mySession.rollback()

#@ IN basic - table delete
for i in fr1_cases_tbl:
  print i
  mySession.start_transaction()
  tabl.delete().where(i).execute()
  mySession.rollback()

#@WL10848 F2 - The evaluation of the IN operation between 2 operands is equivalent to a call to the JSON_CONTAINS() function with said operands Rules defined for JSON_CONTAINS():

# A candidate scalar is contained in a target scalar if and only if they are comparable and are equal. Two scalar values are comparable if they have the same JSON_TYPE() types, with the exception that values of types INTEGER and DECIMAL are also comparable to each other.
# A candidate array is contained in a target array if and only if every element in the candidate is contained in some element of the target
# A candidate nonarray is contained in a target array if and only if the candidate is contained in some element of the target.
# A candidate object is contained in a target object if and only if for each key in the candidate there is a key with the same name in the target and the value associated with the candidate key is contained in the value associated with the target key.

fr2_cases = [
  "'African Egg' IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' }) ",
  "1 IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' }) ",
  "false IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' }) ",
  "[0,1,2] IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' }) ",
  "{ 'title' : 'Atomic Firefighter' } IN ('African Egg', 1, true, NULL, [0,1,2], { 'title' : 'Atomic Firefighter' }) "
]

# These test that boolean expressions are evaluated the way they're supposed to
for i in fr2_cases:
  print i
  movies.find().fields([i]).execute().fetch_all()

#@ WL10848 F3 - If the right side operand of the IN operator is a comma separated list of expressions enclosed in parenthesis -- like ('foo', 'bar', 'baz', current_date()) -- the expression must translate to the existing SQL IN operator

fr3_cases = [
  "title IN ('African Egg', 'The Witcher', 'Jurassic Perk')",
  "releaseyear IN (2006, 2010, 2017)"
]

for i in fr3_cases:
  print i
  movies.find().fields([i]).execute().fetch_all()

#@ WL10848 F4 - If any of the operands is the SQL NULL value (like when a document field that does not exist), the operation evaluates to NULL

fr4_cases = [
  "'African Egg' in movietitle",
  #'(1 = NULL) IN title',
  #'(1 is NULL) IN title',
  "NULL in title",
  #'NOT NULL IN title'
]

for i in fr4_cases:
  print i
  movies.find().fields([i]).execute().fetch_all()

#@ WL10848 F5 - The result of the evaluation of the IN operator is a boolean value. The operation evaluates to TRUE if the left side operand is contained in the right side and FALSE otherwise

fr5_cases = [
"1 IN [1,2,3]",
"0 IN [1,2,3]"
]

for i in fr5_cases:
  print i
  movies.find().fields([i]).execute().fetch_all()


#@ WL10848 F6 - The result of the evaluation of the NOT IN operator is a boolean value. The operation evaluates to True if the left side operand is NOT contained in the right side and False otherwise

fr6_cases = [
"1 NOT IN [1,2,3]",
"0 NOT IN [1,2,3]"
]

for i in fr6_cases:
  print i
  movies.find().fields([i]).execute().fetch_all()

# WL10848 F7 - extras

#@ Search for empty strings in a field
movies.find("'' IN title").execute().fetch_all()

#@ Search for a field in an empty string
movies.find("title IN ('', ' ')").execute().fetch_all()
movies.find("title IN ['', ' ']").execute().fetch_all()

#@ Search for an array in a field
movies.find('["Rusty Couturier", "Angelic Orduno", "Carin Postell"] IN additionalinfo.writers').execute().fetch_all()

#@ Search for a document in a field
movies.find('{ "name" : "MILLA PECK", "country" : "Mexico", "birthdate": "12 Jan 1984"} IN actors').execute().fetch_all()

#@ Search for a field in a custom array
movies.find('releaseyear IN [2006, 2007, 2008]').execute().fetch_all()

#@ Search for a boolean in a field
movies.find("true IN title").execute().fetch_all()
movies.find("false IN genre").execute().fetch_all()

#@ Search for nested values in a document
movies.find("'Sharice Legaspi' IN additionalinfo.director").execute().fetch_all()

#@ Search for field in an array of documents
movies.find("'Mexico' IN actors[*].country").execute().fetch_all()

#@ Search for a value in a nested array
movies.find("'Angelic Orduno' IN additionalinfo.writers").execute().fetch_all()


# Cleanup
mySession.drop_schema('js_shell_test')
mySession.close()
