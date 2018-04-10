/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <gtest/gtest_prod.h>
#include <cstdio>
#include "src/mysqlsh/cmdline_shell.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/shell_test_wrapper.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"

namespace tests {
class Table_select : public Shell_core_test_wrapper {
 public:
  virtual void set_options() {
    _options->interactive = true;
    _options->wizards = true;
    _options->trace_protocol = true;
  }

  virtual void SetUp() {
    // Redirect cout
    _cout_backup = std::cerr.rdbuf();
    std::cout.rdbuf(_cout.rdbuf());

    Shell_core_test_wrapper::SetUp();

    // Connects the two shell instances to be used on these tests
    execute("\\connect -mx " + _uri);

    execute("var schema = session.getSchema('test_locking');");
    execute("var table = schema.getTable('test_table');");

    // Cleans the stream
    _cout.str("");
  }

  virtual void TearDown() {
    execute("session.close()");

    // Restore old cerr.
    std::cout.rdbuf(_cout_backup);

    Shell_core_test_wrapper::TearDown();
  }

  static void SetUpTestCase() {
    tests::Shell_test_wrapper global_shell;
    global_shell.execute("\\connect -mx " + shell_test_server_uri('x'));

    // preparation
    global_shell.execute(
        "session.sql('drop schema if exists test_locking').execute();");
    global_shell.execute("var schema = session.createSchema('test_locking');");

    // global_shell.execute("\\sql");
    global_shell.execute(
        "session.sql('create table test_locking.test_table (_id int "
        "primary key, name varchar(10))').execute();");
    // global_shell.execute("\\js");
    global_shell.execute("var table = schema.getTable('test_table');");
    global_shell.execute("table.insert().values(1, 'one').execute();");
    global_shell.execute("table.insert().values(2, 'two').execute();");
    global_shell.execute("table.insert().values(3, 'three').execute();");

    global_shell.execute("session.close();");

    EXPECT_STREQ("", global_shell.get_output_handler().std_err.c_str());
  }

  static void TearDownTestCase() {
    tests::Shell_test_wrapper global_shell;
    global_shell.execute("\\connect -mx " + shell_test_server_uri('x'));

    global_shell.execute("session.dropSchema('test_locking');");
    global_shell.execute("session.close();");
  }

 protected:
  std::streambuf *_cout_backup;
  std::ostringstream _cout;
};

TEST_F(Table_select, lock_none) {
  execute("table.select().where('_id = \"1\"');");

  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_none",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "}"}),
                             _cout.str());

  _cout.str("");
}

TEST_F(Table_select, lock_shared) {
  execute("session.startTransaction();");
  _cout.str("");
  execute("table.select().where('_id = \"1\"').lockShared();");
  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_shared",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "  locking: SHARED_LOCK",
                                        "}"}),
                             _cout.str());

  execute("session.rollback();");
  _cout.str("");
}

TEST_F(Table_select, lock_shared_nowait_string) {
  execute("session.startTransaction();");
  _cout.str("");
  execute("table.select().where('_id = \"1\"').lockShared('nowait');");
  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_shared",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "  locking: SHARED_LOCK",
                                        "  locking_options: NOWAIT",
                                        "}"}),
                             _cout.str());

  execute("session.rollback();");
  _cout.str("");
}

TEST_F(Table_select, lock_shared_nowait_constant) {
  execute("session.startTransaction();");
  _cout.str("");
  execute(
      "table.select().where('_id = "
      "\"1\"').lockShared(mysqlx.LockContention.NOWAIT);");
  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_shared",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "  locking: SHARED_LOCK",
                                        "  locking_options: NOWAIT",
                                        "}"}),
                             _cout.str());

  execute("session.rollback();");
  _cout.str("");
}

