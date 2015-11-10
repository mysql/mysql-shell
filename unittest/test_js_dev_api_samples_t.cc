/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/pointer_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"

#include "shellcore/shell_core.h"
#include "shellcore/shell_jscript.h"
#include "shell_script_tester.h"

#include "shellcore/server_registry.h"

namespace shcore
{
  class Shell_js_dev_api_sample_tester : public Shell_js_script_tester
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_js_script_tester::SetUp();

      set_config_folder("js_dev_api_examples");
      set_setup_script("setup.js");
    }

    void create_connection()
    {
      Server_registry* sr = new Server_registry("mysqlxconfig.json");
      Connection_options cs;
      try
      {
        cs = sr->get_connection_options("myapp");
      }
      catch (std::runtime_error &e)
      {
        cs = sr->add_connection_options("myapp", "host=localhost; dbUser=mike; schema=test;");
        sr->merge();
      }
    }
  };

  TEST_F(Shell_js_dev_api_sample_tester, transaction_handling)
  {
    validate_interactive("transaction_handling.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, processing_warnings)
  {
    validate_interactive("processing_warnings.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, connecting_via_datasource_file)
  {
    create_connection();
    validate_interactive("connecting_via_datasource_file.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, setting_the_current_schema_2)
  {
    create_connection();
    validate_interactive("setting_the_current_schema_2.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, dynamic_sql)
  {
    create_connection();
    validate_interactive("dynamic_sql.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, connecting_to_a_session_2)
  {
    output_handler.ret_pwd = "s3cr3t!";
    validate_interactive("connecting_to_a_session_2.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, database_connection_example)
  {
    validate_interactive("database_connection_example.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, connecting_to_a_session)
  {
    validate_interactive("connecting_to_a_session.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, working_with_a_session_object)
  {
    validate_interactive("working_with_a_session_object.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, using_sql)
  {
    validate_interactive("using_sql.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, setting_the_current_schema)
  {
    validate_interactive("setting_the_current_schema.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, parameter_binding)
  {
    validate_interactive("parameter_binding.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, preparing_crud_statements)
  {
    validate_interactive("preparing_crud_statements.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, working_with_collections)
  {
    validate_interactive("working_with_collections.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, collection_add)
  {
    validate_interactive("collection_add.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, document_identity_1)
  {
    validate_interactive("document_identity_1.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, document_identity_2_3)
  {
    validate_interactive("document_identity_2_3.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, collection_find)
  {
    validate_interactive("collection_find.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, table_insert)
  {
    validate_interactive("table_insert.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, working_with_datasets)
  {
    validate_interactive("working_with_datasets.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, working_with_datasets_2)
  {
    validate_interactive("working_with_datasets_2.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, working_with_results)
  {
    validate_interactive("working_with_results.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, working_with_results_2)
  {
    validate_interactive("working_with_results_2.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, error_handling)
  {
    validate_interactive("error_handling.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, working_with_sql_results)
  {
    validate_interactive("working_with_sql_results.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, working_with_sql_results_2)
  {
    validate_interactive("working_with_sql_results_2.js");
  }

  TEST_F(Shell_js_dev_api_sample_tester, method_chaining)
  {
    validate_interactive("method_chaining.js");
  }
}