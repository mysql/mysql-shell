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
  class Shell_js_dev_api_sample_tester : public Shell_script_tester
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      // This may be needed in case it is decided to load the
      // scripts from the extraction done on the dev-api docs
      //char *path = getenv("DEVAPI_EXAMPLES");
      //if (path)
      //  _scripts_home.assign(path);

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_JScript, initilaized);
    }
  };

  TEST_F(Shell_js_dev_api_sample_tester, initialization)
  {
    Server_registry* sr = new Server_registry("mysqlxconfig.json");
    Connection_options cs;
    try
    {
      cs = sr->get_connection_by_name("myapp");
    }
    catch (std::runtime_error &e)
    {
      cs = sr->add_connection_options("app=myapp; server=localhost; user=mike; schema=test;");
      sr->merge();
    }

    set_config_folder("devapi");
    set_setup_script("setup.js");

    validate_batch("transaction_handling.js");
    validate_batch("processing_warnings.js");
    validate_batch("connecting_via_datasource_file.js");
    validate_batch("setting_the_current_schema_2.js");
    validate_batch("dynamic_sql.js");
    output_handler.ret_pwd = "s3cr3t!";
    validate_batch("connecting_to_a_session_2.js");
    validate_batch("database_connection_example.js");
    validate_batch("connecting_to_a_session.js");
    validate_batch("working_with_a_session_object.js");
    validate_batch("using_sql.js");
    validate_batch("setting_the_current_schema.js");
    validate_batch("parameter_binding.js");
    validate_batch("preparing_crud_statements.js");
    validate_batch("working_with_collections.js");
    validate_batch("collection_add.js");
    validate_batch("document_identity_2_3.js");
    validate_batch("collection_find.js");
    validate_batch("table_insert.js");
    validate_batch("collection_as_relational_table.js");
    validate_batch("working_with_datasets_1.js");
    validate_batch("working_with_datasets_2.js");
    validate_batch("working_with_sql_resultsets.js");
    validate_batch("error_handling.js");
  }
}