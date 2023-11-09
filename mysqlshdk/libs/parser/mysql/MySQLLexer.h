// clang-format off
/*
 * Copyright (c) 2020, 2023, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation. The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */




// Generated from /home/paandrus/dev/ngshell/mysqlshdk/libs/parser/grammars/MySQLLexer.g4 by ANTLR 4.10.1

#pragma once


#include "antlr4-runtime.h"

#include "mysqlshdk/libs/parser/MySQLBaseLexer.h"

namespace parsers {


class  MySQLLexer : public MySQLBaseLexer {
public:
  enum {
    NOT2_SYMBOL = 1, CONCAT_PIPES_SYMBOL = 2, INT_NUMBER = 3, LONG_NUMBER = 4, 
    ULONGLONG_NUMBER = 5, EQUAL_OPERATOR = 6, ASSIGN_OPERATOR = 7, NULL_SAFE_EQUAL_OPERATOR = 8, 
    GREATER_OR_EQUAL_OPERATOR = 9, GREATER_THAN_OPERATOR = 10, LESS_OR_EQUAL_OPERATOR = 11, 
    LESS_THAN_OPERATOR = 12, NOT_EQUAL_OPERATOR = 13, PLUS_OPERATOR = 14, 
    MINUS_OPERATOR = 15, MULT_OPERATOR = 16, DIV_OPERATOR = 17, MOD_OPERATOR = 18, 
    LOGICAL_NOT_OPERATOR = 19, BITWISE_NOT_OPERATOR = 20, SHIFT_LEFT_OPERATOR = 21, 
    SHIFT_RIGHT_OPERATOR = 22, LOGICAL_AND_OPERATOR = 23, BITWISE_AND_OPERATOR = 24, 
    BITWISE_XOR_OPERATOR = 25, LOGICAL_OR_OPERATOR = 26, BITWISE_OR_OPERATOR = 27, 
    DOT_SYMBOL = 28, COMMA_SYMBOL = 29, SEMICOLON_SYMBOL = 30, COLON_SYMBOL = 31, 
    OPEN_PAR_SYMBOL = 32, CLOSE_PAR_SYMBOL = 33, OPEN_CURLY_SYMBOL = 34, 
    CLOSE_CURLY_SYMBOL = 35, UNDERLINE_SYMBOL = 36, JSON_SEPARATOR_SYMBOL = 37, 
    JSON_UNQUOTED_SEPARATOR_SYMBOL = 38, AT_SIGN_SYMBOL = 39, AT_AT_SIGN_SYMBOL = 40, 
    NULL2_SYMBOL = 41, PARAM_MARKER = 42, HEX_NUMBER = 43, BIN_NUMBER = 44, 
    DECIMAL_NUMBER = 45, FLOAT_NUMBER = 46, ACCESSIBLE_SYMBOL = 47, ACCOUNT_SYMBOL = 48, 
    ACTION_SYMBOL = 49, ADD_SYMBOL = 50, ADDDATE_SYMBOL = 51, AFTER_SYMBOL = 52, 
    AGAINST_SYMBOL = 53, AGGREGATE_SYMBOL = 54, ALGORITHM_SYMBOL = 55, ALL_SYMBOL = 56, 
    ALTER_SYMBOL = 57, ALWAYS_SYMBOL = 58, ANALYSE_SYMBOL = 59, ANALYZE_SYMBOL = 60, 
    AND_SYMBOL = 61, ANY_SYMBOL = 62, AS_SYMBOL = 63, ASC_SYMBOL = 64, ASCII_SYMBOL = 65, 
    ASENSITIVE_SYMBOL = 66, AT_SYMBOL = 67, AUTOEXTEND_SIZE_SYMBOL = 68, 
    AUTO_INCREMENT_SYMBOL = 69, AVG_ROW_LENGTH_SYMBOL = 70, AVG_SYMBOL = 71, 
    BACKUP_SYMBOL = 72, BEFORE_SYMBOL = 73, BEGIN_SYMBOL = 74, BETWEEN_SYMBOL = 75, 
    BIGINT_SYMBOL = 76, BINARY_SYMBOL = 77, BINLOG_SYMBOL = 78, BIN_NUM_SYMBOL = 79, 
    BIT_AND_SYMBOL = 80, BIT_OR_SYMBOL = 81, BIT_SYMBOL = 82, BIT_XOR_SYMBOL = 83, 
    BLOB_SYMBOL = 84, BLOCK_SYMBOL = 85, BOOLEAN_SYMBOL = 86, BOOL_SYMBOL = 87, 
    BOTH_SYMBOL = 88, BTREE_SYMBOL = 89, BY_SYMBOL = 90, BYTE_SYMBOL = 91, 
    CACHE_SYMBOL = 92, CALL_SYMBOL = 93, CASCADE_SYMBOL = 94, CASCADED_SYMBOL = 95, 
    CASE_SYMBOL = 96, CAST_SYMBOL = 97, CATALOG_NAME_SYMBOL = 98, CHAIN_SYMBOL = 99, 
    CHANGE_SYMBOL = 100, CHANGED_SYMBOL = 101, CHANNEL_SYMBOL = 102, CHARSET_SYMBOL = 103, 
    CHAR_SYMBOL = 104, CHECKSUM_SYMBOL = 105, CHECK_SYMBOL = 106, CIPHER_SYMBOL = 107, 
    CLASS_ORIGIN_SYMBOL = 108, CLIENT_SYMBOL = 109, CLOSE_SYMBOL = 110, 
    COALESCE_SYMBOL = 111, CODE_SYMBOL = 112, COLLATE_SYMBOL = 113, COLLATION_SYMBOL = 114, 
    COLUMNS_SYMBOL = 115, COLUMN_SYMBOL = 116, COLUMN_NAME_SYMBOL = 117, 
    COLUMN_FORMAT_SYMBOL = 118, COMMENT_SYMBOL = 119, COMMITTED_SYMBOL = 120, 
    COMMIT_SYMBOL = 121, COMPACT_SYMBOL = 122, COMPLETION_SYMBOL = 123, 
    COMPRESSED_SYMBOL = 124, COMPRESSION_SYMBOL = 125, CONCURRENT_SYMBOL = 126, 
    CONDITION_SYMBOL = 127, CONNECTION_SYMBOL = 128, CONSISTENT_SYMBOL = 129, 
    CONSTRAINT_SYMBOL = 130, CONSTRAINT_CATALOG_SYMBOL = 131, CONSTRAINT_NAME_SYMBOL = 132, 
    CONSTRAINT_SCHEMA_SYMBOL = 133, CONTAINS_SYMBOL = 134, CONTEXT_SYMBOL = 135, 
    CONTINUE_SYMBOL = 136, CONVERT_SYMBOL = 137, COUNT_SYMBOL = 138, CPU_SYMBOL = 139, 
    CREATE_SYMBOL = 140, CROSS_SYMBOL = 141, CUBE_SYMBOL = 142, CURDATE_SYMBOL = 143, 
    CURRENT_SYMBOL = 144, CURRENT_DATE_SYMBOL = 145, CURRENT_TIME_SYMBOL = 146, 
    CURRENT_USER_SYMBOL = 147, CURSOR_SYMBOL = 148, CURSOR_NAME_SYMBOL = 149, 
    CURTIME_SYMBOL = 150, DATABASE_SYMBOL = 151, DATABASES_SYMBOL = 152, 
    DATAFILE_SYMBOL = 153, DATA_SYMBOL = 154, DATETIME_SYMBOL = 155, DATE_ADD_SYMBOL = 156, 
    DATE_SUB_SYMBOL = 157, DATE_SYMBOL = 158, DAY_HOUR_SYMBOL = 159, DAY_MICROSECOND_SYMBOL = 160, 
    DAY_MINUTE_SYMBOL = 161, DAY_SECOND_SYMBOL = 162, DAY_SYMBOL = 163, 
    DEALLOCATE_SYMBOL = 164, DECIMAL_NUM_SYMBOL = 165, DECIMAL_SYMBOL = 166, 
    DECLARE_SYMBOL = 167, DEFAULT_SYMBOL = 168, DEFAULT_AUTH_SYMBOL = 169, 
    DEFINER_SYMBOL = 170, DELAYED_SYMBOL = 171, DELAY_KEY_WRITE_SYMBOL = 172, 
    DELETE_SYMBOL = 173, DESC_SYMBOL = 174, DESCRIBE_SYMBOL = 175, DES_KEY_FILE_SYMBOL = 176, 
    DETERMINISTIC_SYMBOL = 177, DIAGNOSTICS_SYMBOL = 178, DIRECTORY_SYMBOL = 179, 
    DISABLE_SYMBOL = 180, DISCARD_SYMBOL = 181, DISK_SYMBOL = 182, DISTINCT_SYMBOL = 183, 
    DIV_SYMBOL = 184, DOUBLE_SYMBOL = 185, DO_SYMBOL = 186, DROP_SYMBOL = 187, 
    DUAL_SYMBOL = 188, DUMPFILE_SYMBOL = 189, DUPLICATE_SYMBOL = 190, DYNAMIC_SYMBOL = 191, 
    EACH_SYMBOL = 192, ELSE_SYMBOL = 193, ELSEIF_SYMBOL = 194, ENABLE_SYMBOL = 195, 
    ENCLOSED_SYMBOL = 196, ENCRYPTION_SYMBOL = 197, END_SYMBOL = 198, ENDS_SYMBOL = 199, 
    END_OF_INPUT_SYMBOL = 200, ENGINES_SYMBOL = 201, ENGINE_SYMBOL = 202, 
    ENUM_SYMBOL = 203, ERROR_SYMBOL = 204, ERRORS_SYMBOL = 205, ESCAPED_SYMBOL = 206, 
    ESCAPE_SYMBOL = 207, EVENTS_SYMBOL = 208, EVENT_SYMBOL = 209, EVERY_SYMBOL = 210, 
    EXCHANGE_SYMBOL = 211, EXECUTE_SYMBOL = 212, EXISTS_SYMBOL = 213, EXIT_SYMBOL = 214, 
    EXPANSION_SYMBOL = 215, EXPIRE_SYMBOL = 216, EXPLAIN_SYMBOL = 217, EXPORT_SYMBOL = 218, 
    EXTENDED_SYMBOL = 219, EXTENT_SIZE_SYMBOL = 220, EXTRACT_SYMBOL = 221, 
    FALSE_SYMBOL = 222, FAST_SYMBOL = 223, FAULTS_SYMBOL = 224, FETCH_SYMBOL = 225, 
    FILE_SYMBOL = 226, FILE_BLOCK_SIZE_SYMBOL = 227, FILTER_SYMBOL = 228, 
    FIRST_SYMBOL = 229, FIXED_SYMBOL = 230, FLOAT_SYMBOL = 231, FLUSH_SYMBOL = 232, 
    FOLLOWS_SYMBOL = 233, FORCE_SYMBOL = 234, FOREIGN_SYMBOL = 235, FOR_SYMBOL = 236, 
    FORMAT_SYMBOL = 237, FOUND_SYMBOL = 238, FROM_SYMBOL = 239, FULL_SYMBOL = 240, 
    FULLTEXT_SYMBOL = 241, FUNCTION_SYMBOL = 242, GET_SYMBOL = 243, GENERAL_SYMBOL = 244, 
    GENERATED_SYMBOL = 245, GROUP_REPLICATION_SYMBOL = 246, GEOMETRYCOLLECTION_SYMBOL = 247, 
    GEOMETRY_SYMBOL = 248, GET_FORMAT_SYMBOL = 249, GLOBAL_SYMBOL = 250, 
    GRANT_SYMBOL = 251, GRANTS_SYMBOL = 252, GROUP_SYMBOL = 253, GROUP_CONCAT_SYMBOL = 254, 
    HANDLER_SYMBOL = 255, HASH_SYMBOL = 256, HAVING_SYMBOL = 257, HELP_SYMBOL = 258, 
    HIGH_PRIORITY_SYMBOL = 259, HOST_SYMBOL = 260, HOSTS_SYMBOL = 261, HOUR_MICROSECOND_SYMBOL = 262, 
    HOUR_MINUTE_SYMBOL = 263, HOUR_SECOND_SYMBOL = 264, HOUR_SYMBOL = 265, 
    IDENTIFIED_SYMBOL = 266, IF_SYMBOL = 267, IGNORE_SYMBOL = 268, IGNORE_SERVER_IDS_SYMBOL = 269, 
    IMPORT_SYMBOL = 270, INDEXES_SYMBOL = 271, INDEX_SYMBOL = 272, INFILE_SYMBOL = 273, 
    INITIAL_SIZE_SYMBOL = 274, INNER_SYMBOL = 275, INOUT_SYMBOL = 276, INSENSITIVE_SYMBOL = 277, 
    INSERT_SYMBOL = 278, INSERT_METHOD_SYMBOL = 279, INSTANCE_SYMBOL = 280, 
    INSTALL_SYMBOL = 281, INTERVAL_SYMBOL = 282, INTO_SYMBOL = 283, INT_SYMBOL = 284, 
    INVOKER_SYMBOL = 285, IN_SYMBOL = 286, IO_AFTER_GTIDS_SYMBOL = 287, 
    IO_BEFORE_GTIDS_SYMBOL = 288, IO_SYMBOL = 289, IPC_SYMBOL = 290, IS_SYMBOL = 291, 
    ISOLATION_SYMBOL = 292, ISSUER_SYMBOL = 293, ITERATE_SYMBOL = 294, JOIN_SYMBOL = 295, 
    JSON_SYMBOL = 296, KEYS_SYMBOL = 297, KEY_BLOCK_SIZE_SYMBOL = 298, KEY_SYMBOL = 299, 
    KILL_SYMBOL = 300, LANGUAGE_SYMBOL = 301, LAST_SYMBOL = 302, LEADING_SYMBOL = 303, 
    LEAVES_SYMBOL = 304, LEAVE_SYMBOL = 305, LEFT_SYMBOL = 306, LESS_SYMBOL = 307, 
    LEVEL_SYMBOL = 308, LIKE_SYMBOL = 309, LIMIT_SYMBOL = 310, LINEAR_SYMBOL = 311, 
    LINES_SYMBOL = 312, LINESTRING_SYMBOL = 313, LIST_SYMBOL = 314, LOAD_SYMBOL = 315, 
    LOCAL_SYMBOL = 316, LOCATOR_SYMBOL = 317, LOCKS_SYMBOL = 318, LOCK_SYMBOL = 319, 
    LOGFILE_SYMBOL = 320, LOGS_SYMBOL = 321, LONGBLOB_SYMBOL = 322, LONGTEXT_SYMBOL = 323, 
    LONG_NUM_SYMBOL = 324, LONG_SYMBOL = 325, LOOP_SYMBOL = 326, LOW_PRIORITY_SYMBOL = 327, 
    MASTER_AUTO_POSITION_SYMBOL = 328, MASTER_BIND_SYMBOL = 329, MASTER_CONNECT_RETRY_SYMBOL = 330, 
    MASTER_DELAY_SYMBOL = 331, MASTER_HOST_SYMBOL = 332, MASTER_LOG_FILE_SYMBOL = 333, 
    MASTER_LOG_POS_SYMBOL = 334, MASTER_PASSWORD_SYMBOL = 335, MASTER_PORT_SYMBOL = 336, 
    MASTER_RETRY_COUNT_SYMBOL = 337, MASTER_SERVER_ID_SYMBOL = 338, MASTER_SSL_CAPATH_SYMBOL = 339, 
    MASTER_SSL_CA_SYMBOL = 340, MASTER_SSL_CERT_SYMBOL = 341, MASTER_SSL_CIPHER_SYMBOL = 342, 
    MASTER_SSL_CRL_SYMBOL = 343, MASTER_SSL_CRLPATH_SYMBOL = 344, MASTER_SSL_KEY_SYMBOL = 345, 
    MASTER_SSL_SYMBOL = 346, MASTER_SSL_VERIFY_SERVER_CERT_SYMBOL = 347, 
    MASTER_SYMBOL = 348, MASTER_TLS_VERSION_SYMBOL = 349, MASTER_USER_SYMBOL = 350, 
    MASTER_HEARTBEAT_PERIOD_SYMBOL = 351, MATCH_SYMBOL = 352, MAX_CONNECTIONS_PER_HOUR_SYMBOL = 353, 
    MAX_QUERIES_PER_HOUR_SYMBOL = 354, MAX_ROWS_SYMBOL = 355, MAX_SIZE_SYMBOL = 356, 
    MAX_STATEMENT_TIME_SYMBOL = 357, MAX_SYMBOL = 358, MAX_UPDATES_PER_HOUR_SYMBOL = 359, 
    MAX_USER_CONNECTIONS_SYMBOL = 360, MAXVALUE_SYMBOL = 361, MEDIUMBLOB_SYMBOL = 362, 
    MEDIUMINT_SYMBOL = 363, MEDIUMTEXT_SYMBOL = 364, MEDIUM_SYMBOL = 365, 
    MEMORY_SYMBOL = 366, MERGE_SYMBOL = 367, MESSAGE_TEXT_SYMBOL = 368, 
    MICROSECOND_SYMBOL = 369, MID_SYMBOL = 370, MIGRATE_SYMBOL = 371, MINUTE_MICROSECOND_SYMBOL = 372, 
    MINUTE_SECOND_SYMBOL = 373, MINUTE_SYMBOL = 374, MIN_ROWS_SYMBOL = 375, 
    MIN_SYMBOL = 376, MODE_SYMBOL = 377, MODIFIES_SYMBOL = 378, MODIFY_SYMBOL = 379, 
    MOD_SYMBOL = 380, MONTH_SYMBOL = 381, MULTILINESTRING_SYMBOL = 382, 
    MULTIPOINT_SYMBOL = 383, MULTIPOLYGON_SYMBOL = 384, MUTEX_SYMBOL = 385, 
    MYSQL_ERRNO_SYMBOL = 386, NAMES_SYMBOL = 387, NAME_SYMBOL = 388, NATIONAL_SYMBOL = 389, 
    NATURAL_SYMBOL = 390, NCHAR_STRING_SYMBOL = 391, NCHAR_SYMBOL = 392, 
    NDBCLUSTER_SYMBOL = 393, NEG_SYMBOL = 394, NEVER_SYMBOL = 395, NEW_SYMBOL = 396, 
    NEXT_SYMBOL = 397, NODEGROUP_SYMBOL = 398, NONE_SYMBOL = 399, NONBLOCKING_SYMBOL = 400, 
    NOT_SYMBOL = 401, NOW_SYMBOL = 402, NO_SYMBOL = 403, NO_WAIT_SYMBOL = 404, 
    NO_WRITE_TO_BINLOG_SYMBOL = 405, NULL_SYMBOL = 406, NUMBER_SYMBOL = 407, 
    NUMERIC_SYMBOL = 408, NVARCHAR_SYMBOL = 409, OFFLINE_SYMBOL = 410, OFFSET_SYMBOL = 411, 
    OLD_PASSWORD_SYMBOL = 412, ON_SYMBOL = 413, ONE_SYMBOL = 414, ONLINE_SYMBOL = 415, 
    ONLY_SYMBOL = 416, OPEN_SYMBOL = 417, OPTIMIZE_SYMBOL = 418, OPTIMIZER_COSTS_SYMBOL = 419, 
    OPTIONS_SYMBOL = 420, OPTION_SYMBOL = 421, OPTIONALLY_SYMBOL = 422, 
    ORDER_SYMBOL = 423, OR_SYMBOL = 424, OUTER_SYMBOL = 425, OUTFILE_SYMBOL = 426, 
    OUT_SYMBOL = 427, OWNER_SYMBOL = 428, PACK_KEYS_SYMBOL = 429, PAGE_SYMBOL = 430, 
    PARSER_SYMBOL = 431, PARTIAL_SYMBOL = 432, PARTITIONING_SYMBOL = 433, 
    PARTITIONS_SYMBOL = 434, PARTITION_SYMBOL = 435, PASSWORD_SYMBOL = 436, 
    PHASE_SYMBOL = 437, PLUGINS_SYMBOL = 438, PLUGIN_DIR_SYMBOL = 439, PLUGIN_SYMBOL = 440, 
    POINT_SYMBOL = 441, POLYGON_SYMBOL = 442, PORT_SYMBOL = 443, POSITION_SYMBOL = 444, 
    PRECEDES_SYMBOL = 445, PRECISION_SYMBOL = 446, PREPARE_SYMBOL = 447, 
    PRESERVE_SYMBOL = 448, PREV_SYMBOL = 449, PRIMARY_SYMBOL = 450, PRIVILEGES_SYMBOL = 451, 
    PROCEDURE_SYMBOL = 452, PROCESS_SYMBOL = 453, PROCESSLIST_SYMBOL = 454, 
    PROFILE_SYMBOL = 455, PROFILES_SYMBOL = 456, PROXY_SYMBOL = 457, PURGE_SYMBOL = 458, 
    QUARTER_SYMBOL = 459, QUERY_SYMBOL = 460, QUICK_SYMBOL = 461, RANGE_SYMBOL = 462, 
    READS_SYMBOL = 463, READ_ONLY_SYMBOL = 464, READ_SYMBOL = 465, READ_WRITE_SYMBOL = 466, 
    REAL_SYMBOL = 467, REBUILD_SYMBOL = 468, RECOVER_SYMBOL = 469, REDOFILE_SYMBOL = 470, 
    REDO_BUFFER_SIZE_SYMBOL = 471, REDUNDANT_SYMBOL = 472, REFERENCES_SYMBOL = 473, 
    REGEXP_SYMBOL = 474, RELAY_SYMBOL = 475, RELAYLOG_SYMBOL = 476, RELAY_LOG_FILE_SYMBOL = 477, 
    RELAY_LOG_POS_SYMBOL = 478, RELAY_THREAD_SYMBOL = 479, RELEASE_SYMBOL = 480, 
    RELOAD_SYMBOL = 481, REMOVE_SYMBOL = 482, RENAME_SYMBOL = 483, REORGANIZE_SYMBOL = 484, 
    REPAIR_SYMBOL = 485, REPEATABLE_SYMBOL = 486, REPEAT_SYMBOL = 487, REPLACE_SYMBOL = 488, 
    REPLICATION_SYMBOL = 489, REPLICATE_DO_DB_SYMBOL = 490, REPLICATE_IGNORE_DB_SYMBOL = 491, 
    REPLICATE_DO_TABLE_SYMBOL = 492, REPLICATE_IGNORE_TABLE_SYMBOL = 493, 
    REPLICATE_WILD_DO_TABLE_SYMBOL = 494, REPLICATE_WILD_IGNORE_TABLE_SYMBOL = 495, 
    REPLICATE_REWRITE_DB_SYMBOL = 496, REQUIRE_SYMBOL = 497, RESET_SYMBOL = 498, 
    RESIGNAL_SYMBOL = 499, RESTORE_SYMBOL = 500, RESTRICT_SYMBOL = 501, 
    RESUME_SYMBOL = 502, RETURNED_SQLSTATE_SYMBOL = 503, RETURNS_SYMBOL = 504, 
    RETURN_SYMBOL = 505, REVERSE_SYMBOL = 506, REVOKE_SYMBOL = 507, RIGHT_SYMBOL = 508, 
    ROLLBACK_SYMBOL = 509, ROLLUP_SYMBOL = 510, ROTATE_SYMBOL = 511, ROUTINE_SYMBOL = 512, 
    ROWS_SYMBOL = 513, ROW_COUNT_SYMBOL = 514, ROW_FORMAT_SYMBOL = 515, 
    ROW_SYMBOL = 516, RTREE_SYMBOL = 517, SAVEPOINT_SYMBOL = 518, SCHEDULE_SYMBOL = 519, 
    SCHEMA_NAME_SYMBOL = 520, SECOND_MICROSECOND_SYMBOL = 521, SECOND_SYMBOL = 522, 
    SECURITY_SYMBOL = 523, SELECT_SYMBOL = 524, SENSITIVE_SYMBOL = 525, 
    SEPARATOR_SYMBOL = 526, SERIALIZABLE_SYMBOL = 527, SERIAL_SYMBOL = 528, 
    SESSION_SYMBOL = 529, SERVER_SYMBOL = 530, SERVER_OPTIONS_SYMBOL = 531, 
    SESSION_USER_SYMBOL = 532, SET_SYMBOL = 533, SET_VAR_SYMBOL = 534, SHARE_SYMBOL = 535, 
    SHOW_SYMBOL = 536, SHUTDOWN_SYMBOL = 537, SIGNAL_SYMBOL = 538, SIGNED_SYMBOL = 539, 
    SIMPLE_SYMBOL = 540, SLAVE_SYMBOL = 541, SLOW_SYMBOL = 542, SMALLINT_SYMBOL = 543, 
    SNAPSHOT_SYMBOL = 544, SOCKET_SYMBOL = 545, SONAME_SYMBOL = 546, SOUNDS_SYMBOL = 547, 
    SOURCE_SYMBOL = 548, SPATIAL_SYMBOL = 549, SPECIFIC_SYMBOL = 550, SQLEXCEPTION_SYMBOL = 551, 
    SQLSTATE_SYMBOL = 552, SQLWARNING_SYMBOL = 553, SQL_AFTER_GTIDS_SYMBOL = 554, 
    SQL_AFTER_MTS_GAPS_SYMBOL = 555, SQL_BEFORE_GTIDS_SYMBOL = 556, SQL_BIG_RESULT_SYMBOL = 557, 
    SQL_BUFFER_RESULT_SYMBOL = 558, SQL_CACHE_SYMBOL = 559, SQL_CALC_FOUND_ROWS_SYMBOL = 560, 
    SQL_NO_CACHE_SYMBOL = 561, SQL_SMALL_RESULT_SYMBOL = 562, SQL_SYMBOL = 563, 
    SQL_THREAD_SYMBOL = 564, SSL_SYMBOL = 565, STACKED_SYMBOL = 566, STARTING_SYMBOL = 567, 
    STARTS_SYMBOL = 568, START_SYMBOL = 569, STATS_AUTO_RECALC_SYMBOL = 570, 
    STATS_PERSISTENT_SYMBOL = 571, STATS_SAMPLE_PAGES_SYMBOL = 572, STATUS_SYMBOL = 573, 
    STDDEV_SAMP_SYMBOL = 574, STDDEV_SYMBOL = 575, STDDEV_POP_SYMBOL = 576, 
    STD_SYMBOL = 577, STOP_SYMBOL = 578, STORAGE_SYMBOL = 579, STORED_SYMBOL = 580, 
    STRAIGHT_JOIN_SYMBOL = 581, STRING_SYMBOL = 582, SUBCLASS_ORIGIN_SYMBOL = 583, 
    SUBDATE_SYMBOL = 584, SUBJECT_SYMBOL = 585, SUBPARTITIONS_SYMBOL = 586, 
    SUBPARTITION_SYMBOL = 587, SUBSTR_SYMBOL = 588, SUBSTRING_SYMBOL = 589, 
    SUM_SYMBOL = 590, SUPER_SYMBOL = 591, SUSPEND_SYMBOL = 592, SWAPS_SYMBOL = 593, 
    SWITCHES_SYMBOL = 594, SYSDATE_SYMBOL = 595, SYSTEM_USER_SYMBOL = 596, 
    TABLES_SYMBOL = 597, TABLESPACE_SYMBOL = 598, TABLE_REF_PRIORITY_SYMBOL = 599, 
    TABLE_SYMBOL = 600, TABLE_CHECKSUM_SYMBOL = 601, TABLE_NAME_SYMBOL = 602, 
    TEMPORARY_SYMBOL = 603, TEMPTABLE_SYMBOL = 604, TERMINATED_SYMBOL = 605, 
    TEXT_SYMBOL = 606, THAN_SYMBOL = 607, THEN_SYMBOL = 608, TIMESTAMP_SYMBOL = 609, 
    TIMESTAMPADD_SYMBOL = 610, TIMESTAMPDIFF_SYMBOL = 611, TIME_SYMBOL = 612, 
    TINYBLOB_SYMBOL = 613, TINYINT_SYMBOL = 614, TINYTEXT_SYMBOL = 615, 
    TO_SYMBOL = 616, TRAILING_SYMBOL = 617, TRANSACTION_SYMBOL = 618, TRIGGERS_SYMBOL = 619, 
    TRIGGER_SYMBOL = 620, TRIM_SYMBOL = 621, TRUE_SYMBOL = 622, TRUNCATE_SYMBOL = 623, 
    TYPES_SYMBOL = 624, TYPE_SYMBOL = 625, UDF_RETURNS_SYMBOL = 626, UNCOMMITTED_SYMBOL = 627, 
    UNDEFINED_SYMBOL = 628, UNDOFILE_SYMBOL = 629, UNDO_BUFFER_SIZE_SYMBOL = 630, 
    UNDO_SYMBOL = 631, UNICODE_SYMBOL = 632, UNINSTALL_SYMBOL = 633, UNION_SYMBOL = 634, 
    UNIQUE_SYMBOL = 635, UNKNOWN_SYMBOL = 636, UNLOCK_SYMBOL = 637, UNSIGNED_SYMBOL = 638, 
    UNTIL_SYMBOL = 639, UPDATE_SYMBOL = 640, UPGRADE_SYMBOL = 641, USAGE_SYMBOL = 642, 
    USER_RESOURCES_SYMBOL = 643, USER_SYMBOL = 644, USE_FRM_SYMBOL = 645, 
    USE_SYMBOL = 646, USING_SYMBOL = 647, UTC_DATE_SYMBOL = 648, UTC_TIMESTAMP_SYMBOL = 649, 
    UTC_TIME_SYMBOL = 650, VALIDATION_SYMBOL = 651, VALUES_SYMBOL = 652, 
    VALUE_SYMBOL = 653, VARBINARY_SYMBOL = 654, VARCHAR_SYMBOL = 655, VARIABLES_SYMBOL = 656, 
    VARIANCE_SYMBOL = 657, VARYING_SYMBOL = 658, VAR_POP_SYMBOL = 659, VAR_SAMP_SYMBOL = 660, 
    VIEW_SYMBOL = 661, VIRTUAL_SYMBOL = 662, WAIT_SYMBOL = 663, WARNINGS_SYMBOL = 664, 
    WEEK_SYMBOL = 665, WEIGHT_STRING_SYMBOL = 666, WHEN_SYMBOL = 667, WHERE_SYMBOL = 668, 
    WHILE_SYMBOL = 669, WITH_SYMBOL = 670, WITHOUT_SYMBOL = 671, WORK_SYMBOL = 672, 
    WRAPPER_SYMBOL = 673, WRITE_SYMBOL = 674, X509_SYMBOL = 675, XA_SYMBOL = 676, 
    XID_SYMBOL = 677, XML_SYMBOL = 678, XOR_SYMBOL = 679, YEAR_MONTH_SYMBOL = 680, 
    YEAR_SYMBOL = 681, ZEROFILL_SYMBOL = 682, PERSIST_SYMBOL = 683, ROLE_SYMBOL = 684, 
    ADMIN_SYMBOL = 685, INVISIBLE_SYMBOL = 686, VISIBLE_SYMBOL = 687, EXCEPT_SYMBOL = 688, 
    COMPONENT_SYMBOL = 689, RECURSIVE_SYMBOL = 690, JSON_OBJECTAGG_SYMBOL = 691, 
    JSON_ARRAYAGG_SYMBOL = 692, OF_SYMBOL = 693, SKIP_SYMBOL = 694, LOCKED_SYMBOL = 695, 
    NOWAIT_SYMBOL = 696, GROUPING_SYMBOL = 697, PERSIST_ONLY_SYMBOL = 698, 
    HISTOGRAM_SYMBOL = 699, BUCKETS_SYMBOL = 700, REMOTE_SYMBOL = 701, CLONE_SYMBOL = 702, 
    CUME_DIST_SYMBOL = 703, DENSE_RANK_SYMBOL = 704, EXCLUDE_SYMBOL = 705, 
    FIRST_VALUE_SYMBOL = 706, FOLLOWING_SYMBOL = 707, GROUPS_SYMBOL = 708, 
    LAG_SYMBOL = 709, LAST_VALUE_SYMBOL = 710, LEAD_SYMBOL = 711, NTH_VALUE_SYMBOL = 712, 
    NTILE_SYMBOL = 713, NULLS_SYMBOL = 714, OTHERS_SYMBOL = 715, OVER_SYMBOL = 716, 
    PERCENT_RANK_SYMBOL = 717, PRECEDING_SYMBOL = 718, RANK_SYMBOL = 719, 
    RESPECT_SYMBOL = 720, ROW_NUMBER_SYMBOL = 721, TIES_SYMBOL = 722, UNBOUNDED_SYMBOL = 723, 
    WINDOW_SYMBOL = 724, EMPTY_SYMBOL = 725, JSON_TABLE_SYMBOL = 726, NESTED_SYMBOL = 727, 
    ORDINALITY_SYMBOL = 728, PATH_SYMBOL = 729, HISTORY_SYMBOL = 730, REUSE_SYMBOL = 731, 
    SRID_SYMBOL = 732, THREAD_PRIORITY_SYMBOL = 733, RESOURCE_SYMBOL = 734, 
    SYSTEM_SYMBOL = 735, VCPU_SYMBOL = 736, MASTER_PUBLIC_KEY_PATH_SYMBOL = 737, 
    GET_MASTER_PUBLIC_KEY_SYMBOL = 738, RESTART_SYMBOL = 739, DEFINITION_SYMBOL = 740, 
    DESCRIPTION_SYMBOL = 741, ORGANIZATION_SYMBOL = 742, REFERENCE_SYMBOL = 743, 
    OPTIONAL_SYMBOL = 744, SECONDARY_SYMBOL = 745, SECONDARY_ENGINE_SYMBOL = 746, 
    SECONDARY_LOAD_SYMBOL = 747, SECONDARY_UNLOAD_SYMBOL = 748, ACTIVE_SYMBOL = 749, 
    INACTIVE_SYMBOL = 750, LATERAL_SYMBOL = 751, RETAIN_SYMBOL = 752, OLD_SYMBOL = 753, 
    NETWORK_NAMESPACE_SYMBOL = 754, ENFORCED_SYMBOL = 755, ARRAY_SYMBOL = 756, 
    OJ_SYMBOL = 757, MEMBER_SYMBOL = 758, RANDOM_SYMBOL = 759, MASTER_COMPRESSION_ALGORITHM_SYMBOL = 760, 
    MASTER_ZSTD_COMPRESSION_LEVEL_SYMBOL = 761, PRIVILEGE_CHECKS_USER_SYMBOL = 762, 
    MASTER_TLS_CIPHERSUITES_SYMBOL = 763, REQUIRE_ROW_FORMAT_SYMBOL = 764, 
    PASSWORD_LOCK_TIME_SYMBOL = 765, FAILED_LOGIN_ATTEMPTS_SYMBOL = 766, 
    REQUIRE_TABLE_PRIMARY_KEY_CHECK_SYMBOL = 767, STREAM_SYMBOL = 768, OFF_SYMBOL = 769, 
    RETURNING_SYMBOL = 770, JSON_VALUE_SYMBOL = 771, TLS_SYMBOL = 772, ATTRIBUTE_SYMBOL = 773, 
    ENGINE_ATTRIBUTE_SYMBOL = 774, SECONDARY_ENGINE_ATTRIBUTE_SYMBOL = 775, 
    SOURCE_CONNECTION_AUTO_FAILOVER_SYMBOL = 776, ZONE_SYMBOL = 777, GRAMMAR_SELECTOR_DERIVED_EXPR = 778, 
    REPLICA_SYMBOL = 779, REPLICAS_SYMBOL = 780, ASSIGN_GTIDS_TO_ANONYMOUS_TRANSACTIONS_SYMBOL = 781, 
    GET_SOURCE_PUBLIC_KEY_SYMBOL = 782, SOURCE_AUTO_POSITION_SYMBOL = 783, 
    SOURCE_BIND_SYMBOL = 784, SOURCE_COMPRESSION_ALGORITHM_SYMBOL = 785, 
    SOURCE_CONNECT_RETRY_SYMBOL = 786, SOURCE_DELAY_SYMBOL = 787, SOURCE_HEARTBEAT_PERIOD_SYMBOL = 788, 
    SOURCE_HOST_SYMBOL = 789, SOURCE_LOG_FILE_SYMBOL = 790, SOURCE_LOG_POS_SYMBOL = 791, 
    SOURCE_PASSWORD_SYMBOL = 792, SOURCE_PORT_SYMBOL = 793, SOURCE_PUBLIC_KEY_PATH_SYMBOL = 794, 
    SOURCE_RETRY_COUNT_SYMBOL = 795, SOURCE_SSL_SYMBOL = 796, SOURCE_SSL_CA_SYMBOL = 797, 
    SOURCE_SSL_CAPATH_SYMBOL = 798, SOURCE_SSL_CERT_SYMBOL = 799, SOURCE_SSL_CIPHER_SYMBOL = 800, 
    SOURCE_SSL_CRL_SYMBOL = 801, SOURCE_SSL_CRLPATH_SYMBOL = 802, SOURCE_SSL_KEY_SYMBOL = 803, 
    SOURCE_SSL_VERIFY_SERVER_CERT_SYMBOL = 804, SOURCE_TLS_CIPHERSUITES_SYMBOL = 805, 
    SOURCE_TLS_VERSION_SYMBOL = 806, SOURCE_USER_SYMBOL = 807, SOURCE_ZSTD_COMPRESSION_LEVEL_SYMBOL = 808, 
    ST_COLLECT_SYMBOL = 809, KEYRING_SYMBOL = 810, AUTHENTICATION_SYMBOL = 811, 
    FACTOR_SYMBOL = 812, FINISH_SYMBOL = 813, INITIATE_SYMBOL = 814, REGISTRATION_SYMBOL = 815, 
    UNREGISTER_SYMBOL = 816, INITIAL_SYMBOL = 817, CHALLENGE_RESPONSE_SYMBOL = 818, 
    GTID_ONLY_SYMBOL = 819, INTERSECT_SYMBOL = 820, BULK_SYMBOL = 821, URL_SYMBOL = 822, 
    GENERATE_SYMBOL = 823, PARSE_TREE_SYMBOL = 824, LOG_SYMBOL = 825, GTIDS_SYMBOL = 826, 
    PARALLEL_SYMBOL = 827, S3_SYMBOL = 828, WHITESPACE = 829, INVALID_INPUT = 830, 
    UNDERSCORE_CHARSET = 831, IDENTIFIER = 832, NCHAR_TEXT = 833, BACK_TICK_QUOTED_ID = 834, 
    DOUBLE_QUOTED_TEXT = 835, SINGLE_QUOTED_TEXT = 836, DOLLAR_QUOTED_STRING_TEXT = 837, 
    VERSION_COMMENT_START = 838, MYSQL_COMMENT_START = 839, VERSION_COMMENT_END = 840, 
    BLOCK_COMMENT = 841, INVALID_BLOCK_COMMENT = 842, POUND_COMMENT = 843, 
    DASHDASH_COMMENT = 844, SIMPLE_IDENTIFIER = 845, NOT_EQUAL2_OPERATOR = 846
  };

