//@<OUT> Create an index on a single field. 1 (WL10858-FR1_1)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: 10
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index on a single field. 2 (WL10858-FR1_1)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` text GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField'))) VIRTUAL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`(10))
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`(10)),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci

//@<OUT> Create an index on a single field with all the possibles options. 1 (WL10858-FR1_2)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: 10
       Packed: NULL
         Null: 
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index on a single field with all the possibles options. 2 (WL10858-FR1_2)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` text GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField'))) VIRTUAL NOT NULL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`(10))
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`(10)),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci

//@<OUT> Create an index on multiple fields 1 (WL10858-FR1_3)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: 10
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL
*************************** 2. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 2
  Column_name: <<<idx_col_2>>>
    Collation: A
  Cardinality: 0
     Sub_part: 10
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL
*************************** 3. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 3
  Column_name: <<<idx_col_3>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index on multiple fields 2 (WL10858-FR1_3)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` text GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField'))) VIRTUAL,
  `<<<idx_col_2>>>` text GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField2'))) VIRTUAL,
?{VER(<8.0.19)}
  `<<<idx_col_3>>>` int(11) GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField3')) VIRTUAL,
?{}
?{VER(>=8.0.19)}
  `<<<idx_col_3>>>` int GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField3')) VIRTUAL,
?{}
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`(10),`<<<idx_col_2>>>`(10),`<<<idx_col_3>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`(10),`<<<idx_col_2>>>`(10),`<<<idx_col_3>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci

//@<OUT> Create an index on multiple fields with all the possibles options. 1 (WL10858-FR1_4)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: 10
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL
*************************** 2. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 2
  Column_name: <<<idx_col_2>>>
    Collation: A
  Cardinality: 0
     Sub_part: 10
       Packed: NULL
         Null: 
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL
*************************** 3. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 3
  Column_name: <<<idx_col_3>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index on multiple fields with all the possibles options. 2 (WL10858-FR1_4)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` text GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField'))) VIRTUAL,
  `<<<idx_col_2>>>` text GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField2'))) VIRTUAL NOT NULL,
?{VER(<8.0.19)}
  `<<<idx_col_3>>>` int(11) GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField3')) VIRTUAL,
?{}
?{VER(>=8.0.19)}
  `<<<idx_col_3>>>` int GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField3')) VIRTUAL,
?{}
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`(10),`<<<idx_col_2>>>`(10),`<<<idx_col_3>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`(10),`<<<idx_col_2>>>`(10),`<<<idx_col_3>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a geojson datatype field. 1 (WL10858-FR1_5)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: 32
       Packed: NULL
         Null: 
   Index_type: SPATIAL
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a geojson datatype field. 2 (WL10858-FR1_5)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` geometry GENERATED ALWAYS AS (st_geomfromgeojson(json_extract(`doc`,_utf8mb4'$.myGeoJsonField'),1,4326)) STORED NOT NULL /*!80003 SRID 4326 */,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  SPATIAL KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  SPATIAL KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci

//@<OUT> Create an index using a geojson datatype field without specifying the required flag it should be set to true by default. 1 (WL10858-FR1_6)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: 32
       Packed: NULL
         Null: 
   Index_type: SPATIAL
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a geojson datatype field without specifying the required flag it should be set to true by default. 2 (WL10858-FR1_6)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` geometry GENERATED ALWAYS AS (st_geomfromgeojson(json_extract(`doc`,_utf8mb4'$.myGeoJsonField'),1,4326)) STORED NOT NULL /*!80003 SRID 4326 */,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  SPATIAL KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  SPATIAL KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a geojson datatype field with all the possibles options. 1 (WL10858-FR1_7)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: 32
       Packed: NULL
         Null: 
   Index_type: SPATIAL
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a geojson datatype field with all the possibles options. 2 (WL10858-FR1_7)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` geometry GENERATED ALWAYS AS (st_geomfromgeojson(json_extract(`doc`,_utf8mb4'$.myGeoJsonField'),2,4400)) STORED NOT NULL /*!80003 SRID 4400 */,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  SPATIAL KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  SPATIAL KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci

//@<OUT> Create an index using a datetime field. 1 (WL10858-FR1_8)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a datetime field. 2 (WL10858-FR1_8)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` datetime GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField'))) VIRTUAL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a timestamp field. 1 (WL10858-FR1_9)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a timestamp field. 2 (WL10858-FR1_9)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` timestamp GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField'))) VIRTUAL NULL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci

//@<OUT> Create an index using a time field. 1 (WL10858-FR1_10)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a time field. 2 (WL10858-FR1_10)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` time GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField'))) VIRTUAL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a date field. 1 (WL10858-FR1_11)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a date field. 2 (WL10858-FR1_11)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` date GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField'))) VIRTUAL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a numeric field. 1 (WL10858-FR1_12)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a numeric field. 2 (WL10858-FR1_12)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` decimal(10,0) unsigned GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> FR1_13	Create an index using a decimal field. 1 (WL10858-FR1_13)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> FR1_13	Create an index using a decimal field. 2 (WL10858-FR1_13)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` decimal(10,0) GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a double field. 1 (WL10858-FR1_14)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a double field. 2 (WL10858-FR1_14)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` double GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a float field. 1 (WL10858-FR1_15)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a float field. 2 (WL10858-FR1_15)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` float unsigned GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a real field. 1 (WL10858-FR1_16)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a real field. 2 (WL10858-FR1_16)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` double unsigned GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a bigint field. 1 (WL10858-FR1_17)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a bigint field. 2 (WL10858-FR1_17)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(<8.0.19)}
  `<<<idx_col_1>>>` bigint(20) GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
?{}
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
  `<<<idx_col_1>>>` bigint GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
