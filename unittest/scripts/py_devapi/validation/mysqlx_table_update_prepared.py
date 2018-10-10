#@<PROTOCOL> First execution is normal
>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  operation {
    source {
      name: "age"
    }
    operation: SET
    value {
      type: OPERATOR
      operator {
        name: "+"
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
            v_unsigned_int: 1
          }
        }
      }
    }
  }
}

#@<OUT> First execution is normal
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  19 |
|  2 | james  |  18 |
|  3 | luke   |  19 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> Second execution prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 1
  stmt {
    type: UPDATE
    update {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      operation {
        source {
          name: "age"
        }
        operation: SET
        value {
          type: OPERATOR
          operator {
            name: "+"
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
                v_unsigned_int: 1
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
  stmt_id: 1
}

#@<OUT> Second execution prepares statement and executes it
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  20 |
|  2 | james  |  19 |
|  3 | luke   |  20 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> Third execution uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 1
}

#@<OUT> Third execution uses prepared statement
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | george |  21 |
|  2 | james  |  20 |
|  3 | luke   |  21 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> set() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 1
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_table"
    schema: "prepared_stmt"
  }
  data_model: TABLE
  operation {
    source {
      name: "age"
    }
    operation: SET
    value {
      type: OPERATOR
      operator {
        name: "+"
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
            v_unsigned_int: 1
          }
        }
      }
    }
  }
  operation {
    source {
      name: "name"
    }
    operation: SET
    value {
      type: FUNC_CALL
      function_call {
        name {
          name: "concat"
        }
        param {
          type: FUNC_CALL
          function_call {
            name {
              name: "ucase"
            }
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "left"
                }
                param {
                  type: IDENT
                  identifier {
                    name: "name"
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
        param {
          type: FUNC_CALL
          function_call {
            name {
              name: "substring"
            }
            param {
              type: IDENT
              identifier {
                name: "name"
              }
            }
            param {
              type: LITERAL
              literal {
                type: V_UINT
                v_unsigned_int: 2
              }
            }
          }
        }
      }
    }
  }
}

#@<OUT> set() changes statement, back to normal execution
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  22 |
|  2 | James  |  21 |
|  3 | Luke   |  22 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> second execution after set(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 2
  stmt {
    type: UPDATE
    update {
      collection {
        name: "test_table"
        schema: "prepared_stmt"
      }
      data_model: TABLE
      operation {
        source {
          name: "age"
        }
        operation: SET
        value {
          type: OPERATOR
          operator {
            name: "+"
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
                v_unsigned_int: 1
              }
            }
          }
        }
      }
      operation {
        source {
          name: "name"
        }
        operation: SET
        value {
          type: FUNC_CALL
          function_call {
            name {
              name: "concat"
            }
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "ucase"
                }
                param {
                  type: FUNC_CALL
                  function_call {
                    name {
                      name: "left"
                    }
                    param {
                      type: IDENT
                      identifier {
                        name: "name"
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
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "substring"
                }
                param {
                  type: IDENT
                  identifier {
                    name: "name"
                  }
                }
                param {
                  type: LITERAL
                  literal {
                    type: V_UINT
                    v_unsigned_int: 2
                  }
                }
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

#@<OUT> second execution after set(), prepares statement and executes it
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  23 |
|  2 | James  |  22 |
|  3 | Luke   |  23 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> third execution after set(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 2
}

#@<OUT> third execution after set(), uses prepared statement
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  24 |
|  2 | James  |  23 |
|  3 | Luke   |  24 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> where() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 2
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
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
  operation {
    source {
      name: "age"
    }
    operation: SET
    value {
      type: OPERATOR
      operator {
        name: "+"
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
            v_unsigned_int: 1
          }
        }
      }
    }
  }
  operation {
    source {
      name: "name"
    }
    operation: SET
    value {
      type: FUNC_CALL
      function_call {
        name {
          name: "concat"
        }
        param {
          type: FUNC_CALL
          function_call {
            name {
              name: "ucase"
            }
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "left"
                }
                param {
                  type: IDENT
                  identifier {
                    name: "name"
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
        param {
          type: FUNC_CALL
          function_call {
            name {
              name: "substring"
            }
            param {
              type: IDENT
              identifier {
                name: "name"
              }
            }
            param {
              type: LITERAL
              literal {
                type: V_UINT
                v_unsigned_int: 2
              }
            }
          }
        }
      }
    }
  }
}

#@<OUT> where() changes statement, back to normal execution
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  24 |
|  2 | James  |  24 |
|  3 | Luke   |  24 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> second execution after where(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 3
  stmt {
    type: UPDATE
    update {
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
      operation {
        source {
          name: "age"
        }
        operation: SET
        value {
          type: OPERATOR
          operator {
            name: "+"
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
                v_unsigned_int: 1
              }
            }
          }
        }
      }
      operation {
        source {
          name: "name"
        }
        operation: SET
        value {
          type: FUNC_CALL
          function_call {
            name {
              name: "concat"
            }
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "ucase"
                }
                param {
                  type: FUNC_CALL
                  function_call {
                    name {
                      name: "left"
                    }
                    param {
                      type: IDENT
                      identifier {
                        name: "name"
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
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "substring"
                }
                param {
                  type: IDENT
                  identifier {
                    name: "name"
                  }
                }
                param {
                  type: LITERAL
                  literal {
                    type: V_UINT
                    v_unsigned_int: 2
                  }
                }
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
  stmt_id: 3
}

#@<OUT> second execution after where(), prepares statement and executes it
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  24 |
|  2 | James  |  25 |
|  3 | Luke   |  24 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> third execution after where(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 3
}

#@<OUT> third execution after where(), uses prepared statement
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  24 |
|  2 | James  |  26 |
|  3 | Luke   |  24 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> order_by() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 3
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
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
  operation {
    source {
      name: "age"
    }
    operation: SET
    value {
      type: OPERATOR
      operator {
        name: "+"
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
            v_unsigned_int: 1
          }
        }
      }
    }
  }
  operation {
    source {
      name: "name"
    }
    operation: SET
    value {
      type: FUNC_CALL
      function_call {
        name {
          name: "concat"
        }
        param {
          type: FUNC_CALL
          function_call {
            name {
              name: "ucase"
            }
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "left"
                }
                param {
                  type: IDENT
                  identifier {
                    name: "name"
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
        param {
          type: FUNC_CALL
          function_call {
            name {
              name: "substring"
            }
            param {
              type: IDENT
              identifier {
                name: "name"
              }
            }
            param {
              type: LITERAL
              literal {
                type: V_UINT
                v_unsigned_int: 2
              }
            }
          }
        }
      }
    }
  }
}

#@<OUT> order_by() changes statement, back to normal execution
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  24 |
|  2 | James  |  27 |
|  3 | Luke   |  24 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> second execution after order_by(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 4
  stmt {
    type: UPDATE
    update {
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
      operation {
        source {
          name: "age"
        }
        operation: SET
        value {
          type: OPERATOR
          operator {
            name: "+"
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
                v_unsigned_int: 1
              }
            }
          }
        }
      }
      operation {
        source {
          name: "name"
        }
        operation: SET
        value {
          type: FUNC_CALL
          function_call {
            name {
              name: "concat"
            }
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "ucase"
                }
                param {
                  type: FUNC_CALL
                  function_call {
                    name {
                      name: "left"
                    }
                    param {
                      type: IDENT
                      identifier {
                        name: "name"
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
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "substring"
                }
                param {
                  type: IDENT
                  identifier {
                    name: "name"
                  }
                }
                param {
                  type: LITERAL
                  literal {
                    type: V_UINT
                    v_unsigned_int: 2
                  }
                }
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
  stmt_id: 4
}

#@<OUT> second execution after order_by(), prepares statement and executes it
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  24 |
|  2 | James  |  28 |
|  3 | Luke   |  24 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> third execution after order_by(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
}

#@<OUT> third execution after order_by(), uses prepared statement
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  24 |
|  2 | James  |  29 |
|  3 | Luke   |  24 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> limit() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 4
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
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
  operation {
    source {
      name: "age"
    }
    operation: SET
    value {
      type: OPERATOR
      operator {
        name: "+"
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
            v_unsigned_int: 1
          }
        }
      }
    }
  }
  operation {
    source {
      name: "name"
    }
    operation: SET
    value {
      type: FUNC_CALL
      function_call {
        name {
          name: "concat"
        }
        param {
          type: FUNC_CALL
          function_call {
            name {
              name: "ucase"
            }
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "left"
                }
                param {
                  type: IDENT
                  identifier {
                    name: "name"
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
        param {
          type: FUNC_CALL
          function_call {
            name {
              name: "substring"
            }
            param {
              type: IDENT
              identifier {
                name: "name"
              }
            }
            param {
              type: LITERAL
              literal {
                type: V_UINT
                v_unsigned_int: 2
              }
            }
          }
        }
      }
    }
  }
}

#@<OUT> limit() changes statement, back to normal execution
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  24 |
|  2 | James  |  30 |
|  3 | Luke   |  24 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> second execution after limit(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 5
  stmt {
    type: UPDATE
    update {
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
      operation {
        source {
          name: "age"
        }
        operation: SET
        value {
          type: OPERATOR
          operator {
            name: "+"
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
                v_unsigned_int: 1
              }
            }
          }
        }
      }
      operation {
        source {
          name: "name"
        }
        operation: SET
        value {
          type: FUNC_CALL
          function_call {
            name {
              name: "concat"
            }
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "ucase"
                }
                param {
                  type: FUNC_CALL
                  function_call {
                    name {
                      name: "left"
                    }
                    param {
                      type: IDENT
                      identifier {
                        name: "name"
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
            param {
              type: FUNC_CALL
              function_call {
                name {
                  name: "substring"
                }
                param {
                  type: IDENT
                  identifier {
                    name: "name"
                  }
                }
                param {
                  type: LITERAL
                  literal {
                    type: V_UINT
                    v_unsigned_int: 2
                  }
                }
              }
            }
          }
        }
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
  stmt_id: 5
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
}

#@<OUT> second execution after limit(), prepares statement and executes it
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  24 |
|  2 | James  |  31 |
|  3 | Luke   |  24 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> third execution after limit(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 5
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
}

#@<OUT> third execution after limit(), uses prepared statement
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  24 |
|  2 | James  |  32 |
|  3 | Luke   |  24 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> creates statement to test no changes when reusing bind(), limit() and offset()
>>>> SEND Mysqlx.Crud.Update {
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
  operation {
    source {
      name: "age"
    }
    operation: SET
    value {
      type: OPERATOR
      operator {
        name: "+"
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
            v_unsigned_int: 1
          }
        }
      }
    }
  }
  args {
    type: V_STRING
    v_string {
      value: "%"
    }
  }
}

#@<OUT> creates statement to test no changes when reusing bind(), limit() and offset()
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  25 |
|  2 | James  |  33 |
|  3 | Luke   |  25 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> prepares statement to test no changes when reusing bind(), limit() and offset()
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 6
  stmt {
    type: UPDATE
    update {
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
      operation {
        source {
          name: "age"
        }
        operation: SET
        value {
          type: OPERATOR
          operator {
            name: "+"
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
                v_unsigned_int: 1
              }
            }
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
  stmt_id: 6
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

#@<OUT> prepares statement to test no changes when reusing bind(), limit() and offset()
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  26 |
|  2 | James  |  34 |
|  3 | Luke   |  26 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with bind() using g%
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 6
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

#@<OUT> Reusing statement with bind() using g%
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  27 |
|  2 | James  |  34 |
|  3 | Luke   |  26 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with bind() using j%
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 6
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

#@<OUT> Reusing statement with bind() using j%
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  27 |
|  2 | James  |  35 |
|  3 | Luke   |  26 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with bind() using l%
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 6
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

#@<OUT> Reusing statement with bind() using l%
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  27 |
|  2 | James  |  35 |
|  3 | Luke   |  27 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with new limit()
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 6
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

#@<OUT> Reusing statement with new limit()
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  28 |
|  2 | James  |  35 |
|  3 | Luke   |  27 |
+----+--------+-----+
3 rows in set ([[*]] sec)

#@<PROTOCOL> Reusing statement with new limit() and offset()
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 6
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

#@<OUT> Reusing statement with new limit() and offset()
Query OK, 2 items affected ([[*]] sec)

Rows matched: 2  Changed: 2  Warnings: 0
+----+--------+-----+
| id | name   | age |
+----+--------+-----+
|  1 | George |  29 |
|  2 | James  |  36 |
|  3 | Luke   |  27 |
+----+--------+-----+
3 rows in set ([[*]] sec)
