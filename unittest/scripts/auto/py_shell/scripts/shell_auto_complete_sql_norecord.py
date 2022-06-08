#@<> helpers
import random

def EXPECT_SUCCESS(sql, options = {}):
    run_options = { "serverVersion": __version, "sqlMode": "" }
    # add extra options
    run_options.update(options)
    result = None
    def execute():
        nonlocal result
        result = shell.auto_complete_sql(sql, run_options)
    EXPECT_NO_THROWS(execute)
    return result

def EXPECT_FAIL(error, msg, sql, options = {}):
    run_options = { "serverVersion": __version, "sqlMode": "" }
    # add extra options
    run_options.update(options)
    is_re = is_re_instance(msg)
    full_msg = f"{re.escape(error) if is_re else error}: Shell.auto_complete_sql: {msg.pattern if is_re else msg}"
    if is_re:
        full_msg = re.compile("^" + full_msg)
    EXPECT_THROWS(lambda: shell.auto_complete_sql(sql, run_options), full_msg)

# tests if it's possible to complete the whole statement, all-uppercase tokens
# are treated as keywords to be completed
def COMPLETE_WHOLE_STATEMENT(sql):
    full = sql
    current = ""
    token = ""
    while full:
        first = full[0]
        if first == "'" or first == '"' or first == "`":
            # quoted string, we assume quote characters are not escaped
            # copy the whole string and try with the next char
            idx = full.find(first, 1)
            current += full[:idx]
            full = full[idx:]
            first = full[0]
        full = full[1:]
        if first.isupper() or ("_" == first and token):
            token += first
        else:
            if token:
                current += token[0]
                result = EXPECT_SUCCESS(current, { "serverVersion": "8.0.30" })
                # exact match or token followed by a space and another keyword(s)
                if not EXPECT_TRUE("keywords" in result, "Result does not have keywords") or \
                   not EXPECT_CONTAINS_LIKE([ "^" + token + "=?$|^" + token + "( [A-Z_]+)+$" ], result.keywords, f"Result does not have a keyword '{token}'"):
                    print(f"Failed to complete statement '{sql}', current attempt: '{current}'")
                    break
                current += token[1:]
                token = ""
            current += first

#@<> checking results - prefix
EXPECT_EQ("", EXPECT_SUCCESS("").context.prefix)
EXPECT_EQ("A", EXPECT_SUCCESS("A").context.prefix)
EXPECT_EQ("ALTER", EXPECT_SUCCESS("ALTER").context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER ").context.prefix)
EXPECT_EQ("T", EXPECT_SUCCESS("ALTER T").context.prefix)
EXPECT_EQ("TABLE", EXPECT_SUCCESS("ALTER TABLE").context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER TABLE ").context.prefix)
EXPECT_EQ("a", EXPECT_SUCCESS("ALTER TABLE a").context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER TABLE a.").context.prefix)
EXPECT_EQ("`", EXPECT_SUCCESS("ALTER TABLE a.`").context.prefix)
EXPECT_EQ("`b", EXPECT_SUCCESS("ALTER TABLE a.`b").context.prefix)
EXPECT_EQ("`b ", EXPECT_SUCCESS("ALTER TABLE a.`b ").context.prefix)
EXPECT_EQ("`b ", EXPECT_SUCCESS("ALTER TABLE a. `b ").context.prefix)
EXPECT_EQ("`b `", EXPECT_SUCCESS("ALTER TABLE a. `b `").context.prefix)

EXPECT_EQ("`b `", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 19 }).context.prefix)
EXPECT_EQ("`b ", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 18 }).context.prefix)
EXPECT_EQ("`b", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 17 }).context.prefix)
EXPECT_EQ("`", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 16 }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 15 }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 14 }).context.prefix)
EXPECT_EQ("a", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 13 }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 12 }).context.prefix)
EXPECT_EQ("TABLE", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 11 }).context.prefix)
EXPECT_EQ("TABL", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 10 }).context.prefix)
EXPECT_EQ("TA", EXPECT_SUCCESS("ALTER TABLE a. `b `", { "statementOffset": 8 }).context.prefix)

