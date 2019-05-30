//@<PROTOCOL> First execution is normal
>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
}

//@<OUT> First execution is normal
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> Second execution prepares statement and executes it
~>>>> SEND Mysqlx.Crud.Find {

//@<PROTOCOL> Second execution prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 1
  stmt {
    type: FIND
    find {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 1
}

//@<OUT> Second execution prepares statement and executes it
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> Third execution uses prepared statement
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> Third execution uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 1
}

//@<OUT> Third execution uses prepared statement
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> fields() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 1
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  projection {
    source {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "name"
        }
      }
    }
    alias: "name"
  }
  projection {
    source {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "age"
        }
      }
    }
    alias: "age"
  }
}

//@<OUT> fields() changes statement, back to normal execution
{
    "age": 18,
    "name": "george"
}
{
    "age": 17,
    "name": "james"
}
{
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> second execution after fields(), prepares statement and executes it
~>>>> SEND Mysqlx.Crud.Find {

//@<PROTOCOL> second execution after fields(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 2
  stmt {
    type: FIND
    find {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      projection {
        source {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "name"
            }
          }
        }
        alias: "name"
      }
      projection {
        source {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
        alias: "age"
      }
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 2
}


//@<OUT> second execution after fields(), prepares statement and executes it
{
    "age": 18,
    "name": "george"
}
{
    "age": 17,
    "name": "james"
}
{
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> third execution after fields(), uses prepared statement
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> third execution after fields(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 2
}

//@<OUT> third execution after fields(), uses prepared statement
{
    "age": 18,
    "name": "george"
}
{
    "age": 17,
    "name": "james"
}
{
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> sort() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 2
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  projection {
    source {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "name"
        }
      }
    }
    alias: "name"
  }
  projection {
    source {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "age"
        }
      }
    }
    alias: "age"
  }
  order {
    expr {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "age"
        }
      }
    }
    direction: ASC
  }
}

//@<OUT> sort() changes statement, back to normal execution
{
    "age": 17,
    "name": "james"
}
{
    "age": 18,
    "name": "george"
}
{
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> second execution after sort(), prepares statement and executes it
~>>>> SEND Mysqlx.Crud.Find {

//@<PROTOCOL> second execution after sort(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 3
  stmt {
    type: FIND
    find {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      projection {
        source {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "name"
            }
          }
        }
        alias: "name"
      }
      projection {
        source {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
        alias: "age"
      }
      order {
        expr {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
        direction: ASC
      }
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 3
}

//@<OUT> second execution after sort(), prepares statement and executes it
{
    "age": 17,
    "name": "james"
}
{
    "age": 18,
    "name": "george"
}
{
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> third execution after sort(), uses prepared statement
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> third execution after sort(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 3
}

//@<OUT> third execution after sort(), uses prepared statement
{
    "age": 17,
    "name": "james"
}
{
    "age": 18,
    "name": "george"
}
{
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> limit() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 3
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  projection {
    source {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "name"
        }
      }
    }
    alias: "name"
  }
  projection {
    source {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "age"
        }
      }
    }
    alias: "age"
  }
  limit {
    row_count: 2
    offset: 0
  }
  order {
    expr {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "age"
        }
      }
    }
    direction: ASC
  }
}


//@<OUT> limit() changes statement, back to normal execution
{
    "age": 17,
    "name": "james"
}
{
    "age": 18,
    "name": "george"
}
2 documents in set ([[*]] sec)

