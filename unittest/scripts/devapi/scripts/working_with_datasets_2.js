// Assumptions: test schema exists, employee table exist on myTable

var myRows = myTable.select(['name', 'age'])
        .where('name like :name').bind('name','S%')
        .execute();

var row;
while (row = myRows.fetchOne()) {
        // Accessing the fields by array
        print('Name: ' + row['name'] + '\n');

        // Accessing the fields by dynamic attribute
        print(' Age: ' + row.age + '\n');
}