EXPECT_EQ("`b ", EXPECT_SUCCESS("ALTER TABLE a. `b ", { "statementOffset": 18 }).context.prefix)
EXPECT_EQ("`b", EXPECT_SUCCESS("ALTER TABLE a. `b ", { "statementOffset": 17 }).context.prefix)
EXPECT_EQ("`", EXPECT_SUCCESS("ALTER TABLE a. `b ", { "statementOffset": 16 }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER TABLE a. `b ", { "statementOffset": 15 }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER TABLE a. `b ", { "statementOffset": 14 }).context.prefix)
EXPECT_EQ("a", EXPECT_SUCCESS("ALTER TABLE a. `b ", { "statementOffset": 13 }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER TABLE a. `b ", { "statementOffset": 12 }).context.prefix)
EXPECT_EQ("TABLE", EXPECT_SUCCESS("ALTER TABLE a. `b ", { "statementOffset": 11 }).context.prefix)
EXPECT_EQ("TABL", EXPECT_SUCCESS("ALTER TABLE a. `b ", { "statementOffset": 10 }).context.prefix)
EXPECT_EQ("TA", EXPECT_SUCCESS("ALTER TABLE a. `b ", { "statementOffset": 8 }).context.prefix)

EXPECT_EQ("a", EXPECT_SUCCESS("SELECT a").context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("SELECT a,").context.prefix)
EXPECT_EQ("b", EXPECT_SUCCESS("SELECT a,b").context.prefix)

EXPECT_EQ("", EXPECT_SUCCESS("SELECT ").context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("SELECT @").context.prefix)
EXPECT_EQ("a", EXPECT_SUCCESS("SELECT @a").context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("SELECT @@").context.prefix)
EXPECT_EQ("a", EXPECT_SUCCESS("SELECT @@a").context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("SELECT @@global.").context.prefix)
EXPECT_EQ("a", EXPECT_SUCCESS("SELECT @@global.a").context.prefix)

EXPECT_EQ("a", EXPECT_SUCCESS("ALTER USER a").context.prefix)
EXPECT_EQ("ab", EXPECT_SUCCESS("ALTER USER ab").context.prefix)
EXPECT_EQ("ab@", EXPECT_SUCCESS("ALTER USER ab@").context.prefix)
EXPECT_EQ("ab@c", EXPECT_SUCCESS("ALTER USER ab@c").context.prefix)
EXPECT_EQ("ab@cd", EXPECT_SUCCESS("ALTER USER ab@cd").context.prefix)
EXPECT_EQ("ab@cd.", EXPECT_SUCCESS("ALTER USER ab@cd.").context.prefix)
EXPECT_EQ("ab@cd.c", EXPECT_SUCCESS("ALTER USER ab@cd.c").context.prefix)
EXPECT_EQ("ab@cd.co", EXPECT_SUCCESS("ALTER USER ab@cd.co").context.prefix)
EXPECT_EQ("ab@cd.com", EXPECT_SUCCESS("ALTER USER ab@cd.com").context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER USER ab@cd.com ").context.prefix)

EXPECT_EQ("`", EXPECT_SUCCESS("ALTER USER `").context.prefix)
EXPECT_EQ("`a", EXPECT_SUCCESS("ALTER USER `a").context.prefix)
EXPECT_EQ("`ab", EXPECT_SUCCESS("ALTER USER `ab").context.prefix)
EXPECT_EQ("`ab`", EXPECT_SUCCESS("ALTER USER `ab`").context.prefix)
EXPECT_EQ("`ab`@", EXPECT_SUCCESS("ALTER USER `ab`@").context.prefix)
EXPECT_EQ("`ab`@'", EXPECT_SUCCESS("ALTER USER `ab`@'").context.prefix)
EXPECT_EQ("`ab`@'c", EXPECT_SUCCESS("ALTER USER `ab`@'c").context.prefix)
EXPECT_EQ("`ab`@'cd", EXPECT_SUCCESS("ALTER USER `ab`@'cd").context.prefix)
EXPECT_EQ("`ab`@'cd.", EXPECT_SUCCESS("ALTER USER `ab`@'cd.").context.prefix)
EXPECT_EQ("`ab`@'cd.c", EXPECT_SUCCESS("ALTER USER `ab`@'cd.c").context.prefix)
EXPECT_EQ("`ab`@'cd.co", EXPECT_SUCCESS("ALTER USER `ab`@'cd.co").context.prefix)
EXPECT_EQ("`ab`@'cd.com", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com").context.prefix)
EXPECT_EQ("`ab`@'cd.com'", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com'").context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' ").context.prefix)

EXPECT_EQ('a', EXPECT_SUCCESS('ALTER USER a').context.prefix)
EXPECT_EQ('ab', EXPECT_SUCCESS('ALTER USER ab').context.prefix)
EXPECT_EQ('ab@', EXPECT_SUCCESS('ALTER USER ab@').context.prefix)
EXPECT_EQ('ab@"', EXPECT_SUCCESS('ALTER USER ab@"').context.prefix)
EXPECT_EQ('ab@"c', EXPECT_SUCCESS('ALTER USER ab@"c').context.prefix)
EXPECT_EQ('ab@"cd', EXPECT_SUCCESS('ALTER USER ab@"cd').context.prefix)
EXPECT_EQ('ab@"cd.', EXPECT_SUCCESS('ALTER USER ab@"cd.').context.prefix)
EXPECT_EQ('ab@"cd.c', EXPECT_SUCCESS('ALTER USER ab@"cd.c').context.prefix)
EXPECT_EQ('ab@"cd.co', EXPECT_SUCCESS('ALTER USER ab@"cd.co').context.prefix)
EXPECT_EQ('ab@"cd.com', EXPECT_SUCCESS('ALTER USER ab@"cd.com').context.prefix)
EXPECT_EQ('ab@"cd.com"', EXPECT_SUCCESS('ALTER USER ab@"cd.com"').context.prefix)
EXPECT_EQ('', EXPECT_SUCCESS('ALTER USER ab@"cd.com" ').context.prefix)

EXPECT_EQ("`ab`@'cd.com'", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 24 }).context.prefix)
EXPECT_EQ("`ab`@'cd.com", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 23 }).context.prefix)
EXPECT_EQ("`ab`@'cd.co", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 22 }).context.prefix)
EXPECT_EQ("`ab`@'cd.c", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 21 }).context.prefix)
EXPECT_EQ("`ab`@'cd.", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 20 }).context.prefix)
EXPECT_EQ("`ab`@'cd", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 19 }).context.prefix)
EXPECT_EQ("`ab`@'c", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 18 }).context.prefix)
EXPECT_EQ("`ab`@'", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 17 }).context.prefix)
EXPECT_EQ("`ab`@", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 16 }).context.prefix)
EXPECT_EQ("`ab`", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 15 }).context.prefix)
EXPECT_EQ("`ab", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 14 }).context.prefix)
EXPECT_EQ("`a", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 13 }).context.prefix)
EXPECT_EQ("`", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 12 }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 11 }).context.prefix)
EXPECT_EQ("USER", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 10 }).context.prefix)
EXPECT_EQ("USE", EXPECT_SUCCESS("ALTER USER `ab`@'cd.com' IDENTIFIED", { "statementOffset": 9 }).context.prefix)

#@<> WL13397-TSFR_2_1_1
EXPECT_SUCCESS("")
EXPECT_SUCCESS("a\nb")
EXPECT_SUCCESS("/* abc */")

#@<> WL13397-TSFR_2_1_2
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", 1)
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", [])
EXPECT_FAIL("TypeError", "Argument #1 is expected to be a string", {})

