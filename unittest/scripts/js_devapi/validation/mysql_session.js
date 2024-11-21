//@Preparation for transaction tests
||

//@ ClassicSession: Transaction handling: rollback
|Inserted Documents: 0|

//@ ClassicSession: Transaction handling: commit
|Inserted Documents: 3|

//@ ClassicSession: date handling
|9999-12-31 23:59:59.999999|

//@# ClassicSession: runSql errors
||Invalid number of arguments, expected 1 to 2 but got 0
||Invalid number of arguments, expected 1 to 2 but got 3
||Argument #1 is expected to be a string
||Argument #2 is expected to be an array
||Error formatting SQL query: more arguments than escapes while substituting placeholder value at index #2
||Insufficient number of values for placeholders in query


//@<OUT> ClassicSession: runSql placeholders
| hello | 1234 |

//@<OUT> ClassicSession: runSql with various parameter types
[
    null,
    1234,
    "-0.12345",
    "3.14159265359",
    "hellooooo"
]
