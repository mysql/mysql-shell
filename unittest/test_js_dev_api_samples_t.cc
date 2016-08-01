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

      _extension = "js";
      _new_format = true;
    }

    virtual void pre_process_line(const std::string &path, std::string & line)
    {
      // Unit tests work using default ports, if that is not the case
      // We need to update them before being executed
      if (!_port.empty() && _port != "33060")
      {
        // Some examples contain a hardcoded port
        // we need to change that port by the right one
        size_t pos = line.find("33060");
        if (pos != std::string::npos)
        {
          std::string new_line = line.substr(0, pos);
          new_line += _port;
          new_line += line.substr(pos + 5);
          line = new_line;
        }

        // Other examples use an URI assumin the defaut port
        // If that's not the case, we need to add it to the URI
        pos = line.find("'mike:s3cr3t!@localhost'");
        if (pos != std::string::npos)
        {
          std::string new_line = line.substr(0, pos);
          new_line += "'mike:s3cr3t!@localhost:";
          new_line += _port;
          new_line += "');";
          line = new_line;
        }
      }
    }

    void create_connection()
    {
      Server_registry* sr = new Server_registry("mysqlxconfig.json");
      sr->load();
      Connection_options cs;
      try
      {
        cs = sr->get_connection_options("myapp");
      }
      catch (std::runtime_error &e)
      {
        std::string connection_options = "host=localhost; dbUser=mike; schema=test;";

        if (!_port.empty())
          connection_options += " port=" + _port + ";";

        cs = sr->add_connection_options("myapp", connection_options);
        sr->merge();
      }
    }

    void delete_connection()
    {
      Server_registry* sr = new Server_registry("mysqlxconfig.json");
      sr->load();
      Connection_options cs;
      try
      {
        cs = sr->get_connection_options("myapp");
        sr->remove_connection_options(cs);
        sr->merge();
      }
      catch (std::runtime_error &e)
      {
      }
    }
  };
  //==================>>> building_expressions
  TEST_F(Shell_js_dev_api_sample_tester, Expression_Strings)
  {
    validate_interactive("building_expressions/Expression_Strings");
  }

  //==================>>> concepts
  TEST_F(Shell_js_dev_api_sample_tester, Database_Connection_Example)
  {
    validate_interactive("concepts/Database_Connection_Example");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Dynamic_SQL)
  {
    create_connection();
    validate_interactive("concepts/Dynamic_SQL");
    delete_connection();
  }

  TEST_F(Shell_js_dev_api_sample_tester, Setting_the_Current_Schema)
  {
    validate_interactive("concepts/Setting_the_Current_Schema");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Using_SQL_with_NodeSession)
  {
    validate_interactive("concepts/Using_SQL_with_NodeSession");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_a_Session_Object)
  {
    validate_interactive("concepts/Working_with_a_Session_Object");
  }

  //==================>>> crud_operations
  TEST_F(Shell_js_dev_api_sample_tester, Interactive_Shell_Commands)
  {
    validate_interactive("crud_operations/Interactive_Shell_Commands");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Method_Chaining)
  {
    validate_interactive("crud_operations/Method_Chaining");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Parameter_Binding)
  {
    validate_interactive("crud_operations/Parameter_Binding");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Parameter_Binding_1)
  {
    validate_interactive("crud_operations/Parameter_Binding_1");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Preparing_CRUD_Statements)
  {
    validate_interactive("crud_operations/Preparing_CRUD_Statements");
  }

  //==================>>> results
  TEST_F(Shell_js_dev_api_sample_tester, Fetching_All_Data_Items_at_Once)
  {
    validate_interactive("results/Fetching_All_Data_Items_at_Once");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_Data_Sets)
  {
    validate_interactive("results/Working_with_Data_Sets");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_Data_Sets_1)
  {
    validate_interactive("results/Working_with_Data_Sets_1");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_Results)
  {
    validate_interactive("results/Working_with_Results");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_Results_1)
  {
    validate_interactive("results/Working_with_Results_1");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_SQL_Result_Sets)
  {
    validate_interactive("results/Working_with_SQL_Result_Sets");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_SQL_Result_Sets_1)
  {
    validate_interactive("results/Working_with_SQL_Result_Sets_1");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_SQL_Result_Sets_2)
  {
    validate_interactive("results/Working_with_SQL_Result_Sets_2");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_SQL_Result_Sets_3)
  {
    validate_interactive("results/Working_with_SQL_Result_Sets_3");
  }

  //==================>>> statement_execution
  TEST_F(Shell_js_dev_api_sample_tester, Error_Handling)
  {
    validate_interactive("statement_execution/Error_Handling");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Processing_Warnings)
  {
    validate_interactive("statement_execution/Processing_Warnings");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Processing_Warnings_1)
  {
    validate_interactive("statement_execution/Processing_Warnings_1");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Transaction_Handling)
  {
    validate_interactive("statement_execution/Transaction_Handling");
  }

  //==================>>> working_with_collections
  TEST_F(Shell_js_dev_api_sample_tester, Basic_CRUD_Operations_on_Collections)
  {
    validate_interactive("working_with_collections/Basic_CRUD_Operations_on_Collections");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Collection_add)
  {
    validate_interactive("working_with_collections/Collection_add");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Collection_find)
  {
    validate_interactive("working_with_collections/Collection_find");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Creating_a_Collection)
  {
    validate_interactive("working_with_collections/Creating_a_Collection");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Document_Identity)
  {
    validate_interactive("working_with_collections/Document_Identity");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Document_Identity_1)
  {
    validate_interactive("working_with_collections/Document_Identity_1");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Document_Identity_2)
  {
    validate_interactive("working_with_collections/Document_Identity_2");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_Existing_Collections)
  {
    validate_interactive("working_with_collections/Working_with_Existing_Collections");
  }

  //==================>>> working_with_collections
  TEST_F(Shell_js_dev_api_sample_tester, Document_Object___Class_Diagram)
  {
    validate_interactive("working_with_documents/Document_Object___Class_Diagram");
  }

  //==================>>> working_with_relational_tables
  TEST_F(Shell_js_dev_api_sample_tester, Table_insert)
  {
    validate_interactive("working_with_relational_tables/Table_insert");
  }

  TEST_F(Shell_js_dev_api_sample_tester, Working_with_Relational_Tables)
  {
    validate_interactive("working_with_relational_tables/Working_with_Relational_Tables");
  }

  //==================>>> working_with_tables_documents
  TEST_F(Shell_js_dev_api_sample_tester, Collections_as_Relational_Tables)
  {
    validate_interactive("working_with_tables_documents/Collections_as_Relational_Tables");
  }
}