  enum {
    HIDDEN_TOKEN = 1
  };

  explicit MySQLLexer(antlr4::CharStream *input);

  ~MySQLLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  void action(antlr4::RuleContext *context, size_t ruleIndex, size_t actionIndex) override;

  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.
  void LOGICAL_OR_OPERATORAction(antlr4::RuleContext *context, size_t actionIndex);
  void AT_TEXT_SUFFIXAction(antlr4::RuleContext *context, size_t actionIndex);
  void AT_AT_SIGN_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void INT_NUMBERAction(antlr4::RuleContext *context, size_t actionIndex);
  void DOT_IDENTIFIERAction(antlr4::RuleContext *context, size_t actionIndex);
  void ADDDATE_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void BIT_AND_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void BIT_OR_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void BIT_XOR_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void CAST_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void COUNT_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void CURDATE_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void CURRENT_DATE_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void CURRENT_TIME_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void CURTIME_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void DATE_ADD_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void DATE_SUB_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void EXTRACT_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void GROUP_CONCAT_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void MAX_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void MID_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void MIN_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void NOT_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void NOW_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void POSITION_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void SESSION_USER_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void STDDEV_SAMP_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void STDDEV_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void STDDEV_POP_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void STD_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void SUBDATE_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void SUBSTR_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void SUBSTRING_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void SUM_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void SYSDATE_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void SYSTEM_USER_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void TRIM_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void VARIANCE_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void VAR_POP_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void VAR_SAMP_SYMBOLAction(antlr4::RuleContext *context, size_t actionIndex);
  void UNDERSCORE_CHARSETAction(antlr4::RuleContext *context, size_t actionIndex);
  void MYSQL_COMMENT_STARTAction(antlr4::RuleContext *context, size_t actionIndex);
  void VERSION_COMMENT_ENDAction(antlr4::RuleContext *context, size_t actionIndex);

