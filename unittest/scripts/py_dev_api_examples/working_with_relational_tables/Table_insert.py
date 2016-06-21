# Accessing an existing table
myTable = db.get_table('my_table')

# Insert a row of data.
myTable.insert(['id', 'name']).values(1, 'Mike').values(2, 'Jack').execute()
