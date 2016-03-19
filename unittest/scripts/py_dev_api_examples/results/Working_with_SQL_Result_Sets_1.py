res = nodeSession.sql('CALL my_proc()').execute()

if res.hasData():
        
        row = res.fetchOne()
        if row:
                print 'List of row available for fetching.'
                while row:
                        print row
                        row = res.fetchOne()
        else:
                print 'Empty list of rows.'
else:
        print 'No row result.'
