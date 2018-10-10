#@<PROTOCOL> First execution is normal
>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
}

#@<OUT> First execution is normal
[
    {
        "_id": "001",
        "age": 18,
        "name": "george"
    },
    {
        "_id": "002",
        "age": 17,
        "name": "james"
    },
    {
        "_id": "003",
        "age": 18,
        "name": "luke"
    },
    {
        "_id": "004",
        "age": 21,
        "name": "mark"
    }
]
4 documents in set ([[*]] sec)

#@<PROTOCOL> Second execution attempts preparing statement, disables prepared statements on session, executes normal
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

<<<< RECEIVE Mysqlx.Error {
  severity: ERROR
  code: 1047
  msg: "Unexpected message received"
  sql_state: "HY000"
}

>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
}

#@<OUT> Second execution attempts preparing statement, disables prepared statements on session, executes normal
[
    {
        "_id": "001",
        "age": 18,
        "name": "george"
    },
    {
        "_id": "002",
        "age": 17,
        "name": "james"
    },
    {
        "_id": "003",
        "age": 18,
        "name": "luke"
    },
    {
        "_id": "004",
        "age": 21,
        "name": "mark"
    }
]
4 documents in set ([[*]] sec)

#@<PROTOCOL> Third execution does normal execution
>>>> SEND Mysqlx.Crud.Find {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
}

#@<OUT> Third execution does normal execution
[
    {
        "_id": "001",
        "age": 18,
        "name": "george"
    },
    {
        "_id": "002",
        "age": 17,
        "name": "james"
    },
    {
        "_id": "003",
        "age": 18,
        "name": "luke"
    },
    {
        "_id": "004",
        "age": 21,
        "name": "mark"
    }
]
4 documents in set ([[*]] sec)

#@<PROTOCOL> remove() first call
>>>> SEND Mysqlx.Crud.Delete {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: OPERATOR
    operator {
      name: "=="
      param {
        type: IDENT
        identifier {
          document_path {
            type: MEMBER
            value: "_id"
          }
        }
      }
      param {
        type: PLACEHOLDER
        position: 0
      }
    }
  }
  args {
    type: V_STRING
    v_string {
      value: "004"
    }
  }
}

#@<OUT> remove() first call
Query OK, 1 item affected ([[*]] sec)
[
    {
        "_id": "001",
        "age": 18,
        "name": "george"
    },
    {
        "_id": "002",
        "age": 17,
        "name": "james"
    },
    {
        "_id": "003",
        "age": 18,
        "name": "luke"
    }
]
3 documents in set ([[*]] sec)

#@<PROTOCOL> remove() second call, no prepare
>>>> SEND Mysqlx.Crud.Delete {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: OPERATOR
    operator {
      name: "=="
      param {
        type: IDENT
        identifier {
          document_path {
            type: MEMBER
            value: "_id"
          }
        }
      }
      param {
        type: PLACEHOLDER
        position: 0
      }
    }
  }
  args {
    type: V_STRING
    v_string {
      value: "003"
    }
  }
}

#@<OUT> remove() second call, no prepare
Query OK, 1 item affected ([[*]] sec)
[
    {
        "_id": "001",
        "age": 18,
        "name": "george"
    },
    {
        "_id": "002",
        "age": 17,
        "name": "james"
    }
]
2 documents in set ([[*]] sec)

#@<PROTOCOL> modify() first call
>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: OPERATOR
    operator {
      name: "=="
      param {
        type: IDENT
        identifier {
          document_path {
            type: MEMBER
            value: "_id"
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
        v_signed_int: 20
      }
    }
  }
  args {
    type: V_STRING
    v_string {
      value: "001"
    }
  }
}

#@<OUT> modify() first call
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
[
    {
        "_id": "001",
        "age": 20,
        "name": "george"
    },
    {
        "_id": "002",
        "age": 17,
        "name": "james"
    }
]
2 documents in set ([[*]] sec)

#@<PROTOCOL> modify() second call, no prepare
>>>> SEND Mysqlx.Crud.Update {
  collection {
    name: "test_collection"
    schema: "prepared_stmt"
  }
  data_model: DOCUMENT
  criteria {
    type: OPERATOR
    operator {
      name: "=="
      param {
        type: IDENT
        identifier {
          document_path {
            type: MEMBER
            value: "_id"
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
        v_signed_int: 20
      }
    }
  }
  args {
    type: V_STRING
    v_string {
      value: "002"
    }
  }
}

#@<OUT> modify() second call, no prepare
Query OK, 1 item affected ([[*]] sec)

Rows matched: 1  Changed: 1  Warnings: 0
[
    {
        "_id": "001",
        "age": 20,
        "name": "george"
    },
    {
        "_id": "002",
        "age": 20,
        "name": "james"
    }
]
2 documents in set ([[*]] sec)

#@<PROTOCOL> sql() first call
>>>> SEND Mysqlx.Sql.StmtExecute {
  stmt: "select * from prepared_stmt.test_collection"
  namespace: "sql"
}

#@<OUT> sql() first call
+---------------------------------------------+-----+
| doc                                         | _id |
+---------------------------------------------+-----+
| {"_id": "001", "age": 20, "name": "george"} | 001 |
| {"_id": "002", "age": 20, "name": "james"}  | 002 |
+---------------------------------------------+-----+
2 rows in set ([[*]] sec)

#@<PROTOCOL> sql() second call, no prepare
>>>> SEND Mysqlx.Sql.StmtExecute {
  stmt: "select * from prepared_stmt.test_collection"
  namespace: "sql"
}

#@<OUT> sql() second call, no prepare
+---------------------------------------------+-----+
| doc                                         | _id |
+---------------------------------------------+-----+
| {"_id": "001", "age": 20, "name": "george"} | 001 |
| {"_id": "002", "age": 20, "name": "james"}  | 002 |
+---------------------------------------------+-----+
2 rows in set ([[*]] sec)
