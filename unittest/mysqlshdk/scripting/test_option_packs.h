/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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
#ifndef UNITTEST_MYSQLSHDK_SCRIPTING_TEST_OPTION_PACKS_H_
#define UNITTEST_MYSQLSHDK_SCRIPTING_TEST_OPTION_PACKS_H_

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/scripting/types_cpp.h"

namespace tests {

/**
 * Options to be included, they only require the static options() function
 */
struct Aggregate_options {
  static const shcore::Option_pack_def<Aggregate_options> &options() {
    static shcore::Option_pack_def<Aggregate_options> opts;

    if (opts.empty()) {
      opts.optional("myAString", &Aggregate_options::str)
          .optional("myAInt", &Aggregate_options::integer)
          .optional("myABool", &Aggregate_options::boolean)
          .optional("myAList", &Aggregate_options::list);
    }

    return opts;
  }

  std::string str;
  std::vector<std::string> list;
  int integer = 0;
  bool boolean = false;
};
struct Parent_option_pack {
  static const shcore::Option_pack_def<Parent_option_pack> &options() {
    static shcore::Option_pack_def<Parent_option_pack> opts;

    if (opts.empty()) {
      opts.optional("myPString", &Parent_option_pack::parent_str, "s")
          .optional("myPInt", &Parent_option_pack::parent_integer, "i")
          .optional("myPIndirect", &Parent_option_pack::set_string_option)
          .optional("myPBool", &Parent_option_pack::parent_boolean, "b")
          .optional("myPList", &Parent_option_pack::parent_list, "l");
    }

    return opts;
  }

  std::string parent_str;
  std::string parent_indirect_str;
  std::vector<std::string> parent_list;
  Aggregate_options parent_other_opts;
  int parent_integer = 0;
  bool parent_boolean = false;

 private:
  void set_string_option(const std::string &option, const std::string &data) {
    if (option == "myPIndirect") {
      parent_indirect_str = "Modified Parent " + data;
    }
  }
};

struct Templated_options {
  std::string str;
  std::string indirect_str;
  std::vector<std::string> list;
  int integer = 0;
  bool boolean = false;
  bool done_called = false;
};

enum class Templated_options_type { One, Two };

template <Templated_options_type T>
struct Templated_option_unpacker {
  Templated_option_unpacker(Templated_options &target) : m_target(target) {}
  static const shcore::Option_pack_def<Templated_option_unpacker> &options() {
    static shcore::Option_pack_def<Templated_option_unpacker> opts;
    if (opts.empty()) {
      if (T == Templated_options_type::One) {
        opts.optional("myString",
                      &Templated_option_unpacker<T>::set_string_option, "s")
            .optional("myInt", &Templated_option_unpacker<T>::set_int_option,
                      "i")
            .optional("myIndirect",
                      &Templated_option_unpacker<T>::set_string_option);
      }

      if (T == Templated_options_type::Two) {
        opts.optional("myBool", &Templated_option_unpacker<T>::set_bool_option,
                      "b")
            .optional("myList", &Templated_option_unpacker<T>::set_array_option,
                      "l")
            .on_done(&Templated_option_unpacker<T>::done_callback);
      }
    }

    return opts;
  }

  void set_string_option(const std::string &option, const std::string &value) {
    if (option == "myIndirect") {
      m_target.indirect_str = "Modified " + value;
    } else if (option == "myString") {
      m_target.str = value;
    }
  }
  void set_int_option(const std::string & /*option*/, const int &value) {
    m_target.integer = value;
  }
  void set_bool_option(const std::string & /*option*/, const bool &value) {
    m_target.boolean = value;
  }

  void set_array_option(const std::string & /*option*/,
                        const std::vector<std::string> &value) {
    m_target.list = value;
  }

  void done_callback() { m_target.done_called = true; }

 private:
  Templated_options &m_target;
};

template <Templated_options_type T>
struct Templated_options_pack : public Templated_options {
  static const shcore::Option_pack_def<Templated_options_pack> &options() {
    static shcore::Option_pack_def<Templated_options_pack> opts;

    if (opts.empty()) {
      if (T == Templated_options_type::One) {
        opts.optional("myString", &Templated_options_pack<T>::set_string_option,
                      "s")
            .optional("myInt", &Templated_options_pack<T>::set_int_option, "i")
            .optional("myIndirect",
                      &Templated_options_pack<T>::set_string_option);
      }

      if (T == Templated_options_type::Two) {
        opts.optional("myBool", &Templated_options_pack<T>::set_bool_option,
                      "b")
            .optional("myList", &Templated_options_pack<T>::set_array_option,
                      "l")
            .on_done(&Templated_options_pack<T>::done_callback);
      }
    }

    return opts;
  }

  void set_string_option(const std::string &option, const std::string &value) {
    if (option == "myIndirect") {
      indirect_str = "Modified " + value;
    } else if (option == "myString") {
      str = value;
    }
  }
  void set_int_option(const std::string & /*option*/, const int &value) {
    integer = value;
  }
  void set_bool_option(const std::string & /*option*/, const bool &value) {
    boolean = value;
  }

  void set_array_option(const std::string & /*option*/,
                        const std::vector<std::string> &value) {
    list = value;
  }

  void done_callback() { done_called = true; }
};

/**
 * Main option pack, requires the static options() function and the unpack()
 * function.
 */
struct Sample_options : public Parent_option_pack {
  static const shcore::Option_pack_def<Sample_options> &options() {
    static shcore::Option_pack_def<Sample_options> opts;

    if (opts.empty()) {
      opts.optional("myString", &Sample_options::str, "s")
          .optional("myInt", &Sample_options::integer, "i")
          .optional("myIndirect", &Sample_options::set_string_option)
          .optional("myBool", &Sample_options::boolean, "b")
          .optional("myList", &Sample_options::list, "l")
          .optional("myDict", &Sample_options::dictionary, "l")
          .include(&Sample_options::other_opts)
          .include<Parent_option_pack>()
          .on_done(&Sample_options::done_callback);
    }

    return opts;
  }

  void done_callback() { done_called = true; }

  std::string str;
  std::string indirect_str;
  std::vector<std::string> list;
  shcore::Dictionary_t dictionary;
  Aggregate_options other_opts;
  int integer = 0;
  bool boolean = false;
  bool done_called = false;
  Templated_options no_option_pack;

 private:
  void set_string_option(const std::string &option, const std::string &data) {
    if (option == "myIndirect") {
      indirect_str = "Modified " + data;
    }
  }
};

}  // namespace tests

#endif  // UNITTEST_MYSQLSHDK_SCRIPTING_TEST_OPTION_PACKS_H_