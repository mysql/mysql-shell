/*
  Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <iostream>
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <winsock2.h>
#endif

#include "mysql_server_mock.h"

const unsigned DEFAULT_MOCK_SERVER_PORT = 3306;

using namespace server_mock;

void print_usage(const char* name) {
  std::cout << "Usage: \n";
  std::cout << name << " <expected_json_file_name> [port]\n";
  exit(-1);
}

int main(int argc, char* argv[]) {
  std::string queries_filename;
  unsigned port = DEFAULT_MOCK_SERVER_PORT;

#ifdef _WIN32
  WSADATA wsaData;
  int result;
  result = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (result != 0) {
    std::cerr << "WSAStartup failed with error: " << result << std::endl;
    return -1;
  }
#endif

  if (argc < 2 || argc > 3) {
    print_usage(argv[0]);
  }

  queries_filename = argv[1];
  if (argc > 2) {
    try {
      port =  static_cast<unsigned>(std::stoul(std::string(argv[2])));
    }
    catch (...) {
      print_usage(argv[0]);
    }
  }

  try {
    MySQLServerMock mock(queries_filename, port);
    std::cout << "Starting MySQLServerMock" << std::endl;
    mock.run();
    std::cout << "MySQLServerMock::run() exited" << std::endl;
  }
  catch (const std::exception& e) {
    std::cout << "MySQLServerMock ERROR: " << e.what() << std::endl;
    return -1;
  }

  return 0;
}
