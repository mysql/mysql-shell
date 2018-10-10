//@<PROTOCOL> First execution is normal
>>>> SEND Mysqlx.Crud.Delete {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
}

//@<OUT> First execution is normal
Query OK, 3 items affected ([[*]] sec)
+----+------+-----+
| id | name | age |
+----+------+-----+
+----+------+-----+
Empty set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> Second execution prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 1
  stmt {
    type: DELETE
    delete {
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

//@<OUT> Second execution prepares statement and executes it
Query OK, 3 items affected ([[*]] sec)
+----+------+-----+
| id | name | age |
+----+------+-----+
+----+------+-----+
Empty set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> Third execution uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 1
}

//@<OUT> Third execution uses prepared statement
Query OK, 3 items affected ([[*]] sec)
+----+------+-----+
| id | name | age |
+----+------+-----+
+----+------+-----+
Empty set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> where() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 1
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Delete {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  criteria {
    type: OPERATOR
    operator {
      name: "=="
      param {
        type: IDENT
        identifier {
          name: "name"
        }
      }
      param {
        type: LITERAL
        literal {
          type: V_OCTETS
          v_octets {
            value: "james"
          }
        }
      }
    }
  }
}

//@<OUT> where() changes statement, back to normal execution
Query OK, 1 item affected ([[*]] sec)
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> second execution after where(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 2
  stmt {
    type: DELETE
    delete {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      criteria {
        type: OPERATOR
        operator {
          name: "=="
          param {
            type: IDENT
            identifier {
              name: "name"
            }
          }
          param {
            type: LITERAL
            literal {
              type: V_OCTETS
              v_octets {
                value: "james"
              }
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

//@<OUT> second execution after where(), prepares statement and executes it
Query OK, 1 item affected ([[*]] sec)
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> third execution after where(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 2
}

//@<OUT> third execution after where(), uses prepared statement
Query OK, 1 item affected ([[*]] sec)
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> orderBy() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 2
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Delete {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  criteria {
    type: OPERATOR
    operator {
      name: "=="
      param {
        type: IDENT
        identifier {
          name: "name"
        }
      }
      param {
        type: LITERAL
        literal {
          type: V_OCTETS
          v_octets {
            value: "james"
          }
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

//@<OUT> orderBy() changes statement, back to normal execution
Query OK, 1 item affected ([[*]] sec)
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> second execution after orderBy(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 3
  stmt {
    type: DELETE
    delete {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      criteria {
        type: OPERATOR
        operator {
          name: "=="
          param {
            type: IDENT
            identifier {
              name: "name"
            }
          }
          param {
            type: LITERAL
            literal {
              type: V_OCTETS
              v_octets {
                value: "james"
              }
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

//@<OUT> second execution after orderBy(), prepares statement and executes it
Query OK, 1 item affected ([[*]] sec)
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> third execution after orderBy(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 3
}

//@<OUT> third execution after orderBy(), uses prepared statement
Query OK, 1 item affected ([[*]] sec)
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> limit() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 3
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Delete {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  criteria {
    type: OPERATOR
    operator {
      name: "=="
      param {
        type: IDENT
        identifier {
          name: "name"
        }
      }
      param {
        type: LITERAL
        literal {
          type: V_OCTETS
          v_octets {
            value: "james"
          }
        }
      }
    }
  }
  limit {
    row_count: 1
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

//@<OUT> limit() changes statement, back to normal execution
Query OK, 1 item affected ([[*]] sec)
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> second execution after limit(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 4
  stmt {
    type: DELETE
    delete {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      criteria {
        type: OPERATOR
        operator {
          name: "=="
          param {
            type: IDENT
            identifier {
              name: "name"
            }
          }
          param {
            type: LITERAL
            literal {
              type: V_OCTETS
              v_octets {
                value: "james"
              }
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
}

//@<OUT> second execution after limit(), prepares statement and executes it
Query OK, 1 item affected ([[*]] sec)
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> third execution after limit(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
}

//@<OUT> third execution after limit(), uses prepared statement
Query OK, 1 item affected ([[*]] sec)
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
|  3 | luke   |  18 |
+----+--------+-----+
2 rows in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> creates statement to test no changes when reusing bind(), limit() and offset()
>>>> SEND Mysqlx.Crud.Delete {
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
  }
  args {
    type: V_STRING
    v_string {
      value: "%"
    }
  }
}

//@<OUT> creates statement to test no changes when reusing bind(), limit() and offset()
Query OK, 3 items affected ([[*]] sec)
+----+------+-----+
| id | name | age |
+----+------+-----+
+----+------+-----+
Empty set ([[*]] sec)


//@<PROTOCOL> prepares statement to test no changes when reusing bind(), limit() and offset()
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 5
  stmt {
    type: DELETE
    delete {
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
}

//@<OUT> prepares statement to test no changes when reusing bind(), limit() and offset()
Query OK, 3 items affected ([[*]] sec)
+----+------+-----+
| id | name | age |
+----+------+-----+
+----+------+-----+
Empty set ([[*]] sec)

//@<PROTOCOL> Reusing statement with bind() using g%
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 5
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
}

//@<OUT> Reusing statement with bind() using g%
Query OK, 1 item affected ([[*]] sec)
+----+-------+-----+
| id | name  | age |
+----+-------+-----+
|  2 | james |  17 |
|  3 | luke  |  18 |
+----+-------+-----+
2 rows in set ([[*]] sec)

//@<PROTOCOL> Reusing statement with bind() using j%
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 5
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
}

//@<OUT> Reusing statement with bind() using j%
Query OK, 1 item affected ([[*]] sec)
+----+------+-----+
| id | name | age |
+----+------+-----+
|  3 | luke |  18 |
+----+------+-----+
1 row in set ([[*]] sec)

//@<PROTOCOL> Reusing statement with bind() using l%
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 5
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
}

//@<OUT> Reusing statement with bind() using l%
Query OK, 1 item affected ([[*]] sec)
+----+------+-----+
| id | name | age |
+----+------+-----+
+----+------+-----+
Empty set ([[*]] sec)

//@<PROTOCOL> Reusing statement with new limit()
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 5
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
}

//@<OUT> Reusing statement with new limit()
Query OK, 1 item affected ([[*]] sec)
+----+-------+-----+
| id | name  | age |
+----+-------+-----+
|  2 | james |  17 |
|  3 | luke  |  18 |
+----+-------+-----+
2 rows in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> Reusing statement with new limit() and offset()
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 5
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
}

//@<OUT> Reusing statement with new limit() and offset()
Query OK, 2 items affected ([[*]] sec)
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  18 |
+----+--------+-----+
1 row in set ([[*]] sec)
