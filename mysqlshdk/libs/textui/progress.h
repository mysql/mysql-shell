/*
 * Copyright (c) 2018, 2021, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_TEXTUI_PROGRESS_H__
#define MYSQLSHDK_LIBS_TEXTUI_PROGRESS_H__

#include <memory>
#include <string>
#include <vector>

#include "mysqlshdk/libs/textui/textui.h"

namespace mysqlshdk {
namespace textui {

class Progress {
 public:
  struct Item {
    std::string label;
    std::string rlabel;
    int64_t total = 0;
    int64_t current = 0;
  };

  virtual ~Progress() {}

  virtual bool start() = 0;
  virtual void end() = 0;
  virtual void update() = 0;

  virtual void set_label(const std::string &label) = 0;
  virtual void set_total(int64_t total) = 0;
  virtual void set_current(int64_t current) = 0;
  virtual void set_right_label(const std::string &text) = 0;

  virtual void set_secondary_label(int i, const std::string &label) = 0;
  virtual void set_secondary_total(int i, int64_t total) = 0;
  virtual void set_secondary_current(int i, int64_t current) = 0;
  virtual void set_secondary_right_label(int i, const std::string &label) = 0;
};

class Progress_vt100 : public Progress {
 public:
  explicit Progress_vt100(int num_secondary_bars);

  void set_style(const Style &text_style, const Style &fg_style,
                 const Style &bg_style, const std::string &fg,
                 const std::string &bg);

  void set_secondary_style(const Style &text_style, const Style &fg_style,
                           const Style &bg_style);

  bool start() override;
  void end() override;
  void update() override;

  void set_label(const std::string &label) override;
  void set_total(int64_t total) override;
  void set_current(int64_t current) override;
  void set_right_label(const std::string &label) override;

  void set_secondary_label(int i, const std::string &label) override;
  void set_secondary_total(int i, int64_t total) override;
  void set_secondary_current(int i, int64_t current) override;
  void set_secondary_right_label(int i, const std::string &label) override;

 private:
  Item m_bar;
  int m_screen_width;
  int m_top_row;
  std::vector<Item> m_secondary_bars;

  Style m_text_style;
  Style m_fg_style;
  Style m_bg_style;

  Style m_text2_style;
  Style m_fg2_style;
  Style m_bg2_style;

  std::string m_fg = "#";
  std::string m_bg = "=";

  void draw_bar(const Item &item, int indent, const Style &text_style,
                const Style &bg_style, const Style &fg_style);
};

class Spinny_stick {
 public:
  explicit Spinny_stick(const std::string &label, bool use_json = false)
      : m_label(label), m_use_json(use_json) {}
  virtual ~Spinny_stick() = default;

  void set_right_label(const std::string &label);
  void update();
  void done(const std::string &text);

 private:
  void print(const std::string &text = {});

  std::string m_label;
  std::string m_right_label;
  int m_step = 0;
  bool m_use_json = false;
  bool m_needs_refresh = true;
};

class Threaded_spinny_stick : Spinny_stick {
 public:
  explicit Threaded_spinny_stick(const std::string &label,
                                 const std::string &done = "",
                                 uint32_t update_every_ms = 250);

  Threaded_spinny_stick(const Threaded_spinny_stick &) = delete;
  Threaded_spinny_stick(Threaded_spinny_stick &&) = delete;

  Threaded_spinny_stick &operator=(const Threaded_spinny_stick &) = delete;
  Threaded_spinny_stick &operator=(Threaded_spinny_stick &&) = delete;

  ~Threaded_spinny_stick() override;

  void start();

 private:
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

}  // namespace textui
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_TEXTUI_PROGRESS_H__
