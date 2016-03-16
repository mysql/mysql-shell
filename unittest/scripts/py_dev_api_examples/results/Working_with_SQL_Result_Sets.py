
res = nodeSession.sql('SELECT name, age FROM users').execute()

row = res.fetchOne()

while row:
        print 'Name: %s\n' % row[0]
        print ' Age: %s\n' % row.age
        row = res.fetchOne()
