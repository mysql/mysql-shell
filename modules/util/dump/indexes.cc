/*
 * Copyright (c) 2023, Oracle and/or its affiliates.
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

#include "modules/util/dump/indexes.h"

#include <iterator>
#include <vector>

namespace mysqlsh {
namespace dump {

std::pair<const Instance_cache::Index *, bool> select_index(
    const Instance_cache::Table &table) {
  using Indexes = std::vector<const Instance_cache::Index *>;

  const auto filter_indexes = [](const Indexes &indexes) {
    Indexes filtered;
    filtered.reserve(indexes.size());

    for (const auto index : indexes) {
      bool unsafe = false;

      for (const auto column : index->columns()) {
        // BUG#35180061 - do not use indexes on ENUM columns
        unsafe |= (mysqlshdk::db::Type::Enum == column->type);
        break;
      }

      if (!unsafe) {
        filtered.emplace_back(index);
      }
    }

    return filtered;
  };

  if (table.primary_key && !filter_indexes({table.primary_key}).empty()) {
    // use primary key
    return {table.primary_key, true};
  }

  const auto choose_index = [](const Indexes &indexes) {
    if (1 == indexes.size()) {
      return indexes.front();
    }

    const auto mark_integer_columns = [](Indexes::const_iterator idx) {
      std::vector<bool> result;

      result.reserve((*idx)->columns().size());

      for (const auto c : (*idx)->columns()) {
        result.emplace_back(c->type == mysqlshdk::db::Type::Integer ||
                            c->type == mysqlshdk::db::Type::UInteger);
      }

      return result;
    };

    Indexes::const_iterator chosen_index;
    std::vector<bool> chosen_columns;

    const auto use_given_index = [&](Indexes::const_iterator idx,
                                     std::vector<bool> &&cols) {
      chosen_index = idx;
      chosen_columns = std::move(cols);
    };

    const auto use_index = [&](Indexes::const_iterator idx) {
      use_given_index(idx, mark_integer_columns(idx));
    };

    use_index(indexes.begin());

    // skip first index
    for (auto it = std::next(indexes.begin()); it != indexes.end(); ++it) {
      const auto size = (*it)->columns().size();

      if (size < (*chosen_index)->columns().size()) {
        // prefer shorter index
        use_index(it);
      } else if (size == (*chosen_index)->columns().size()) {
        // if indexes have equal length, prefer the one with longer run of
        // integer columns
        auto candidate = mark_integer_columns(it);
        auto candidate_column = (*it)->columns().begin();
        auto current_column = (*chosen_index)->columns().begin();

        for (std::size_t i = 0; i < size; ++i) {
          if (candidate[i] == chosen_columns[i]) {
            // both columns are of the same type, check if they are nullable
            if ((*candidate_column)->nullable == (*current_column)->nullable) {
              // both columns are NULL or NOT NULL
              if (!chosen_columns[i]) {
                // both columns are not integers, use the current index
                break;
              }
              // else, both column are integers, check next column
            } else {
              // prefer NOT NULL column
              if (!(*candidate_column)->nullable) {
                // candidate is NOT NULL, use that
                use_given_index(it, std::move(candidate));
              }
              // else current column is NOT NULL
              break;
            }
          } else {
            // columns are different
            if (candidate[i]) {
              // candidate column is an integer, use candidate index
              use_given_index(it, std::move(candidate));
            }
            // else, current column is an integer, use the current index
            break;
          }

          ++candidate_column;
          ++current_column;
        }
      }
      // else, candidate is longer, ignore it
    }

    return *chosen_index;
  };

  if (const auto filtered = filter_indexes(table.primary_key_equivalents);
      !filtered.empty()) {
    // use PKE
    return {choose_index(filtered), true};
  }

  if (const auto filtered = filter_indexes(table.unique_keys);
      !filtered.empty()) {
    // use unique key with nullable columns
    return {choose_index(filtered), false};
  }

  return {nullptr, false};
}

}  // namespace dump
}  // namespace mysqlsh
