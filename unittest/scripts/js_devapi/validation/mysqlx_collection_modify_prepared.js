//@<PROTOCOL> First execution is normal
>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: LITERAL
    literal {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grade"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_SINT
        v_signed_int: 1
      }
    }
  }
}

//@<OUT> First execution is normal
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "age": 18,
    "name": "george",
    "grade": 1
}
{
    "_id": "002",
    "age": 17,
    "name": "james",
    "grade": 1
}
{
    "_id": "003",
    "age": 18,
    "name": "luke",
    "grade": 1
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> Second execution prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 1
  stmt {
    type: UPDATE
    update {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      criteria {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 1
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grade"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_SINT
            v_signed_int: 1
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

//@<OUT> Second execution prepares statement and executes it
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "age": 18,
    "name": "george",
    "grade": 1
}
{
    "_id": "002",
    "age": 17,
    "name": "james",
    "grade": 1
}
{
    "_id": "003",
    "age": 18,
    "name": "luke",
    "grade": 1
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> Third execution uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 1
}

//@<OUT> Third execution uses prepared statement
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "age": 18,
    "name": "george",
    "grade": 1
}
{
    "_id": "002",
    "age": 17,
    "name": "james",
    "grade": 1
}
{
    "_id": "003",
    "age": 18,
    "name": "luke",
    "grade": 1
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> set() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 1
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: LITERAL
    literal {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grade"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_SINT
        v_signed_int: 1
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "group"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A"
        }
      }
    }
  }
}

