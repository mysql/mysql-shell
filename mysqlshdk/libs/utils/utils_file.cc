/*
 * Copyright (c) 2015, 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/utils/utils_file.h"

#include <climits>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <random>
#include <stdexcept>
#include <type_traits>
#include "utils/utils_general.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"

#ifdef _WIN32
#include <AccCtrl.h>
#include <AclAPI.h>
#include <Lmcons.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <comdef.h>
#include <direct.h>
#include <windows.h>
#pragma comment(lib, "Shlwapi.lib")
#else
#include <dirent.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __APPLE__
#include <limits.h>
#include <mach-o/dyld.h>
#else
#ifdef __sun
#include <limits.h>
#else
#include <linux/limits.h>
#endif
#endif
#endif

namespace shcore {
namespace {
#ifdef _WIN32
std::string get_error(DWORD error_code) {
  return "SystemError: " + last_error_to_string(error_code) +
         str_format(" (code %lu)", error_code);
}
#else
std::string get_error(int error_code) {
  return errno_to_string(error_code) + str_format(" (errno %d)", error_code);
}
#endif
}  // namespace

/*
 * Returns the config path
 * (~/.mysqlsh in Unix or %AppData%\MySQL\mysqlsh in Windows).
 * May be overriden with MYSQLSH_USER_CONFIG_HOME
 * (specially for tests)
 */
std::string get_user_config_path() {
  // Check if there's an override of the config directory
  // This is needed required for unit-tests
  const char *usr_config_path = getenv("MYSQLSH_USER_CONFIG_HOME");
  if (usr_config_path) {
    return std::string(usr_config_path);
  }

  std::string path;
  std::vector<std::string> to_append;

#ifdef _WIN32
  wchar_t szPath[MAX_PATH + 1] = {};
  HRESULT hr;

  if (SUCCEEDED(hr = SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
    path = shcore::wide_to_utf8(szPath, wcslen(szPath));
  } else {
    _com_error err(hr);
    throw std::runtime_error(
        str_format("Error when gathering the APPDATA folder path: %s",
                   err.ErrorMessage()));
  }

  to_append.push_back("MySQL");
  to_append.push_back("mysqlsh");
#else
  char *cpath = std::getenv("HOME");

  if (cpath != NULL) {
    if (access(cpath, X_OK) != 0)
      throw std::runtime_error(str_format(
          "Home folder '%s' does not exist or is not accessible", cpath));
    path.assign(cpath);
  }

  to_append.push_back(".mysqlsh");
#endif

  // Up to know the path must exist since it was retrieved from OS standard
  // means we need to guarantee the rest of the path exists
  if (!path.empty()) {
    for (const auto &directory_name : to_append) {
      path += shcore::path::path_separator + directory_name;
      ensure_dir_exists(path);
    }
    path += shcore::path::path_separator;
  }

  return path;
}

/*
 * Returns the config path (/etc/mysql/mysqlsh in Unix or
 * %ProgramData%\MySQL\mysqlsh in Windows).
 */
std::string get_global_config_path() {
  std::string path;
  std::vector<std::string> to_append;

#ifdef _WIN32
  wchar_t szPath[MAX_PATH + 1] = {};
  HRESULT hr;

  if (SUCCEEDED(
          hr = SHGetFolderPathW(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath))) {
    path = shcore::wide_to_utf8(szPath, wcslen(szPath));
  } else {
    _com_error err(hr);
    throw std::runtime_error(
        str_format("Error when gathering the PROGRAMDATA folder path: %s",
                   err.ErrorMessage()));
  }

  to_append.push_back("MySQL");
  to_append.push_back("mysqlsh");
#else
  path = "/etc/mysql/mysqlsh";
#endif

  // Up to know the path must exist since it was retrieved from OS standard
  // means we need to guarantee the rest of the path exists
  if (!path.empty()) {
    for (size_t index = 0; index < to_append.size(); index++) {
      path += shcore::path::path_separator + to_append[index];
      ensure_dir_exists(path);
    }

    path += shcore::path::path_separator;
  }

  return path;
}

std::string get_binary_path() {
  std::string exe_path;

  // todo(.): warning should be printed with log_warning when available
#ifdef _WIN32
  HMODULE hModule = GetModuleHandleW(NULL);
  if (hModule) {
    // todo(kg): check last error from GetModuleFileNameW and if to is equal to
    // ERROR_INSUFFICIENT_BUFFER, then grow buffer, and retry. As temporary
    // solution we use 4k (> PATH_MAX) buffer and hope it fits.
    wchar_t path[4096] = {'\0'};
    const auto path_size = GetModuleFileNameW(hModule, path, 4096);
    const auto last_error = GetLastError();
    if (path_size == 0 || last_error == ERROR_INSUFFICIENT_BUFFER) {
      throw std::runtime_error(
          "get_binary_folder: GetModuleFileName failed with error " +
          get_error(last_error));
    } else {
      // on success path_size does not include terminated null character
      exe_path = shcore::wide_to_utf8(path, path_size);
    }
  } else {
    throw std::runtime_error(
        "get_binary_folder: GetModuleHandle failed with error " +
        get_last_error());
  }
#else
#ifdef __APPLE__
  char path[PATH_MAX]{'\0'};
  char real_path[PATH_MAX]{'\0'};
  uint32_t buffsize = sizeof(path);
  if (!_NSGetExecutablePath(path, &buffsize)) {
    // _NSGetExecutablePath may return tricky constructs on paths
    // like symbolic links or things like i.e /path/to/./mysqlsh
    // we need to normalize that
    if (realpath(path, real_path)) {
      exe_path.assign(real_path);
    } else {
      throw std::runtime_error(str_format(
          "get_binary_folder: Readlink failed with error %d", errno));
    }
  } else {
    throw std::runtime_error("get_binary_folder: _NSGetExecutablePath failed.");
  }

#else
#ifdef __sun
  char cwd[PATH_MAX]{'\0'};

  const char *path = getexecname();

  if (path && getcwd(cwd, PATH_MAX)) {
    exe_path = shcore::path::join_path(cwd, path);
  } else {
    throw std::runtime_error(
        str_format("get_binary_folder: Realpath failed with error %d", errno));
  }
#else
#ifdef __linux__
  char path[PATH_MAX]{'\0'};
  ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);
  if (-1 != len) {
    path[len] = '\0';
    exe_path.assign(path);
  } else {
    throw std::runtime_error(
        str_format("get_binary_folder: Readlink failed with error %d", errno));
  }
#endif
#endif
#endif
#endif

  return exe_path;
}

