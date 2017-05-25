/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _CMDLINE_OPTIONS_H_
#define _CMDLINE_OPTIONS_H_

#include <stdlib.h>
#include <iostream>
#include <cstring>
#include "scripting/common.h"

class Command_line_options
{
public:
  int exit_code;
protected:
  Command_line_options(int UNUSED(argc), const char **UNUSED(argv))
    : exit_code(0)
  {
  }

  bool check_arg(const char **argv, int &argi, const char *arg, const char *larg)
  {
    if ((arg && strcmp(argv[argi], arg) == 0) || (larg && strcmp(argv[argi], larg) == 0))
      return true;
    return false;
  }

  // Will verify if the argument is the one specified by arg or larg and return its associated value
  // The function has different behaviors:
  //
  // Options come in three flavors
  // 1) --option [value] or -o [value]
  // 2) --option or -o
  // 3) --option=[value]
  //
  // They behavie differently:
  // --option <value> can take the default argumemt (def) if def has data and value is missing, error if missing and def is not defined
  // --option=[value] will get NULL value if value is missing and can accept_null(i.e. --option=)
  //
  // ReturnValue: Returns the # of format found based on the list above, or 0 if no valid value was found
  int check_arg_with_value(const char **argv, int &argi, const char *arg, const char *larg, const char *&value, bool accept_null = false)
  {
    int ret_val = 0;

    value = NULL;

    // --option [value] or -o [value]
    if (strcmp(argv[argi], arg) == 0 || (larg && strcmp(argv[argi], larg) == 0))
    {
      ret_val = 1;

      // value can be in next arg and can't start with - which indicates next option
      if (argv[argi + 1] != NULL && strncmp(argv[argi + 1], "-", 1) != 0)
      {
        ++argi;
        value = argv[argi];
      }
    }
    // -o<value>
    else if (larg && strncmp(argv[argi], larg, strlen(larg)) == 0 && strlen(argv[argi]) > strlen(larg))
    {
      ret_val = 2;
      value = argv[argi] + strlen(larg);
    }

    // --option=[value]
    else if (strncmp(argv[argi], arg, strlen(arg)) == 0 && argv[argi][strlen(arg)] == '=')
    {
      ret_val = 3;

      // Value was specified
      if (strlen(argv[argi]) > (strlen(arg) + 1))
        value = argv[argi] + strlen(arg) + 1;
    }

    if (ret_val && !value && !accept_null)
    {
      std::cerr << argv[0] << ": option " << argv[argi] << " requires an argument\n";
      exit_code = 1;
      ret_val = 0;
    }

    return ret_val;
  }
};

#endif
