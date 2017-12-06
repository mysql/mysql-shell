/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlsh/prompt_renderer.h"

#include <cassert>

#include <algorithm>
#include <locale>
#include <set>
#ifdef WIN32
#include <Windows.h>
#undef max
#endif

#include "mysqlshdk/libs/textui/term_vt100.h"

#include "ext/linenoise-ng/src/ConvertUTF.h"

// Use stuff from linenoise
#ifdef _WIN32
inline std::wstring from_utf8(const std::string &s) {
  std::wstring ws;
  const linenoise_ng::UTF8 *start =
    reinterpret_cast<const linenoise_ng::UTF8 *>(s.data());

  ws.resize(s.size());
  linenoise_ng::UTF16 *target_start =
    reinterpret_cast<linenoise_ng::UTF16 *>(&ws[0]);
  linenoise_ng::UTF16 *new_target_start = target_start;
  if (linenoise_ng::ConvertUTF8toUTF16(
    &start, start + s.size(), &new_target_start,
    new_target_start + ws.size(),
    linenoise_ng::lenientConversion) != linenoise_ng::conversionOK) {
    throw std::logic_error("Could not convert from utf8 string");
  }
  ws.resize(new_target_start - target_start);
  return ws;
}
#else
inline std::wstring from_utf8(const std::string &s) {
  std::wstring ws;
  const linenoise_ng::UTF8 *start =
      reinterpret_cast<const linenoise_ng::UTF8 *>(s.data());

  ws.resize(s.size());
  linenoise_ng::UTF32 *target_start =
      reinterpret_cast<linenoise_ng::UTF32 *>(&ws[0]);
  linenoise_ng::UTF32 *new_target_start = target_start;
  if (linenoise_ng::ConvertUTF8toUTF32(
          &start, start + s.size(), &new_target_start,
          new_target_start + ws.size(),
          linenoise_ng::lenientConversion) != linenoise_ng::conversionOK) {
    throw std::logic_error("Could not convert from utf8 string");
  }
  ws.resize(new_target_start - target_start);
  return ws;
}
#endif

inline size_t utf8_length(const std::string &s) {
  return from_utf8(s).length();
}

#ifdef _WIN32
inline std::string to_utf8(const std::wstring &s) {
  std::string ns;
  const linenoise_ng::UTF16 *start =
    reinterpret_cast<const linenoise_ng::UTF16 *>(s.data());

  ns.resize(s.size() * 4);
  linenoise_ng::UTF8 *target_start =
    reinterpret_cast<linenoise_ng::UTF8 *>(&ns[0]);
  linenoise_ng::UTF8 *new_target_start = target_start;

  if (linenoise_ng::ConvertUTF16toUTF8(
    &start, start + s.length(), &new_target_start,
    new_target_start + ns.size(),
    linenoise_ng::lenientConversion) != linenoise_ng::conversionOK) {
    throw std::logic_error("Could not convert to utf8 string");
  }
  ns.resize(new_target_start - target_start);
  return ns;
}
#else
inline std::string to_utf8(const std::wstring &s) {
  std::string ns;
  const linenoise_ng::UTF32 *start =
      reinterpret_cast<const linenoise_ng::UTF32 *>(s.data());

  ns.resize(s.size() * 4);
  linenoise_ng::UTF8 *target_start =
      reinterpret_cast<linenoise_ng::UTF8 *>(&ns[0]);
  linenoise_ng::UTF8 *new_target_start = target_start;

  if (linenoise_ng::ConvertUTF32toUTF8(
          &start, start + s.length(), &new_target_start,
          new_target_start + ns.size(),
          linenoise_ng::lenientConversion) != linenoise_ng::conversionOK) {
    throw std::logic_error("Could not convert to utf8 string");
  }
  ns.resize(new_target_start - target_start);
  return ns;
}
#endif

namespace mysqlsh {

const char* Prompt_renderer::k_symbol_sep_right_pl = u8"\ue0b0";
const char* Prompt_renderer::k_symbol_sep_right_hollow_pl = u8"\ue0b1";
const char* Prompt_renderer::k_symbol_ellipsis_pl = u8"\u2026";

class Prompt_segment {
 public:
  typedef Prompt_renderer::Shrinker_type Shrinker_type;

  Prompt_segment(const std::string &text, const mysqlshdk::textui::Style &style,
                 int prio, int min_width, int padding, Shrinker_type stype,
                 const std::string *separator, const std::string &ellipsis)
      : full_text_(text),
        style_(style),
        priority_(prio),
        min_width_(min_width),
        padding_(padding),
        shrinker_type_(stype),
        ellipsis_(ellipsis) {
    text_ = text;
    hidden_ = false;
    if (separator)
      separator_.reset(new std::string(*separator));
  }