std::string get_binary_folder() {
  // todo(.): warning should be printed with log_warning when available
  std::string ret_val;
  std::string exe_path = get_binary_path();

  // If the exe path was found now we check if it can be considered the standard
  // installation by checking the parent folder is "bin"
  if (!exe_path.empty()) {
    const std::string path_separator{shcore::path::path_separator};
    std::vector<std::string> tokens;
    tokens = split_string(exe_path, path_separator, true);
    tokens.erase(tokens.end() - 1);

    ret_val = str_join(tokens, path_separator);
  }

  return ret_val;
}

std::string get_share_folder() {
  std::string path =
      shcore::path::join_path(get_mysqlx_home_path(), "share", "mysqlsh");
  if (!shcore::path::exists(path))
    throw std::runtime_error(
        path + ": share folder not found, shell installation likely invalid");

  return path;
}

std::string get_library_folder() {
  std::string path =
      shcore::path::join_path(get_mysqlx_home_path(), "lib", "mysqlsh");
  if (!shcore::path::exists(path))
    throw std::runtime_error(
        path + ": lib folder not found, shell installation likely invalid");

  return path;
}

#ifdef HAVE_LIBEXEC_DIR
std::string get_libexec_folder() {
  std::string path =
      shcore::path::join_path(get_mysqlx_home_path(), LIBEXECDIR, "mysqlsh");
  if (!shcore::path::exists(path))
    throw std::runtime_error(
        path + ": " + LIBEXECDIR +
        " folder not found, shell installation likely invalid");

  return path;
}
#endif  // HAVE_LIBEXEC_DIR

/*
 * Returns what should be considered the HOME folder for the shell.
 * If MYSQLSH_HOME is defined, returns its value.
 * If not, it will try to identify the value based on the binary full path:
 * In a standard setup the binary will be at <MYSQLX_HOME>/bin
 *
 * If that is the case MYSQLX_HOME is determined by trimming out
 * /bin/mysqlsh from the full executable name.
 *
 * An empty value would indicate MYSQLX_HOME is unknown.
 */
std::string get_mysqlx_home_path() {
  std::string ret_val;
  std::string binary_folder;
  std::string path_separator;
  const char *env_home = getenv("MYSQLSH_HOME");

  if (env_home) {
    ret_val.assign(env_home);
  } else {
    binary_folder = get_binary_folder();

    // If the exe path was found now we check if it can be considered the
    // standard installation by checking the parent folder is "bin"
    if (!binary_folder.empty()) {
      if (shcore::path::basename(binary_folder) == "bin") {
        ret_val = shcore::path::dirname(binary_folder);
      }
    }
  }
  return ret_val;
}

/*
 * Returns whether a file or a directory exists at the given path (true) or
 * doesn't (false);
 */
bool path_exists(const std::string &path) {
#ifdef _WIN32
  const auto wide_path = utf8_to_wide(path);
  return GetFileAttributesW(wide_path.c_str()) != INVALID_FILE_ATTRIBUTES;
#else
  return ::access(path.c_str(), F_OK) != -1;
#endif
}

#ifdef _WIN32
bool is_file(const char *path, const size_t path_length) {
  const auto wide_path = utf8_to_wide(path, path_length);
  DWORD attributes = GetFileAttributesW(wide_path.c_str());
  return attributes != INVALID_FILE_ATTRIBUTES &&
         !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}
#endif

bool is_file(const char *path) {
#ifdef _WIN32
  return is_file(path, strlen(path));
#else
  struct stat st;
  if (::stat(path, &st) < 0) return false;
  return S_ISREG(st.st_mode);
#endif
}

bool is_file(const std::string &path) {
#ifdef _WIN32
  return is_file(path.c_str(), path.size());
#else
  return is_file(path.c_str());
#endif
}

bool is_fifo(const char *path) {
#ifdef _WIN32
  // There is no FIFO files on windows in linux meaning
  return false;
#else
  struct stat st;
  if (::stat(path, &st) < 0) return false;
  return S_ISFIFO(st.st_mode);
#endif
}

bool is_fifo(const std::string &path) { return is_fifo(path.c_str()); }

size_t file_size(const char *path) {
#if defined(_WIN32)
  struct _stat64 file_stat = {};
  const auto ret = _wstat64(utf8_to_wide(path).c_str(), &file_stat);
#elif defined(__APPLE__) || defined(__SUNPRO_CC)
  struct stat file_stat = {};
  const auto ret = ::stat(path, &file_stat);
#else
  struct stat64 file_stat = {};
  const auto ret = stat64(path, &file_stat);
#endif

  if (0 != ret) {
    throw std::runtime_error(
        str_format("Failed to get the file size of '%s': %s", path,
                   errno_to_string(errno).c_str()));
  }

  return file_stat.st_size;
}

