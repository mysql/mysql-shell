// Assumptions: test schema assigned to db, empty my_table table exists

// Accessing an existing table
var myTable = db.getTable('my_table');

// Insert a row of data.
myTable.insert(['id', 'name']).
        values(1, 'Mike').
        values(2, 'Jack').
        execute();