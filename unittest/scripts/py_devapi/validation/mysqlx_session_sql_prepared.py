#@<PROTOCOL> First execution is normal
>>>> SEND Mysqlx.Sql.StmtExecute {
  stmt: "select * from test_table where name like ?"
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "g%"
      }
    }
  }
  namespace: "sql"
}

#@<OUT> First execution is normal
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
+----+--------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> Second execution prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 1
  stmt {
    type: STMT
    stmt_execute {
      stmt: "select * from test_table where name like ?"
      namespace: "sql"
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 1
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "j%"
      }
    }
  }
}

#@<OUT> Second execution prepares statement and executes it
+----+-------+-----+
| id | name  | age |
+----+-------+-----+
|  2 | james |  17 |
+----+-------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> Third execution uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 1
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "l%"
      }
    }
  }
}

#@<OUT> Third execution uses prepared statement
+----+------+-----+
| id | name | age |
+----+------+-----+
|  3 | luke |  18 |
+----+------+-----+
1 row in set ([[*]] sec)