//@<OUT> set() changes statement, back to normal execution
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "age": 18,
    "name": "george",
    "grade": 1,
    "group": "A"
}
{
    "_id": "002",
    "age": 17,
    "name": "james",
    "grade": 1,
    "group": "A"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke",
    "grade": 1,
    "group": "A"
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> second execution after set(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 2
  stmt {
    type: UPDATE
    update {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      criteria {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 1
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grade"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_SINT
            v_signed_int: 1
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "group"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A"
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

//@<OUT> second execution after set(), prepares statement and executes it
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "age": 18,
    "name": "george",
    "grade": 1,
    "group": "A"
}
{
    "_id": "002",
    "age": 17,
    "name": "james",
    "grade": 1,
    "group": "A"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke",
    "grade": 1,
    "group": "A"
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> third execution after set(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 2
}

//@<OUT> third execution after set(), uses prepared statement
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "age": 18,
    "name": "george",
    "grade": 1,
    "group": "A"
}
{
    "_id": "002",
    "age": 17,
    "name": "james",
    "grade": 1,
    "group": "A"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke",
    "grade": 1,
    "group": "A"
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> unset() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 2
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: LITERAL
    literal {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grade"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_SINT
        v_signed_int: 1
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "group"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A"
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "age"
      }
    }
    operation: ITEM_REMOVE
  }
}

//@<OUT> unset() changes statement, back to normal execution
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A"
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A"
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A"
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> second execution after unset(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 3
  stmt {
    type: UPDATE
    update {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      criteria {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 1
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grade"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_SINT
            v_signed_int: 1
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "group"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A"
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "age"
          }
        }
        operation: ITEM_REMOVE
      }
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 3
}

//@<OUT> second execution after unset(), prepares statement and executes it
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A"
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A"
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A"
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> third execution after unset(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 3
}

//@<OUT> third execution after unset(), uses prepared statement
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A"
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A"
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A"
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> patch() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 3
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: LITERAL
    literal {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grade"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_SINT
        v_signed_int: 1
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "group"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A"
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "age"
      }
    }
    operation: ITEM_REMOVE
  }
  operation {
    source {
    }
    operation: MERGE_PATCH
    value {
      type: OBJECT
      object {
        fld {
          key: "grades"
          value {
            type: ARRAY
            array {
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "A"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "B"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "C"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "D"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "E"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "F"
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

//@<OUT> patch() changes statement, back to normal execution
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> second execution after patch(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 4
  stmt {
    type: UPDATE
    update {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      criteria {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 1
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grade"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_SINT
            v_signed_int: 1
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "group"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A"
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "age"
          }
        }
        operation: ITEM_REMOVE
      }
      operation {
        source {
        }
        operation: MERGE_PATCH
        value {
          type: OBJECT
          object {
            fld {
              key: "grades"
              value {
                type: ARRAY
                array {
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "A"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "B"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "C"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "D"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "E"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "F"
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
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
}

//@<OUT> second execution after patch(), prepares statement and executes it
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> third execution after patch(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
}

//@<OUT> third execution after patch(), uses prepared statement
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> arrayInsert() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 4
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: LITERAL
    literal {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grade"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_SINT
        v_signed_int: 1
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "group"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A"
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "age"
      }
    }
    operation: ITEM_REMOVE
  }
  operation {
    source {
    }
    operation: MERGE_PATCH
    value {
      type: OBJECT
      object {
        fld {
          key: "grades"
          value {
            type: ARRAY
            array {
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "A"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "B"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "C"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "D"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "E"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "F"
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grades"
      }
      document_path {
        type: ARRAY_INDEX
        index: 1
      }
    }
    operation: ARRAY_INSERT
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A+"
        }
      }
    }
  }
}

//@<OUT> arrayInsert() changes statement, back to normal execution
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> second execution after arrayInsert(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 5
  stmt {
    type: UPDATE
    update {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      criteria {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 1
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grade"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_SINT
            v_signed_int: 1
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "group"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A"
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "age"
          }
        }
        operation: ITEM_REMOVE
      }
      operation {
        source {
        }
        operation: MERGE_PATCH
        value {
          type: OBJECT
          object {
            fld {
              key: "grades"
              value {
                type: ARRAY
                array {
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "A"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "B"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "C"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "D"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "E"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "F"
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grades"
          }
          document_path {
            type: ARRAY_INDEX
            index: 1
          }
        }
        operation: ARRAY_INSERT
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A+"
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
  stmt_id: 5
}

//@<OUT> second execution after arrayInsert(), prepares statement and executes it
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> third execution after arrayInsert(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 5
}

//@<OUT> third execution after arrayInsert(), uses prepared statement
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> arrayAppend() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 5
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: LITERAL
    literal {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grade"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_SINT
        v_signed_int: 1
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "group"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A"
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "age"
      }
    }
    operation: ITEM_REMOVE
  }
  operation {
    source {
    }
    operation: MERGE_PATCH
    value {
      type: OBJECT
      object {
        fld {
          key: "grades"
          value {
            type: ARRAY
            array {
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "A"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "B"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "C"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "D"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "E"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "F"
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grades"
      }
      document_path {
        type: ARRAY_INDEX
        index: 1
      }
    }
    operation: ARRAY_INSERT
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A+"
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grades"
      }
    }
    operation: ARRAY_APPEND
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "G"
        }
      }
    }
  }
}

//@<OUT> arrayAppend() changes statement, back to normal execution
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> second execution after arrayAppend(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 6
  stmt {
    type: UPDATE
    update {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      criteria {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 1
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grade"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_SINT
            v_signed_int: 1
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "group"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A"
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "age"
          }
        }
        operation: ITEM_REMOVE
      }
      operation {
        source {
        }
        operation: MERGE_PATCH
        value {
          type: OBJECT
          object {
            fld {
              key: "grades"
              value {
                type: ARRAY
                array {
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "A"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "B"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "C"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "D"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "E"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "F"
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grades"
          }
          document_path {
            type: ARRAY_INDEX
            index: 1
          }
        }
        operation: ARRAY_INSERT
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A+"
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grades"
          }
        }
        operation: ARRAY_APPEND
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "G"
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
  stmt_id: 6
}

//@<OUT> second execution after arrayAppend(), prepares statement and executes it
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> third execution after arrayAppend(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 6
}

//@<OUT> third execution after arrayAppend(), uses prepared statement
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> sort() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 6
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: LITERAL
    literal {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  order {
    expr {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "name"
        }
      }
    }
    direction: DESC
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grade"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_SINT
        v_signed_int: 1
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "group"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A"
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "age"
      }
    }
    operation: ITEM_REMOVE
  }
  operation {
    source {
    }
    operation: MERGE_PATCH
    value {
      type: OBJECT
      object {
        fld {
          key: "grades"
          value {
            type: ARRAY
            array {
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "A"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "B"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "C"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "D"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "E"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "F"
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grades"
      }
      document_path {
        type: ARRAY_INDEX
        index: 1
      }
    }
    operation: ARRAY_INSERT
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A+"
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grades"
      }
    }
    operation: ARRAY_APPEND
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "G"
        }
      }
    }
  }
}

//@<OUT> sort() changes statement, back to normal execution
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> second execution after sort(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 7
  stmt {
    type: UPDATE
    update {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      criteria {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 1
        }
      }
      order {
        expr {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "name"
            }
          }
        }
        direction: DESC
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grade"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_SINT
            v_signed_int: 1
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "group"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A"
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "age"
          }
        }
        operation: ITEM_REMOVE
      }
      operation {
        source {
        }
        operation: MERGE_PATCH
        value {
          type: OBJECT
          object {
            fld {
              key: "grades"
              value {
                type: ARRAY
                array {
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "A"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "B"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "C"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "D"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "E"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "F"
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grades"
          }
          document_path {
            type: ARRAY_INDEX
            index: 1
          }
        }
        operation: ARRAY_INSERT
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A+"
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grades"
          }
        }
        operation: ARRAY_APPEND
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "G"
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
  stmt_id: 7
}

//@<OUT> second execution after sort(), prepares statement and executes it
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> third execution after sort(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 7
}

//@<OUT> third execution after sort(), uses prepared statement
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0
{
    "_id": "001",
    "name": "george",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 3 items affected ([[*]] sec)

Rows matched: 3  Changed: 3  Warnings: 0

//@<PROTOCOL> limit() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 7
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: LITERAL
    literal {
      type: V_UINT
      v_unsigned_int: 1
    }
  }
  limit {
    row_count: 2
  }
  order {
    expr {
      type: IDENT
      identifier {
        document_path {
          type: MEMBER
          value: "name"
        }
      }
    }
    direction: DESC
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grade"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_SINT
        v_signed_int: 1
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "group"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A"
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "age"
      }
    }
    operation: ITEM_REMOVE
  }
  operation {
    source {
    }
    operation: MERGE_PATCH
    value {
      type: OBJECT
      object {
        fld {
          key: "grades"
          value {
            type: ARRAY
            array {
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "A"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "B"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "C"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "D"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "E"
                  }
                }
              }
              value {
                type: LITERAL
                literal {
                  type: V_STRING
                  v_string {
                    value: "F"
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grades"
      }
      document_path {
        type: ARRAY_INDEX
        index: 1
      }
    }
    operation: ARRAY_INSERT
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "A+"
        }
      }
    }
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "grades"
      }
    }
    operation: ARRAY_APPEND
    value {
      type: LITERAL
      literal {
        type: V_STRING
        v_string {
          value: "G"
        }
      }
    }
  }
}


