res = mySession.sql('CALL my_proc()').execute()

if res.has_data():
    
    row = res.fetch_one()
    if row:
        print 'List of row available for fetching.'
        while row:
            print row
            row = res.fetch_one()
    else:
        print 'Empty list of rows.'
else:
    print 'No row result.'