//@<PROTOCOL> second execution after limit(), prepares statement and executes it
~>>>> SEND Mysqlx.Crud.Find {

//@<PROTOCOL> second execution after limit(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 4
  stmt {
    type: FIND
    find {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      projection {
        source {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "name"
            }
          }
        }
        alias: "name"
      }
      projection {
        source {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
        alias: "age"
      }
      order {
        expr {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
        direction: ASC
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
      v_unsigned_int: 2
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

//@<OUT> second execution after limit(), prepares statement and executes it
{
    "age": 17,
    "name": "james"
}
{
    "age": 18,
    "name": "george"
}
2 documents in set ([[*]] sec)

//@<PROTOCOL> third execution after limit(), uses prepared statement
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> third execution after limit(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
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
      v_unsigned_int: 0
    }
  }
}

//@<OUT> third execution after limit(), uses prepared statement
{
    "age": 17,
    "name": "james"
}
{
    "age": 18,
    "name": "george"
}
2 documents in set ([[*]] sec)

//@<PROTOCOL> offset() does not change the statement, uses prepared one
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
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

//@<OUT> offset() does not change the statement, uses prepared one
{
    "age": 18,
    "name": "george"
}
{
    "age": 18,
    "name": "luke"
}
2 documents in set ([[*]] sec)

//@<PROTOCOL> lockExclusive() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 4
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  projection {
    source {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "name"
        }
      }
    }
    alias: "name"
  }
  projection {
    source {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "age"
        }
      }
    }
    alias: "age"
  }
  limit {
    row_count: 2
    offset: 1
  }
  order {
    expr {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "age"
        }
      }
    }
    direction: ASC
  }
  locking: EXCLUSIVE_LOCK
}


//@<OUT> lockExclusive() changes statement, back to normal execution
{
    "age": 18,
    "name": "george"
}
{
    "age": 18,
    "name": "luke"
}
2 documents in set ([[*]] sec)

