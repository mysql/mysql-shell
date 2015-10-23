#@# Stored sessions add errors
shell.storedSessions.add()
shell.storedSessions.add(56, 'test@uri')
shell.storedSessions.add('my sample', 5)
shell.storedSessions.add('my sample', 'test@uri')
shell.storedSessions.add('my_sample', {})
shell.storedSessions.add('my_sample', {'host':''})
shell.storedSessions.add('my_sample', 'user@host', 5)


#@ Adding entry to the Stored sessions
done = shell.storedSessions.add('my_sample', 'root:pwd@samplehost:44000/sakila')
print 'Added: %s\n' % done, '\n' 
print 'Host: %s\n' % shell.storedSessions.my_sample.host
print 'Port: %s\n' % shell.storedSessions.my_sample.port
print 'User: %s\n' % shell.storedSessions.my_sample.dbUser
print 'Password: %s\n' % shell.storedSessions.my_sample.dbPassword
print 'Schema: %s\n' % shell.storedSessions.my_sample.schema

#@ Attempt to override connection without override
done = shell.storedSessions.add('my_sample', 'admin@localhost')

#@ Attempt to override connection with override=False
done = shell.storedSessions.add('my_sample', 'admin@localhost', False)

#@ Attempt to override connection with override=True
done = False
done = shell.storedSessions.add('my_sample', 'admin@localhost', True)
print 'Added: %s\n' % done
print 'Host: %s\n' % shell.storedSessions.my_sample.host
print 'User: %s\n' % shell.storedSessions.my_sample.dbUser

#@# Stored sessions update errors
shell.storedSessions.update()
shell.storedSessions.update(56, 'test@uri')
shell.storedSessions.update('my sample', 5)
shell.storedSessions.update('my sample', 'test@uri')
shell.storedSessions.update('my_sample', {})
shell.storedSessions.update('my_sample', {'host':''})


#@ Updates a connection
done = False
done = shell.storedSessions.update('my_sample', 'guest@localhost')
print 'Updated: %s\n' % done
print 'User: %s\n' % shell.storedSessions.my_sample.dbUser

#@# Stored sessions remove errors
shell.storedSessions.remove()
shell.storedSessions.remove(56)
shell.storedSessions.remove('my sample')

#@ Remove connection
done = False
done = shell.storedSessions.remove('my_sample')
print 'Removed: %s\n' % done

