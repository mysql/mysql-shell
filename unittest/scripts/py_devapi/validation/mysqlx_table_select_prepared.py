#@<PROTOCOL> First execution is normal
>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
}

#@<OUT> First execution is normal
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  2 | james  |  17 |
|  3 | luke   |  18 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> Second execution prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 1
  stmt {
    type: FIND
    find {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 1
}

#@<OUT> Second execution prepares statement and executes it
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  2 | james  |  17 |
|  3 | luke   |  18 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> Third execution uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 1
}

#@<OUT> Third execution uses prepared statement
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  2 | james  |  17 |
|  3 | luke   |  18 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> where() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 1
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  criteria {
    type: OPERATOR
    operator {
      name: ">="
      param {
        type: IDENT
        identifier {
          name: "age"
        }
      }
      param {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 18
        }
      }
    }
  }
}

#@<OUT> where() changes statement, back to normal execution
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)

#@<PROTOCOL> second execution after where(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 2
  stmt {
    type: FIND
    find {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      criteria {
        type: OPERATOR
        operator {
          name: ">="
          param {
            type: IDENT
            identifier {
              name: "age"
            }
          }
          param {
            type: LITERAL
            literal {
              type: V_UINT
              v_unsigned_int: 18
            }
          }
        }
      }
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 2
}

#@<OUT> second execution after where(), prepares statement and executes it
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)

#@<PROTOCOL> third execution after where(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 2
}

#@<OUT> third execution after where(), uses prepared statement
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)

#@<PROTOCOL> order_by() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 2
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  criteria {
    type: OPERATOR
    operator {
      name: ">="
      param {
        type: IDENT
        identifier {
          name: "age"
        }
      }
      param {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 18
        }
      }
    }
  }
  order {
    expr {
      type: IDENT
      identifier {
        name: "name"
      }
    }
    direction: DESC
  }
}

#@<OUT> order_by() changes statement, back to normal execution
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  3 | luke   |  18 |
|  1 | george |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)

#@<PROTOCOL> second execution after order_by(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 3
  stmt {
    type: FIND
    find {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      criteria {
        type: OPERATOR
        operator {
          name: ">="
          param {
            type: IDENT
            identifier {
              name: "age"
            }
          }
          param {
            type: LITERAL
            literal {
              type: V_UINT
              v_unsigned_int: 18
            }
          }
        }
      }
      order {
        expr {
          type: IDENT
          identifier {
            name: "name"
          }
        }
        direction: DESC
      }
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 3
}

#@<OUT> second execution after order_by(), prepares statement and executes it
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  3 | luke   |  18 |
|  1 | george |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)

#@<PROTOCOL> third execution after order_by(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 3
}

#@<OUT> third execution after order_by(), uses prepared statement
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  3 | luke   |  18 |
|  1 | george |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)

#@<PROTOCOL> limit() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 3
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  criteria {
    type: OPERATOR
    operator {
      name: ">="
      param {
        type: IDENT
        identifier {
          name: "age"
        }
      }
      param {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 18
        }
      }
    }
  }
  limit {
    row_count: 1
    offset: 0
  }
  order {
    expr {
      type: IDENT
      identifier {
        name: "name"
      }
    }
    direction: DESC
  }
}



#@<OUT> limit() changes statement, back to normal execution
+----+------+-----+
| id | name | age |
+----+------+-----+
|  3 | luke |  18 |
+----+------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> second execution after limit(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 4
  stmt {
    type: FIND
    find {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      criteria {
        type: OPERATOR
        operator {
          name: ">="
          param {
            type: IDENT
            identifier {
              name: "age"
            }
          }
          param {
            type: LITERAL
            literal {
              type: V_UINT
              v_unsigned_int: 18
            }
          }
        }
      }
      order {
        expr {
          type: IDENT
          identifier {
            name: "name"
          }
        }
        direction: DESC
      }
      limit_expr {
        row_count {
          type: PLACEHOLDER
          position: 0
        }
        offset {
          type: PLACEHOLDER
          position: 1
        }
      }
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 0
    }
  }
}

#@<OUT> second execution after limit(), prepares statement and executes it
+----+------+-----+
| id | name | age |
+----+------+-----+
|  3 | luke |  18 |
+----+------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> third execution after limit(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 0
    }
  }
}

#@<OUT> third execution after limit(), uses prepared statement
+----+------+-----+
| id | name | age |
+----+------+-----+
|  3 | luke |  18 |
+----+------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> offset() does not change the statement, uses prepared one
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
}

