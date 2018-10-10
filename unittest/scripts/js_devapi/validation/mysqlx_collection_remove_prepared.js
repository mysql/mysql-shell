//@<PROTOCOL> First execution is normal
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
        type: LITERAL
        literal {
          type: V_OCTETS
          v_octets {
            value: "001"
          }
        }
      }
    }
  }
}

//@<OUT> First execution is normal
Query OK, 1 item affected ([[*]] sec)
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> Second execution prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 1
  stmt {
    type: DELETE
    delete {
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
            type: LITERAL
            literal {
              type: V_OCTETS
              v_octets {
                value: "001"
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

//@<OUT> Second execution prepares statement and executes it
Query OK, 1 item affected ([[*]] sec)
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> Third execution uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 1
}

//@<OUT> Third execution uses prepared statement
Query OK, 1 item affected ([[*]] sec)
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> sort() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 1
}

<<<< RECEIVE Mysqlx.Ok {
}

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
        type: LITERAL
        literal {
          type: V_OCTETS
          v_octets {
            value: "001"
          }
        }
      }
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
}

//@<OUT> sort() changes statement, back to normal execution
Query OK, 1 item affected ([[*]] sec)
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> second execution after sort(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 2
  stmt {
    type: DELETE
    delete {
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
            type: LITERAL
            literal {
              type: V_OCTETS
              v_octets {
                value: "001"
              }
            }
          }
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
    }
  }
}

<<<< RECEIVE Mysqlx.Ok {
}

>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 2
}

//@<OUT> second execution after sort(), prepares statement and executes it
Query OK, 1 item affected ([[*]] sec)
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> third execution after set(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 2
}

//@<OUT> third execution after set(), uses prepared statement
Query OK, 1 item affected ([[*]] sec)
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> limit() changes statement, back to normal execution
>>>> SEND Mysqlx.Prepare.Deallocate {
  stmt_id: 2
}

<<<< RECEIVE Mysqlx.Ok {
}

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
        type: LITERAL
        literal {
          type: V_OCTETS
          v_octets {
            value: "001"
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
        document_path {
          type: MEMBER
          value: "name"
        }
      }
    }
    direction: DESC
  }
}


//@<OUT> limit() changes statement, back to normal execution
Query OK, 1 item affected ([[*]] sec)
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> second execution after limit(), prepares statement and executes it
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 3
  stmt {
    type: DELETE
    delete {
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
            type: LITERAL
            literal {
              type: V_OCTETS
              v_octets {
                value: "001"
              }
            }
          }
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
  stmt_id: 3
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
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> third execution after limit(), uses prepared statement
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 3
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
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> Prepares statement to test re-usability of bind() and limit()
>>>> SEND Mysqlx.Crud.Delete {
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
  limit {
    row_count: 1
  }
  args {
    type: V_STRING
    v_string {
      value: "001"
    }
  }
}


//@<OUT> Prepares statement to test re-usability of bind() and limit()
Query OK, 1 item affected ([[*]] sec)
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> Prepares and executes statement
>>>> SEND Mysqlx.Prepare.Prepare {
  stmt_id: 4
  stmt {
    type: DELETE
    delete {
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
  stmt_id: 4
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "002"
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

//@<OUT> Prepares and executes statement
Query OK, 1 item affected ([[*]] sec)
[
    {
        "_id": "001",
        "age": 18,
        "name": "george"
    },
    {
        "_id": "003",
        "age": 18,
        "name": "luke"
    }
]
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> Executes prepared statement with bind()
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
  args {
    type: SCALAR
    scalar {
      type: V_STRING
      v_string {
        value: "003"
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

//@<OUT> Executes prepared statement with bind()
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
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> Executes prepared statement with limit(1)
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
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

//@<OUT> Executes prepared statement with limit(1)
Query OK, 1 item affected ([[*]] sec)
[
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
2 documents in set ([[*]] sec)
Query OK, 1 item affected ([[*]] sec)

//@<PROTOCOL> Executes prepared statement with limit(2)
>>>> SEND Mysqlx.Prepare.Execute {
  stmt_id: 4
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

//@<OUT> Executes prepared statement with limit(2)
Query OK, 2 items affected ([[*]] sec)
[
    {
        "_id": "003",
        "age": 18,
        "name": "luke"
    }
]
1 document in set ([[*]] sec)