size_t file_size(const std::string &path) { return file_size(path.c_str()); }

/*
 * Returns true when the specified path is a folder
 */
bool is_folder(const std::string &path) {
#ifdef _WIN32
  const auto wide_path = utf8_to_wide(path);
  DWORD attributes = GetFileAttributesW(wide_path.c_str());

  return (attributes != INVALID_FILE_ATTRIBUTES &&
          (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
  struct stat stbuf;
  if (::stat(path.c_str(), &stbuf) < 0) return false;
  return S_ISDIR(stbuf.st_mode);
#endif
}

/*
 * Attempts to create directory if doesn't exists, otherwise just returns.
 * If there is an error, an exception is thrown.
 */
void ensure_dir_exists(const std::string &path) {
#ifdef _WIN32
  const auto wide_path = utf8_to_wide(path);
  DWORD attributes = GetFileAttributesW(wide_path.c_str());

  if (attributes != INVALID_FILE_ATTRIBUTES) {
    return;
  } else if (!CreateDirectoryW(wide_path.c_str(), NULL)) {
    throw std::runtime_error(
        str_format("Error when creating directory %s with error: %s",
                   path.c_str(), shcore::get_last_error().c_str()));
  }
#else
  const char *dir_path = path.c_str();
  DIR *dir = opendir(dir_path);
  if (dir) {
    /* Directory exists. */
    closedir(dir);
  } else if (ENOENT == errno) {
    /* Directory does not exist. */
    if (mkdir(dir_path, 0700) != 0)
      throw std::runtime_error(
          str_format("Error when verifying dir %s exists: %s", dir_path,
                     shcore::get_last_error().c_str()));
  } else {
    throw std::runtime_error(
        str_format("Error when verifying dir %s exists: %s", dir_path,
                   shcore::get_last_error().c_str()));
  }
#endif
}

/*
 * Recursively create a directory and its parents if they don't exist.
 */
void SHCORE_PUBLIC create_directory(const std::string &path, bool recursive,
                                    int mode) {
  assert(!path.empty());
  for (;;) {
#ifdef _WIN32
    const auto wide_path = utf8_to_wide(path);
    if (CreateDirectoryW(wide_path.c_str(), nullptr) != 0) {
      break;
    }

    const auto last_error = GetLastError();

    if (ERROR_ALREADY_EXISTS == last_error) {
      if (!is_folder(path)) {
        // path already exist but it's not a directory
        throw std::runtime_error(
            str_format("Could not create directory %s: %s", path.c_str(),
                       last_error_to_string(last_error).c_str()));
      }

      break;
    }

    if (ERROR_PATH_NOT_FOUND == last_error && recursive) {
      create_directory(path::dirname(path), recursive, mode);
    } else {
      throw std::runtime_error(
          str_format("Could not create directory %s: %s", path.c_str(),
                     last_error_to_string(last_error).c_str()));
    }
#else
    if (mkdir(path.c_str(), mode) == 0 || errno == EEXIST) {
      const auto error = errno;

      if (error == EEXIST && !is_folder(path)) {
        // path already exist but it's not a directory
        throw std::runtime_error(str_format("Could not create directory %s: %s",
                                            path.c_str(), strerror(error)));
      }

      break;
    }
    if (errno == ENOENT && recursive) {
      create_directory(path::dirname(path), recursive, mode);
    } else {
      throw std::runtime_error(str_format("Could not create directory %s: %s",
                                          path.c_str(), strerror(errno)));
    }
#endif
  }
}

std::vector<std::string> listdir(const std::string &path) {
  std::vector<std::string> files;
  iterdir(path, [&files](const std::string &name) -> bool {
    files.push_back(name);
    return true;
  });
  return files;
}

/**
 * Iterate contents of given directory, calling the given function on each
 * entry.
 */
bool iterdir(const std::string &path,
             const std::function<bool(const std::string &)> &fun) {
  bool stopped = false;
#ifdef _WIN32
  WIN32_FIND_DATAW ffd;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  auto wide_path = utf8_to_wide(path);

  // Add wildcard to search for all contents in path.
  const std::wstring search_path =
      wide_path + (path::is_path_separator(path.back()) ? L"*" : L"\\*");
  hFind = FindFirstFileW(search_path.c_str(), &ffd);
  if (hFind == INVALID_HANDLE_VALUE)
    throw std::runtime_error(
        str_format("%s: %s", path.c_str(), shcore::get_last_error().c_str()));

  // Remove all elements in directory (recursively)
  do {
    // Skip directories "." and ".."
    if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L"..")) {
      continue;
    }

    const auto file_name = shcore::wide_to_utf8(ffd.cFileName);
    if (!fun(file_name)) {
      stopped = true;
      break;
    }
  } while (FindNextFileW(hFind, &ffd) != 0);
  FindClose(hFind);
#else
  DIR *dir = opendir(path.c_str());
  if (dir) {
    // Remove all elements in directory (recursively)
    struct dirent *p_dir_entry;
    while ((p_dir_entry = readdir(dir))) {
      // Skip directories "." and ".."
      if (!strcmp(p_dir_entry->d_name, ".") ||
          !strcmp(p_dir_entry->d_name, "..")) {
        continue;
      }

      if (!fun(p_dir_entry->d_name)) {
        stopped = true;
        break;
      }
    }
    closedir(dir);
  } else {
    throw std::runtime_error(
        str_format("%s: %s", path.c_str(), get_last_error().c_str()));
  }
#endif
  return !stopped;
}