?{}
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a integer field. 1 (WL10858-FR1_18)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a integer field. 2 (WL10858-FR1_18)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(<8.0.19)}
  `<<<idx_col_1>>>` int(10) unsigned GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
?{}
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
  `<<<idx_col_1>>>` int unsigned GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
?{}
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a mediumint field. 1 (WL10858-FR1_19)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a mediumint field. 2 (WL10858-FR1_19)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(<8.0.19)}
  `<<<idx_col_1>>>` mediumint(8) unsigned GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
?{}
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
  `<<<idx_col_1>>>` mediumint unsigned GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
?{}
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a smallint field. 1 (WL10858-FR1_20)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a smallint field. 2 (WL10858-FR1_20)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(<8.0.19)}
  `<<<idx_col_1>>>` smallint(6) GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
?{}
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
  `<<<idx_col_1>>>` smallint GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
?{}
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Create an index using a tinyint field. 1 (WL10858-FR1_21)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: NULL
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Create an index using a tinyint field. 2 (WL10858-FR1_21)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(<8.0.19)}
  `<<<idx_col_1>>>` tinyint(3) unsigned GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
?{}
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
  `<<<idx_col_1>>>` tinyint unsigned GENERATED ALWAYS AS (json_extract(`doc`,_utf8mb4'$.myField')) VIRTUAL,
?{}
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`)
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@<OUT> Verify that the dropIndex function removes the index entry from the table schema of a collection. 1 (WL10858-FR4_1)
*************************** 1. row ***************************
        Table: my_coll
   Non_unique: 1
     Key_name: myIndex
 Seq_in_index: 1
  Column_name: <<<idx_col_1>>>
    Collation: A
  Cardinality: 0
     Sub_part: 10
       Packed: NULL
         Null: YES
   Index_type: BTREE
      Comment: 
Index_comment: 
      Visible: YES
   Expression: NULL

//@<OUT> Verify that the dropIndex function removes the index entry from the table schema of a collection. 2 (WL10858-FR4_1)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
?{}
  `<<<idx_col_1>>>` text GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$.myField'))) VIRTUAL,
  PRIMARY KEY (`_id`),
?{VER(<8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`(10))
?{}
?{VER(>=8.0.19)}
  KEY `myIndex` (`<<<idx_col_1>>>`(10)),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci


//@ Verify that the dropIndex function removes the index entry from the table schema of a collection. 3 (WL10858-FR4_1)
|Empty set|

//@<OUT> Verify that the dropIndex function removes the index entry from the table schema of a collection. 4 (WL10858-FR4_1)
*************************** 1. row ***************************
       Table: my_coll
Create Table: CREATE TABLE `my_coll` (
  `doc` json DEFAULT NULL,
  `_id` varbinary(32) GENERATED ALWAYS AS (json_unquote(json_extract(`doc`,_utf8mb4'$._id'))) STORED NOT NULL,
?{VER(<8.0.19)}
  PRIMARY KEY (`_id`)
?{}
?{VER(>=8.0.19)}
  `_json_schema` json GENERATED ALWAYS AS (_utf8mb4'{"type":"object"}') VIRTUAL,
  PRIMARY KEY (`_id`),
  CONSTRAINT `$val_strict_98ECC39AA1BEFEB54F58E37A530CD5D1BD7631C5` CHECK (json_schema_valid(`_json_schema`,`doc`)) /*!80016 NOT ENFORCED */
?{}
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci

//@ Verify that the dropIndex silently succeeds if the index does not exist. (WL10858-FR4_2)
||

//@ Create an index with the name of an index that already exists. (WL10858-FR5_2)
||Duplicate key name 'myIndex' (MySQL Error 1061)

//@ Create an index with a not valid JSON document definition. (WL10858-FR5_3)
||Invalid shorthand property name initializer in object literal (SyntaxError)
||Expected comma but found ] (SyntaxError)
||Expected comma but found } (SyntaxError)

//@ Create an index where its definition is a JSON document but its structure is not valid. (WL10858-FR5_4)
||Invalid number of arguments, expected value for 'fields[0].field' (MySQL Error 5015)

//@ Create an index with the index type not "INDEX" or "SPATIAL" (case insensitive). (WL10858-FR5_5)
||Argument value 'IDX' for index type is invalid (MySQL Error 5017)
||Argument value 'SPATIAL_' for index type is invalid (MySQL Error 5017)
||Argument value 'INVALID' for index type is invalid (MySQL Error 5017)

//@ Create a 'SPATIAL' index with "required" flag set to false. (WL10858-FR5_6)
||GEOJSON index requires 'field.required: TRUE (MySQL Error 5117)

//@ Create an index with an invalid "type" specified (type names are case insensitive). (WL10858-FR5_7)
||Invalid or unsupported type specification '_Text(10)' (MySQL Error 5017)
||Invalid or unsupported type specification 'Invalid' (MySQL Error 5017)
||Invalid or unsupported type specification 'Timestamps' (MySQL Error 5017)
||Invalid or unsupported type specification 'Dates' (MySQL Error 5017)

//@ Create an index specifiying geojson options for non geojson data type. (WL10858-FR5_8)
||Unsupported argument specification for '$.myField' (MySQL Error 5017)

//@ Create an index with mismatched data types (WL10858-ET_1)
||Incorrect datetime value: '10' for column
||at row 1 (MySQL Error 1292)

//@ Create an index specifiying SPATIAL as the index type for a non spatial data type (WL10858-ET_2)
||'Spatial index on virtual generated column' is not supported for generated columns. (MySQL Error 3106)

//@ Create an index specifiying INDEX as the index type for a spatial data type (WL10858-ET_3)
||Column '$ix_gj_r_B4C4FDF5AD30671EF010BCE1E67FA76778A889F7' cannot be null
