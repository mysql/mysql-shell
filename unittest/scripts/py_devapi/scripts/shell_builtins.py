#@# Stored sessions add errors
shell.stored_sessions.add()
shell.stored_sessions.add(56, 'test@uri')
shell.stored_sessions.add('my sample', 5)
shell.stored_sessions.add('my sample', 'test@uri')
shell.stored_sessions.add('my_sample', {})
shell.stored_sessions.add('my_sample', {'host':''})
shell.stored_sessions.add('my_sample', 'user@host', 5)


#@ Adding entry to the Stored sessions
done = shell.stored_sessions.add('my_sample', 'root:pwd@samplehost:44000/sakila')
print 'Added: %s\n' % done, '\n'
print 'Host: %s\n' % shell.stored_sessions.my_sample.host
print 'Port: %s\n' % shell.stored_sessions.my_sample.port
print 'User: %s\n' % shell.stored_sessions.my_sample.dbUser
print 'Password: %s\n' % shell.stored_sessions.my_sample.dbPassword
print 'Schema: %s\n' % shell.stored_sessions.my_sample.schema

#@ Attempt to override connection without override
done = shell.stored_sessions.add('my_sample', 'admin@localhost')

#@ Attempt to override connection with override=False
done = shell.stored_sessions.add('my_sample', 'admin@localhost', False)

#@ Attempt to override connection with override=True
done = False
done = shell.stored_sessions.add('my_sample', 'admin@localhost', True)
print 'Added: %s\n' % done
print 'Host: %s\n' % shell.stored_sessions.my_sample.host
print 'User: %s\n' % shell.stored_sessions.my_sample.dbUser

#@# Stored sessions update errors
shell.stored_sessions.update()
shell.stored_sessions.update(56, 'test@uri')
shell.stored_sessions.update('my sample', 5)
shell.stored_sessions.update('my sample', 'test@uri')
shell.stored_sessions.update('my_sample', {})
shell.stored_sessions.update('my_sample', {'host':''})


#@ Updates a connection
done = False
done = shell.stored_sessions.update('my_sample', 'guest@localhost')
print 'Updated: %s\n' % done
print 'User: %s\n' % shell.stored_sessions.my_sample.dbUser

#@# Stored sessions remove errors
shell.stored_sessions.remove()
shell.stored_sessions.remove(56)
shell.stored_sessions.remove('my sample')

#@ Remove connection
done = False
done = shell.stored_sessions.remove('my_sample')
print 'Removed: %s\n' % done