#@<> WL13397-TSFR_2_2_1
# WL13397-TSFR_2_2_1_1_3
# WL13397-TSFR_2_2_2_4
EXPECT_THROWS(lambda: shell.auto_complete_sql("", {}), "ValueError: Shell.auto_complete_sql: Argument #2: Missing required options: serverVersion, sqlMode")
EXPECT_FAIL("ValueError", "Argument #2: Invalid options: unknown", "", { "unknown": "unknown" })

#@<> WL13397-TSFR_2_2_2
EXPECT_THROWS(lambda: shell.auto_complete_sql("", 1), "TypeError: Shell.auto_complete_sql: Argument #2 is expected to be a map")
EXPECT_THROWS(lambda: shell.auto_complete_sql("", []), "TypeError: Shell.auto_complete_sql: Argument #2 is expected to be a map")
EXPECT_THROWS(lambda: shell.auto_complete_sql("", ""), "TypeError: Shell.auto_complete_sql: Argument #2 is expected to be a map")

#@<> WL13397-TSFR_2_2_1_1
EXPECT_FAIL("TypeError", "Argument #2: Option 'serverVersion' is expected to be of type String, but is Map", "", { "serverVersion": {} })

#@<> WL13397-TSFR_2_2_1_1_1
EXPECT_SUCCESS("", { "serverVersion": "8.0.0-dmr" })
EXPECT_SUCCESS("", { "serverVersion": "5.7.38-debug" })

#@<> WL13397-TSFR_2_2_1_1_2
EXPECT_FAIL("ValueError", "Argument #2: Error parsing version: ", "", { "serverVersion": "A.B.C" })

#@<> WL13397-TSFR_2_2_1_2_1
EXPECT_SUCCESS("", { "serverVersion": "3.23.13" })
EXPECT_SUCCESS("", { "serverVersion": "99.9.9" })

#@<> WL13397-TSFR_2_2_1_2_2
EXPECT_SUCCESS("", { "serverVersion": "5.7.38" })
EXPECT_SUCCESS("", { "serverVersion": __version })

#@<> WL13397-TSFR_2_2_2_1
sql_modes_80 = [
    "ALLOW_INVALID_DATES",
    "ANSI_QUOTES",
    "ERROR_FOR_DIVISION_BY_ZERO",
    "HIGH_NOT_PRECEDENCE",
    "IGNORE_SPACE",
    "NO_AUTO_VALUE_ON_ZERO",
    "NO_BACKSLASH_ESCAPES",
    "NO_DIR_IN_CREATE",
    "NO_ENGINE_SUBSTITUTION",
    "NO_UNSIGNED_SUBTRACTION",
    "NO_ZERO_DATE",
    "NO_ZERO_IN_DATE",
    "ONLY_FULL_GROUP_BY",
    "PAD_CHAR_TO_FULL_LENGTH",
    "PIPES_AS_CONCAT",
    "REAL_AS_FLOAT",
    "STRICT_ALL_TABLES",
    "STRICT_TRANS_TABLES",
    "TIME_TRUNCATE_FRACTIONAL",
]

for i in range(10):
    modes = set()
    for j in range(random.randint(1, len(sql_modes_80))):
        modes.add(sql_modes_80[random.randint(1, len(sql_modes_80)) - 1])
    print(",".join(modes))
    EXPECT_SUCCESS("", { "serverVersion": "8.0.30", "sqlMode" : ",".join(modes) })

#@<> WL13397-TSFR_2_2_2_2
EXPECT_FAIL("ValueError", "Argument #2: MySQL server 8.0.30 does not support the following SQL modes: NO_AUTO_CREATE_USER", "", { "serverVersion": "8.0.30", "sqlMode" : "NO_AUTO_CREATE_USER" })
EXPECT_FAIL("ValueError", "Argument #2: MySQL server 8.0.30 does not support the following SQL modes: NO_FIELD_OPTIONS, NO_KEY_OPTIONS", "", { "serverVersion": "8.0.30", "sqlMode" : "NO_FIELD_OPTIONS,NO_KEY_OPTIONS" })
EXPECT_FAIL("ValueError", "Argument #2: MySQL server 8.0.30 does not support the following SQL modes: NO_TABLE_OPTIONS", "", { "serverVersion": "8.0.30", "sqlMode" : "NO_TABLE_OPTIONS" })

#@<> WL13397-TSFR_2_2_2_3
EXPECT_FAIL("ValueError", "Argument #2: MySQL server 5.7.38 does not support the following SQL modes: TIME_TRUNCATE_FRACTIONAL", "", { "serverVersion": "5.7.38", "sqlMode" : "TIME_TRUNCATE_FRACTIONAL" })

#@<> WL13397-TSFR_2_2_3_1
EXPECT_FAIL("TypeError", "Argument #2: Option 'statementOffset' is expected to be of type UInteger, but is Map", "", { "statementOffset": {} })
EXPECT_FAIL("TypeError", "Argument #2: Option 'statementOffset' UInteger expected, but Integer value is out of range", "", { "statementOffset": -1 })

#@<> WL13397-TSFR_2_2_3_2 + WL13397-TSFR_2_2_3_3
statement = "CREATE\nFUNCTION\nIF\nB"
EXPECT_EQ([ "NOT" ], EXPECT_SUCCESS(statement, { "serverVersion": "8.0.30", "statementOffset" : len(statement) - 1 }).keywords)

statement = "CREATE\r\nFUNCTION\r\nIF\r\nB"
EXPECT_EQ([ "NOT" ], EXPECT_SUCCESS(statement, { "serverVersion": "8.0.30", "statementOffset" : len(statement) - 1 }).keywords)

statement = "CREATE\r\nFUNCTION\nIF\r\nB"
EXPECT_EQ([ "NOT" ], EXPECT_SUCCESS(statement, { "serverVersion": "8.0.30", "statementOffset" : len(statement) - 1 }).keywords)

#@<> WL13397-TSFR_2_2_3_4
statement = "CREATE\nFUNCTION\nIF\nB"
EXPECT_CONTAINS([ "DATABASE", "FUNCTION", "TABLE" ], EXPECT_SUCCESS(statement, { "serverVersion": "8.0.30", "statementOffset" : len("CREATE") + 1 }).keywords)

