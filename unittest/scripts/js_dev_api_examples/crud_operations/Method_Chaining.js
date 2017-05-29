// New method chaining used for executing an SQL SELECT statement
// Recommended way for executing queries
var employees = db.getTable('employee');

var res = employees.select(['name', 'age']).
        where('name like :param').
        orderBy(['name']).
        bind('param', 'm%').execute();

// Traditional SQL execution by passing an SQL string
// This is only available when using a Session
// It should only be used when absolutely necessary
var result = session.sql('SELECT name, age ' +
  'FROM employee ' +
  'WHERE name like ? ' +
  'ORDER BY name').bind('m%').execute();