  // Individual semantic predicate functions triggered by sempred() above.
  bool JSON_SEPARATOR_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool JSON_UNQUOTED_SEPARATOR_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ACCOUNT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ALWAYS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ANALYSE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool BIN_NUM_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool CHANNEL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool COMPRESSION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool DECIMAL_NUM_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool DES_KEY_FILE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ENCRYPTION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool END_OF_INPUT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool FILE_BLOCK_SIZE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool GENERATED_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool GROUP_REPLICATION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool INSTANCE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool JSON_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool LOCATOR_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool LONG_NUM_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool MASTER_TLS_VERSION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool MAX_STATEMENT_TIME_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool NCHAR_STRING_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool NEG_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool NEVER_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool NONBLOCKING_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool OLD_PASSWORD_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool OPTIMIZER_COSTS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool REDOFILE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ROTATE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SERVER_OPTIONS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SET_VAR_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SQL_CACHE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool STORED_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool TABLE_REF_PRIORITY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool UDF_RETURNS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool VALIDATION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool VIRTUAL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool XID_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool PERSIST_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ROLE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ADMIN_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool INVISIBLE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool VISIBLE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool EXCEPT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool COMPONENT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool RECURSIVE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool JSON_OBJECTAGG_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool JSON_ARRAYAGG_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool OF_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SKIP_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool LOCKED_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool NOWAIT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool GROUPING_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool PERSIST_ONLY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool HISTOGRAM_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool BUCKETS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool REMOTE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool CLONE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool CUME_DIST_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool DENSE_RANK_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool EXCLUDE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool FIRST_VALUE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool FOLLOWING_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool GROUPS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool LAG_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool LAST_VALUE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool LEAD_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool NTH_VALUE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool NTILE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool NULLS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool OTHERS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool OVER_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool PERCENT_RANK_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool PRECEDING_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool RANK_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool RESPECT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ROW_NUMBER_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool TIES_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool UNBOUNDED_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool WINDOW_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool EMPTY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool JSON_TABLE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool NESTED_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ORDINALITY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool PATH_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool HISTORY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool REUSE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SRID_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool THREAD_PRIORITY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool RESOURCE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SYSTEM_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool VCPU_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool MASTER_PUBLIC_KEY_PATH_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool GET_MASTER_PUBLIC_KEY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool RESTART_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool DEFINITION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool DESCRIPTION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ORGANIZATION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool REFERENCE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool OPTIONAL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SECONDARY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SECONDARY_ENGINE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SECONDARY_LOAD_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SECONDARY_UNLOAD_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ACTIVE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool INACTIVE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool LATERAL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool RETAIN_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool OLD_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool NETWORK_NAMESPACE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ENFORCED_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ARRAY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool OJ_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool MEMBER_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool RANDOM_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool MASTER_COMPRESSION_ALGORITHM_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool MASTER_ZSTD_COMPRESSION_LEVEL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool PRIVILEGE_CHECKS_USER_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool MASTER_TLS_CIPHERSUITES_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool REQUIRE_ROW_FORMAT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool PASSWORD_LOCK_TIME_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool FAILED_LOGIN_ATTEMPTS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool REQUIRE_TABLE_PRIMARY_KEY_CHECK_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool STREAM_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool OFF_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool RETURNING_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool JSON_VALUE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool TLS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ATTRIBUTE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ENGINE_ATTRIBUTE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SECONDARY_ENGINE_ATTRIBUTE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_CONNECTION_AUTO_FAILOVER_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ZONE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool GRAMMAR_SELECTOR_DERIVED_EXPRSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool REPLICA_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool REPLICAS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ASSIGN_GTIDS_TO_ANONYMOUS_TRANSACTIONS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool GET_SOURCE_PUBLIC_KEY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_AUTO_POSITION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_BIND_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_COMPRESSION_ALGORITHM_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_CONNECT_RETRY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_DELAY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_HEARTBEAT_PERIOD_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_HOST_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_LOG_FILE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_LOG_POS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_PASSWORD_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_PORT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_PUBLIC_KEY_PATH_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_RETRY_COUNT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_SSL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_SSL_CA_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_SSL_CAPATH_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_SSL_CERT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_SSL_CIPHER_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_SSL_CRL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_SSL_CRLPATH_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_SSL_KEY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_SSL_VERIFY_SERVER_CERT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_TLS_CIPHERSUITES_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_TLS_VERSION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_USER_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SOURCE_ZSTD_COMPRESSION_LEVEL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool ST_COLLECT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool KEYRING_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool AUTHENTICATION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool FACTOR_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool FINISH_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool INITIATE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool REGISTRATION_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool UNREGISTER_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool INITIAL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool CHALLENGE_RESPONSE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool GTID_ONLY_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool INTERSECT_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool BULK_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool URL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool GENERATE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool PARSE_TREE_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool LOG_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool GTIDS_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool PARALLEL_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool S3_SYMBOLSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool BACK_TICK_QUOTED_IDSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool DOUBLE_QUOTED_TEXTSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool SINGLE_QUOTED_TEXTSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool DOLLAR_QUOTED_STRING_TEXTSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool VERSION_COMMENT_STARTSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);
  bool VERSION_COMMENT_ENDSempred(antlr4::RuleContext *_localctx, size_t predicateIndex);

};

}  // namespace parsers
