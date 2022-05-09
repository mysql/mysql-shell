#@ {VER(>=8.0)}
#@<> Setup
testutil.deploy_raw_sandbox(port=__mysql_sandbox_port1, rootpass="root")
testutil.snapshot_sandbox_conf(port=__mysql_sandbox_port1)


rc = testutil.call_mysqlsh([__sandbox_uri1, "--sql", "--file", os.path.join(__data_path, "sql", "fieldtypes_all.sql")])


expected = {
    "t_bit": {
        "c1": { # BIT(1)
            "FLAGS": ["UNSIGNED"], 
            "LENGTH": 1, 
            "TYPE": "BIT"
        }, 
        "c2": { # BIT(64)
            "FLAGS": ["UNSIGNED"], 
            "LENGTH": 64, 
            "TYPE": "BIT"
        }
    }, 
    "t_char": { # CHARACTER SET 'utf8mb4';
        "c1": { # CHAR(32)
            "FLAGS": [], 
            "LENGTH": 128, 
            "TYPE": "STRING"
        }, 
        "c2": { # VARCHAR(32)  []
            "FLAGS": [], 
            "LENGTH": 128, 
            "TYPE": "STRING"
        }, 
        "c3": { # BINARY(32)
            "FLAGS": ["BINARY"], 
            "LENGTH": 32, 
            "TYPE": "BYTES"
        }, 
        "c4": { # VARBINARY(32)
            "FLAGS": ["BINARY"], 
            "LENGTH": 32, 
            "TYPE": "BYTES"
        }, 
        "c5": { # VARCHAR(32) COLLATE 'latin1_swedish_ci'
            "FLAGS": [], 
            "LENGTH": 32, 
            "TYPE": "STRING"
        }
    }, 
    "t_date": {
        "c1": { # DATE
            "FLAGS": ["BINARY"], 
            "LENGTH": 10, 
            "TYPE": "DATE"
        }, 
        "c2": { # TIME
            "FLAGS": ["BINARY"], 
            "LENGTH": 10, 
            "TYPE": "TIME"
        }, 
        "c3": { # TIMESTAMP
            "FLAGS": {
                "X": ["BINARY", "TIMESTAMP"],
                "CLASSIC": ["BINARY"],
            },
            "LENGTH": 19, 
            "TYPE": "DATETIME"
        }, 
        "c4": { # DATETIME
            "FLAGS": ["BINARY"], 
            "LENGTH": 19, 
            "TYPE": "DATETIME"
        }, 
        "c5": { # YEAR
            "FLAGS": {
                "X": ["UNSIGNED", "NUM"],
                "CLASSIC": ["UNSIGNED", "ZEROFILL", "NUM"],
            },
            "LENGTH": 4, 
            "TYPE": "TINYINT"
        }
    }, 
    "t_decimal1": {
        "c1": { # DECIMAL(2,1)
            "FLAGS": ["NUM"], 
            "LENGTH": 4, 
            "TYPE": "DECIMAL"
        }, 
        "c2": { # DECIMAL(2,1) UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 3, 
            "TYPE": "DECIMAL"
        }
    }, 
    "t_decimal2": {
        "c1": { # DECIMAL(64,30)
            "FLAGS": ["NUM"], 
            "LENGTH": 66, 
            "TYPE": "DECIMAL"
        }, 
        "c2": { # DECIMAL(64,30) UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 65, 
            "TYPE": "DECIMAL"
        }
    }, 
    "t_decimal3": {
        "c1": { # DECIMAL(2,0)
            "FLAGS": ["NUM"], 
            "LENGTH": 3, 
            "TYPE": "DECIMAL"
        }, 
        "c2": { # DECIMAL(2,0) UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 2, 
            "TYPE": "DECIMAL"
        }
    }, 
    "t_double": {
        "c1": { # DOUBLE
            "FLAGS": ["NUM"], 
            "LENGTH": 22, 
            "TYPE": "DOUBLE"
        }, 
        "c2": { # DOUBLE UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 22, 
            "TYPE": "DOUBLE"
        }
    }, 
    "t_enum": {
        "c1": { # ENUM('v1','v2','v3')
            "FLAGS": ["ENUM"], 
            "LENGTH": 0, 
            "TYPE": "ENUM"
        }, 
        "c2": { # ENUM('')
            "FLAGS": ["ENUM"], 
            "LENGTH": 0, 
            "TYPE": "ENUM"
        }
    }, 
    "t_float": {
        "c1": { # FLOAT
            "FLAGS": ["NUM"], 
            "LENGTH": 12, 
            "TYPE": "FLOAT"
        }, 
        "c2": { # FLOAT UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 12, 
            "TYPE": "FLOAT"
        }
    }, 
    "t_geom": {
        "g": { # GEOMETRY
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 0, 
            "TYPE": "GEOMETRY"
        }
    }, 
    "t_geom_all": {
        "g": { # GEOMETRY
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 0, 
            "TYPE": "GEOMETRY"
        }, 
        "gc": { # POINT
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 0, 
            "TYPE": "GEOMETRY"
        }, 
        "l": { # LINESTRING
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 0, 
            "TYPE": "GEOMETRY"
        }, 
        "ml": { # POLYGON
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 0, 
            "TYPE": "GEOMETRY"
        }, 
        "mp": { # MULTIPOINT
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 0, 
            "TYPE": "GEOMETRY"
        }, 
        "mpl": { # MULTILINESTRING
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 0, 
            "TYPE": "GEOMETRY"
        }, 
        "p": { # MULTIPOLYGON
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 0, 
            "TYPE": "GEOMETRY"
        }, 
        "pl": { # GEOMETRYCOLLECTION
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 0, 
            "TYPE": "GEOMETRY"
        }
    }, 
    "t_int": {
        "c1": { # INT
            "FLAGS": ["NUM"], 
            "LENGTH": 11, 
            "TYPE": "INT"
        }, 
        "c2": { # INT UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 10, 
            "TYPE": "INT"
        }
    }, 
    "t_integer": {
        "c1": { # INTEGER
            "FLAGS": ["NUM"], 
            "LENGTH": 11, 
            "TYPE": "INT"
        }, 
        "c2": { # INTEGER UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 10, 
            "TYPE": "INT"
        }
    }, 
    "t_lchar": { # CHARACTER SET 'latin1';
        "c1": { # CHAR(32)
            "FLAGS": [], 
            "LENGTH": 32, 
            "TYPE": "STRING"
        }, 
        "c2": { # VARCHAR(32)
            "FLAGS": [], 
            "LENGTH": 32, 
            "TYPE": "STRING"
        }, 
        "c3": { # BINARY(32)
            "FLAGS": ["BINARY"], 
            "LENGTH": 32, 
            "TYPE": "BYTES"
        }, 
        "c4": { # VARBINARY(32)
            "FLAGS": ["BINARY"], 
            "LENGTH": 32, 
            "TYPE": "BYTES"
        }
    }, 
    "t_lob": {
        "c1": { # TINYBLOB
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 255, 
            "TYPE": "BYTES"
        }, 
        "c2": { # BLOB
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 65535, 
            "TYPE": "BYTES"
        }, 
        "c3": { # MEDIUMBLOB
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 16777215, 
            "TYPE": "BYTES"
        }, 
        "c4": { # LONGBLOB
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 4294967295, 
            "TYPE": "BYTES"
        }, 
        "c5": { # TINYTEXT
            "FLAGS": ["BLOB"], 
            "LENGTH": 255, 
            "TYPE": "STRING"
        }, 
        "c6": { # TEXT
            "FLAGS": ["BLOB"], 
            "LENGTH": 65535, 
            "TYPE": "STRING"
        }, 
        "c7": { # MEDIUMTEXT
            "FLAGS": ["BLOB"], 
            "LENGTH": 16777215, 
            "TYPE": "STRING"
        }, 
        "c8": { # LONGTEXT
            "FLAGS": ["BLOB"], 
            "LENGTH": 4294967295, 
            "TYPE": "STRING"
        }, 
        "c9": { # TINYTEXT BINARY
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 255, 
            "TYPE": "STRING"
        },
        "c10": { # TEXT BINARY
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 65535, 
            "TYPE": "STRING"
        }, 
        "c11": { # MEDIUMTEXT BINARY
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 16777215, 
            "TYPE": "STRING"
        }, 
        "c12": { # LONGTEXT BINARY
            "FLAGS": ["BLOB", "BINARY"], 
            "LENGTH": 4294967295, 
            "TYPE": "STRING"
        }, 
    }, 
    "t_mediumint": {
        "c1": { # MEDIUMINT
            "FLAGS": ["NUM"], 
            "LENGTH": 9, 
            "TYPE": "MEDIUMINT"
        }, 
        "c2": { # MEDIUMINT UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 8, 
            "TYPE": "MEDIUMINT"
        }
    }, 
    "t_numeric1": {
        "c1": { # NUMERIC(2,1)
            "FLAGS": ["NUM"], 
            "LENGTH": 4, 
            "TYPE": "DECIMAL"
        }, 
        "c2": { # NUMERIC(2,1) UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 3, 
            "TYPE": "DECIMAL"
        }
    }, 
    "t_numeric2": {
        "c1": { # NUMERIC(64,30)
            "FLAGS": ["NUM"], 
            "LENGTH": 66, 
            "TYPE": "DECIMAL"
        }, 
        "c2": { # NUMERIC(64,30) UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 65, 
            "TYPE": "DECIMAL"
        }
    }, 
    "t_real": {
        "c1": { # REAL
            "FLAGS": ["NUM"], 
            "LENGTH": 22, 
            "TYPE": "DOUBLE"
        }, 
        "c2": { # REAL UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 22, 
            "TYPE": "DOUBLE"
        }
    }, 
    "t_set": {
        "c1": { # SET('v1','v2','v3')
            "FLAGS": ["SET"], 
            "LENGTH": 0, 
            "TYPE": "SET"
        }, 
        "c2": { # SET('')
            "FLAGS": ["SET"], 
            "LENGTH": 0, 
            "TYPE": "SET"
        }
    }, 
    "t_smallint": {
        "c1": { # SMALLINT
            "FLAGS": ["NUM"], 
            "LENGTH": 6, 
            "TYPE": "SMALLINT"
        }, 
        "c2": { # SMALLINT UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 5, 
            "TYPE": "SMALLINT"
        }
    }, 
    "t_tinyint": {
        "c1": { # TINYINT
            "FLAGS": ["NUM"], 
            "LENGTH": 4, 
            "TYPE": "TINYINT"
        }, 
        "c2": { # TINYINT UNSIGNED
            "FLAGS": ["UNSIGNED", "NUM"], 
            "LENGTH": 3, 
            "TYPE": "TINYINT"
        }
    }
}



