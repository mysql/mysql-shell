# Parser Utilities

MySQL Shell features a full MySQL parser which is also exposed for external use.
The public Shell API lives on the `mysql` module and is implemented in
`modules/mod_mysql.cc`.

JavaScript uses camelCase names. Python uses snake_case names.

| JavaScript | Python | Purpose |
| --- | --- | --- |
| `mysql.splitScript()` | `mysql.split_script()` | Split a SQL script into statements separated by the designated delimiter (; by default). |
| `mysql.parseStatementAst()` | `mysql.parse_statement_ast()` | Parse statements and return an AST dictionary as a JSON-like structure. |
| `mysql.parseDataType()` | `mysql.parse_data_type()` | Parse a MySQL data type definition into its normalized components. |
| `mysql.parseDdlScript()` | `mysql.parse_ddl_script()` | Parse DDL statements and return JSON-like object representation of the object or delta. |
| `mysql.tokenizeStatement()` | `mysql.tokenize_statement()` | Lex a statement into tokens. |
| `mysql.quoteString()` | `mysql.quote_string()` | Quote a value as a SQL string literal. |
| `mysql.quoteIdentifier()` | `mysql.quote_identifier()` | Quote a value as a MySQL identifier. |
| `mysql.unquoteString()` | `mysql.unquote_string()` | Unquote a SQL string literal. |
| `mysql.unquoteIdentifier()` | `mysql.unquote_identifier()` | Unquote a backtick-quoted identifier. |

## Options

Parser and lexer utilities use option dictionaries. Unknown options are
rejected by the Shell option unpacker.

| Option | Type | Applies to | Meaning |
| --- | --- | --- | --- |
| `ansiQuotes` | Boolean | `splitScript`, `parseStatementAst`, `parseDdlScript`, `tokenizeStatement` | Treat double quotes as identifier quotes instead of string quotes. |
| `noBackslashEscapes` | Boolean | `splitScript`, `parseStatementAst`, `parseDdlScript`, `tokenizeStatement` | Do not treat backslashes as escapes inside string literals. |
| `schema` | String | `parseDdlScript` | Default schema for unqualified DDL object names. |
| `ignoreErrors` | Boolean | `parseDdlScript` | Keep parsing and return an `errors` array instead of throwing on parser or non-DDL statement failures. |
| `version` | String | `parseDdlScript` | MySQL server version used for version-gated grammar; defaults to the Shell version. |

## Lexer

### Tokenize Statement

`tokenizeStatement(statement[, options])` returns all lexer tokens except EOF.
The output includes hidden-channel tokens such as whitespace and comments.

```javascript
mysql.tokenizeStatement("select * from /* comment */ foo.bar")
```

```json
[
  { "type": "SELECT_SYMBOL", "token": "select" },
  { "type": "WHITESPACE", "token": " " },
  { "type": "MULT_OPERATOR", "token": "*" },
  { "type": "WHITESPACE", "token": " " },
  { "type": "FROM_SYMBOL", "token": "from" },
  { "type": "WHITESPACE", "token": " " },
  { "type": "BLOCK_COMMENT", "token": "/* comment */" },
  { "type": "WHITESPACE", "token": " " },
  { "type": "IDENTIFIER", "token": "foo" },
  { "type": "DOT_SYMBOL", "token": "." },
  { "type": "IDENTIFIER", "token": "bar" }
]
```

The token `type` is the symbolic ANTLR lexer token name. The `token` value is
the exact source text consumed for that token. Lexer errors throw.

`ansiQuotes` changes the token class for double-quoted text:

```javascript
mysql.tokenizeStatement('SELECT "a"', {ansiQuotes: false})
// includes { "type": "SINGLE_QUOTED_TEXT", "token": "\"a\"" }

mysql.tokenizeStatement('SELECT "a"', {ansiQuotes: true})
// includes { "type": "BACK_TICK_QUOTED_ID", "token": "\"a\"" }
```

## Parser

### Split Script

`splitScript(script[, options])` splits SQL text into individual statements.
The delimiter defaults to `;`.

```javascript
mysql.splitScript("select 1; select 2;")
// ["select 1", "select 2"]
```

The splitter recognizes the `DELIMITER` command when it is followed by the new
delimiter and a newline.

```javascript
mysql.splitScript("DELIMITER $$\nselect 1$$select 2;select3$$")
// ["select 1", "select 2;select3"]
```