#@<> WL13397-TSFR_2_2_3_1_1
EXPECT_CONTAINS([ "DUAL" ], EXPECT_SUCCESS("SELECT * FROM ").keywords)

#@<> WL13397-TSFR_2_2_3_2_1
EXPECT_CONTAINS([ "USER" ], EXPECT_SUCCESS("DROP ", { "statementOffset" : 1000 }).keywords)

#@<> WL13397-TSFR_2_2_4_1
EXPECT_FAIL("TypeError", "Argument #2: Option 'uppercaseKeywords' is expected to be of type Bool, but is Map", "", { "uppercaseKeywords": {} })

#@<> WL13397-TSFR_2_2_4_2
EXPECT_CONTAINS([ "USER" ], EXPECT_SUCCESS("DROP ").keywords)
EXPECT_CONTAINS([ "USER" ], EXPECT_SUCCESS("DROP ", { "uppercaseKeywords": True }).keywords)

EXPECT_CONTAINS([ "Sample" ], EXPECT_SUCCESS("DROP TABLE Sample.T").context.qualifier)
EXPECT_CONTAINS([ "Sample" ], EXPECT_SUCCESS("DROP TABLE Sample.T", { "uppercaseKeywords": True }).context.qualifier)

#@<> WL13397-TSFR_2_2_4_2_1
EXPECT_CONTAINS([ "user" ], EXPECT_SUCCESS("DROP ", { "uppercaseKeywords": False }).keywords)
EXPECT_CONTAINS([ "Sample" ], EXPECT_SUCCESS("DROP TABLE Sample.T", { "uppercaseKeywords": False }).context.qualifier)

#@<> WL13397-TSFR_2_2_5_1
EXPECT_FAIL("TypeError", "Argument #2: Option 'filtered' is expected to be of type Bool, but is Map", "", { "filtered": {} })

#@<> WL13397-TSFR_2_2_5_2
# WL13397-TSFR_2_2_5_1_1
EXPECT_EQ([ "TABLE", "TABLESPACE" ], EXPECT_SUCCESS("CREATE TABL").keywords)
EXPECT_EQ([ "TABLE", "TABLESPACE" ], EXPECT_SUCCESS("CREATE TABL", { "filtered": True }).keywords)

#@<> WL13397-TSFR_2_2_5_3
EXPECT_CONTAINS([ "TABLE", "FUNCTION", "USER" ], EXPECT_SUCCESS("CREATE TABL", { "filtered": False }).keywords)

#@<> WL13397-TSFR_2_2_5_5
# WL13397-TSFR_2_2_5_1_1
EXPECT_EQ([ "STDDEV_POP()", "STDDEV_SAMP()" ], EXPECT_SUCCESS("SELECT STDDEV_").functions)
EXPECT_EQ([ "STDDEV_POP()", "STDDEV_SAMP()" ], EXPECT_SUCCESS("SELECT STDDEV_", { "filtered": True }).functions)

#@<> WL13397-TSFR_2_2_5_6
EXPECT_CONTAINS([ "STDDEV_POP()", "STDDEV_SAMP()", "MAX()", "AVG()" ], EXPECT_SUCCESS("SELECT STDDEV_", { "filtered": False }).functions)

#@<> WL13397-TSFR_2_3_1
# note: direct comparison does not work
result = EXPECT_SUCCESS("statement = UNLOCK TABLES ")
EXPECT_EQ(1, len(result))
EXPECT_EQ(1, len(result.context))
EXPECT_EQ("", result.context.prefix)