#@<OUT> offset() does not change the statement, uses prepared one
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
+----+--------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> lock_exclusive() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 4
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  criteria {
    type: OPERATOR
    operator {
      name: ">="
      param {
        type: IDENT
        identifier {
          name: "age"
        }
      }
      param {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 18
        }
      }
    }
  }
  limit {
    row_count: 1
    offset: 1
  }
  order {
    expr {
      type: IDENT
      identifier {
        name: "name"
      }
    }
    direction: DESC
  }
  locking: EXCLUSIVE_LOCK
}


#@<OUT> lock_exclusive() changes statement, back to normal execution
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
+----+--------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> second execution after lock_exclusive(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 5
  stmt {
    type: FIND
    find {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      criteria {
        type: OPERATOR
        operator {
          name: ">="
          param {
            type: IDENT
            identifier {
              name: "age"
            }
          }
          param {
            type: LITERAL
            literal {
              type: V_UINT
              v_unsigned_int: 18
            }
          }
        }
      }
      order {
        expr {
          type: IDENT
          identifier {
            name: "name"
          }
        }
        direction: DESC
      }
      locking: EXCLUSIVE_LOCK
      limit_expr {
        row_count {
          type: PLACEHOLDER
          position: 0
        }
        offset {
          type: PLACEHOLDER
          position: 1
        }
      }
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 5
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
}

#@<OUT> second execution after lock_exclusive(), prepares statement and executes it
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
+----+--------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> third execution after lock_exclusive(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 5
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
}

#@<OUT> third execution after lock_exclusive(), uses prepared statement
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
+----+--------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> creates statement to test lock_shared()
>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
}

#@<OUT> creates statement to test lock_shared()
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  2 | james  |  17 |
|  3 | luke   |  18 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> prepares statement to test lock_shared()
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 6
  stmt {
    type: FIND
    find {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 6
}

#@<OUT> prepares statement to test lock_shared()
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  2 | james  |  17 |
|  3 | luke   |  18 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> lock_shared() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 6
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  locking: SHARED_LOCK
}

#@<OUT> lock_shared() changes statement, back to normal execution
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  2 | james  |  17 |
|  3 | luke   |  18 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> second execution after lock_shared(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 7
  stmt {
    type: FIND
    find {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      locking: SHARED_LOCK
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 7
}

#@<OUT> second execution after lock_shared(), prepares statement and executes it
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  2 | james  |  17 |
|  3 | luke   |  18 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> third execution after lock_shared(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 7
}

#@<OUT> third execution after lock_shared(), uses prepared statement
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  2 | james  |  17 |
|  3 | luke   |  18 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> creates statement with aggregate function to test having()
>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  projection {
    source {
      type: IDENT
      identifier {
        name: "age"
      }
    }
  }
  projection {
    source {
      type: FUNC_CALL
      function_call {
        name {
          name: "count"
        }
        param {
          type: IDENT
          identifier {
            name: "age"
          }
        }
      }
    }
    alias: "number"
  }
  grouping {
    type: IDENT
    identifier {
      name: "age"
    }
  }
}

#@<OUT> creates statement with aggregate function to test having()
+-----+--------+
| age | number |
+-----+--------+
|  18 |      2 |
|  17 |      1 |
+-----+--------+
2 rows in set ([[*]] sec)

#@<PROTOCOL> prepares statement with aggregate function to test having()
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 8
  stmt {
    type: FIND
    find {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      projection {
        source {
          type: IDENT
          identifier {
            name: "age"
          }
        }
      }
      projection {
        source {
          type: FUNC_CALL
          function_call {
            name {
              name: "count"
            }
            param {
              type: IDENT
              identifier {
                name: "age"
              }
            }
          }
        }
        alias: "number"
      }
      grouping {
        type: IDENT
        identifier {
          name: "age"
        }
      }
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 8
}

#@<OUT> prepares statement with aggregate function to test having()
+-----+--------+
| age | number |
+-----+--------+
|  18 |      2 |
|  17 |      1 |
+-----+--------+
2 rows in set ([[*]] sec)

#@<PROTOCOL> having() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 8
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  projection {
    source {
      type: IDENT
      identifier {
        name: "age"
      }
    }
  }
  projection {
    source {
      type: FUNC_CALL
      function_call {
        name {
          name: "count"
        }
        param {
          type: IDENT
          identifier {
            name: "age"
          }
        }
      }
    }
    alias: "number"
  }
  grouping {
    type: IDENT
    identifier {
      name: "age"
    }
  }
  grouping_criteria {
    type: OPERATOR
    operator {
      name: ">"
      param {
        type: IDENT
        identifier {
          name: "number"
        }
      }
      param {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 1
        }
      }
    }
  }
}

#@<OUT> having() changes statement, back to normal execution
+-----+--------+
| age | number |
+-----+--------+
|  18 |      2 |
+-----+--------+
1 row in set ([[*]] sec)

#@<PROTOCOL> second execution after having(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 9
  stmt {
    type: FIND
    find {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      projection {
        source {
          type: IDENT
          identifier {
            name: "age"
          }
        }
      }
      projection {
        source {
          type: FUNC_CALL
          function_call {
            name {
              name: "count"
            }
            param {
              type: IDENT
              identifier {
                name: "age"
              }
            }
          }
        }
        alias: "number"
      }
      grouping {
        type: IDENT
        identifier {
          name: "age"
        }
      }
      grouping_criteria {
        type: OPERATOR
        operator {
          name: ">"
          param {
            type: IDENT
            identifier {
              name: "number"
            }
          }
          param {
            type: LITERAL
            literal {
              type: V_UINT
              v_unsigned_int: 1
            }
          }
        }
      }
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 9
}

#@<OUT> second execution after having(), prepares statement and executes it
+-----+--------+
| age | number |
+-----+--------+
|  18 |      2 |
+-----+--------+
1 row in set ([[*]] sec)

#@<PROTOCOL> third execution after having(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 9
}

#@<OUT> third execution after having(), uses prepared statement
+-----+--------+
| age | number |
+-----+--------+
|  18 |      2 |
+-----+--------+
1 row in set ([[*]] sec)

#@<PROTOCOL> creates statement to test no changes when reusing bind(), limit() and offset()
>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  criteria {
    type: OPERATOR
    operator {
      name: "like"
      param {
        type: IDENT
        identifier {
          name: "name"
        }
      }
      param {
        type: PLACEHOLDER
        position: 0
      }
    }
  }
  limit {
    row_count: 3
    offset: 0
  }
  args {
    type: V_STRING
    v_string {
      value: "%"
    }
  }
}