/*
 * Remove the specified directory and all its contents.
 */
void remove_directory(const std::string &path, bool recursive) {
  const char *dir_path = path.c_str();
#ifdef _WIN32
  const auto wide_path = utf8_to_wide(path);
  if (recursive) {
    DWORD attributes = GetFileAttributesW(wide_path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
      throw std::runtime_error("Unable to remove directory " + path + ": " +
                               get_last_error());
    } else if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
      throw std::runtime_error("Not a directory, unable to remove " + path);
    } else {
      WIN32_FIND_DATAW ffd;
      HANDLE hFind = INVALID_HANDLE_VALUE;

      // Add wildcard to search for all contents in path.
      const std::wstring search_path =
          wide_path + (path::is_path_separator(path.back()) ? L"*" : L"\\*");

      hFind = FindFirstFileW(search_path.c_str(), &ffd);
      if (hFind == INVALID_HANDLE_VALUE)
        throw std::runtime_error(
            str_format("Unable to remove directory %s. Error searching for "
                       "files in directory: %s",
                       dir_path, shcore::get_last_error().c_str()));

      // Remove all elements in directory (recursively)
      do {
        // Skip directories "." and ".."
        if (!wcscmp(ffd.cFileName, L".") || !wcscmp(ffd.cFileName, L"..")) {
          continue;
        }

        // Use the full path to the dir element.
        std::string dir_elem =
            path + "\\" + shcore::wide_to_utf8(ffd.cFileName);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          // It is a directory then do a recursive call to remove it.
          remove_directory(dir_elem);
        } else {
          // It is a file, remove it.
          const auto delete_file_path =
              wide_path + (path::is_path_separator(path.back()) ? L"" : L"\\") +
              ffd.cFileName;
          int res = DeleteFileW(delete_file_path.c_str());
          if (!res) {
            throw std::runtime_error(str_format(
                "Unable to remove directory %s. Error removing file %s: %s",
                dir_path, dir_elem.c_str(), shcore::get_last_error().c_str()));
          }
        }
      } while (FindNextFileW(hFind, &ffd) != 0);
      FindClose(hFind);
    }
  }
  // The directory is now empty and can be removed.
  int res = RemoveDirectoryW(wide_path.c_str());
  if (!res) {
    throw std::runtime_error(str_format("Error removing directory %s: %s",
                                        dir_path,
                                        shcore::get_last_error().c_str()));
  }
#else
  if (recursive) {
    DIR *dir = opendir(dir_path);
    if (dir) {
      // Remove all elements in directory (recursively)
      struct dirent *p_dir_entry;
      while ((p_dir_entry = readdir(dir))) {
        // Skip directories "." and ".."
        if (!strcmp(p_dir_entry->d_name, ".") ||
            !strcmp(p_dir_entry->d_name, "..")) {
          continue;
        }

        // Use the full path to the dir element.
        std::string dir_elem = path + "/" + p_dir_entry->d_name;

        // Get the information about the dir element to act accordingly
        // depending if it is a directory or a file.
        struct stat stat_info;
        if (!stat(dir_elem.c_str(), &stat_info)) {
          if (S_ISDIR(stat_info.st_mode)) {
            // It is a directory then do a recursive call to remove it.
            remove_directory(dir_elem);
          } else {
            // It is a file, remove it.
            int res = ::remove(dir_elem.c_str());
            if (res && errno != ENOENT) {
              throw std::runtime_error(str_format(
                  "Unable to remove directory %s. Error removing %s: %s",
                  dir_path, dir_elem.c_str(),
                  shcore::get_last_error().c_str()));
            }
          }
        }
      }
      closedir(dir);
    } else if (ENOENT == errno) {
      throw std::runtime_error("Directory " + path +
                               " does not exist and cannot be removed.");
    } else if (ENOTDIR == errno) {
      throw std::runtime_error("Not a directory, unable to remove " + path);
    } else {
      throw std::runtime_error(str_format("Unable to remove directory %s: %s",
                                          dir_path,
                                          shcore::get_last_error().c_str()));
    }
  }
  // The directory is now empty and can be removed.
  int res = rmdir(dir_path);
  if (res) {
    throw std::runtime_error(str_format("Error remove directory %s: %s",
                                        dir_path,
                                        shcore::get_last_error().c_str()));
  }
#endif
}

/*
 * Returns the last system specific error description (using GetLastError in
 * Windows or errno in Unix/OSX).
 */
std::string get_last_error() {
#ifdef _WIN32
  DWORD dwCode = GetLastError();
  return get_error(dwCode);
#else
  int errnum = errno;
  return get_error(errnum);
#endif
}

bool load_text_file(const std::string &path, std::string &data) {
#ifdef _WIN32
  std::ifstream s(utf8_to_wide(path));
#else
  std::ifstream s(path);
#endif
  if (s.fail()) {
    return false;
  }
  s.seekg(0, std::ios_base::end);
  std::streamsize fsize = s.tellg();
  s.seekg(0, std::ios_base::beg);
  data.resize(fsize);
  s.read(data.data(), fsize);
  return s.gcount() == fsize;
}

std::string SHCORE_PUBLIC get_text_file(const std::string &path) {
  std::string data;
  if (!load_text_file(path, data)) {
    throw std::runtime_error(path + ": " + errno_to_string(errno));
  }
  return data;
}