Returned statements do not include the delimiter. SQL mode options are used
while locating statement boundaries, so `noBackslashEscapes` can change whether
a semicolon is considered inside or outside a string literal.

### Parse Statement AST

`parseStatementAst(statement[, options])` parses SQL and returns the ANTLR parse
tree encoded as dictionaries and arrays. It is intended for parser diagnostics
and tests, not as a stable semantic object model.

Rule nodes have a `rule` field and a `children` array:

```json
{
  "rule": "query",
  "children": []
}
```

Terminal nodes have `symbol` and `text` fields:

```json
{ "symbol": "SELECT_SYMBOL", "text": "SELECT" }
```

Error nodes include the terminal information plus `line`, `offset`, and
`error: 1`:

```json
{
  "symbol": "IDENTIFIER",
  "text": "bad",
  "line": 1,
  "offset": 0,
  "error": 1
}
```

### Parse Data Type

`parseDataType(typeDefinition)` parses one MySQL data type definition and
returns its normalized components. It does not accept parser options.

```javascript
mysql.parseDataType("DECIMAL(10,2) UNSIGNED")
```

```json
{
  "type": "DECIMAL",
  "precision": 10,
  "scale": 2,
  "length": null,
  "explicitParams": "",
  "flags": ["UNSIGNED"]
}
```

The result fields are:

- `type`: normalized MySQL type name.
- `precision` and `scale`: numeric precision and scale, or `null` when the
  type does not specify them.
- `length`: type length, or `null` when the type has no length.
- `explicitParams`: explicit `ENUM` or `SET` values, including parentheses.
- `flags`: type flags such as `UNSIGNED`, `SIGNED`, `ZEROFILL`, `ASCII`, or
  `BINARY`.

Invalid type definitions raise a parser error.

## DDL Parser

MySQL DDL statements such as CREATE TABLE, ALTER TABLE and DROP TABLE can be
parsed to JSON change object lists. These change objects represent the full
object (for CREATE) or the changes (ALTER, DROP, RENAME etc) to be applied
to an object in an easy to traverse structured format.

### Parse DDL Script

`parseDdlScript(script[, options])` parses DDL statements and returns a semantic
change list. It tracks `USE schema` statements while parsing; unqualified object
names use the current script schema or the `schema` option.

```javascript
mysql.parseDdlScript("CREATE TABLE s.t (id INT)")
```

```json
{
  "changes": [
    {
      "create": {
        "_type": "table",
        "_schema": "s",
        "name": "t",
        "columns": [
          { "name": "id", "dataType": "INT" }
        ]
      }
    }
  ]
}
```

`version` controls version-gated grammar and query classification. For example,
syntax added after MySQL 8.0 may parse with the default Shell version but fail
when `version: "8.0.27"` is specified.

### Object Format

`parseDdlScript()` returns a dictionary with a `changes` array. If parsing was
run with `ignoreErrors: true`, it can also return an `errors` array containing
line-qualified parser and non-DDL statement messages.

```json
{
  "changes": [
    { "create": { "_type": "table", "_schema": "s", "name": "t" } },
    { "alter": { "_type": "table", "_schema": "s", "_name": "t" } },
    { "drop": { "_type": "table", "_schema": "s", "_name": "t" } }
  ],
  "errors": ["at line 3: ..."]
}
```

Each entry in `changes` is a single operation envelope:

- `create`: object definition created by the statement.
- `alter`: object identity plus the parsed properties changed by the
  statement.
- `drop`: object identity removed by the statement.

Most object dictionaries use these common fields:

- `_type`: normalized object type, such as `schema`, `table`, `view`,
  `procedure`, `function`, `library`, or `resourceGroup`.
- `_schema`: owning schema for schema-qualified object types. If the statement
  uses an unqualified name, this is the current schema from the input script or
  from the `schema` argument.
- `name`: object name for `create` records.
- `_name`: existing object name for `alter` and `drop` records.
- `_table`: parent table name for child objects such as indexes, columns, and
  constraints.
- `_replace`: present on alter records that replace an existing child object.

Non-underscore fields are parsed object properties. They are emitted only when
the DDL contains enough information for that property. Identifiers and string
literals are unquoted according to the parser SQL mode, including
`ANSI_QUOTES` and `NO_BACKSLASH_ESCAPES`.

#### Schemas