def check_flags(session, protocol):
    for table in expected.keys():
        res = session.run_sql(f"select * from xtest.{table}")
        columns = res.get_columns()
        for column in columns:
            EXPECT_EQ(expected[table][column.column_name]['TYPE'], column.type.data)
            if isinstance(expected[table][column.column_name]['FLAGS'], dict):
                # NOTE: There are a couple of inconsistencies on the reported flags between
                # X protocol and classic, as follows:
                # - Columns with type TIMESTAMP are reported with TIMESTAMP flag in X but not in Classic
                #   i.e. t_date.c3
                # - Columns with type YEAR are reported with ZEROFILL flag in Classic, but not in X
                #   i.e. t_date.c5
                EXPECT_EQ(expected[table][column.column_name]['FLAGS'][protocol], column.flags.split(), f"{table}.{column.column_name}")
            else:
                EXPECT_EQ(expected[table][column.column_name]['FLAGS'], column.flags.split(), f"{table}.{column.column_name}")


#@<> Check Flags in Classic Protocol
shell.connect(__sandbox_uri1)
check_flags(session, "CLASSIC")
session.close()

shell.connect(__sandbox_uri1.replace("mysql", "mysqlx")+"0")
check_flags(session, "X")
session.close()

#@<> Cleanup
testutil.destroy_sandbox(port=__mysql_sandbox_port1)