/*
 * Deletes a file in a cross platform manner. If file removal fails,
 * an exception is thrown.
 *
 * If file does not exist, and quiet is true, fails silently.
 */
void SHCORE_PUBLIC delete_file(const std::string &filename, bool quiet) {
  if (quiet && !path_exists(filename)) return;
#ifdef _WIN32
  const auto wide_filename = utf8_to_wide(filename);
  if (!DeleteFileW(wide_filename.c_str()))
    throw std::runtime_error("Cannot delete file \"" + filename + "\". " +
                             get_last_error());
#else
  if (remove(filename.c_str()))
    throw std::runtime_error("Cannot delete file \"" + filename + "\". " +
                             get_last_error());
#endif
}

/*
 * Returns the HOME path (~ in Unix or %AppData%\ in Windows).
 */
std::string get_home_dir() {
  std::string path;
  std::vector<std::string> to_append;

#ifdef _WIN32
  wchar_t szPath[MAX_PATH + 1] = {};
  HRESULT hr;

  if (SUCCEEDED(hr = SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, szPath))) {
    path = shcore::wide_to_utf8(szPath, wcslen(szPath));
  } else {
    _com_error err(hr);
    throw std::runtime_error(
        str_format("Error when gathering the PROFILE folder path: %s",
                   err.ErrorMessage()));
  }
#else
  char *cpath = std::getenv("HOME");

  if (cpath != NULL) path.assign(cpath);
#endif

  // Up to know the path must exist since it was retrieved from OS standard
  // means we need to guarantee the rest of the path exists
  if (!path.empty()) {
    for (size_t index = 0; index < to_append.size(); index++) {
      path += shcore::path::path_separator + to_append[index];
      ensure_dir_exists(path);
    }

    path += shcore::path::path_separator;
  }

  return path;
}

bool create_file(const std::string &name, const std::string &content,
                 bool binary_mode) {
  auto open_mode_flags = std::ofstream::out | std::ofstream::trunc;
  if (binary_mode) {
    open_mode_flags |= std::ofstream::binary;
  }
#ifdef _WIN32
  // msvc c++ lib has non-standard ofstream constructor wchar_t overload
  const auto file_name = utf8_to_wide(name);
  std::ofstream file(file_name, open_mode_flags);
#else
  std::ofstream file(name, open_mode_flags);
#endif

  if (file.is_open()) {
    file << content;
    file.close();
    return true;
  }

  return false;
}

void copy_file(const std::string &from, const std::string &to,
               bool copy_attributes) {
  std::ofstream ofile;
  std::ifstream ifile;

  ofile.open(to,
             std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);
  if (ofile.fail()) {
    throw std::runtime_error("Could not create file '" + to +
                             "': " + errno_to_string(errno));
  }
  ifile.open(from, std::ofstream::in | std::ofstream::binary);
  if (ifile.fail()) {
    throw std::runtime_error("Could not open file '" + from +
                             "': " + errno_to_string(errno));
  }

  ofile << ifile.rdbuf();

  ofile.close();
  ifile.close();

  if (copy_attributes) {
#ifndef _WIN32
    // Change the destination file ownership and permissions to match the ones
    // from the source file.
    struct stat result;
    if (stat(from.c_str(), &result) == 0) {
      if (getuid() == 0) {
        // Only change file ownership if executed by the root user.
        if (chown(to.c_str(), result.st_uid, result.st_gid) != 0) {
          throw std::runtime_error("Unable to change ownership for file " + to +
                                   " : " + errno_to_string(errno));
        }
      }
      if (chmod(to.c_str(), result.st_mode) != 0) {
        throw std::runtime_error("Unable to set file mode to " + to + ": " +
                                 errno_to_string(errno));
      }
    } else {
      throw std::runtime_error("Unable to get file mode from " + from + ": " +
                               errno_to_string(errno));
    }
#endif
  }
}

void rename_file(const std::string &from, const std::string &to) {
#ifdef _WIN32
  const auto w_from = utf8_to_wide(from);
  const auto w_to = utf8_to_wide(to);
  auto ret = _wrename(w_from.c_str(), w_to.c_str());

  if (ret < 0 && (EACCES == errno || EEXIST == errno)) {
    try {
      // rename on Windows fails if destination file exists, delete it first
      delete_file(to, true);
    } catch (const std::runtime_error &e) {
      throw std::runtime_error(
          shcore::str_format("Could not rename '%s' to '%s': destination "
                             "exists and could not be deleted: %s",
                             from.c_str(), to.c_str(), e.what()));
    }

    ret = _wrename(w_from.c_str(), w_to.c_str());
  }

#else
  const auto ret = rename(from.c_str(), to.c_str());
#endif

  if (ret < 0) {
    throw std::runtime_error(
        shcore::str_format("Could not rename '%s' to '%s': %s", from.c_str(),
                           to.c_str(), strerror(errno)));
  }
}

void copy_dir(const std::string &from, const std::string &to) {
  create_directory(to);
  iterdir(from, [from, to](const std::string &name) -> bool {
    try {
      if (is_folder(path::join_path(from, name)))
        copy_dir(path::join_path(from, name), path::join_path(to, name));
      else
        copy_file(path::join_path(from, name), path::join_path(to, name));
    } catch (const std::runtime_error &) {
      if (errno != ENOENT) throw;
    }
    return true;
  });
}

/*
 * Check if the file has write permissions. If the file does not exist,
 * checks if it can be created.
 *
 * @param filename the full path of the file to be checked
 *
 * Throws an exception with the reason if the file cannot be written to or
 * created.
 */
