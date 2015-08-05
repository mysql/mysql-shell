/*
 * Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.
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
#include "shellcore/common.h"

class Command_line_options
{
public:
  int exit_code;
  bool needs_password;
protected:
  Command_line_options(int UNUSED(argc), char **UNUSED(argv))
          : exit_code(0)
  {
  }

  bool check_arg(char **argv, int &argi, const char *arg, const char *larg)
  {
    if (strcmp(argv[argi], arg) == 0 || strcmp(argv[argi], larg) == 0)
      return true;
    return false;
  }

  bool check_arg_with_value(char **argv, int &argi, const char *arg, const char *larg, char *&value, const char* def = NULL)
  {
    // --option value or -o value
    if (strcmp(argv[argi], arg) == 0 || (larg && strcmp(argv[argi], larg) == 0))
    {
      // value must be in next arg and can't start with - which indicates next option
      if (argv[argi + 1] != NULL && strncmp(argv[argi + 1], "-", 1)!=0)
      {
        ++argi;
        value = argv[argi];
      }
      else
      {
        if (def)
          strcpy(value, def);
        else
        {
          std::cerr << argv[0] << ": option " << argv[argi] << " requires an argument\n";
          exit_code = 1;
          return false;
        }
      }
      return true;
    }
    // -ovalue
    else if (larg && strncmp(argv[argi], larg, strlen(larg)) == 0 && strlen(argv[argi]) > strlen(larg))
    {
      value = argv[argi] + strlen(larg);
      return true;
    }
    // --option=value
    else if (strncmp(argv[argi], arg, strlen(arg)) == 0 && argv[argi][strlen(arg)] == '=')
    {
      // value must be after =
      value = argv[argi] + strlen(arg)+1;
      return true;
    }
    return false;
  }
};


#endif
