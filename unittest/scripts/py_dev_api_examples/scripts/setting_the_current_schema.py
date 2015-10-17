# Assumptions: test schema exists

import mysqlx

# Direct connect with no client side default schema defined
mySession = mysqlx.getNodeSession('mike:s3cr3t!@localhost')
mySession.setCurrentSchema("test")