```json
{ "create": { "_type": "schema", "name": "s" } }
{ "alter": { "_type": "schema", "_name": "s", "defaultCharacterSet": "utf8mb4" } }
{ "drop": { "_type": "schema", "_name": "s" } }
```

#### Tables, Views, And Routines

Tables are emitted with `_type: "table"` and may include parsed columns,
indexes, foreign keys, checks, options, and partitioning details.

Regular views are emitted with `_type: "view"` and may include `sqlBody`,
`algorithm`, `definer`, `sqlSecurity`, `columnList`, and `checkOption`.

JSON duality views are also represented as views, with an extra discriminator:

```json
{
  "create": {
    "_type": "view",
    "_schema": "s",
    "name": "jv",
    "viewType": "JSON_DUALITY",
    "sqlBody": "SELECT ..."
  }
}
```

Stored procedures and stored functions use `_type: "procedure"` and
`_type: "function"`. UDFs use `_type: "udf"`. JavaScript routines may include
`language: "JAVASCRIPT"` and a `libraries` array:

```json
{
  "create": {
    "_type": "procedure",
    "_schema": "s",
    "name": "p",
    "language": "JAVASCRIPT",
    "libraries": [
      { "_schema": "s", "name": "lib", "alias": "mylib" }
    ]
  }
}
```

`RENAME TABLE` is represented as an `alter` on the source table with the target
identity in non-underscore fields:

```json
{
  "alter": {
    "_type": "table",
    "_schema": "old_s",
    "_name": "old_t",
    "schema": "new_s",
    "name": "new_t"
  }
}
```

#### Libraries

```json
{
  "create": {
    "_type": "library",
    "_schema": "s",
    "name": "lib",
    "language": "JAVASCRIPT",
    "sqlBody": "AS $$ ... $$"
  }
}
{
  "alter": {
    "_type": "library",
    "_schema": "s",
    "_name": "lib",
    "comment": "changed"
  }
}
{ "drop": { "_type": "library", "_schema": "s", "_name": "lib" } }
```

#### Roles

`CREATE ROLE` and `DROP ROLE` can name multiple roles. Each role is emitted as a
separate change entry. Quoted account-style role names keep their original
quotes so the user and host parts remain unambiguous.

```json
{ "create": { "_type": "role", "name": "r1" } }
{ "drop": { "_type": "role", "_name": "r1" } }
```

#### Resource Groups

```json
{
  "create": {
    "_type": "resourceGroup",
    "name": "rg",
    "type": "USER",
    "vcpu": "0-1, 3",
    "threadPriority": 5,
    "enabled": true
  }
}
{
  "alter": {
    "_type": "resourceGroup",
    "_name": "rg",
    "vcpu": "2",
    "threadPriority": 7,
    "enabled": false,
    "force": true
  }
}
{
  "drop": {
    "_type": "resourceGroup",
    "_name": "rg",
    "force": true
  }
}
```

#### Tablespaces And Spatial Reference Systems

Regular tablespaces use `_type: "tablespace"`. Undo tablespaces use
`_type: "undoTablespace"`:

```json
{
  "create": {
    "_type": "undoTablespace",
    "name": "uts",
    "dataFile": "uts.ibu"
  }
}
{
  "alter": {
    "_type": "undoTablespace",
    "_name": "uts",
    "state": "INACTIVE"
  }
}
{ "drop": { "_type": "undoTablespace", "_name": "uts" } }
```

Spatial reference systems are keyed by SRID string:

```json
{ "create": { "_type": "spatialReferenceSystem", "name": "9999" } }
{ "drop": { "_type": "spatialReferenceSystem", "_name": "9999" } }
```

## Utilities

### Quoting

`quoteString(value)` returns a single-quoted MySQL string literal with the
necessary escapes.

```javascript
mysql.quoteString("a'b")
// "'a\\'b'"
```

`quoteIdentifier(value)` returns a backtick-quoted identifier with embedded
backticks escaped.

```javascript
mysql.quoteIdentifier("a`b")
// "`a``b`"
```

### Unquoting

`unquoteString(value)` removes SQL string quoting and unescapes the literal
according to the quote character used by the input string.

```javascript
mysql.unquoteString("'a\\'b'")
// "a'b"
```

`unquoteIdentifier(value)` removes MySQL identifier quoting. The input must be
quoted with backticks.

```javascript
mysql.unquoteIdentifier("`a``b`")
// "a`b"
```

Copyright (c) 2026, Oracle and/or its affiliates.
