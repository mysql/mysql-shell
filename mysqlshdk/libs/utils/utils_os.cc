/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_os.h"

#ifdef _WIN32
#include <Windows.h>
#include <processthreadsapi.h>
#include "utils/utils_string.h"
#elif __APPLE__
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/mach_types.h>
#include <mach/vm_statistics.h>
#include <pthread.h>
#else
#include <pthread.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#endif

namespace shcore {

uint64_t available_memory() {
#ifdef _WIN32
  MEMORYSTATUSEX statex;
  statex.dwLength = sizeof(statex);

  if (GlobalMemoryStatusEx(&statex)) {
    return statex.ullAvailPhys;
  }
#elif __APPLE__
  const auto mach_port = mach_host_self();
  vm_size_t page_size = 0;
  vm_statistics64_data_t vm_stats;
  mach_msg_type_number_t count = sizeof(vm_stats) / sizeof(natural_t);

  if (KERN_SUCCESS == host_page_size(mach_port, &page_size) &&
      KERN_SUCCESS == host_statistics64(mach_port, HOST_VM_INFO,
                                        (host_info64_t)&vm_stats, &count)) {
    return static_cast<uint64_t>(vm_stats.free_count) * page_size;
  }
#else
  struct sysinfo info;

  if (0 == sysinfo(&info)) {
    return static_cast<uint64_t>(info.freeram) * info.mem_unit;
  }
#endif

  return 0;
}

void set_current_thread_name(const char *name) {
#if defined _WIN32
  SetThreadDescription(GetCurrentThread(), utf8_to_wide(name).c_str());
#elif defined __APPLE__
  pthread_setname_np(name);
#else
  pthread_setname_np(pthread_self(), name);
#endif
}

}  // namespace shcore
