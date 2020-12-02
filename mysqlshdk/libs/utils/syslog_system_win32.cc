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

#include "mysqlshdk/libs/utils/syslog_system.h"

#include <Windows.h>

#include <AccCtrl.h>
#include <AclAPI.h>

#include <cassert>
#include <memory>
#include <string>
#include <type_traits>

#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"
// this header file is compiled from res/syslog_event.mc
#include "mysqlshdk/libs/utils/syslog_event.h"

namespace shcore {
namespace syslog {

namespace {

// unique_ptr is a better fit here, but using a deleter makes writing code with
// nullptr cumbersome
using local = std::shared_ptr<std::remove_pointer_t<HLOCAL>>;

local make_local(HLOCAL ptr) { return {ptr, &LocalFree}; }

bool check_status(LSTATUS status) {
  if (ERROR_SUCCESS != status) {
    const auto console = mysqlsh::current_console();

    console->print_error(
        "Could not create or access the registry key needed for the MySQL "
        "Shell to log to the Windows EventLog.");

    if (ERROR_ACCESS_DENIED == status) {
      console->print_note(
          "Run the application with sufficient privileges once to create the "
          "key, add the key manually, or turn off logging.");
    }

    return false;
  } else {
    return true;
  }
}

local create_sid(WELL_KNOWN_SID_TYPE type) {
  DWORD size = SECURITY_MAX_SID_SIZE;
  auto sid = make_local(LocalAlloc(LMEM_FIXED, size));

  if (CreateWellKnownSid(type,       // WellKnownSidType
                         NULL,       // DomainSid
                         sid.get(),  // pSid
                         &size       // cbSid
                         )) {
    return sid;
  } else {
    return nullptr;
  }
}

local create_acl() {
  // grant full access to the created key to the authenticated users, system and
  // local administrators

  // registry key needs to be created by the administrator, but the access
  // rights we're giving to the authenticated users allow the values to be
  // modified without administrator privileges, meaning they can be updated by
  // regular users

  // system and local administrators would have full access anyway

  // based on:
  // https://docs.microsoft.com/en-us/windows/win32/secauthz/creating-a-security-descriptor-for-a-new-object-in-c--
  WELL_KNOWN_SID_TYPE well_known_sids[] = {
      WinAuthenticatedUserSid,
      WinLocalSystemSid,
      WinBuiltinAdministratorsSid,
  };
  constexpr auto sids_count = shcore::array_size(well_known_sids);

  local sids[sids_count];
  EXPLICIT_ACCESS ea[sids_count];

  ZeroMemory(&ea, sids_count * sizeof(EXPLICIT_ACCESS));

  for (std::size_t i = 0; i < sids_count; ++i) {
    sids[i] = create_sid(well_known_sids[i]);

    if (!sids[i]) {
      return nullptr;
    }

    // when trustee form is TRUSTEE_IS_SID, the ptstrName is a pointer to a SID
    ea[i].grfAccessPermissions = KEY_ALL_ACCESS;
    ea[i].grfAccessMode = SET_ACCESS;
    ea[i].grfInheritance = NO_INHERITANCE;
    ea[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[i].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[i].Trustee.ptstrName = (LPTSTR)sids[i].get();
  }

  // create ACL for the well known SIDs specified above
  PACL acl = NULL;

  if (ERROR_SUCCESS == SetEntriesInAcl(sids_count,  // cCountOfExplicitEntries
                                       ea,          // pListOfExplicitEntries
                                       NULL,        // OldAcl
                                       &acl         // NewAcl
                                       )) {
    // ACL allocated by SetEntriesInAcl() needs to be released
    return make_local(acl);
  } else {
    return nullptr;
  }
}

LSTATUS open_registry_key(const char *name, HKEY *key) {
  // create ACL
  const auto acl = create_acl();

  // initialize security descriptor and allow access to the registry key using
  // ACL, SetSecurityDescriptorDacl() does not copy the ACL, but references it
  const auto sd = make_local(LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH));

  if (!InitializeSecurityDescriptor(sd.get(),  // pSecurityDescriptor
                                    SECURITY_DESCRIPTOR_REVISION  // dwRevision
                                    ) ||
      !SetSecurityDescriptorDacl(sd.get(),         // pSecurityDescriptor
                                 TRUE,             // bDaclPresent
                                 (PACL)acl.get(),  // pDacl
                                 FALSE             // bDaclDefaulted
                                 )) {
    return ERROR_ACCESS_DENIED;
  }

  // initialize security attributes with security descriptor, key handle is not
  // inherited when a new process is created
  SECURITY_ATTRIBUTES sa;

  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = sd.get();
  sa.bInheritHandle = FALSE;

  // create key if it doesn't exist and open it
  const std::string registry_prefix =
      "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";
  const auto registry_key = registry_prefix + name;

  return RegCreateKeyExA(HKEY_LOCAL_MACHINE,       // hKey
                         registry_key.c_str(),     // lpSubKey
                         0,                        // Reserved
                         NULL,                     // lpClass
                         REG_OPTION_NON_VOLATILE,  // dwOptions
                         KEY_WRITE,                // samDesired
                         &sa,                      // lpSecurityAttributes
                         key,                      // phkResult
                         NULL                      // lpdwDisposition
  );
}

LSTATUS write_event_message_file_value(HKEY key) {
  const auto full_path = utf8_to_wide(get_binary_path());

  // size needs to be given in bytes, and has to include the terminating null
  // character
  return RegSetValueExW(
      key,                                             // hKey
      L"EventMessageFile",                             // lpValueName
      0,                                               // Reserved
      REG_EXPAND_SZ,                                   // dwType
      (const BYTE *)full_path.c_str(),                 // lpData
      (full_path.length() + 1) * sizeof(full_path[0])  // cbData
  );
}

LSTATUS write_types_supported_value(HKEY key) {
  DWORD types =
      EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;

  return RegSetValueExA(key,                   // hKey
                        "TypesSupported",      // lpValueName
                        0,                     // Reserved
                        REG_DWORD,             // dwType
                        (const BYTE *)&types,  // lpData
                        sizeof(types)          // cbData
  );
}

bool initialize_registry(const char *name) {
  assert(name);

  HKEY key = NULL;

  if (!check_status(open_registry_key(name, &key))) {
    return false;
  }

  on_leave_scope close_key{[key]() { RegCloseKey(key); }};

  if (!check_status(write_event_message_file_value(key))) {
    return false;
  }

  if (!check_status(write_types_supported_value(key))) {
    return false;
  }

  return true;
}

HANDLE g_event_log = NULL;

}  // namespace

void open(const char *name) {
  static const bool init_status = initialize_registry(name);

  // close previous handle if there's one
  close();

  // register new event source
  g_event_log = RegisterEventSource(NULL,  // lpUNCServerName
                                    name   // lpSourceName
  );

  if (!init_status && NULL != g_event_log) {
    // we failed to initialize the registry, but successfully registered the
    // event source, events are still going to be logged
    mysqlsh::current_console()->print_warning(
        "Access to the Windows EventLog could not be initialized properly, "
        "events will still be reported to the Application log, but Event "
        "Viewer will complain that description for events is missing.");
  }
}

void log(Level level, const char *msg) {
  assert(msg);

  if (NULL != g_event_log) {
    WORD type = EVENTLOG_INFORMATION_TYPE;
    DWORD event = SYSLOG_EVENT_INFO;

    switch (level) {
      case Level::ERROR:
        type = EVENTLOG_ERROR_TYPE;
        event = SYSLOG_EVENT_ERROR;
        break;

      case Level::WARN:
        type = EVENTLOG_WARNING_TYPE;
        event = SYSLOG_EVENT_WARN;
        break;

      case Level::INFO:
        type = EVENTLOG_INFORMATION_TYPE;
        event = SYSLOG_EVENT_INFO;
        break;
    }

    const auto wmsg = utf8_to_wide(msg);
    const auto wmsg_ptr = wmsg.c_str();

    ReportEventW(g_event_log,           // hEventLog
                 type,                  // wType
                 0,                     // wCategory
                 event,                 // dwEventID
                 NULL,                  // lpUserSid
                 1,                     // wNumStrings
                 0,                     // dwDataSize
                 (LPCWSTR *)&wmsg_ptr,  // lpStrings
                 NULL                   // lpRawData
    );
  }
}

void close() {
  if (NULL != g_event_log) {
    DeregisterEventSource(g_event_log);
    g_event_log = NULL;
  }
}

}  // namespace syslog
}  // namespace shcore