#@<OUT> creates statement to test no changes when reusing bind(), limit() and offset()
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  2 | james  |  17 |
|  3 | luke   |  18 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> prepares statement to test no changes when reusing bind(), limit() and offset()
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 10
  stmt {
    type: FIND
    find {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      criteria {
        type: OPERATOR
        operator {
          name: "like"
          param {
            type: IDENT
            identifier {
              name: "name"
            }
          }
          param {
            type: PLACEHOLDER
            position: 0
          }
        }
      }
      limit_expr {
        row_count {
          type: PLACEHOLDER
          position: 1
        }
        offset {
          type: PLACEHOLDER
          position: 2
        }
      }
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 10
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "%"
      }
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 3
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 0
    }
  }
}

#@<OUT> prepares statement to test no changes when reusing bind(), limit() and offset()
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  2 | james  |  17 |
|  3 | luke   |  18 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with bind() using g%
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 10
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "g%"
      }
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 3
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 0
    }
  }
}

#@<OUT> Reusing statement with bind() using g%
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
+----+--------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with bind() using j%
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 10
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "j%"
      }
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 3
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 0
    }
  }
}

#@<OUT> Reusing statement with bind() using j%
+----+-------+-----+
| id | name  | age |
+----+-------+-----+
|  2 | james |  17 |
+----+-------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with bind() using l%
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 10
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "l%"
      }
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 3
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 0
    }
  }
}

#@<OUT> Reusing statement with bind() using l%
+----+------+-----+
| id | name | age |
+----+------+-----+
|  3 | luke |  18 |
+----+------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with new limit()
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 10
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "%"
      }
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 0
    }
  }
}

#@<OUT> Reusing statement with new limit()
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
+----+--------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with new offset()
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 10
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "%"
      }
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
}

#@<OUT> Reusing statement with new offset()
+----+-------+-----+
| id | name  | age |
+----+-------+-----+
|  2 | james |  17 |
+----+-------+-----+
1 row in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with new limit() and offset()
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 10
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "%"
      }
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 2
    }
  }
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
}

#@<OUT> Reusing statement with new limit() and offset()
+----+-------+-----+
| id | name  | age |
+----+-------+-----+
|  2 | james |  17 |
|  3 | luke  |  18 |
+----+-------+-----+
2 rows in set ([[*]] sec)