  int effective_width() const { return hidden_ ? 0 : shrunk_width_; }

  int padding() const { return padding_; }

  const mysqlshdk::textui::Style &style() const { return style_; }

  /** Try to shrink contents by the given amount. Returns how much got shrunk */
  int shrink_by(int length) {
    int old_width = shrunk_width_;

    switch (shrinker_type_) {
      case Shrinker_type::No_shrink:
        return 0;

      case Shrinker_type::Ellipsize_on_char:
        // blablabla -> blabl...
        shrunk_width_ = std::max(shrunk_width_ - length, min_width_);
        if (shrunk_width_ < old_width) {
          std::wstring a(from_utf8(text_));
          a = a.substr(0, shrunk_width_ - 1).append(from_utf8(ellipsis_));
          text_ = to_utf8(a);
          return old_width - shrunk_width_;
        }
        return 0;

      case Shrinker_type::Truncate_on_dot_from_right: {
        // foo.bar.com -> foo
        std::wstring a(from_utf8(text_));
        size_t until = a.length();
        size_t pos = a.find_last_of('.', until);
        size_t prev = pos;
        size_t non_negative_length = ((length < 0) ? 0 : length);
        while (pos != std::string::npos && (a.length() - pos) < non_negative_length) {
          until = pos - 1;
          prev = pos;
          pos = a.find_last_of('.', until);
        }
        a = a.substr(0, pos == std::string::npos ? prev : pos);
        text_ = to_utf8(a);
        shrunk_width_ = a.length();
        return old_width - shrunk_width_;
      }
    }
    return 0;
  }

  void hide() { hidden_ = true; }

  bool hidden() const { return hidden_; }

  int priority() const { return priority_; }

  void update(int *out_min_width, int *out_width) {
    text_.clear();
    if (!full_text_.empty())
      text_.append(padding_, ' ').append(full_text_).append(padding_, ' ');
    *out_min_width = min_width_;
    *out_width = utf8_length(text_);
    shrunk_width_ = *out_width;
    hidden_ = *out_width == 0 ? true : false;
  }

  std::string *separator() const { return separator_.get(); }

  void set_text(const std::string &s) { full_text_ = s; }

  const std::string &get_text() const { return text_; }

 protected:
  std::string full_text_;
  mysqlshdk::textui::Style style_;
  int priority_;
  int min_width_;
  int padding_;
  Shrinker_type shrinker_type_;
  std::string ellipsis_;
  std::unique_ptr<std::string> separator_;

