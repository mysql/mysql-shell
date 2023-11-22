/*
 * Copyright (c) 2016, 2023, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#pragma once

#include <set>
#include <string>

static const std::set<std::string> systemFunctions57 = {
    "ABS",
    "ACOS",
    "ADDDATE",
    "ADDTIME",
    "AES_DECRYPT",
    "AES_ENCRYPT",
    "ANY_VALUE",
    // "Area",  // deprecated, use ST_Area() instead
    // "AsBinary",  // deprecated, use ST_AsBinary() instead
    "ASCII",
    "ASIN",
    // "AsText",  // deprecated, use ST_AsText() instead
    // "AsWKB",  // deprecated, use ST_AsWKB() instead
    // "AsWKT",  // deprecated, use ST_AsWKT() instead
    "ATAN",
    "ATAN2",
    "AVG",
    "BENCHMARK",
    "BIN",
    "BIT_AND",
    "BIT_COUNT",
    "BIT_LENGTH",
    "BIT_OR",
    "BIT_XOR",
    // "Buffer",  // deprecated, use ST_Buffer() instead
    "CAST",
    "CEIL",
    "CEILING",
    // "Centroid",  // deprecated, use ST_Centroid() instead
    "CHAR",
    "CHAR_LENGTH",
    "CHARACTER_LENGTH",
    "CHARSET",
    "COALESCE",
    "COERCIBILITY",
    "COLLATION",
    "COMPRESS",
    "CONCAT",
    "CONCAT_WS",
    "CONNECTION_ID",
    // "Contains",  // deprecated, use MBRContains() instead
    "CONV",
    "CONVERT",
    "CONVERT_TZ",
    // "ConvexHull",  // deprecated, use ST_ConvexHull() instead
    "COS",
    "COT",
    "COUNT",
    "CRC32",
    // "Crosses",  // deprecated, use ST_Crosses() instead
    "CURDATE",
    "CURRENT_DATE",
    "CURRENT_TIME",
    "CURRENT_TIMESTAMP",
    "CURRENT_USER",
    "CURTIME",
    "DATABASE",
    "DATE",
    "DATE_ADD",
    "DATE_FORMAT",
    "DATE_SUB",
    "DATEDIFF",
    "DAY",
    "DAYNAME",
    "DAYOFMONTH",
    "DAYOFWEEK",
    "DAYOFYEAR",
    "DECODE",  // deprecated in 5.7
    "DEFAULT",
    "DEGREES",
    "DES_DECRYPT",  // deprecated in 5.7
    "DES_ENCRYPT",  // deprecated in 5.7
    // "Dimension",  // deprecated, use ST_Dimension() instead
    // "Disjoint",  // deprecated, use MBRDisjoint() instead
    // "Distance",  // deprecated, use ST_Distance() instead
    "ELT",
    "ENCODE",   // deprecated in 5.7
    "ENCRYPT",  // deprecated in 5.7
    // "EndPoint",  // deprecated, use ST_EndPoint() instead
    // "Envelope",  // deprecated, use ST_Envelope() instead
    // "Equals",  // deprecated, use MBREquals() instead
    "EXP",
    "EXPORT_SET",
    // "ExteriorRing",  // deprecated, use ST_ExteriorRing() instead
    "EXTRACT",
    "ExtractValue",
    "FIELD",
    "FIND_IN_SET",
    "FLOOR",
    "FORMAT",
    "FOUND_ROWS",
    "FROM_BASE64",
    "FROM_DAYS",
    "FROM_UNIXTIME",
    // "GeomCollFromText",  // deprecated, use ST_GeomCollFromText() instead
    // "GeomCollFromWKB",  // deprecated, use ST_GeomCollFromWKB() instead
    "GeometryCollection",
    // "GeometryCollectionFromText",  // use ST_GeometryCollectionFromText()
    // "GeometryCollectionFromWKB",  // use ST_GeometryCollectionFromWKB()
    // "GeometryFromText",  // deprecated, use ST_GeometryFromText() instead
    // "GeometryFromWKB",  // deprecated, use ST_GeometryFromWKB() instead
    // "GeometryN",  // deprecated, use ST_GeometryN() instead
    // "GeometryType",  // deprecated, use ST_GeometryType() instead
    // "GeomFromText",  // deprecated, use ST_GeomFromText() instead
    // "GeomFromWKB",  // deprecated, use ST_GeomFromWKB() instead
    "GET_FORMAT",
    "GET_LOCK",
    // "GLength",  // deprecated, use ST_Length() instead
    "GREATEST",
    "GROUP_CONCAT",
    "GTID_SUBSET",
    "GTID_SUBTRACT",
    "HEX",
    "HOUR",
    "IF",
    "IFNULL",
    // "IN",  // operator
    "INET_ATON",
    "INET_NTOA",
    "INET6_ATON",
    "INET6_NTOA",
    "INSERT",
    "INSTR",
    // "InteriorRingN",  // deprecated, use ST_InteriorRingN() instead
    // "Intersects",  // deprecated, use MBRIntersects() instead
    "INTERVAL",
    "IS_FREE_LOCK",
    "IS_IPV4",
    "IS_IPV4_COMPAT",
    "IS_IPV4_MAPPED",
    "IS_IPV6",
    "IS_USED_LOCK",
    // "IsClosed",  // deprecated, use ST_IsClosed() instead
    // "IsEmpty",  // deprecated, use ST_IsEmpty() instead
    "ISNULL",
    // "IsSimple",  // deprecated, use ST_IsSimple() instead
    // "JSON_APPEND",  // deprecated, use JSON_ARRAY_APPEND() instead
    "JSON_ARRAY",
    "JSON_ARRAY_APPEND",
    "JSON_ARRAY_INSERT",
    "JSON_ARRAYAGG",  // added: 5.7.22
    "JSON_CONTAINS",
    "JSON_CONTAINS_PATH",
    "JSON_DEPTH",
    "JSON_EXTRACT",
    "JSON_INSERT",
    "JSON_KEYS",
    "JSON_LENGTH",
    // "JSON_MERGE",  // deprecated, use JSON_MERGE_PRESERVE() instead
    "JSON_MERGE_PATCH",     // added: 5.7.22
    "JSON_MERGE_PRESERVE",  // added: 5.7.22
    "JSON_OBJECT",
    "JSON_OBJECTAGG",  // added: 5.7.22
    "JSON_PRETTY",     // added: 5.7.22
    "JSON_QUOTE",
    "JSON_REMOVE",
    "JSON_REPLACE",
    "JSON_SEARCH",
    "JSON_SET",
    "JSON_STORAGE_SIZE",
    "JSON_TYPE",
    "JSON_UNQUOTE",
    "JSON_VALID",
    "LAST_DAY",
    "LAST_INSERT_ID",
    "LCASE",
    "LEAST",
    "LEFT",
    "LENGTH",
    // "LineFromText",  // deprecated, use ST_LineFromText() instead
    // "LineFromWKB",  // deprecated, use ST_LineFromWKB() instead
    // "LineStringFromText",  // deprecated, use ST_LineStringFromText() instead
    // "LineStringFromWKB",  // deprecated, use ST_LineStringFromWKB() instead
    "LineString",
    "LN",
    "LOAD_FILE",
    "LOCALTIME",
    "LOCALTIMESTAMP",
    "LOCATE",
    "LOG",
    "LOG10",
    "LOG2",
    "LOWER",
    "LPAD",
    "LTRIM",
    "MAKE_SET",
    "MAKEDATE",
    "MAKETIME",
    "MASTER_POS_WAIT",
    "MATCH",
    "MAX",
    "MBRContains",
    "MBRCoveredBy",
    "MBRCovers",
    "MBRDisjoint",
    // "MBREqual",  // deprecated, use MBREquals() instead
    "MBREquals",
    "MBRIntersects",
    "MBROverlaps",
    "MBRTouches",
    "MBRWithin",
    "MD5",
    "MICROSECOND",
    "MID",
    "MIN",
    "MINUTE",
    // "MLineFromText",  // deprecated, use ST_MLineFromText() instead
    // "MLineFromWKB",  // deprecated, use ST_MLineFromWKB() instead
    // "MultiLineStringFromText",  // use ST_MultiLineStringFromText() instead
    // "MultiLineStringFromWKB",  // deprecated, use ST_MultiLineStringFromWKB()
    "MOD",
    "MONTH",
    "MONTHNAME",
    // "MPointFromText",  // deprecated, use ST_MPointFromText() instead
    // "MPointFromWKB",  // deprecated, use ST_MPointFromWKB() instead
    // "MPolyFromText",  // deprecated, use ST_MPolyFromText() instead
    // "MPolyFromWKB",  // deprecated, use ST_MPolyFromWKB() instead
    // "MultiPointFromText",  // deprecated, use ST_MultiPointFromText() instead
    // "MultiPointFromWKB",  // deprecated, use ST_MultiPointFromWKB() instead
    // "MultiPolygonFromText",  // deprecated, use ST_MultiPolygonFromText()
    // "MultiPolygonFromWKB",  // deprecated, use ST_MultiPolygonFromWKB()
    "MultiLineString",
    "MultiPoint",
    "MultiPolygon",
    "NAME_CONST",
    "NOW",
    "NULLIF",
    // "NumGeometries",  // deprecated, use ST_NumGeometries() instead
    // "NumInteriorRings",  // deprecated, use ST_NumInteriorRings() instead
    // "NumPoints",  // deprecated, use ST_NumPoints() instead
    "OCT",
    "OCTET_LENGTH",
    "ORD",
    // "Overlaps",  // deprecated, use MBROverlaps() instead
    "PASSWORD",  // deprecated in 5.7
    "PERIOD_ADD",
    "PERIOD_DIFF",
    "PI",
    "Point",
    // "PointFromText",  // deprecated, use ST_PointFromText() instead
    // "PointFromWKB",  // deprecated, use ST_PointFromWKB() instead
    // "PointN",  // deprecated, use ST_PointN() instead
    // "PolyFromText",  // deprecated, use ST_PolyFromText() instead
    // "PolyFromWKB",  // deprecated, use ST_PolyFromWKB() instead
    // "PolygonFromText",  // deprecated, use ST_PolygonFromText() instead
    // "PolygonFromWKB",  // deprecated, use ST_PolygonFromWKB() instead
    "Polygon",
    "POSITION",
    "POW",
    "POWER",
    // "PROCEDURE ANALYSE",  // deprecated in 5.7.18, handled by grammar
    "QUARTER",
    "QUOTE",
    "RADIANS",
    "RAND",
    "RANDOM_BYTES",
    "RELEASE_ALL_LOCKS",
    "RELEASE_LOCK",
    "REPEAT",
    "REPLACE",
    "REVERSE",
    "RIGHT",
    "ROUND",
    "ROW_COUNT",
    "RPAD",
    "RTRIM",
    "SCHEMA",
    "SEC_TO_TIME",
    "SECOND",
    "SESSION_USER",
    "SHA",
    "SHA1",
    "SHA2",
    "SIGN",
    "SIN",
    "SLEEP",
    "SOUNDEX",
    "SPACE",
    "SQRT",
    // "SRID",  // deprecated, use ST_SRID() instead
    "ST_Area",
    "ST_AsBinary",
    "ST_AsGeoJSON",
    "ST_AsText",
    "ST_AsWKB",
    "ST_AsWKT",
    "ST_Buffer",
    "ST_Buffer_Strategy",
    "ST_Centroid",
    "ST_Contains",
    "ST_ConvexHull",
    "ST_Crosses",
    "ST_Difference",
    "ST_Dimension",
    "ST_Disjoint",
    "ST_Distance",
    "ST_Distance_Sphere",
    "ST_EndPoint",
    "ST_Envelope",
    "ST_Equals",
    "ST_ExteriorRing",
    "ST_GeoHash",
    "ST_GeomCollFromText",
    "ST_GeomCollFromTxt",
    "ST_GeomCollFromWKB",
    "ST_GeometryCollectionFromText",
    "ST_GeometryCollectionFromWKB",
    "ST_GeometryN",
    "ST_GeometryType",
    "ST_GeomFromGeoJSON",
    "ST_GeomFromText",
    "ST_GeomFromWKB",
    "ST_GeometryFromText",
    "ST_GeometryFromWKB",
    "ST_InteriorRingN",
    "ST_Intersection",
    "ST_Intersects",
    "ST_IsClosed",
    "ST_IsEmpty",
    "ST_IsSimple",
    "ST_IsValid",
    "ST_LatFromGeoHash",
    "ST_Length",
    "ST_LineFromText",
    "ST_LineFromWKB",
    "ST_LineStringFromText",
    "ST_LineStringFromWKB",
    "ST_LongFromGeoHash",
    "ST_MakeEnvelope",
    "ST_MLineFromText",
    "ST_MLineFromWKB",
    "ST_MPointFromText",
    "ST_MPointFromWKB",
    "ST_MPolyFromText",
    "ST_MPolyFromWKB",
    "ST_MultiLineStringFromText",
    "ST_MultiLineStringFromWKB",
    "ST_MultiPointFromText",
    "ST_MultiPointFromWKB",
    "ST_MultiPolygonFromText",
    "ST_MultiPolygonFromWKB",
    "ST_NumGeometries",
    "ST_NumInteriorRing",
    "ST_NumInteriorRings",
    "ST_NumPoints",
    "ST_Overlaps",
    "ST_PointFromGeoHash",
    "ST_PointFromText",
    "ST_PointFromWKB",
    "ST_PointN",
    "ST_PolyFromText",
    "ST_PolyFromWKB",
    "ST_PolygonFromText",
    "ST_PolygonFromWKB",
    "ST_Simplify",
    "ST_SRID",
    "ST_StartPoint",
    "ST_SymDifference",
    "ST_Touches",
    "ST_Union",
    "ST_Validate",
    "ST_Within",
    "ST_X",
    "ST_Y",
    // "StartPoint",  // deprecated, use ST_StartPoint() instead
    "STD",
    "STDDEV",
    "STDDEV_POP",
    "STDDEV_SAMP",
    "STR_TO_DATE",
    "STRCMP",
    "SUBDATE",
    "SUBSTR",
    "SUBSTRING",
    "SUBSTRING_INDEX",
    "SUBTIME",
    "SUM",
    "SYSDATE",
    "SYSTEM_USER",
    "TAN",
    "TIME",
    "TIME_FORMAT",
    "TIME_TO_SEC",
    "TIMEDIFF",
    "TIMESTAMP",
    "TIMESTAMPADD",
    "TIMESTAMPDIFF",
    "TO_BASE64",
    "TO_DAYS",
    "TO_SECONDS",
    // "Touches",  // deprecated, use ST_Touches() instead
    "TRIM",
    "TRUNCATE",
    "UCASE",
    "UNCOMPRESS",
    "UNCOMPRESSED_LENGTH",
    "UNHEX",
    "UNIX_TIMESTAMP",
    "UpdateXML",
    "UPPER",
    "USER",
    "UTC_DATE",
    "UTC_TIME",
    "UTC_TIMESTAMP",
    "UUID",
    "UUID_SHORT",
    "VALIDATE_PASSWORD_STRENGTH",
    "VALUES",
    "VAR_POP",
    "VAR_SAMP",
    "VARIANCE",
    "VERSION",
    "WAIT_FOR_EXECUTED_GTID_SET",
    "WAIT_UNTIL_SQL_THREAD_AFTER_GTIDS",
    "WEEK",
    "WEEKDAY",
    "WEEKOFYEAR",
    "WEIGHT_STRING",
    // "Within",  // deprecated, use MBRWithin() instead
    // "X",  // deprecated, use ST_X() instead
    // "Y",  // deprecated, use ST_Y() instead
    "YEAR",
    "YEARWEEK",
};

std::set<std::string> systemFunctions80 = {
    "ABS",
    "ACOS",
    "ADDDATE",
    "ADDTIME",
    "AES_DECRYPT",
    "AES_ENCRYPT",
    "ANY_VALUE",
    "ASCII",
    "ASIN",
    "asynchronous_connection_failover_add_managed",     //  added 8.0.23
    "asynchronous_connection_failover_add_source",      //  added 8.0.22
    "asynchronous_connection_failover_delete_managed",  //  added 8.0.23
    "asynchronous_connection_failover_delete_source",   //  added 8.0.22
    "asynchronous_connection_failover_reset",           //  added 8.0.27
    "ATAN",
    "ATAN2",
    "AVG",
    "BENCHMARK",
    "BIN",
    "BIN_TO_UUID",
    "BIT_AND",
    "BIT_COUNT",
    "BIT_LENGTH",
    "BIT_OR",
    "BIT_XOR",
    // "CAN_ACCESS_COLUMN",  // internal
    // "CAN_ACCESS_DATABASE",  // internal
    // "CAN_ACCESS_TABLE",  // internal
    // "CAN_ACCESS_USER",  // internal, added 8.0.22
    // "CAN_ACCESS_VIEW",  // internal
    "CAST",
    "CEIL",
    "CEILING",
    "CHAR",
    "CHAR_LENGTH",
    "CHARACTER_LENGTH",
    "CHARSET",
    "COALESCE",
    "COERCIBILITY",
    "COLLATION",
    "COMPRESS",
    "CONCAT",
    "CONCAT_WS",
    "CONNECTION_ID",
    "CONV",
    "CONVERT",
    "CONVERT_TZ",
    "COS",
    "COT",
    "COUNT",
    "CRC32",
    "CUME_DIST",
    "CURDATE",
    "CURRENT_DATE",
    "CURRENT_ROLE",
    "CURRENT_TIME",
    "CURRENT_TIMESTAMP",
    "CURRENT_USER",
    "CURTIME",
    "DATABASE",
    "DATE",
    "DATE_ADD",
    "DATE_FORMAT",
    "DATE_SUB",
    "DATEDIFF",
    "DAY",
    "DAYNAME",
    "DAYOFMONTH",
    "DAYOFWEEK",
    "DAYOFYEAR",
    "DEFAULT",
    "DEGREES",
    "DENSE_RANK",
    "ELT",
    "EXP",
    "EXPORT_SET",
    "EXTRACT",
    "ExtractValue",
    "FIELD",
    "FIND_IN_SET",
    "FIRST_VALUE",
    "FLOOR",
    "FORMAT",
    "FORMAT_BYTES",      // added 8.0.16
    "FORMAT_PICO_TIME",  // added 8.0.16
    "FOUND_ROWS",
    "FROM_BASE64",
    "FROM_DAYS",
    "FROM_UNIXTIME",
    "GeomCollection",
    "GeometryCollection",
    // "GET_DD_COLUMN_PRIVILEGES",  // internal
    // "GET_DD_CREATE_OPTIONS",  // internal
    // "GET_DD_INDEX_SUB_PART_LENGTH",  // internal
    "GET_FORMAT",
    "GET_LOCK",
    "GREATEST",
    "GROUP_CONCAT",
    "group_replication_disable_member_action",          // added 8.0.26
    "group_replication_enable_member_action",           // added 8.0.26
    "group_replication_get_communication_protocol",     // added 8.0.16
    "group_replication_get_write_concurrency",          // added 8.0.13
    "group_replication_reset_member_actions",           // added 8.0.26
    "group_replication_set_as_primary",                 // added 8.0.29
    "group_replication_set_communication_protocol",     // added 8.0.16
    "group_replication_set_write_concurrency",          // added 8.0.13
    "group_replication_switch_to_multi_primary_mode",   // added 8.0.13
    "group_replication_switch_to_single_primary_mode",  // added 8.0.13
    "GROUPING",
    "GTID_SUBSET",
    "GTID_SUBTRACT",
    "HEX",
    "HOUR",
    "ICU_VERSION",
    "IF",
    "IFNULL",
    // "IN",  // operator
    "INET_ATON",
    "INET_NTOA",
    "INET6_ATON",
    "INET6_NTOA",
    "INSERT",
    "INSTR",
    // "INTERNAL_AUTO_INCREMENT",  // internal
    // "INTERNAL_AVG_ROW_LENGTH",  // internal
    // "INTERNAL_CHECK_TIME",  // internal
    // "INTERNAL_CHECKSUM",  // internal
    // "INTERNAL_DATA_FREE",  // internal
    // "INTERNAL_DATA_LENGTH",  // internal
    // "INTERNAL_DD_CHAR_LENGTH",  // internal
    // "INTERNAL_GET_COMMENT_OR_ERROR",  // internal
    // "INTERNAL_GET_ENABLED_ROLE_JSON",  // internal, added 8.0.19
    // "INTERNAL_GET_HOSTNAME",  // internal, added 8.0.19
    // "INTERNAL_GET_USERNAME",  // internal
    // "INTERNAL_GET_VIEW_WARNING_OR_ERROR",  // internal
    // "INTERNAL_INDEX_COLUMN_CARDINALITY",  // internal
    // "INTERNAL_INDEX_LENGTH",  // internal
    // "INTERNAL_IS_ENABLED_ROLE",  // internal, added 8.0.19
    // "INTERNAL_IS_MANDATORY_ROLE",  // internal, added 8.0.19
    // "INTERNAL_KEYS_DISABLED",  // internal
    // "INTERNAL_MAX_DATA_LENGTH",  // internal
    // "INTERNAL_TABLE_ROWS",  // internal
    // "INTERNAL_UPDATE_TIME",  // internal
    "INTERVAL",
    "IS_FREE_LOCK",
    "IS_IPV4",
    "IS_IPV4_COMPAT",
    "IS_IPV4_MAPPED",
    "IS_IPV6",
    "IS_USED_LOCK",
    "IS_UUID",
    "ISNULL",
    "JSON_ARRAY",
    "JSON_ARRAY_APPEND",
    "JSON_ARRAY_INSERT",
    "JSON_ARRAYAGG",
    "JSON_CONTAINS",
    "JSON_CONTAINS_PATH",
    "JSON_DEPTH",
    "JSON_EXTRACT",
    "JSON_INSERT",
    "JSON_KEYS",
    "JSON_LENGTH",
    // "JSON_MERGE",  // deprecated, use JSON_MERGE_PRESERVE() instead
    "JSON_MERGE_PATCH",
    "JSON_MERGE_PRESERVE",
    "JSON_OBJECT",
    "JSON_OBJECTAGG",
    "JSON_OVERLAPS",  // added 8.0.17
    "JSON_PRETTY",
    "JSON_QUOTE",
    "JSON_REMOVE",
    "JSON_REPLACE",
    "JSON_SCHEMA_VALID",              // added 8.0.17
    "JSON_SCHEMA_VALIDATION_REPORT",  // added 8.0.17
    "JSON_SEARCH",
    "JSON_SET",
    "JSON_STORAGE_FREE",
    "JSON_STORAGE_SIZE",
    "JSON_TABLE",
    "JSON_TYPE",
    "JSON_UNQUOTE",
    "JSON_VALID",
    "JSON_VALUE",  // added 8.0.21
    "LAG",
    "LAST_DAY",
    "LAST_INSERT_ID",
    "LAST_VALUE",
    "LCASE",
    "LEAD",
    "LEAST",
    "LEFT",
    "LENGTH",
    "LineString",
    "LN",
    "LOAD_FILE",
    "LOCALTIME",
    "LOCALTIMESTAMP",
    "LOCATE",
    "LOG",
    "LOG10",
    "LOG2",
    "LOWER",
    "LPAD",
    "LTRIM",
    "MAKE_SET",
    "MAKEDATE",
    "MAKETIME",
    "MASTER_POS_WAIT",  // deprecated 8.0.26, use SOURCE_POS_WAIT() instead
    "MATCH",
    "MAX",
    "MBRContains",
    "MBRCoveredBy",
    "MBRCovers",
    "MBRDisjoint",
    "MBREquals",
    "MBRIntersects",
    "MBROverlaps",
    "MBRTouches",
    "MBRWithin",
    "MD5",
    // "MEMBER OF",  // operator, added 8.0.17
    "MICROSECOND",
    "MID",
    "MIN",
    "MINUTE",
    "MOD",
    "MONTH",
    "MONTHNAME",
    "MultiLineString",
    "MultiPoint",
    "MultiPolygon",
    "NAME_CONST",
    "NOW",
    "NTH_VALUE",
    "NTILE",
    "NULLIF",
    "OCT",
    "OCTET_LENGTH",
    "ORD",
    "PERCENT_RANK",
    "PERIOD_ADD",
    "PERIOD_DIFF",
    "PI",
    "Point",
    "Polygon",
    "POSITION",
    "POW",
    "POWER",
    "PS_CURRENT_THREAD_ID",  // added 8.0.16
    "PS_THREAD_ID",          // added 8.0.16
    "QUARTER",
    "QUOTE",
    "RADIANS",
    "RAND",
    "RANDOM_BYTES",
    "RANK",
    "REGEXP_INSTR",
    "REGEXP_LIKE",
    "REGEXP_REPLACE",
    "REGEXP_SUBSTR",
    "RELEASE_ALL_LOCKS",
    "RELEASE_LOCK",
    "REPEAT",
    "REPLACE",
    "REVERSE",
    "RIGHT",
    "ROLES_GRAPHML",
    "ROUND",
    "ROW_COUNT",
    "ROW_NUMBER",
    "RPAD",
    "RTRIM",
    "SCHEMA",
    "SEC_TO_TIME",
    "SECOND",
    "SESSION_USER",
    "SHA",
    "SHA1",
    "SHA2",
    "SIGN",
    "SIN",
    "SLEEP",
    "SOUNDEX",
    "SOURCE_POS_WAIT",  // added 8.0.26
    "SPACE",
    "SQRT",
    "ST_Area",
    "ST_AsBinary",
    "ST_AsGeoJSON",
    "ST_AsText",
    "ST_AsWKB",
    "ST_AsWKT",
    "ST_Buffer",
    "ST_Buffer_Strategy",
    "ST_Centroid",
    "ST_Collect",  // added 8.0.24
    "ST_Contains",
    "ST_ConvexHull",
    "ST_Crosses",
    "ST_Difference",
    "ST_Dimension",
    "ST_Disjoint",
    "ST_Distance",
    "ST_Distance_Sphere",
    "ST_EndPoint",
    "ST_Envelope",
    "ST_Equals",
    "ST_ExteriorRing",
    "ST_FrechetDistance",  // added 8.0.23
    "ST_GeoHash",
    "ST_GeomCollFromText",
    "ST_GeomCollFromTxt",
    "ST_GeomCollFromWKB",
    "ST_GeometryCollectionFromText",
    "ST_GeometryCollectionFromWKB",
    "ST_GeometryN",
    "ST_GeometryType",
    "ST_GeomFromGeoJSON",
    "ST_GeomFromText",
    "ST_GeomFromWKB",
    "ST_GeometryFromText",
    "ST_GeometryFromWKB",
    "ST_HausdorffDistance",  // added 8.0.23
    "ST_InteriorRingN",
    "ST_Intersection",
    "ST_Intersects",
    "ST_IsClosed",
    "ST_IsEmpty",
    "ST_IsSimple",
    "ST_IsValid",
    "ST_LatFromGeoHash",
    "ST_Latitude",  // added 8.0.12
    "ST_Length",
    "ST_LineFromText",
    "ST_LineFromWKB",
    "ST_LineInterpolatePoint",   // added 8.0.24
    "ST_LineInterpolatePoints",  // added 8.0.24
    "ST_LineStringFromText",
    "ST_LineStringFromWKB",
    "ST_LongFromGeoHash",
    "ST_Longitude",  // added 8.0.12
    "ST_MakeEnvelope",
    "ST_MLineFromText",
    "ST_MLineFromWKB",
    "ST_MPointFromText",
    "ST_MPointFromWKB",
    "ST_MPolyFromText",
    "ST_MPolyFromWKB",
    "ST_MultiLineStringFromText",
    "ST_MultiLineStringFromWKB",
    "ST_MultiPointFromWKB",
    "ST_MultiPointFromText",
    "ST_MultiPolygonFromText",
    "ST_MultiPolygonFromWKB",
    "ST_NumGeometries",
    "ST_NumInteriorRing",
    "ST_NumInteriorRings",
    "ST_NumPoints",
    "ST_Overlaps",
    "ST_PointAtDistance",  // added 8.0.24
    "ST_PointFromGeoHash",
    "ST_PointFromText",
    "ST_PointFromWKB",
    "ST_PointN",
    "ST_PolyFromText",
    "ST_PolyFromWKB",
    "ST_PolygonFromText",
    "ST_PolygonFromWKB",
    "ST_Simplify",
    "ST_SRID",
    "ST_StartPoint",
    "ST_SwapXY",
    "ST_SymDifference",
    "ST_Touches",
    "ST_Transform",  // added 8.0.13
    "ST_Union",
    "ST_Validate",
    "ST_Within",
    "ST_X",
    "ST_Y",
    "STATEMENT_DIGEST",
    "STATEMENT_DIGEST_TEXT",
    "STD",
    "STDDEV",
    "STDDEV_POP",
    "STDDEV_SAMP",
    "STR_TO_DATE",
    "STRCMP",
    "SUBDATE",
    "SUBSTR",
    "SUBSTRING",
    "SUBSTRING_INDEX",
    "SUBTIME",
    "SUM",
    "SYSDATE",
    "SYSTEM_USER",
    "TAN",
    "TIME",
    "TIME_FORMAT",
    "TIME_TO_SEC",
    "TIMEDIFF",
    "TIMESTAMP",
    "TIMESTAMPADD",
    "TIMESTAMPDIFF",
    "TO_BASE64",
    "TO_DAYS",
    "TO_SECONDS",
    "TRIM",
    "TRUNCATE",
    "UCASE",
    "UNCOMPRESS",
    "UNCOMPRESSED_LENGTH",
    "UNHEX",
    "UNIX_TIMESTAMP",
    "UpdateXML",
    "UPPER",
    "USER",
    "UTC_DATE",
    "UTC_TIME",
    "UTC_TIMESTAMP",
    "UUID",
    "UUID_SHORT",
    "UUID_TO_BIN",
    "VALIDATE_PASSWORD_STRENGTH",
    "VALUES",
    "VAR_POP",
    "VAR_SAMP",
    "VARIANCE",
    "VERSION",
    "WAIT_FOR_EXECUTED_GTID_SET",
    // "WAIT_UNTIL_SQL_THREAD_AFTER_GTIDS",  // use WAIT_FOR_EXECUTED_GTID_SET()
    "WEEK",
    "WEEKDAY",
    "WEEKOFYEAR",
    "WEIGHT_STRING",
    "YEAR",
    "YEARWEEK",
};
