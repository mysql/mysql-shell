/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "utils_path.h"

namespace shcore {

/*
 * Join two or more pathname components, inserting '\\' as needed.
 * If any component is an absolute path, all previous path components
 * will be discarded.  An empty last part will result in a path that
 * ends with a separator.
 * @param components vector of strings with the paths to join
 *
 * @return the concatenation of all paths with exactly one directory separator
 *         following each non-empty part except the last, meaning that the
 *         result will only end in a separator if the last part is empty.
 */
std::string SHCORE_PUBLIC join_path(const std::vector<std::string>
                                    &components) {
  std::string result, result_drive, result_path;
  std::string p_drive, p_path, lp_drive, lresult_drive;
  if (!components.empty())
    std::tie(result_drive, result_path) = splitdrive(components.at(0));
  else
    return "";

  for (size_t i = 1; i < components.size(); ++i) {
    std::tie(p_drive, p_path) = splitdrive(components.at(i));
    if (!p_path.empty() && (p_path.front() == '\\' || p_path.front() == '/')) {
      // second path is absolute, so forget about first
      if (!p_drive.empty() || result_drive.empty())
        result_drive = p_drive;
      result_path = p_path;
      continue;
    } else if (!p_drive.empty() && p_drive != result_drive) {
      // Convert drives to lower case.
      // TODO(nelson): This will fail for non unicode characters
      lp_drive.assign(p_drive);
      lresult_drive.assign(result_drive);
      std::transform(p_drive.begin(), p_drive.end(),
                     lp_drive.begin(), ::tolower);
      std::transform(result_drive.begin(), result_drive.end(),
                     lresult_drive.begin(), ::tolower);

      if (lp_drive != lresult_drive) {
        // Different drives, ignore the first path entirely
        result_drive = p_drive;
        result_path = p_path;
        continue;
      }
      // same drive but in different case
      result_drive = p_drive;
    }
    // Second path is relative to the first
    if (!result_path.empty() && result_path.back() != '\\' &&
        result_path.back() != '/')
      result_path += '\\';
    result_path += p_path;
  }
  // Add separator between UNC and non-absolute path
  if (!result_path.empty() && result_path.front() != '\\' &&
      result_path.front() != '/' && !result_drive.empty() &&
      result_drive.back() != ':')
    return result_drive + "\\" + result_path;
  return result_drive + result_path;
}

/*
 * Split a pathname into drive/UNC sharepoint and relative path specifiers.
 *  Returns an std::pair of std::strings (drive_or_unc, path); either part may
 *  be empty.
 *
 *  If you assign
 *      result = splitdrive(p)
 *  It is always true that:
 *      result.first + result.second == p
 *
 *  If the path contained a drive letter, drive_or_unc will contain everything
 *  up to and including the colon.  e.g. splitdrive("c:/dir") returns
 *  ("c:", "/dir")
 *
 *  If the path contained a UNC path, the drive_or_unc will contain the host
 *  name and share up to but not including the fourth directory separator
 *  character. e.g. splitdrive("//host/computer/dir") returns
 *  ("//host/computer", "/dir")
 *
 *  Paths cannot contain both a drive letter and a UNC path.
 * @param string with the path name to be split
 *
 * @return a pair (drive, tail) where the drive is either a drive specification
 *         or empty string. On Unix systems drive will always be empty string.
 *
 */
std::pair<std::string, std::string> SHCORE_PUBLIC splitdrive(
    const std::string &path) {
  if (path.size() > 1) {
    std::string norm_path = path;
    // replace all '/' with '\' character
    std::replace(norm_path.begin(), norm_path.end(), '/', '\\');
    std::string first_two_chars, third_char;
    try {
      first_two_chars = norm_path.substr(0, 2);
    } catch (std::out_of_range &e) {
      first_two_chars = "";
    }
    try {
      third_char = norm_path.substr(2, 1);
    } catch (std::out_of_range &e) {
      third_char = "";
    }
    if (first_two_chars == std::string(2, '\\') && third_char != "\\") {
      // it is a UNC path:
      auto index = norm_path.find('\\', 2);
      if (index == std::string::npos) {
        return std::make_pair("", path);
      }
      auto index2 = norm_path.find('\\', index+1);
      // a UNC path can't have two slashes in a row
      // (after the initial two)
      if (index2 == index + 1)
        return std::make_pair("", path);
      if (index2 == std::string::npos)
        index2 = path.size();
      return std::make_pair(path.substr(0, index2), path.substr(index2));
    }
    if (norm_path.at(1) == ':')
      return std::make_pair(path.substr(0, 2), path.substr(2));
  }
  return std::make_pair("", path);
}

}  // namespace shcore