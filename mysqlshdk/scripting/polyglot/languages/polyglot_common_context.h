/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_SCRIPTING_POLYGLOT_LANGUAGES_POLYGLOT_COMMON_CONTEXT_H_
#define MYSQLSHDK_SCRIPTING_POLYGLOT_LANGUAGES_POLYGLOT_COMMON_CONTEXT_H_

#include "mysqlshdk/scripting/polyglot/utils/polyglot_api_clean.h"

#include <memory>

#include "mysqlshdk/scripting/polyglot/native_wrappers/polyglot_collectable.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_scope.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_store.h"

namespace shcore {
namespace polyglot {

/**
 * Common context for GraalVM Languages
 *
 * Encloses the following global routines/components:
 *
 * - Initialization/TearDown of the GraalVM Isolate.
 * - Registration of global logging callbacks.
 * - Setup of the global engine.
 * - Provides access to the garbage collector for event registration.
 *
 * This class can be specialized to:
 *
 * - Provide custom callbacks for the global logging functions.
 * - Create a custom Engine to be used in the contexts created from this class.
 * - Provide custom configuration for the garbage collector.
 */
class Polyglot_common_context {
 public:
  Polyglot_common_context() = default;
  virtual ~Polyglot_common_context() = default;

  virtual void initialize(const std::vector<std::string> &isolate_args);
  virtual void finalize();

  poly_reference engine() const { return m_engine.get(); }
  poly_isolate isolate() const { return m_isolate; }
  poly_thread thread() const { return m_thread; }

  void clean_collectables();

  Collectable_registry *collectable_registry() { return &m_registry; }

 protected:
  poly_isolate m_isolate = nullptr;
  poly_thread m_thread = nullptr;

 private:
  void init_engine();
  static void fatal_error_callback(void *data);
  static void flush_callback(void *data);
  static void log_callback(const char *bytes, size_t length, void *data);
  virtual void fatal_error() = 0;
  virtual void flush() = 0;
  virtual void log(const char *bytes, size_t length) = 0;
  virtual poly_engine create_engine() { return nullptr; }

  Store m_engine;
  std::unique_ptr<Polyglot_scope> m_scope;
  Collectable_registry m_registry;
};

}  // namespace polyglot
}  // namespace shcore

#endif  // MYSQLSHDK_SCRIPTING_POLYGLOT_LANGUAGES_POLYGLOT_COMMON_CONTEXT_H_