//@<OUT> limit() changes statement, back to normal execution
Query OK, 2 items affected ([[*]] sec)

Rows matched: 2  Changed: 2  Warnings: 0
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 2 items affected ([[*]] sec)

Rows matched: 3  Changed: 2  Warnings: 0

//@<PROTOCOL> second execution after limit(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 8
  stmt {
    type: UPDATE
    update {
      collection {
        name: "test_collection"
        schema: "prepared_stmt"
      }
      data_model: DOCUMENT
      criteria {
        type: LITERAL
        literal {
          type: V_UINT
          v_unsigned_int: 1
        }
      }
      order {
        expr {
          type: IDENT
          identifier {
            document_path {
              type: MEMBER
              value: "name"
            }
          }
        }
        direction: DESC
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grade"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_SINT
            v_signed_int: 1
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "group"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A"
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "age"
          }
        }
        operation: ITEM_REMOVE
      }
      operation {
        source {
        }
        operation: MERGE_PATCH
        value {
          type: OBJECT
          object {
            fld {
              key: "grades"
              value {
                type: ARRAY
                array {
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "A"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "B"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "C"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "D"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "E"
                      }
                    }
                  }
                  value {
                    type: LITERAL
                    literal {
                      type: V_STRING
                      v_string {
                        value: "F"
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grades"
          }
          document_path {
            type: ARRAY_INDEX
            index: 1
          }
        }
        operation: ARRAY_INSERT
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "A+"
            }
          }
        }
      }
      operation {
        source {
          document_path {
            type: MEMBER
            value: "grades"
          }
        }
        operation: ARRAY_APPEND
        value {
          type: LITERAL
          literal {
            type: V_STRING
            v_string {
              value: "G"
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
  stmt_id: 8
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 2
    }
  }
}

//@<OUT> second execution after limit(), prepares statement and executes it
Query OK, 2 items affected ([[*]] sec)

Rows matched: 2  Changed: 2  Warnings: 0
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 2 items affected ([[*]] sec)

Rows matched: 3  Changed: 2  Warnings: 0

//@<PROTOCOL> third execution after limit(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 8
  args {
    type: SCALAR
    scalar {
      type: V_UINT
      v_unsigned_int: 2
    }
  }
}