void check_file_writable_or_throw(const std::string &filename) {
  std::ofstream ofs;

  if (is_file(filename)) {
    // Use openmode 'out' to open the file for writing
#ifdef _WIN32
    const auto wide_filename = utf8_to_wide(filename);
    ofs.open(wide_filename, std::ofstream::out | std::ofstream::app);
#else
    ofs.open(filename.c_str(), std::ofstream::out | std::ofstream::app);
#endif
    std::string error = shcore::errno_to_string(errno);
    // If it could open the file, it's writable
    if (ofs.is_open()) {
      ofs.close();
    } else {
      throw std::runtime_error(error);
    }
  } else {
    // Use openmode 'out' to open the file for writing
#ifdef _WIN32
    const auto wide_filename = utf8_to_wide(filename);
    ofs.open(wide_filename, std::ofstream::out | std::ofstream::app);
#else
    ofs.open(filename.c_str(), std::ofstream::out | std::ofstream::app);
#endif
    std::string error = shcore::errno_to_string(errno);
    // If it could open the file, it's writable
    if (ofs.is_open()) {
      ofs.close();
      delete_file(filename);
    } else {
      throw std::runtime_error(error);
    }
  }
}

/*
 * Check if the file has read permissions.
 *
 * @param filename the full path of the file to be checked
 *
 * Throws an exception with the reason if the file is not found or cannot be
 * read.
 */
void SHCORE_PUBLIC check_file_readable_or_throw(const std::string &filename) {
  if (is_file(filename)) {
    std::ifstream ifs;
#ifdef _WIN32
    const auto wide_filename = utf8_to_wide(filename);
    ifs.open(wide_filename, std::ofstream::in);
#else
    ifs.open(filename.c_str(), std::ofstream::in);
#endif
    std::string error = shcore::errno_to_string(errno);
    // If it could open the file, it's readable
    if (ifs.is_open()) {
      ifs.close();
    } else {
      throw std::runtime_error("Unable to open file '" + filename +
                               "': " + error);
    }
  } else {
    throw std::runtime_error("Path '" + filename + "' is not a file'");
  }
}

/**
 * Changes access attributes to a file to be read only.
 * @param path The path to the file to be made read only.
 * @returns 0 on success, -1 on failure
 */

int SHCORE_PUBLIC make_file_readonly(const std::string &path) {
#ifndef _WIN32
  // Set permissions on configuration file to 444 (chmod only works on
  // unix systems).
  unsigned int ro = S_IRUSR | S_IRGRP | S_IROTH;
  return chmod(path.c_str(), ro);
#else
  const auto wide_path = utf8_to_wide(path);
  auto attributes = GetFileAttributesW(wide_path.c_str());
  // set permissions on configuration file to read only
  if (SetFileAttributesW(wide_path.c_str(),
                         attributes | FILE_ATTRIBUTE_READONLY)) {
    return 0;
  }
  return -1;
#endif
}

/**
 * Changes the file permissions of the indicated path.
 * @param path The path of the file/folder which will be updated.
 * @param mode The User/Group/Other permissions to be set.
 *
 * Permissions should be given in binary mask format, this is
 * as a number in the format of 0UGO
 *
 * Where:
 *  @li U is the binary mask for User's permissions
 *  @li G is the binary mask for User Group's permissions
 *  @li O is the binary mask for Other's permissions
 *
 * Each binary mask contains the a permission combination that includes:
 * @li 0x001: Indicates execution permission
 * @li 0x010: Indicates write permission
 * @li 0x100: Indicates read permission
 *
 * On Windows, only the User permissions are considered:
 * @li if the user write permission is OFF, the file will be set as
 * Read Only.
 * @li if the user write permission is ON, the Read Only flag will be
 * removed from the file.
 *
 * This function does not work with Windows folders.
 */
int SHCORE_PUBLIC ch_mod(const std::string &path, int mode) {
#ifndef _WIN32
  return chmod(path.c_str(), mode);
#else
  const auto wide_path = utf8_to_wide(path);
  const auto attributes = GetFileAttributesW(wide_path.c_str());
  // If user write permission is off, sets the file read only
  // otherwise, cleans the file read only
  int user_write = (2 << 6) & mode;
  const DWORD new_attributes = user_write
                                   ? attributes & ~FILE_ATTRIBUTE_READONLY
                                   : attributes | FILE_ATTRIBUTE_READONLY;
  if (!SetFileAttributesW(wide_path.c_str(), new_attributes)) {
    return -1;
  }
  return 0;
#endif
}

/**
 * Updates file or folder permissions so it is accessible only to the current
 * user.
 *
 * In linux, sets files with RW permissions and folders with RWX.
 * In windows, sets all with Full Control for:
 * @li Current User
 * @li System User
 * @li Admin Group
 */
