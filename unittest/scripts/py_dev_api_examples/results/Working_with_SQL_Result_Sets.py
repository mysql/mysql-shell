res = mySession.sql('SELECT name, age FROM users').execute()

row = res.fetch_one()

while row:
    print 'Name: %s\n' % row[0]
    print ' Age: %s\n' % row.age
    row = res.fetch_one()
