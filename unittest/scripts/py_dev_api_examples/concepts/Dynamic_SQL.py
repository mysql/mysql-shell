def createTestTable(session, name):
    
    # use escape function to quote names/identifier
    quoted_name = session.quote_name(name)
    
    session.sql("DROP TABLE IF EXISTS " + quoted_name).execute()
    
    create = "CREATE TABLE "
    create += quoted_name
    create += " (id INT NOT NULL PRIMARY KEY AUTO_INCREMENT)"
    
    session.sql(create).execute()
    
    return session.get_current_schema().get_table(name)

import mysqlx

session = mysqlx.get_node_session({
        'dataSourceFile': 'mysqlxconfig.json', 'app': 'myapp',
        'dbUser': 'mike', 'dbPassword': 's3cr3t!'})

default_schema = session.get_default_schema().name
session.set_current_schema(default_schema)

# Creates some tables
table1 = createTestTable(session, 'test1')
table2 = createTestTable(session, 'test2')
