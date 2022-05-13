//@<ERR> getClassicSession errors
mysql.getClassicSession: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
mysql.getClassicSession: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
mysql.getClassicSession: Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary (TypeError)
mysql.getClassicSession: Argument #2 is expected to be a string (TypeError)

//@<ERR> getSession errors
mysql.getSession: Invalid number of arguments, expected 1 to 2 but got 0 (ArgumentError)
mysql.getSession: Invalid number of arguments, expected 1 to 2 but got 3 (ArgumentError)
mysql.getSession: Argument #1: Invalid connection options, expected either a URI or a Connection Options Dictionary (TypeError)
mysql.getSession: Argument #2 is expected to be a string (TypeError)


//@<ERR> parseStatementAst errors
mysql.parseStatementAst: Argument #1 is expected to be a string (TypeError)
mysql.parseStatementAst: Argument #1 is expected to be a string (TypeError)

//@<OUT> parseStatementAst
{
    "children": [], 
    "rule": "query"
}
{
    "children": [
        {
            "symbol": "EOF", 
            "text": "<EOF>"
        }
    ], 
    "rule": "query"
}
{
    "children": [
        {
            "children": [
                {
                    "children": [], 
                    "rule": "selectStatement"
                }
            ], 
            "rule": "simpleStatement"
        }, 
        {
            "symbol": "EOF", 
            "text": "<EOF>"
        }
    ], 
    "rule": "query"
}

//@<OUT> splitScript
[
    "select 1"
]
[
    "select 1;", 
    "select 2;"
]
[
    "select 1$$", 
    "select 2;select3$$"
]
[
    "SELECT 1 A", 
    "SELECT 2 A", 
    "SELECT 3; SELECT 4"
]

//@<ERR> splitScript errors
mysql.splitScript: Argument #1 is expected to be a string (TypeError)
mysql.splitScript: Argument #1 is expected to be a string (TypeError)
