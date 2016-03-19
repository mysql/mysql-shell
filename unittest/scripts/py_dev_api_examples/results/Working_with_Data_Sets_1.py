myRows = myTable.select(['name', 'age']).where('name like :name').bind('name','S%').execute()

row = myRows.fetchOne()
while row:
        # Accessing the fields by array
        print 'Name: %s\n' % row[0]
        
        # Accessing the fields by dynamic attribute
        print 'Age: %s\n' % row.age
        
        row = myRows.fetchOne()