int SHCORE_PUBLIC set_user_only_permissions(const std::string &path) {
  int ret_val = -1;
#ifndef _WIN32
  int mode = 0600;
  if (shcore::is_folder(path)) mode = 0700;
  ret_val = chmod(path.c_str(), mode);
#else
  DWORD dwRes = 0;
  PSID pSystemSID = NULL, pAdminSID = NULL;
  SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
  PACL pNewDACL = NULL;
  EXPLICIT_ACCESSW ea[3];

  if (!path.empty()) {
    DWORD inheritance = NO_INHERITANCE;
    if (shcore::is_folder(path))
      inheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;

    // Create a SIDs for the BUILTIN\Administrators group
    // and for the SYSTEM
    if (AllocateAndInitializeSid(&SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                 &pAdminSID) &&
        AllocateAndInitializeSid(&SIDAuthNT, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0,
                                 0, 0, 0, 0, 0, &pSystemSID)) {
      DWORD user_full_control = GENERIC_ALL | STANDARD_RIGHTS_ALL;
      // Initialize an EXPLICIT_ACCESS structure for the new ACE.
      // Sets full access to administrators group
      ZeroMemory(&ea, 3 * sizeof(EXPLICIT_ACCESS));
      ea[0].grfAccessPermissions = user_full_control;
      ea[0].grfAccessMode = SET_ACCESS;
      ea[0].grfInheritance = inheritance;
      ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
      ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
      ea[0].Trustee.ptstrName = (LPWSTR)pAdminSID;

      // Sets full access to system user
      ea[1].grfAccessPermissions = user_full_control;
      ea[1].grfAccessMode = SET_ACCESS;
      ea[1].grfInheritance = inheritance;
      ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
      ea[1].Trustee.TrusteeType = TRUSTEE_IS_COMPUTER;
      ea[1].Trustee.ptstrName = (LPWSTR)pSystemSID;

      // Sets full access to current user
      wchar_t username[UNLEN + 1] = {};
      DWORD username_len = UNLEN + 1;
      const auto get_username_retval = GetUserNameW(username, &username_len);
      if (get_username_retval == 0) {
        throw std::runtime_error(
            "Cannot obtain user name while setting permissions to \"" + path +
            "\". " + get_last_error());
      }
      ea[2].grfAccessPermissions = user_full_control;
      ea[2].grfAccessMode = SET_ACCESS;
      ea[2].grfInheritance = inheritance;
      ea[2].Trustee.TrusteeForm = TRUSTEE_IS_NAME;
      ea[2].Trustee.TrusteeType = TRUSTEE_IS_USER;
      ea[2].Trustee.ptstrName = username;

      const auto wide_target_path = shcore::utf8_to_wide(path);
      const auto target_path = wide_target_path.c_str();

      // Create a new ACL with defined ACEs
      if (ERROR_SUCCESS == SetEntriesInAclW(3, ea, nullptr, &pNewDACL)) {
        // Sets the new ACL as the object's DACL.
        dwRes = SetNamedSecurityInfoW(
            (LPWSTR)target_path, SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
            NULL, NULL, pNewDACL, NULL);
        if (ERROR_SUCCESS == dwRes) {
          ret_val = 0;
        }
      }
    }

    // Cleanup
    if (pAdminSID != NULL) FreeSid(pAdminSID);
    if (pSystemSID != NULL) FreeSid(pSystemSID);
    if (pNewDACL != NULL) LocalFree((HLOCAL)pNewDACL);
  }
#endif
  return ret_val;
}

std::string get_absolute_path(const std::string &file_path,
                              const std::string &base_dir) {
#ifdef _WIN32
  const auto long_path = [](const std::string &path) {
    if (path.length() > 0 && '\\' != path[0]) {
      return R"(\\?\)" + path;
    } else {
      return path;
    }
  };
#else   // !_WIN32
  const auto long_path = [](const std::string &path) -> const std::string & {
    return path;
  };
#endif  // _WIN32

  if (path::is_absolute(file_path)) {
    return path::normalize(long_path(path::expand_user(file_path)));
  } else {
    return path::normalize(long_path(shcore::path::join_path(
        base_dir.empty() ? path::getcwd() : base_dir, file_path)));
  }
}

#ifdef _WIN32

