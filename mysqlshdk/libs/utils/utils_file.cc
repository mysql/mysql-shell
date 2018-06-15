/*
 * Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "mysqlshdk/include/mysh_config.h"
#include "utils/utils_general.h"
#include "utils/utils_path.h"
#include "utils/utils_string.h"

#ifdef WIN32
#include <ShlObj.h>
#include <comdef.h>
#include <direct.h>
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
#include <linux/limits.h>
#endif
#endif

namespace shcore {
/*
 * Returns the config path
 * (~/.mysqlsh in Unix or %AppData%\MySQL\mysqlsh in Windows).
 * May be overriden with MYSQLSH_USER_CONFIG_HOME
 * (specially for tests)
 */
std::string get_user_config_path() {
  std::string path_separator;
  std::string path;
  std::vector<std::string> to_append;

  // Check if there's an override of the config directory
  // This is needed required for unit-tests
  const char *usr_config_path = getenv("MYSQLSH_USER_CONFIG_HOME");
  if (usr_config_path) {
    path.assign(usr_config_path);
  } else {
#ifdef WIN32
    path_separator = "\\";
    char szPath[MAX_PATH];
    HRESULT hr;

    if (SUCCEEDED(hr =
                      SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
      path.assign(szPath);
    } else {
      _com_error err(hr);
      throw std::runtime_error(
          str_format("Error when gathering the APPDATA folder path: %s",
                     err.ErrorMessage()));
    }

    to_append.push_back("MySQL");
    to_append.push_back("mysqlsh");
#else
    path_separator = "/";
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
      for (size_t index = 0; index < to_append.size(); index++) {
        path += path_separator + to_append[index];
        ensure_dir_exists(path);
      }

      path += path_separator;
    }
  }
  return path;
}

/*
 * Returns the config path (/etc/mysql/mysqlsh in Unix or
 * %ProgramData%\MySQL\mysqlsh in Windows).
 */
std::string get_global_config_path() {
  std::string path_separator;
  std::string path;
  std::vector<std::string> to_append;

#ifdef WIN32
  path_separator = "\\";
  char szPath[MAX_PATH];
  HRESULT hr;

  if (SUCCEEDED(
          hr = SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, szPath)))
    path.assign(szPath);
  else {
    _com_error err(hr);
    throw std::runtime_error(
        str_format("Error when gathering the PROGRAMDATA folder path: %s",
                   err.ErrorMessage()));
  }

  to_append.push_back("MySQL");
  to_append.push_back("mysqlsh");
#else
  path_separator = "/";
  path = "/etc/mysql/mysqlsh";
#endif

  // Up to know the path must exist since it was retrieved from OS standard
  // means we need to guarantee the rest of the path exists
  if (!path.empty()) {
    for (size_t index = 0; index < to_append.size(); index++) {
      path += path_separator + to_append[index];
      ensure_dir_exists(path);
    }

    path += path_separator;
  }

  return path;
}

std::string get_binary_path() {
  std::string exe_path;

  // TODO: warning should be printed with log_warning when available
#ifdef WIN32
  HMODULE hModule = GetModuleHandleA(NULL);
  if (hModule) {
    char path[MAX_PATH]{'\0'};
    if (GetModuleFileNameA(hModule, path, MAX_PATH)) {
      exe_path.assign(path);
    } else
      throw std::runtime_error(str_format(
          "get_binary_folder: GetModuleFileNameA failed with error %s\n",
          GetLastError()));
  } else
    throw std::runtime_error(
        str_format("get_binary_folder: GetModuleHandleA failed with error %s\n",
                   GetLastError()));
#else
#ifdef __APPLE__
  char path[PATH_MAX]{'\0'};
  char real_path[PATH_MAX]{'\0'};
  uint32_t buffsize = sizeof(path);
  if (!_NSGetExecutablePath(path, &buffsize)) {
    // _NSGetExecutablePath may return tricky constructs on paths
    // like symbolic links or things like i.e /path/to/./mysqlsh
    // we need to normalize that
    if (realpath(path, real_path))
      exe_path.assign(real_path);
    else
      throw std::runtime_error(str_format(
          "get_binary_folder: Readlink failed with error %d\n", errno));
  } else
    throw std::runtime_error(
        "get_binary_folder: _NSGetExecutablePath failed.\n");

#else
#ifdef __linux__
  char path[PATH_MAX]{'\0'};
  ssize_t len = readlink("/proc/self/exe", path, PATH_MAX);
  if (-1 != len) {
    path[len] = '\0';
    exe_path.assign(path);
  } else
    throw std::runtime_error(str_format(
        "get_binary_folder: Readlink failed with error %d\n", errno));
#endif
#endif
#endif

  return exe_path;
}

std::string get_binary_folder() {
  std::string path_separator;

  // TODO: warning should be printed with log_warning when available
#ifdef WIN32
  path_separator = "\\";
#else
  path_separator = "/";
#endif

  std::string ret_val;
  std::string exe_path = get_binary_path();

  // If the exe path was found now we check if it can be considered the standard
  // installation by checking the parent folder is "bin"
  if (!exe_path.empty()) {
    std::vector<std::string> tokens;
    tokens = split_string(exe_path, path_separator, true);
    tokens.erase(tokens.end() - 1);

    ret_val = str_join(tokens, path_separator);
  }

  return ret_val;
}

