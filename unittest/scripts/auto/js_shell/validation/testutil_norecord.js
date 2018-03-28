
//@ init
||

//@<OUT> importData check
[
    [
        1,
        "bla"
    ],
    [
        2,
        "ble"
    ]
]
[
    [
        10,
        "foo"
    ]
]

//@! importData badfile
||badfile.sql: Input file does not exist (RuntimeError)

//@! importData badpass
||mysql exited with code 1: mysql: [Warning] Using a password on the command line interface can be insecure.
||ERROR 1045 (28000): Access denied for user 'root'@'localhost' (using password: YES)

//@ cleanup
||