//@<PROTOCOL> second execution after lockExclusive(), prepares statement and executes it
~>>>> SEND Mysqlx.Crud.Find {

//@<PROTOCOL> second execution after lockExclusive(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 5
  stmt {
    type: FIND
    find {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      projection {
        source {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "name"
            }
          }
        }
        alias: "name"
      }
      projection {
        source {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
        alias: "age"
      }
      order {
        expr {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
        direction: ASC
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

//@<OUT> second execution after lockExclusive(), prepares statement and executes it
{
    "age": 18,
    "name": "george"
}
{
    "age": 18,
    "name": "luke"
}
2 documents in set ([[*]] sec)


//@<PROTOCOL> third execution after lockExclusive(), uses prepared statement
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> third execution after lockExclusive(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 5
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

//@<OUT> third execution after lockExclusive(), uses prepared statement
{
    "age": 18,
    "name": "george"
}
{
    "age": 18,
    "name": "luke"
}
2 documents in set ([[*]] sec)

//@<PROTOCOL> creates statement to test lockShared()
>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
}

//@<OUT> creates statement to test lockShared()
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> prepares statement to test lockShared()
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 6
  stmt {
    type: FIND
    find {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 6
}

//@<OUT> prepares statement to test lockShared()
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> lockShared() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 6
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  locking: SHARED_LOCK
}

//@<OUT> lockShared() changes statement, back to normal execution
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> second execution after lockShared(), prepares statement and executes it
~>>>> SEND Mysqlx.Crud.Find {

//@<PROTOCOL> second execution after lockShared(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 7
  stmt {
    type: FIND
    find {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      locking: SHARED_LOCK
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 7
}

//@<OUT> second execution after lockShared(), prepares statement and executes it
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> third execution after lockShared(), uses prepared statement
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> third execution after lockShared(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 7
}

//@<OUT> third execution after lockShared(), uses prepared statement
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> creates statement with aggregate function to test having()
>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  projection {
    source {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "age"
        }
      }
    }
    alias: "age"
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
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
      }
    }
    alias: "count(age)"
  }
  grouping {
    type: IDENT
    identifier {
      document_path {
        type: MEMBER
        value: "age"
      }
    }
  }
}

//@<OUT> creates statement with aggregate function to test having()
{
    "age": 18,
    "count(age)": 2
}
{
    "age": 17,
    "count(age)": 1
}
2 documents in set ([[*]] sec)

//@<PROTOCOL> prepares statement with aggregate function to test having()
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 8
  stmt {
    type: FIND
    find {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      projection {
        source {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
        alias: "age"
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
                document_path {
                  type: MEMBER
                  value: "age"
                }
              }
            }
          }
        }
        alias: "count(age)"
      }
      grouping {
        type: IDENT
        identifier {
          document_path {
            type: MEMBER
            value: "age"
          }
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

//@<OUT> prepares statement with aggregate function to test having()
{
    "age": 18,
    "count(age)": 2
}
{
    "age": 17,
    "count(age)": 1
}
2 documents in set ([[*]] sec)

//@<PROTOCOL> having() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 8
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  projection {
    source {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "age"
        }
      }
    }
    alias: "age"
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
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
      }
    }
    alias: "count(age)"
  }
  grouping {
    type: IDENT
    identifier {
      document_path {
        type: MEMBER
        value: "age"
      }
    }
  }
  grouping_criteria {
    type: OPERATOR
    operator {
      name: ">"
      param {
        type: FUNC_CALL
        function_call {
          name {
            name: "count"
          }
          param {
            type: IDENT
            identifier {
              document_path {
                type: MEMBER
                value: "age"
              }
            }
          }
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


//@<OUT> having() changes statement, back to normal execution
{
    "age": 18,
    "count(age)": 2
}
1 document in set ([[*]] sec)

//@<PROTOCOL> second execution after having(), prepares statement and executes it
~>>>> SEND Mysqlx.Crud.Find {

//@<PROTOCOL> second execution after having(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 9
  stmt {
    type: FIND
    find {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      projection {
        source {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "age"
            }
          }
        }
        alias: "age"
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
                document_path {
                  type: MEMBER
                  value: "age"
                }
              }
            }
          }
        }
        alias: "count(age)"
      }
      grouping {
        type: IDENT
        identifier {
          document_path {
            type: MEMBER
            value: "age"
          }
        }
      }
      grouping_criteria {
        type: OPERATOR
        operator {
          name: ">"
          param {
            type: FUNC_CALL
            function_call {
              name {
                name: "count"
              }
              param {
                type: IDENT
                identifier {
                  document_path {
                    type: MEMBER
                    value: "age"
                  }
                }
              }
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

//@<OUT> second execution after having(), prepares statement and executes it
{
    "age": 18,
    "count(age)": 2
}
1 document in set ([[*]] sec)

//@<PROTOCOL> third execution after having(), uses prepared statement
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> third execution after having(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 9
}

//@<OUT> third execution after having(), uses prepared statement
{
    "age": 18,
    "count(age)": 2
}
1 document in set ([[*]] sec)

//@<PROTOCOL> creates statement to test no changes when reusing bind(), limit() and offset()
>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: OPERATOR
    operator {
      name: "like"
      param {
        type: IDENT
        identifier {
          document_path {
            type: MEMBER
            value: "name"
          }
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


//@<OUT> creates statement to test no changes when reusing bind(), limit() and offset()
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> prepares statement to test no changes when reusing bind(), limit() and offset()
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 10
  stmt {
    type: FIND
    find {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      criteria {
        type: OPERATOR
        operator {
          name: "like"
          param {
            type: IDENT
            identifier {
              document_path {
                type: MEMBER
                value: "name"
              }
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

//@<OUT> prepares statement to test no changes when reusing bind(), limit() and offset()
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)


//@<PROTOCOL> Reusing statement with bind() using g%
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> Reusing statement with bind() using g%
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

//@<OUT> Reusing statement with bind() using g%
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
1 document in set ([[*]] sec)

//@<PROTOCOL> Reusing statement with bind() using j%
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> Reusing statement with bind() using j%
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

//@<OUT> Reusing statement with bind() using j%
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<OUT> Reusing statement with bind() using j%
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
1 document in set ([[*]] sec)

//@<PROTOCOL> Reusing statement with bind() using l%
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

//@<OUT> Reusing statement with bind() using l%
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
1 document in set ([[*]] sec)

//@<PROTOCOL> Reusing statement with new limit()
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> Reusing statement with new limit()
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

//@<OUT> Reusing statement with new limit()
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
1 document in set ([[*]] sec)

//@<PROTOCOL> Reusing statement with new offset()
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> Reusing statement with new offset()
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

//@<OUT> Reusing statement with new offset()
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
1 document in set ([[*]] sec)

//@<PROTOCOL> Reusing statement with new limit() and offset()
~>>>> SEND Mysqlx.Prepare.Prepare {

//@<PROTOCOL> Reusing statement with new limit() and offset()
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

//@<OUT> Reusing statement with new limit() and offset()
{
    "_id": "002",
    "age": 17,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
2 documents in set ([[*]] sec)
