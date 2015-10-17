// Assumptions: users table exists
var res = nodeSession.sql('SELECT name, age FROM users').execute();

var row;
while (row = res.fetchOne()) {
  print('Name: ' + row['name'] + '\n');
  print(' Age: ' + row.age + '\n');
}