std::string get_share_folder() {
  std::string path;
  path = shcore::path::join_path(get_mysqlx_home_path(), "share", "mysqlsh");
  if (!shcore::path::exists(path))
    throw std::runtime_error(
        path + ": share folder not found, shell installation likely invalid");

  return path;
}

std::string SHCORE_PUBLIC get_mp_path() {
  // Determine provisioning path
  std::string path;
  path = shcore::path::join_path(get_mysqlx_home_path(), "share", "mysqlsh",
                                 "mysqlprovision.zip");
  if (!shcore::file_exists(path))
    throw std::runtime_error(
        path + ": mysqlprovision not found, shell installation likely invalid");

  return path;
}

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
 * Returns whether if a file exists (true) or doesn't (false);
 */
bool file_exists(const std::string &filename) {
#ifdef WIN32
  DWORD dwAttrib = GetFileAttributesA(filename.c_str());

  if (dwAttrib != INVALID_FILE_ATTRIBUTES &&
      !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    return true;
  else
    return false;
#else
  if (access(filename.c_str(), F_OK) != -1)
    return true;
  else
    return false;
#endif
}

/*
 * Returns true when the specified path is a folder
 */
bool is_folder(const std::string &path) {
#ifdef WIN32
  DWORD dwAttrib = GetFileAttributesA(path.c_str());

  return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
          (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
  struct stat stbuf;
  if (stat(path.c_str(), &stbuf) < 0) return false;
  return (stbuf.st_mode & S_IFDIR) != 0;
#endif
}

/*
 * Attempts to create directory if doesn't exists, otherwise just returns.
 * If there is an error, an exception is thrown.
 */
void ensure_dir_exists(const std::string &path) {
  const char *dir_path = path.c_str();
#ifdef WIN32
  DWORD dwAttrib = GetFileAttributesA(dir_path);

  if (dwAttrib != INVALID_FILE_ATTRIBUTES)
    return;
  else if (!CreateDirectoryA(dir_path, NULL)) {
    throw std::runtime_error(
        str_format("Error when creating directory %s with error: %s", dir_path,
                   shcore::get_last_error().c_str()));
  }
#else
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
void SHCORE_PUBLIC create_directory(const std::string &path, bool recursive) {
  assert(!path.empty());
  for (;;) {
#ifdef WIN32
    if (_mkdir(path.c_str()) == 0 || errno == EEXIST) {
      break;
    }
#else
    if (mkdir(path.c_str(), 0700) == 0 || errno == EEXIST) {
      break;
    }
#endif
    if (errno == ENOENT && recursive) {
      create_directory(path::dirname(path), recursive);
    } else {
      throw std::runtime_error(str_format("Could not create directory %s: %s",
                                          path.c_str(), strerror(errno)));
    }
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
#ifdef WIN32
  WIN32_FIND_DATA ffd;
  HANDLE hFind = INVALID_HANDLE_VALUE;

  // Add wildcard to search for all contents in path.
  std::string search_path = path + "\\*";
  hFind = FindFirstFile(search_path.c_str(), &ffd);
  if (hFind == INVALID_HANDLE_VALUE)
    throw std::runtime_error(
        str_format("%s: %s", path.c_str(), shcore::get_last_error().c_str()));

  // Remove all elements in directory (recursively)
  do {
    // Skip directories "." and ".."
    if (!strcmp(ffd.cFileName, ".") || !strcmp(ffd.cFileName, "..")) {
      continue;
    }

    if (!fun(ffd.cFileName)) {
      stopped = true;
      break;
    }
  } while (FindNextFile(hFind, &ffd) != 0);
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
#ifdef WIN32
  if (recursive) {
    DWORD dwAttrib = GetFileAttributesA(dir_path);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
      throw std::runtime_error(str_format("Unable to remove directory %s: %s",
                                          dir_path,
                                          shcore::get_last_error().c_str()));
    } else if (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY)) {
      throw std::runtime_error(
          str_format("Not a directory, unable to remove %s.", dir_path));
    } else {
      WIN32_FIND_DATA ffd;
      HANDLE hFind = INVALID_HANDLE_VALUE;

      // Add wildcard to search for all contents in path.
      std::string search_path = path + "\\*";
      hFind = FindFirstFile(search_path.c_str(), &ffd);
      if (hFind == INVALID_HANDLE_VALUE)
        throw std::runtime_error(
            str_format("Unable to remove directory %s. Error searching for "
                       "files in directory: %s",
                       dir_path, shcore::get_last_error().c_str()));

      // Remove all elements in directory (recursively)
      do {
        // Skip directories "." and ".."
        if (!strcmp(ffd.cFileName, ".") || !strcmp(ffd.cFileName, "..")) {
          continue;
        }

        // Use the full path to the dir element.
        std::string dir_elem = path + "\\" + ffd.cFileName;

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
          // It is a directory then do a recursive call to remove it.
          remove_directory(dir_elem);
        } else {
          // It is a file, remove it.
          int res = DeleteFile(dir_elem.c_str());
          if (!res) {
            throw std::runtime_error(str_format(
                "Unable to remove directory %s. Error removing file %s: %s",
                dir_path, dir_elem.c_str(), shcore::get_last_error().c_str()));
          }
        }
      } while (FindNextFile(hFind, &ffd) != 0);
      FindClose(hFind);
    }
  }
  // The directory is now empty and can be removed.
  int res = RemoveDirectory(dir_path);
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
#ifdef WIN32
  DWORD dwCode = GetLastError();
  return "SystemError: " + last_error_to_string(dwCode) +
         str_format(" (code %lu)", dwCode);
#else
  int errnum = errno;
  return errno_to_string(errnum) + str_format(" (errno %d)", errnum);
#endif
}

bool load_text_file(const std::string &path, std::string &data) {
  bool ret_val = false;

  std::ifstream s(path.c_str());
  if (!s.fail()) {
    s.seekg(0, std::ios_base::end);
    std::streamsize fsize = s.tellg();
    s.seekg(0, std::ios_base::beg);
    char *fdata = new char[fsize + 1];
    s.read(fdata, fsize);

    // Adds string terminator at the position next to the last
    // read character
    fdata[s.gcount()] = '\0';

    data.assign(fdata);
    delete[] fdata;
    ret_val = true;

    s.close();
  }

  return ret_val;
}

std::string SHCORE_PUBLIC get_text_file(const std::string &path) {
  std::string data;
  if (!load_text_file(path, data)) {
    throw std::runtime_error(path + ": " + errno_to_string(errno));
  }
  return data;
}

/*
 * Deletes a file in a cross platform manner. If file removal file,
 * an exception is thrown.
 *
 * If file does not exist, and quiet is true, fails silently.
 */
void SHCORE_PUBLIC delete_file(const std::string &filename, bool quiet) {
  if (quiet && !shcore::file_exists(filename)) return;
#ifdef WIN32
  if (!DeleteFile(filename.c_str()))
    throw std::runtime_error(
        str_format("Error when deleting file  %s exists: %s", filename.c_str(),
                   shcore::get_last_error().c_str()));
#else
  if (remove(filename.c_str()))
    throw std::runtime_error(
        str_format("Error when deleting file  %s exists: %s", filename.c_str(),
                   shcore::get_last_error().c_str()));
#endif
}

/*
 * Returns the HOME path (~ in Unix or %AppData%\ in Windows).
 */
std::string get_home_dir() {
  std::string path_separator;
  std::string path;
  std::vector<std::string> to_append;

#ifdef WIN32
  path_separator = "\\";
  char szPath[MAX_PATH];
  HRESULT hr;

  if (SUCCEEDED(hr = SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, szPath)))
    path.assign(szPath);
  else {
    _com_error err(hr);
    throw std::runtime_error(
        str_format("Error when gathering the PROFILE folder path: %s",
                   err.ErrorMessage()));
  }
#else
  path_separator = "/";
  char *cpath = std::getenv("HOME");

  if (cpath != NULL) path.assign(cpath);
#endif

  // Up to know the path must exist since it was retrieved from OS standard
  // means we need to guarantee the rest of the path exists
  if (!path.empty()) {
    for (size_t index = 0; index < to_append.size(); index++) {
      path += path_separator + to_append[index];
      ensure_dir_exists(path);
    }

    path += path_separator;
  }

  return path;
}