  std::string text_;
  int shrunk_width_ = 0;
  bool hidden_ = false;
};

Prompt_renderer::Prompt_renderer(int min_empty_space)
    : symbols_((size_t)Symbol::Last), min_empty_space_(min_empty_space) {
  if (!mysqlshdk::vt100::get_screen_size(&height_, &width_)) {
    // if we can't get the console size, stdout is probably not a tty,
    // so we set prompt to virtually unlimited size
    width_ = 1000;
  }
  symbols_[Symbol::Ellipsis] = "-";
  sep_ = " ";
  sep_alt_ = " ";
}

Prompt_renderer::~Prompt_renderer() {
  clear();
}

void Prompt_renderer::clear() {
  for (auto x : segments_) {
    delete x;
  }
  segments_.clear();
}

void Prompt_renderer::set_separator(const std::string &sep,
                                    const std::string &sep_alt) {
  sep_ = sep;
  sep_alt_ = sep_alt;
}

void Prompt_renderer::set_prompt(const std::string &text,
                                 const std::string &cont_text,
                                 const mysqlshdk::textui::Style &style) {
  prompt_.reset(new Prompt_segment(text, style, -1000, utf8_length(text), 0,
                                   Shrinker_type::No_shrink, nullptr, ""));
  cont_text_ = cont_text;
}

void Prompt_renderer::add_segment(const std::string &text,
                                  const mysqlshdk::textui::Style &style,
                                  int prio, int min_width, int padding,
                                  Shrinker_type stype,
                                  const std::string *separator) {
  segments_.push_back(new Prompt_segment(text, style, prio, min_width, padding,
                                         stype, separator,
                                         symbols_[Symbol::Ellipsis]));
}

void Prompt_renderer::shrink_to_fit() {
  std::set<Prompt_segment*> hidden_segments;
  int usable_space = width_ - min_empty_space_;

  for (;;) {
    int total_width = 0;
    std::vector<Prompt_segment *> ordered_segments;

    for (auto seg : segments_) {
      if (hidden_segments.find(seg) == hidden_segments.end()) {
        int min_seg_width = 0;
        int seg_width = 0;
        seg->update(&min_seg_width, &seg_width);
        total_width += seg_width + 1;
        ordered_segments.push_back(seg);
      } else {
        seg->hide();
      }
    }
    if (total_width > usable_space) {
      std::sort(ordered_segments.begin(), ordered_segments.end(),
                [](const Prompt_segment *a, const Prompt_segment *b) -> bool {
                  return a->priority() < b->priority();
                });
      for (auto seg : ordered_segments) {
        if (!seg->hidden()) {
          int shrinked = seg->shrink_by(total_width - usable_space);
          if (shrinked > 0) {
            total_width -= shrinked;
          }
          if (total_width <= usable_space)
            break;
        }
      }
      if (total_width > usable_space) {
        bool ok = false;
        // still not enough space. hide the lowest weight segment and retry
        for (auto seg : ordered_segments) {
          if (!seg->hidden()) {
            hidden_segments.insert(seg);
            ok = true;
            break;
          }
        }
        // nothing else left to shrink
        if (!ok)
          break;
      } else {
        break;
      }
    } else {
      break;
    }
  }
}

std::string Prompt_renderer::render_prompt_line() {
  std::string line;
  bool needs_reset = false;
  Prompt_segment *prev = nullptr;

  if (continued_prompt_) {
    if (prompt_) {
      int padding = last_prompt_width_ - utf8_length(cont_text_) +
                    utf8_length(prompt_->get_text());
      std::string prompt_str(padding < 0 ? 0 : padding, ' ');
      prompt_str.append(cont_text_);
      line.append(cont_style_).append(prompt_str);
      if (cont_style_)
        needs_reset = true;
    }
  } else {
    shrink_to_fit();

    last_segment_ = nullptr;
    last_prompt_width_ = 0;
    for (auto seg : segments_) {
      if (!seg->hidden()) {
        std::string seg_text = seg->get_text();
        if (prev != nullptr && !prev->get_text().empty()) {
          std::string sep;
          mysqlshdk::textui::Style style = seg->style();

          // if we're using the triangle separator, do the special handling
          // where the foreground of the separator is the background of the
          // previous segment

          if (seg->separator()) {
            sep = *seg->separator();
          } else {
            // if the bg color is the same, use the alt version
            if (prev->style().compare(
                    style, mysqlshdk::textui::Style::Color_bg_mask)) {
              sep = sep_alt_;
              if (sep == k_symbol_sep_right_hollow_pl) {
                style.field_mask |= (prev->style().field_mask &
                                     mysqlshdk::textui::Style::Color_fg_mask);
                style.fg = prev->style().fg;
              }
            } else {
              sep = sep_;
              if (sep == k_symbol_sep_right_pl) {
                style.field_mask |= (prev->style().field_mask &
                                     mysqlshdk::textui::Style::Color_bg_mask) >>
                                    3;
                style.fg = prev->style().bg;
              }
            }
          }
          if (!sep.empty() && sep[0] != ' ' && style) {
            line.append(style);
          }
          line.append(sep);

          last_prompt_width_ += utf8_length(sep);
        }
        // reset attributes after each segment
        if (needs_reset) {
          line.append(mysqlshdk::textui::Style::clear());
          needs_reset = false;
        }
        if (seg->style())
          needs_reset = true;
        line.append(seg->style()).append(seg_text);
        last_segment_ = seg;
        last_prompt_width_ += utf8_length(seg_text);
        prev = seg;
      }
    }
    if (needs_reset) {
      line.append(mysqlshdk::textui::Style::clear());
      needs_reset = false;
    }
    if (prompt_) {
      std::string prompt = prompt_->get_text();
      mysqlshdk::textui::Style style = prompt_->style();

      // if the prompt is the triangle and fg color is not overriden, do special
      // handling
      if ((style.field_mask & mysqlshdk::textui::Style::Color_fg_mask) == 0 &&
          (to_utf8(from_utf8(prompt).substr(0, 1)) == k_symbol_sep_right_pl) &&
          prev != nullptr) {
        style.field_mask |= (prev->style().field_mask &
                             mysqlshdk::textui::Style::Color_bg_mask) >>
                            3;
        style.fg = prev->style().bg;
      }
      cont_style_ = style;
      line.append(style).append(prompt);

      if (style)
        needs_reset = true;
    }
  }
  if (needs_reset)
    line.append(mysqlshdk::textui::Style::clear());
  return line;
}

std::string Prompt_renderer::render() {
  if (!width_override_) {
    // update the width if possible (won't do it if stdout is not a tty)
    // we update every time b/c the window could have been resized
    // don't need to overcomplicate with SIGWINCH
    mysqlshdk::vt100::get_screen_size(&height_, &width_);
  }

  return render_prompt_line();
}

void Prompt_renderer::set_width(int width) {
  width_ = width;
  width_override_ = true;
}

void Prompt_renderer::set_is_continuing(bool flag) {
  continued_prompt_ = flag;
}

}  // namespace mysqlsh
