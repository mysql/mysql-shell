
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

//@ cleanup
||

//@ async mysqlsh
|Abra|
|Cadabra|
|--js' did not finish in time and will be killed|