bool create_file(const std::string &name, const std::string &content) {
  bool ret_val = false;
  std::ofstream file(name, std::ofstream::out | std::ofstream::trunc);

  if (file.is_open()) {
    file << content;
    file.close();
    ret_val = true;
  }

  return ret_val;
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
  if (rename(from.c_str(), to.c_str()) < 0) {
    throw std::runtime_error(
        shcore::str_format("Could not rename '%s' to '%s': %s", from.c_str(),
                           to.c_str(), strerror(errno)));
  }
}

void copy_dir(const std::string &from, const std::string &to) {
  create_directory(to);
  iterdir(from, [from, to](const std::string &name) {
    try {
      if (is_folder(path::join_path(from, name)))
        copy_dir(path::join_path(from, name), path::join_path(to, name));
      else
        copy_file(path::join_path(from, name), path::join_path(to, name));
    } catch (std::runtime_error &e) {
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

  if (file_exists(filename)) {
    // Use openmode 'out' to open the file for writing
    ofs.open(filename.c_str(), std::ofstream::out | std::ofstream::app);
    std::string error = shcore::errno_to_string(errno);
    // If it could open the file, it's writable
    if (ofs.is_open()) {
      ofs.close();
    } else {
      throw std::runtime_error(error);
    }
  } else {
    // Use openmode 'out' to open the file for writing
    ofs.open(filename.c_str(), std::ofstream::out | std::ofstream::app);
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

}  // namespace shcore