//@<OUT> third execution after limit(), uses prepared statement
Query OK, 2 items affected ([[*]] sec)

Rows matched: 2  Changed: 2  Warnings: 0
{
    "_id": "001",
    "age": 18,
    "name": "george"
}
{
    "_id": "002",
    "name": "james",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
{
    "_id": "003",
    "name": "luke",
    "grade": 1,
    "group": "A",
    "grades": [
        "A",
        "A+",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G"
    ]
}
3 documents in set ([[*]] sec)
Query OK, 2 items affected ([[*]] sec)

Rows matched: 3  Changed: 2  Warnings: 0

//@<PROTOCOL> prepares statement to test no changes when reusing bind() and limit()
>>>> SEND Mysqlx.Crud.Update {
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
    row_count: 1
  }
  operation {
    source {
      document_path {
        type: MEMBER
        value: "age"
      }
    }
    operation: ITEM_SET
    value {
      type: LITERAL
      literal {
        type: V_SINT
        v_signed_int: 19
      }
    }
  }
  args {
    type: V_STRING
    v_string {
      value: "g%"
    }
  }
}


//@<OUT> prepares statement to test no changes when reusing bind() and limit()
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
{
    "_id": "001",
    "age": 19,
    "name": "george"
}
{
    "_id": "002",
    "age": 18,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> Reusing statement with bind() using j%
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 9
  stmt {
    type: UPDATE
    update {
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
      operation {
        source {
          document_path {
            type: MEMBER
            value: "age"
          }
        }
        operation: ITEM_SET
        value {
          type: LITERAL
          literal {
            type: V_SINT
            v_signed_int: 19
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
  stmt_id: 9
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
      v_unsigned_int: 1
    }
  }
}

//@<OUT> Reusing statement with bind() using j%
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
{
    "_id": "001",
    "age": 19,
    "name": "george"
}
{
    "_id": "002",
    "age": 19,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)

//@<PROTOCOL> Reusing statement with new limit()
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 9
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

//@<OUT> Reusing statement with new limit()
Query OK, 2 items affected ([[*]] sec)

Rows matched: 2  Changed: 0  Warnings: 0
{
    "_id": "001",
    "age": 19,
    "name": "george"
}
{
    "_id": "002",
    "age": 19,
    "name": "james"
}
{
    "_id": "003",
    "age": 18,
    "name": "luke"
}
3 documents in set ([[*]] sec)
