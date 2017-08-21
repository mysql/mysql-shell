/* Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.

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

#include "unittest/test_utils/server_mock.h"
#include <condition_variable>
#include <fstream>
#include <random>
#include <vector>
#include "mysqlshdk/libs/db/column.h"
#include "unittest/test_utils/shell_base_test.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_json.h"

namespace tests {

// TODO(rennox) This function should be deleted and a UUID should be used
// instead
std::string random_json_name(std::string::size_type length) {
  std::string alphanum =
      "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::random_device seed;
  std::mt19937 rng{seed()};
  std::uniform_int_distribution<std::string::size_type> dist(
      0, alphanum.size() - 1);

  std::string result;
  result.reserve(length);
  while (length--)
    result += alphanum[dist(rng)];

  return result + ".json";
}

Server_mock::Server_mock() {}

std::string Server_mock::create_data_file(
    const std::vector<testing::Fake_result_data> &data) {
  shcore::JSON_dumper dumper;

  dumper.start_object();
  dumper.append_string("stmts");
  dumper.start_array();

  for (auto result : data) {
    dumper.start_object();
    dumper.append_string("stmt");
    dumper.append_string(result.sql);
    dumper.append_string("result");
    dumper.start_object();
    dumper.append_string("columns");
    dumper.start_array();
    for (size_t index = 0; index < result.names.size(); index++) {
      dumper.start_object();
      dumper.append_string("type");
      dumper.append_string(map_column_type(result.types[index]));
      dumper.append_string("name");
      dumper.append_string(result.names[index]);
      dumper.end_object();
    }
    dumper.end_array();

    dumper.append_string("rows");
    dumper.start_array();
    for (auto row : result.rows) {
      dumper.start_array();
      for (size_t field_index = 0; field_index < row.size(); field_index++) {
        auto type = map_column_type(result.types[field_index]);
        if (type == "STRING")
          dumper.append_string(row[field_index]);
        else
          dumper.append_int64(std::stoi(row[field_index]));
      }
      dumper.end_array();
    }
    dumper.end_array();

    dumper.end_object();

    dumper.end_object();
  }

  dumper.end_array();
  dumper.end_object();

  std::string prefix = shcore::get_binary_folder();

#ifdef _WIN32
  std::string name = prefix + "\\" + random_json_name(15);
#else
  std::string name = prefix + "/" + random_json_name(15);
#endif

  if (!shcore::create_file(name, dumper.str()))
    throw std::runtime_error("Error creating Mock Server data file");

  return name;
}

std::string Server_mock::map_column_type(mysqlshdk::db::Type type) {
  switch (type) {
    case mysqlshdk::db::Type::Null:
      return "null";
    case mysqlshdk::db::Type::Date:
    case mysqlshdk::db::Type::Time:
    case mysqlshdk::db::Type::String:
    case mysqlshdk::db::Type::Bytes:
    case mysqlshdk::db::Type::Geometry:
    case mysqlshdk::db::Type::Json:
    case mysqlshdk::db::Type::DateTime:
    case mysqlshdk::db::Type::Enum:
    case mysqlshdk::db::Type::Set:
      return "STRING";
    case mysqlshdk::db::Type::Integer:
    case mysqlshdk::db::Type::UInteger:
    case mysqlshdk::db::Type::Float:
    case mysqlshdk::db::Type::Double:
    case mysqlshdk::db::Type::Bit:
      return "LONGLONG";
    case mysqlshdk::db::Type::Decimal:
      return "LONG";
  }

  throw std::runtime_error("Invalid column type found");

  return "";
}

std::string Server_mock::get_path_to_binary() {
  std::string command;

  std::string prefix = shcore::get_binary_folder();

#ifdef _WIN32
  command = prefix + "\\" + "mysql_server_mock.exe";
#else
  command = prefix + "/" + "mysql_server_mock";
#endif

  return command;
}

void Server_mock::start(int port,
                        const std::vector<testing::Fake_result_data> &data) {
  std::string binary_path = get_path_to_binary();
  std::string data_path = create_data_file(data);
  std::string strport = std::to_string(port);
  int server_status = -1;
  bool started = false;

  std::vector<const char *> args = {binary_path.c_str(), data_path.c_str(),
                                    strport.c_str(), NULL};

  _thread = std::shared_ptr<std::thread>(
      new std::thread([this, args, &server_status, &started]() {
        try {
          _process.reset(new shcore::Process_launcher(&args[0]));
          _process->start();

          char c;
          while (_process->read(&c, 1) > 0) {
            _server_output += c;
            if (_server_output.find("Starting to handle connections") !=
                std::string::npos) {
              if (server_status < 0) {
                started = true;
                {
                  std::unique_lock<std::mutex> lock(_mutex);
                  server_status = 0;
                }
                _cond.notify_one();
              }
            }
          }

          int exit_code = _process->wait();
          if (server_status < 0) {
            {
              std::unique_lock<std::mutex> lock(_mutex);
              server_status = exit_code;
            }
            _cond.notify_one();
          }
        } catch (const std::exception &e) {
          std::cerr << e.what() << std::endl;
        }
      }));

  {
    std::unique_lock<std::mutex> lock(_mutex);
    _cond.wait(lock, [&server_status](){ return server_status != -1; });
  }

  // Deletes the temporary data file
  shcore::delete_file(data_path);

  if (!started)
    throw std::runtime_error(
        _server_output +
        (server_status > 0 ? "(exit code " + std::to_string(server_status) + ")"
                           : 0));
}

void Server_mock::stop() {
  _thread->join();
}

}  // namespace tests