#@<> WL13397-TSFR_2_3_1_1_1
EXPECT_EQ("'\\'", EXPECT_SUCCESS("SELECT 'a b'.'\\'", { "filtered": True }).context.prefix)
EXPECT_EQ("\"\\'", EXPECT_SUCCESS("SELECT 'a b'.\"\\'", { "filtered": True }).context.prefix)
# BUG#34342873: code completion has problems with unfinished quoted strings that contain escaped quote
# EXPECT_EQ('"\\"c', EXPECT_SUCCESS('SELECT "a b"."\\"c', { "filtered": True }).context.prefix)
EXPECT_EQ("'", EXPECT_SUCCESS("SELECT * FROM JSON_TABLE('", { "filtered": True }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("INSERT ", { "filtered": True }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("SELECT d1.t1.c1 ", { "filtered": True }).context.prefix)

#@<> WL13397-TSFR_2_3_1_2_1
EXPECT_EQ([ "a b" ], EXPECT_SUCCESS("SELECT `a b`.`\'").context.qualifier)
# BUG#34342873: code completion has problems with unfinished quoted strings that contain escaped quote
# EXPECT_EQ([ "a b" ], EXPECT_SUCCESS('SELECT "a b"."\\"c', { "sqlMode": "ANSI_QUOTES" }).context.qualifier)
EXPECT_TRUE("qualifier" not in EXPECT_SUCCESS("SELECT mycol").context)
EXPECT_EQ([ "db1", "t1" ], EXPECT_SUCCESS("SELECT db1.t1.col1").context.qualifier)
# BUG#34342946: bug in code completion, in this case it suggests columns from db1.t1
# EXPECT_TRUE("qualifier" not in EXPECT_SUCCESS("SELECT db1.t1.col1 ").context)

#@<> WL13397-TSFR_2_3_1_3_1
EXPECT_TRUE("references" not in EXPECT_SUCCESS("""SELECT * FROM
    JSON_TABLE(
      '[ {"c1": 1} ]',
      '$[*]' COLUMNS(c1 INT PATH '$.c1')
    ) AS jt""").context)

#@<> WL13397-TSFR_2_3_1_3_2
EXPECT_TRUE("references" not in EXPECT_SUCCESS("""SELECT
  (SELECT * FROM
    JSON_TABLE(
      '[ {"c1": 1} ]',
      '$[*]' COLUMNS(c1 INT PATH '$.c1')
    ) AS jt
  ) AS c
FROM
  DUAL""").context)

#@<> WL13397-TSFR_2_3_1_3_3
EXPECT_CONTAINS([ {"schema": "mysql", "table": "user", "alias": "ua"}, {"schema": "mysql", "table": "user", "alias": "ub"} ], EXPECT_SUCCESS("""SELECT
   AVG(LENGTH(ua.user)),
  (SELECT MAX(Host) FROM mysql.db AS ub) AS ub,
  (SELECT MIN(Host) FROM mysql.db AS ub) AS ua
FROM
  mysql.user AS ua,
  mysql.user AS ub
WHERE
  ua.user != ub.user
GROUP BY ub""").context.references)

#@<> WL13397-TSFR_2_3_1_4_1
EXPECT_EQ([ "label1" ], EXPECT_SUCCESS("""CREATE PROCEDURE doiterate(p1 INT)
BEGIN
  label1: LOOP
    SET p1 = p1 + 1;
    IF p1 < 10 THEN ITERATE label1; END IF;
    LEAVE label1;
  END LOOP """).context.labels)

EXPECT_EQ([ "label2" ], EXPECT_SUCCESS("""CREATE PROCEDURE doiterate(p1 INT)
BEGIN
  label1: LOOP
    SET p1 = p1 + 1;
    IF p1 < 10 THEN ITERATE label1; END IF;
    LEAVE label1;
  END LOOP label1;
  label2: LOOP
    SET p1 = p1 -1;
    IF p1 > 1 THEN ITERATE label2; END IF;
    LEAVE label2;
  END LOOP """).context.labels)


#@<> WL13397-TSFR_2_3_2_1
EXPECT_LT(50, len(EXPECT_SUCCESS("").keywords))

#@<> WL13397-TSFR_2_3_2_2
EXPECT_EQ([ "XA" ], EXPECT_SUCCESS("X").keywords)

#@<> WL13397-TSFR_2_3_4_1
# note: "userVars", "systemVars" are not candidates, because there's no '@'
# BUG#34342966: candidates should contain "functions", "udfs"
EXPECT_CONTAINS([ "schemas", "tables", "views", "runtimeFunctions", "functions", "udfs" ], EXPECT_SUCCESS("SELECT ").candidates)

#@<> WL13397-TSFR_2_3_4_2
EXPECT_EQ(set([ "schemas", "tables" ]), set(EXPECT_SUCCESS("HANDLER ").candidates))

#@<> WL13397-TSFR_2_3_4_3
EXPECT_EQ([ "schemas" ], EXPECT_SUCCESS("DROP DATABASE IF EXISTS ").candidates)

#@<> WL13397-TSFR_2_3_4_4
EXPECT_EQ(set([ "schemas", "views" ]), set(EXPECT_SUCCESS("DROP VIEW IF EXISTS v1, ").candidates))

#@<> WL13397-TSFR_2_3_4_5
EXPECT_EQ([ "internalColumns" ], EXPECT_SUCCESS("ALTER TABLE t ALTER COLUMN ").candidates)

#@<> WL13397-TSFR_2_3_4_6
EXPECT_TRUE("candidates" not in EXPECT_SUCCESS("ALTER TABLE t ALTER INDEX "))

#@<> WL13397-TSFR_2_3_4_7
EXPECT_EQ([ "internalColumns" ], EXPECT_SUCCESS("ALTER TABLE t DROP FOREIGN KEY ").candidates)

#@<> WL13397-TSFR_2_3_4_8
EXPECT_EQ(set([ "schemas" ]), set(EXPECT_SUCCESS("CREATE VIEW ").candidates))

#@<> WL13397-TSFR_2_3_4_9
EXPECT_EQ(set([ "schemas", "views" ]), set(EXPECT_SUCCESS("ALTER VIEW ").candidates))

#@<> WL13397-TSFR_2_3_4_10
EXPECT_EQ([ "internalColumns" ], EXPECT_SUCCESS("ALTER TABLE t DROP COLUMN ").candidates)

#@<> WL13397-TSFR_2_3_4_11
EXPECT_NOT_CONTAINS([ "columns", "internalColumns" ], EXPECT_SUCCESS("SELECT 1 AS a FROM DUAL WHERE ").candidates)

#@<> WL13397-TSFR_2_3_4_12
EXPECT_CONTAINS([ "columns" ], EXPECT_SUCCESS("SELECT db1.table1.").candidates)
# BUG#34342982: when looking for table references, only parts of the statement after FROM are checked,
# TABLE clause does not have it, there are no references and columns are not suggested
# EXPECT_CONTAINS([ "columns" ], EXPECT_SUCCESS("TABLE mysql.user ORDER BY ", { "serverVersion": "8.0.30" }).candidates)

#@<> WL13397-TSFR_2_3_4_13
EXPECT_NOT_CONTAINS([ "internalColumns" ], EXPECT_SUCCESS("SELECT mysql.user.User FROM mysql.user WHERE ").candidates)

#@<> WL13397-TSFR_2_3_4_14
EXPECT_CONTAINS([ "procedures" ], EXPECT_SUCCESS("DROP PROCEDURE ").candidates)
EXPECT_CONTAINS([ "procedures" ], EXPECT_SUCCESS("CALL ").candidates)

#@<> WL13397-TSFR_2_3_4_15
# BUG#34342982: grammar currently treats this ORDER BY as any other, and does not limit the scope to just implicit names column_0, ...
# EXPECT_TRUE("candidates" not in EXPECT_SUCCESS("VALUES ROW(1, 2, 3) ORDER BY ", { "serverVersion": "8.0.30" }))
EXPECT_TRUE("candidates" not in EXPECT_SUCCESS("DROP SERVER "))

#@<> WL13397-TSFR_2_3_4_16
EXPECT_CONTAINS([ "schemas", "tables" ], EXPECT_SUCCESS("WITH cte AS (SELECT User FROM ", { "serverVersion": "8.0.30" }).candidates)

#@<> WL13397-TSFR_2_3_4_17
EXPECT_CONTAINS([ "functions" ], EXPECT_SUCCESS("ALTER FUNCTION ").candidates)

#@<> WL13397-TSFR_2_3_4_18
EXPECT_CONTAINS([ "triggers" ], EXPECT_SUCCESS("DROP TRIGGER ").candidates)

#@<> WL13397-TSFR_2_3_4_19
EXPECT_CONTAINS([ "events" ], EXPECT_SUCCESS("DROP EVENT ").candidates)

#@<> WL13397-TSFR_2_3_4_20
EXPECT_EQ([ "engines" ], EXPECT_SUCCESS("CREATE TABLE t(id INT) ENGINE=").candidates)
EXPECT_EQ([ "engines" ], EXPECT_SUCCESS("DROP UNDO TABLESPACE ts ENGINE=", { "serverVersion": "8.0.30" }).candidates)

#@<> WL13397-TSFR_2_3_4_21
EXPECT_CONTAINS([ "functions", "schemas", "udfs" ], EXPECT_SUCCESS("DROP FUNCTION ").candidates)

#@<> WL13397-TSFR_2_3_4_22
EXPECT_CONTAINS([ "logfileGroups" ], EXPECT_SUCCESS("ALTER LOGFILE GROUP ").candidates)

#@<> WL13397-TSFR_2_3_4_23
# note: "userVars" is not expected here due to double @
EXPECT_CONTAINS([ "systemVars" ], EXPECT_SUCCESS("SELECT @@").candidates)
EXPECT_CONTAINS([ "userVars" ], EXPECT_SUCCESS("SET @").candidates)

#@<> WL13397-TSFR_2_3_4_24
EXPECT_EQ([ "tablespaces" ], EXPECT_SUCCESS("DROP UNDO TABLESPACE ", { "serverVersion": "8.0.30" }).candidates)

#@<> WL13397-TSFR_2_3_4_25
EXPECT_EQ([ "users" ], EXPECT_SUCCESS("DROP USER ").candidates)

#@<> WL13397-TSFR_2_3_4_26
EXPECT_EQ([ "charsets" ], EXPECT_SUCCESS("LOAD DATA INFILE 'file_name' INTO TABLE tbl_name CHARACTER SET ").candidates)

#@<> WL13397-TSFR_2_3_4_27
EXPECT_EQ([ "collations" ], EXPECT_SUCCESS("CREATE TABLE t(col1 CHAR(32) COLLATE ").candidates)

#@<> WL13397-TSFR_2_3_4_28
EXPECT_EQ([ "plugins" ], EXPECT_SUCCESS("UNINSTALL PLUGIN ").candidates)

#@<> WL13397-TSFR_2_3_4_29
EXPECT_TRUE("candidates" not in EXPECT_SUCCESS("INSTALL PLUGIN "))

#@<> WL13397-TSFR_2_3_4_30
EXPECT_EQ([ "labels" ], EXPECT_SUCCESS("""CREATE PROCEDURE doiterate(p1 INT)
BEGIN
  label1: LOOP
    SET p1 = p1 + 1;
    IF p1 < 10 THEN ITERATE """).candidates)

#@<> completing various statements
COMPLETE_WHOLE_STATEMENT("CREATE SCHEMA `sakila` DEFAULT CHARACTER SET latin2 DEFAULT COLLATE latin2_bin DEFAULT ENCRYPTION 'N';")
COMPLETE_WHOLE_STATEMENT("ALTER SCHEMA `sakila` DEFAULT CHARACTER SET latin2 DEFAULT COLLATE latin2_bin DEFAULT ENCRYPTION 'N' READ ONLY DEFAULT;")
COMPLETE_WHOLE_STATEMENT("DROP SCHEMA IF EXISTS `sakila`;")
COMPLETE_WHOLE_STATEMENT("CREATE TABLE `tbl` ( `a` INT ) COMMENT='comment' REPLACE AS SELECT 1234;")
COMPLETE_WHOLE_STATEMENT("CREATE TABLE `tbl` ( `col` BIGINT UNSIGNED ZEROFILL COLLATE utf8mb4_bin NOT NULL AUTO_INCREMENT UNIQUE KEY COMMENT 'comment' DEFAULT (uuid2()) COLUMN_FORMAT FIXED ENGINE_ATTRIBUTE='{}' SECONDARY_ENGINE_ATTRIBUTE='{}' STORAGE DISK VISIBLE );")
COMPLETE_WHOLE_STATEMENT("""CREATE TABLE `tbl` ( `pk` INT ) SECONDARY_ENGINE='heatwave' SECONDARY_ENGINE_ATTRIBUTE='{"key":"value;", "key2": 1234}';""")
COMPLETE_WHOLE_STATEMENT("CREATE TABLE `tbl` ( `a` INT COMMENT 'hello' CHECK (1+1=3) NOT ENFORCED );")
COMPLETE_WHOLE_STATEMENT("ALTER TABLE `tbl` CONVERT TO CHARACTER SET latin2 COLLATE latin2_general_ci;")
COMPLETE_WHOLE_STATEMENT("DROP TEMPORARY TABLE IF EXISTS tbl1, tbl2, `sakila`.tbl3 RESTRICT;")
COMPLETE_WHOLE_STATEMENT("ALTER TABLE `tbl` DROP COLUMN `col`;")
COMPLETE_WHOLE_STATEMENT("CREATE INDEX `idx` ON `tbl` (`c1`, `c2`, `c3`) KEY_BLOCK_SIZE 1024 USING HASH COMMENT 'comment' VISIBLE ENGINE_ATTRIBUTE='{}' SECONDARY_ENGINE_ATTRIBUTE='{\"more\": 42}' ALGORITHM=inplace LOCK=shared;")
COMPLETE_WHOLE_STATEMENT("ALTER TABLE `tbl` ADD KEY `idx` USING HASH (`c1`, `c2`, `c3`) KEY_BLOCK_SIZE=1024 COMMENT 'comment' VISIBLE ENGINE_ATTRIBUTE='{}' SECONDARY_ENGINE_ATTRIBUTE='{\"more\": 42}';")
COMPLETE_WHOLE_STATEMENT("ALTER TABLE `tbl` ALTER INDEX `idx` INVISIBLE, RENAME INDEX `idx` TO `iiiidx`;")
COMPLETE_WHOLE_STATEMENT("DROP INDEX `idx` ON `tbl` ALGORITHM inplace LOCK shared;")
COMPLETE_WHOLE_STATEMENT("ALTER TABLE `tbl` ADD CONSTRAINT `fk` FOREIGN KEY `fki` (`c`, `c2`) REFERENCES `tbl` (`c`, `c2`) MATCH PARTIAL ON DELETE CASCADE ON UPDATE SET DEFAULT;")
COMPLETE_WHOLE_STATEMENT("ALTER TABLE `tbl` DROP FOREIGN KEY fk;")
COMPLETE_WHOLE_STATEMENT("CREATE TABLE `tbl` ( `a` INT, CONSTRAINT `chk` CHECK (col+32>1) ENFORCED );")
COMPLETE_WHOLE_STATEMENT("ALTER TABLE `tbl` ALTER CONSTRAINT `chk` NOT ENFORCED;")
COMPLETE_WHOLE_STATEMENT("ALTER TABLE `tbl` DROP CONSTRAINT `chk`;")
COMPLETE_WHOLE_STATEMENT("CREATE DEFINER=root@localhost TRIGGER IF NOT EXISTS `trg` BEFORE UPDATE ON `tbl` FOR EACH ROW FOLLOWS `trg0` BEGIN SHUTDOWN; RESTART; END;")
COMPLETE_WHOLE_STATEMENT("DROP TRIGGER IF EXISTS `trg`;")
COMPLETE_WHOLE_STATEMENT("CREATE OR REPLACE ALGORITHM=UNDEFINED DEFINER=root@localhost SQL SECURITY INVOKER VIEW `vw` (`col1`, `col2`) AS SELECT 12, 34 WITH CASCADED CHECK OPTION;")
COMPLETE_WHOLE_STATEMENT("ALTER ALGORITHM=UNDEFINED DEFINER=root@localhost SQL SECURITY INVOKER VIEW `vw` (`col1`, `col2`) AS SELECT 12, 34 WITH CASCADED CHECK OPTION;")
COMPLETE_WHOLE_STATEMENT("DROP VIEW IF EXISTS `vw1`, `vw2` RESTRICT;")
COMPLETE_WHOLE_STATEMENT("CREATE DEFINER=root@localhost PROCEDURE IF NOT EXISTS `sp`(IN p1 INT, OUT p2 CHAR(32)) COMMENT 'comment' LANGUAGE SQL NOT DETERMINISTIC CONTAINS SQL SQL SECURITY INVOKER BEGIN CREATE TABLE a (x INT); CREATE PROCEDURE spp() SHUTDOWN; END;")
COMPLETE_WHOLE_STATEMENT("ALTER PROCEDURE `sp` COMMENT 'comment' LANGUAGE SQL MODIFIES SQL DATA SQL SECURITY INVOKER;")
COMPLETE_WHOLE_STATEMENT("DROP PROCEDURE IF EXISTS `sp`;")
COMPLETE_WHOLE_STATEMENT("CREATE DEFINER=root@localhost FUNCTION IF NOT EXISTS `fn`(p1 INT, p2 CHAR(32)) RETURNS INT COMMENT 'comment' LANGUAGE SQL NOT DETERMINISTIC CONTAINS SQL SQL SECURITY INVOKER BEGIN CREATE TABLE a (x INT); RETURN 1234; END;")
COMPLETE_WHOLE_STATEMENT("ALTER FUNCTION `fn` COMMENT 'comment' LANGUAGE SQL MODIFIES SQL DATA SQL SECURITY INVOKER;")
COMPLETE_WHOLE_STATEMENT("DROP FUNCTION IF EXISTS `fn`;")
COMPLETE_WHOLE_STATEMENT("CREATE EVENT `ev` ON SCHEDULE EVERY 3 YEAR STARTS '00:00' ENDS now() + INTERVAL 423 MINUTE ON COMPLETION PRESERVE DISABLE ON SLAVE DO BEGIN SELECT 1; END;")
COMPLETE_WHOLE_STATEMENT("ALTER DEFINER=me@'%' EVENT `ev` ON SCHEDULE EVERY 3 YEAR STARTS '00:00' ENDS now() + INTERVAL 423 MINUTE ON COMPLETION NOT PRESERVE RENAME TO `ev2` DISABLE COMMENT 'comment' DO RESTART;")
COMPLETE_WHOLE_STATEMENT("DROP EVENT IF EXISTS `ev`;")
COMPLETE_WHOLE_STATEMENT("SET PERSIST group_replication_group_name='';")
COMPLETE_WHOLE_STATEMENT("SELECT @@global.server_uuid;")
COMPLETE_WHOLE_STATEMENT("SHOW FULL PROCESSLIST;")
COMPLETE_WHOLE_STATEMENT("SHOW FULL TABLES LIKE '';")
COMPLETE_WHOLE_STATEMENT("USE information_schema;")

#@<> BUG#34334554: delimiter is not suggested as a candidate
COMPLETE_WHOLE_STATEMENT("DELIMITER $$")
EXPECT_CONTAINS([ "DELIMITER" ], EXPECT_SUCCESS("DEL").keywords)
EXPECT_CONTAINS([ "DELIMITER" ], EXPECT_SUCCESS("DELIMITER $$", { "statementOffset": 3 }).keywords)

#@<> BUG#34343266
EXPECT_CONTAINS([ "@", "GLOBAL", "LOCAL", "PERSIST", "PERSIST_ONLY", "SESSION" ], EXPECT_SUCCESS("SET ", { "serverVersion": "8.0.30" }).keywords)
EXPECT_CONTAINS([ "@", "GLOBAL", "LOCAL", "SESSION" ], EXPECT_SUCCESS("SET ", { "serverVersion": "5.7.33" }).keywords)
EXPECT_NOT_CONTAINS([ "PERSIST", "PERSIST_ONLY" ], EXPECT_SUCCESS("SET ", { "serverVersion": "5.7.33" }).keywords)

EXPECT_CONTAINS([ "systemVars" ], EXPECT_SUCCESS("SET ").candidates)
EXPECT_NOT_CONTAINS([ "userVars" ], EXPECT_SUCCESS("SET ").candidates)

EXPECT_CONTAINS([ "systemVars" ], EXPECT_SUCCESS("SET GLOBAL ").candidates)
EXPECT_NOT_CONTAINS([ "userVars" ], EXPECT_SUCCESS("SET GLOBAL ").candidates)

EXPECT_EQ([ "@" ], EXPECT_SUCCESS("SET @").keywords)
EXPECT_EQ([ "userVars" ], EXPECT_SUCCESS("SET @").candidates)

EXPECT_EQ([ "GLOBAL", "LOCAL", "PERSIST", "PERSIST_ONLY", "SESSION" ], EXPECT_SUCCESS("SET @@", { "serverVersion": "8.0.30" }).keywords)
EXPECT_EQ([ "GLOBAL", "LOCAL", "SESSION" ], EXPECT_SUCCESS("SET @@", { "serverVersion": "5.7.33" }).keywords)
EXPECT_EQ([ "systemVars" ], EXPECT_SUCCESS("SET @@").candidates)
EXPECT_EQ([ "systemVars" ], EXPECT_SUCCESS("SET @@GLOBAL.").candidates)

EXPECT_CONTAINS([ "@" ], EXPECT_SUCCESS("SELECT ").keywords)
EXPECT_NOT_CONTAINS([ "systemVars", "userVars" ], EXPECT_SUCCESS("SELECT ").candidates)

EXPECT_EQ([ "@" ], EXPECT_SUCCESS("SELECT @").keywords)
EXPECT_EQ([ "userVars" ], EXPECT_SUCCESS("SELECT @").candidates)

EXPECT_EQ([ "GLOBAL.", "LOCAL.", "SESSION." ], EXPECT_SUCCESS("SELECT @@").keywords)
EXPECT_EQ([ "systemVars" ], EXPECT_SUCCESS("SELECT @@").candidates)
EXPECT_EQ([ "systemVars" ], EXPECT_SUCCESS("SELECT @@GLOBAL.").candidates)

#@<> BUG#34370621
EXPECT_EQ([ "systemVars" ], EXPECT_SUCCESS("RESET PERSIST ", { "serverVersion": "8.0.30" }).candidates)
EXPECT_EQ([ "systemVars" ], EXPECT_SUCCESS("RESET PERSIST IF EXISTS ", { "serverVersion": "8.0.30" }).candidates)

#@<> BUG#34370524
EXPECT_EQ("", EXPECT_SUCCESS("EXPLAIN", { "statementOffset": 0 }).context.prefix)
EXPECT_EQ("E", EXPECT_SUCCESS("EXPLAIN", { "statementOffset": 1 }).context.prefix)
EXPECT_EQ("EX", EXPECT_SUCCESS("EXPLAIN", { "statementOffset": 2 }).context.prefix)
EXPECT_EQ("EXP", EXPECT_SUCCESS("EXPLAIN", { "statementOffset": 3 }).context.prefix)
EXPECT_EQ("EXPL", EXPECT_SUCCESS("EXPLAIN", { "statementOffset": 4 }).context.prefix)
EXPECT_EQ("EXPLA", EXPECT_SUCCESS("EXPLAIN", { "statementOffset": 5 }).context.prefix)
EXPECT_EQ("EXPLAI", EXPECT_SUCCESS("EXPLAIN", { "statementOffset": 6 }).context.prefix)
EXPECT_EQ("EXPLAIN", EXPECT_SUCCESS("EXPLAIN", { "statementOffset": 7 }).context.prefix)
EXPECT_EQ("EXPLAIN", EXPECT_SUCCESS("EXPLAIN", { "statementOffset": 10 }).context.prefix)

EXPECT_EQ("", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 0 }).context.prefix)
EXPECT_EQ("E", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 1 }).context.prefix)
EXPECT_EQ("EX", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 2 }).context.prefix)
EXPECT_EQ("EXP", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 3 }).context.prefix)
EXPECT_EQ("EXPL", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 4 }).context.prefix)
EXPECT_EQ("EXPLA", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 5 }).context.prefix)
EXPECT_EQ("EXPLAI", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 6 }).context.prefix)
EXPECT_EQ("EXPLAIN", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 7 }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 8 }).context.prefix)
EXPECT_EQ("S", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 9 }).context.prefix)
EXPECT_EQ("SE", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 10 }).context.prefix)
EXPECT_EQ("SEL", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 11 }).context.prefix)
EXPECT_EQ("SELE", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 12 }).context.prefix)
EXPECT_EQ("SELEC", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 13 }).context.prefix)
EXPECT_EQ("SELECT", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 14 }).context.prefix)
EXPECT_EQ("SELECT", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 15 }).context.prefix)
EXPECT_EQ("SELECT", EXPECT_SUCCESS("EXPLAIN SELECT", { "statementOffset": 20 }).context.prefix)

EXPECT_EQ("EXPLAIN", EXPECT_SUCCESS("EXPLAIN  SELECT", { "statementOffset": 7 }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("EXPLAIN  SELECT", { "statementOffset": 8 }).context.prefix)
EXPECT_EQ("", EXPECT_SUCCESS("EXPLAIN  SELECT", { "statementOffset": 9 }).context.prefix)
EXPECT_EQ("S", EXPECT_SUCCESS("EXPLAIN  SELECT", { "statementOffset": 10 }).context.prefix)

EXPECT_EQ("", EXPECT_SUCCESS("EXPLAIN SELECT ", { "statementOffset": 15 }).context.prefix)