namespace {

using Security_descriptor_t =
    std::unique_ptr<std::remove_pointer_t<PSECURITY_DESCRIPTOR>,
                    decltype(&LocalFree)>;

Security_descriptor_t get_security_descriptor(const std::string &file_name) {
  static constexpr SECURITY_INFORMATION kReqInfo = DACL_SECURITY_INFORMATION;

  const auto wide_file_name = utf8_to_wide(file_name);
  PSECURITY_DESCRIPTOR sec_desc = nullptr;
  const auto ret_val =
      GetNamedSecurityInfoW(wide_file_name.c_str(), SE_FILE_OBJECT, kReqInfo,
                            nullptr, nullptr, nullptr, nullptr, &sec_desc);

  if (ret_val != ERROR_SUCCESS) {
    throw std::system_error(ret_val, std::system_category(),
                            "GetNamedSecurityInfo() failed (" + file_name +
                                "): " + std::to_string(ret_val));
  }

  return {sec_desc, &LocalFree};
}

std::unique_ptr<SID, decltype(&free)> create_sid() {
  DWORD sid_size = SECURITY_MAX_SID_SIZE;

  std::unique_ptr<SID, decltype(&free)> everyone_sid(
      static_cast<SID *>(malloc(sid_size)), &free);

  if (CreateWellKnownSid(WinWorldSid, nullptr, everyone_sid.get(), &sid_size) ==
      FALSE) {
    throw std::system_error(
        std::error_code(GetLastError(), std::system_category()),
        "CreateWellKnownSid() failed");
  }

  return everyone_sid;
}
/**
 * Verifies permissions of an access ACE entry.
 *
 * @param[in] access_ace Access ACE entry.
 *
 * @throw std::exception Everyone has access to the ACE access entry or
 *                        an error occurred.
 */
void check_ace_access_rights(ACCESS_ALLOWED_ACE *access_ace, SID *esid) {
  SID *sid = reinterpret_cast<SID *>(&access_ace->SidStart);

  if (EqualSid(sid, esid)) {
    if (access_ace->Mask & (FILE_EXECUTE)) {
      throw std::system_error(make_error_code(std::errc::permission_denied),
                              "Expected no 'Execute' for 'Everyone'.");
    }
    if (access_ace->Mask &
        (FILE_WRITE_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES)) {
      throw std::system_error(make_error_code(std::errc::permission_denied),
                              "Expected no 'Write' for 'Everyone'.");
    }
    if (access_ace->Mask &
        (FILE_READ_DATA | FILE_READ_EA | FILE_READ_ATTRIBUTES)) {
      throw std::system_error(make_error_code(std::errc::permission_denied),
                              "Expected no 'Read' for 'Everyone'.");
    }
  }
}

/**
 * Verifies access permissions in a DACL.
 *
 * @param[in] dacl DACL to be verified.
 *
 * @throw std::exception DACL contains an ACL entry that grants full access to
 *                        Everyone or an error occurred.
 */
void check_acl_access_rights(ACL *dacl) {
  ACL_SIZE_INFORMATION dacl_size_info;

  if (GetAclInformation(dacl, &dacl_size_info, sizeof(dacl_size_info),
                        AclSizeInformation) == FALSE) {
    throw std::system_error(
        std::error_code(GetLastError(), std::system_category()),
        "GetAclInformation() failed");
  }

  auto everyone_sid = create_sid();

  for (DWORD ace_idx = 0; ace_idx < dacl_size_info.AceCount; ++ace_idx) {
    LPVOID ace = nullptr;

    if (GetAce(dacl, ace_idx, &ace) == FALSE) {
      throw std::system_error(
          std::error_code(GetLastError(), std::system_category()),
          "GetAce() failed");
    }

    if (static_cast<ACE_HEADER *>(ace)->AceType == ACCESS_ALLOWED_ACE_TYPE)
      check_ace_access_rights(static_cast<ACCESS_ALLOWED_ACE *>(ace),
                              everyone_sid.get());
  }
}

/**
 * Verifies access permissions in a security descriptor.
 *
 * @param[in] sec_desc Security descriptor to be verified.
 *
 * @throw std::exception Security descriptor grants full access to
 *                        Everyone or an error occurred.
 */
void check_security_descriptor_access_rights(
    const Security_descriptor_t &sec_desc) {
  BOOL dacl_present;
  ACL *dacl;
  BOOL dacl_defaulted;

  if (GetSecurityDescriptorDacl(sec_desc.get(), &dacl_present, &dacl,
                                &dacl_defaulted) == FALSE) {
    throw std::system_error(
        std::error_code(GetLastError(), std::system_category()),
        "GetSecurityDescriptorDacl() failed");
  }

  if (!dacl_present) {
    // No DACL means: no access allowed. Which is fine.
    return;
  }

  if (!dacl) {
    // Empty DACL means: all access allowed.
    throw std::system_error(make_error_code(std::errc::permission_denied),
                            "Expected access denied for 'Everyone'.");
  }

  check_acl_access_rights(dacl);
}
}  // namespace

#endif  // _WIN32

void check_file_access_rights_to_open(const std::string &file_name) {
#ifdef _WIN32
  Security_descriptor_t sec_descr{nullptr, &LocalFree};
  try {
    sec_descr = get_security_descriptor(file_name);
  } catch (const std::system_error &) {
    // that means that the system does not support ACL, in that case nothing
    // really to check
    return;
  }
  try {
    check_security_descriptor_access_rights(sec_descr);
  } catch (const std::system_error &e) {
    if (e.code() == std::errc::permission_denied) {
      throw std::system_error(
          e.code(),
          "'" + file_name + "' has insecure permissions. " + e.what());
    } else {
      throw;
    }
  } catch (...) {
    throw;
  }
#else
  struct stat status;

  if (stat(file_name.c_str(), &status) != 0) {
    if (errno == ENOENT) return;
    throw std::system_error(errno, std::generic_category(),
                            "stat() failed for " + file_name + "'");
  }

  static constexpr mode_t kFullAccessMask = (S_IRWXU | S_IRWXG | S_IRWXO);
  static constexpr mode_t kRequiredAccessMask = (S_IRUSR | S_IWUSR);

  if ((status.st_mode & kFullAccessMask) != kRequiredAccessMask) {
    throw std::system_error(
        make_error_code(std::errc::permission_denied),
        "'" + file_name + "' has insecure permissions. Expected u+rw only");
  }
#endif  // _WIN32
}

std::string get_tempfile_path(const std::string &cnf_path) {
  std::string tmp_file_path = cnf_path + ".tmp";
  if (path_exists(tmp_file_path)) {
    // Attempt use a temp file with a random component if it already exists.
    int attempts = 0;
    bool temp_file_not_exist = false;

    // Setup uniform random generation of integers between [0, INT_MAX] using
    // Mersenne Twister algorithm and a non-determinist seed.
    std::random_device rd_seed;
    std::mt19937 rnd_gen(rd_seed());
    std::uniform_int_distribution<int> distribution(0, INT_MAX);

    // Try at most 5 times to use a random file name that does not exist.
    while (attempts < 5) {
      int rand_num = distribution(rnd_gen);
      tmp_file_path = cnf_path + ".tmp" + std::to_string(rand_num);
      attempts++;

      if (!path_exists(tmp_file_path)) {
        temp_file_not_exist = true;
        break;
      }
    }

    if (!temp_file_not_exist) {
      // This error is not expected to be thrown (only if all attempts failed).
      throw std::runtime_error(
          "Unable to generate a non existing temporary file to write the "
          "configuration for the target option file: " +
          cnf_path + ".");
    }
  }

  return tmp_file_path;
}

}  // namespace shcore