TEST_F(Table_select, lock_shared_skip_lock_string) {
  execute("session.startTransaction();");
  _cout.str("");
  execute("table.select().where('_id = \"1\"').lockShared('skip_locked');");
  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_shared",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "  locking: SHARED_LOCK",
                                        "  locking_options: SKIP_LOCKED",
                                        "}"}),
                             _cout.str());

  execute("session.rollback();");
  _cout.str("");
}

TEST_F(Table_select, lock_shared_skip_lock_constant) {
  execute("session.startTransaction();");
  _cout.str("");
  execute(
      "table.select().where('_id = "
      "\"1\"').lockShared(mysqlx.LockContention.SKIP_LOCKED);");
  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_shared",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "  locking: SHARED_LOCK",
                                        "  locking_options: SKIP_LOCKED",
                                        "}"}),
                             _cout.str());

  execute("session.rollback();");
  _cout.str("");
}

TEST_F(Table_select, lock_exclusive) {
  execute("session.startTransaction();");
  _cout.str("");
  execute("table.select().where('_id = \"1\"').lockExclusive();");
  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_exclusive",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "  locking: EXCLUSIVE_LOCK",
                                        "}"}),
                             _cout.str());

  execute("session.rollback();");
  _cout.str("");
}

TEST_F(Table_select, lock_exclusive_nowait_string) {
  execute("session.startTransaction();");
  _cout.str("");
  execute("table.select().where('_id = \"1\"').lockExclusive('nowait');");
  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_exclusive",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "  locking: EXCLUSIVE_LOCK",
                                        "  locking_options: NOWAIT",
                                        "}"}),
                             _cout.str());

  execute("session.rollback();");
  _cout.str("");
}

TEST_F(Table_select, lock_exclusive_nowait_constant) {
  execute("session.startTransaction();");
  _cout.str("");
  execute(
      "table.select().where('_id = "
      "\"1\"').lockExclusive(mysqlx.LockContention.SKIP_LOCKED);");
  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_exclusive",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "  locking: EXCLUSIVE_LOCK",
                                        "  locking_options: SKIP_LOCKED",
                                        "}"}),
                             _cout.str());

  execute("session.rollback();");
  _cout.str("");
}

TEST_F(Table_select, lock_exclusive_skip_lock_string) {
  execute("session.startTransaction();");
  _cout.str("");
  execute("table.select().where('_id = \"1\"').lockExclusive('skip_locked');");
  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_exclusive",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "  locking: EXCLUSIVE_LOCK",
                                        "  locking_options: SKIP_LOCKED",
                                        "}"}),
                             _cout.str());

  execute("session.rollback();");
  _cout.str("");
}

TEST_F(Table_select, lock_exclusive_skip_lock_constant) {
  execute("session.startTransaction();");
  _cout.str("");
  execute(
      "table.select().where('_id = "
      "\"1\"').lockExclusive(mysqlx.LockContention.SKIP_LOCKED);");
  // Validates the stream
  MY_EXPECT_MULTILINE_OUTPUT("Table_select.lock_exclusive",
                             multiline({">>>> SEND Mysqlx.Crud.Find {",
                                        "  collection {",
                                        "    name: \"test_table\"",
                                        "    schema: \"test_locking\"",
                                        "  }",
                                        "  data_model: TABLE",
                                        "  criteria {",
                                        "    type: OPERATOR",
                                        "    operator {",
                                        "      name: \"==\"",
                                        "      param {",
                                        "        type: IDENT",
                                        "        identifier {",
                                        "          name: \"_id\"",
                                        "        }",
                                        "      }",
                                        "      param {",
                                        "        type: LITERAL",
                                        "        literal {",
                                        "          type: V_OCTETS",
                                        "          v_octets {",
                                        "            value: \"1\"",
                                        "          }",
                                        "        }",
                                        "      }",
                                        "    }",
                                        "  }",
                                        "  locking: EXCLUSIVE_LOCK",
                                        "  locking_options: SKIP_LOCKED",
                                        "}"}),
                             _cout.str());

  execute("session.rollback();");
  _cout.str("");
}

}  // namespace tests
