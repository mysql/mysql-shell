# Copyright (c) 2026, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation. The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

"""Functional ELF/copy checks, run directly with Python 3 on Linux."""

import argparse
import hashlib
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest


def run(*args):
    result = subprocess.run(args, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    if result.returncode:
        raise AssertionError(f"Command failed ({result.returncode}): {args}\n{result.stdout}")
    return result.stdout


def snapshot(path):
    info = path.stat()
    return (hashlib.sha256(path.read_bytes()).hexdigest(), info.st_size,
            info.st_mtime_ns, info.st_ctime_ns)


class LinuxRpathTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="mysqlsh rpath ")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        source = self.root / "main.c"
        source.write_text("int main(void) { return 0; }\n")
        self.binary = self.root / "libfixture.so"
        run(OPTIONS.cc, "-fPIE", "-pie", str(source), "-o", str(self.binary))
        run(OPTIONS.patchelf, "--remove-rpath", str(self.binary))
        self.baseline = self.root / "baseline"
        shutil.copy2(self.binary, self.baseline)
        self.missing_tool = str(self.root / "missing tool")

    def prepare_path(self, value, force=False):
        args = [OPTIONS.patchelf]
        if force:
            args.append("--force-rpath")
        run(*args, "--set-rpath", value, str(self.binary))

    def tags(self, binary=None):
        output = run(OPTIONS.readelf, "--dynamic", "--wide",
                     str(binary or self.binary))
        return re.findall(r"^\s*0x[0-9a-fA-F]+\s+\((RPATH|RUNPATH)\).*?\[(.*)\]",
                          output, re.MULTILINE)

    def use_executable(self, name="python-fixture"):
        self.binary = self.root / name
        shutil.copy2(self.baseline, self.binary)

    def apply(self, **variables):
        driver = self.root / "process.cmake"
        driver.write_text('''
cmake_minimum_required(VERSION 3.14.6)
include("${exeutils}")
set(CONFIG_BINARY_DIR "${root}")
set(INSTALL_LIBDIR "../lib")
set(PYTHON_EXECUTABLE "${root}/python-fixture")
get_linux_rpath_command(TARGET_BINARY "${copied_binary}" SOURCE_BINARY "${binary}"
  DESTINATION "${destination}" OUT_COMMAND command)
if(NOT "${binary}" STREQUAL "${copied_binary}")
  get_filename_component(copy_dir "${copied_binary}" DIRECTORY)
  file(COPY "${binary}" DESTINATION "${copy_dir}")
endif()
if(command)
  execute_process(COMMAND ${command} RESULT_VARIABLE result)
  if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "RPATH command failed: ${result}")
  endif()
endif()
''')
        values = dict(binary=str(self.binary), copied_binary=str(self.binary),
                      destination=".", root=str(self.root),
                      exeutils=str(OPTIONS.exeutils),
                      CMAKE_READELF=OPTIONS.readelf,
                      PATCHELF_EXECUTABLE=OPTIONS.patchelf)
        values.update(variables)
        return subprocess.run(
            [OPTIONS.cmake, *[f"-D{k}={v}" for k, v in values.items()],
             "-P", str(driver)], text=True, stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT)

    def assert_success(self, result):
        self.assertEqual(0, result.returncode, result.stdout)

    def test_remove_absent_is_untouched(self):
        before = snapshot(self.binary)
        self.assert_success(self.apply(PATCHELF_EXECUTABLE=self.missing_tool))
        self.assertEqual(before, snapshot(self.binary))

    def test_inspects_source_and_patches_explicit_copy(self):
        self.prepare_path("/original/library")
        before = snapshot(self.binary)
        copied = self.root / "copied binaries" / self.binary.name
        self.assert_success(self.apply(copied_binary=str(copied),
                                       destination="copied binaries"))
        self.assertEqual(before, snapshot(self.binary))
        self.assertEqual([], self.tags(copied))

    def test_remove_empty_and_populated_tags(self):
        for force in (False, True):
            for value in ("", "/original/build/library"):
                with self.subTest(force=force, value=value):
                    shutil.copy2(self.baseline, self.binary)
                    # Setting an absent path to empty can itself be a no-op.
                    self.prepare_path("/initial", force)
                    self.prepare_path(value, force)
                    self.assertTrue(self.tags())
                    self.assert_success(self.apply())
                    self.assertEqual([], self.tags())
                    run(str(self.binary))

    def test_system_openssl_helpers_remove_existing_paths(self):
        for name in ("mysqlbinlog", "mysql_config_editor"):
            for force in (False, True):
                for value in (None, "", ":" * 30, "/original/build/helper"):
                    with self.subTest(name=name, force=force, value=value):
                        self.use_executable(name)
                        if value is not None:
                            self.prepare_path("/initial", force)
                            self.prepare_path(value, force)
                            self.assertEqual([("RPATH" if force else "RUNPATH", value)],
                                             self.tags())
                        source = snapshot(self.binary)
                        copied = self.root / "copied helpers" / name
                        if copied.exists():
                            copied.unlink()
                        self.assert_success(self.apply(copied_binary=str(copied),
                                                       BUNDLED_OPENSSL="OFF"))
                        self.assertEqual([], self.tags(copied))
                        self.assertEqual(source, snapshot(self.binary))
                        if value is None:
                            self.assertEqual(source[0], snapshot(copied)[0])
                        run(str(copied))

                        before = snapshot(copied)
                        self.assert_success(self.apply(binary=str(copied),
                                                       copied_binary=str(copied),
                                                       BUNDLED_OPENSSL="OFF",
                                                       PATCHELF_EXECUTABLE=self.missing_tool))
                        self.assertEqual(before, snapshot(copied))

    def test_needed_name_is_not_a_path_tag(self):
        run(OPTIONS.patchelf, "--add-needed", "lib(RPATH).so", str(self.binary))
        before = snapshot(self.binary)
        self.assert_success(self.apply(PATCHELF_EXECUTABLE=self.missing_tool))
        self.assertEqual(before, snapshot(self.binary))

    def test_set_without_path(self):
        self.use_executable()
        self.assert_success(self.apply())
        self.assertEqual([("RPATH", "$ORIGIN/../lib")], self.tags())
        run(str(self.binary))

    def test_set_over_short_and_long_paths(self):
        self.use_executable()
        for value in ("x", "/original/build/" + "long-directory/" * 20):
            with self.subTest(value=value):
                shutil.copy2(self.baseline, self.binary)
                self.prepare_path(value)
                self.assert_success(self.apply())
                self.assertEqual([("RPATH", "$ORIGIN/../lib")], self.tags())
                run(str(self.binary))

    def test_equal_runpath_is_converted(self):
        self.use_executable()
        self.prepare_path("$ORIGIN/../lib")
        self.assertEqual([("RUNPATH", "$ORIGIN/../lib")], self.tags())
        self.assert_success(self.apply())
        self.assertEqual([("RPATH", "$ORIGIN/../lib")], self.tags())

    def test_equal_rpath_is_untouched(self):
        self.use_executable()
        self.prepare_path("$ORIGIN/../lib", force=True)
        before = snapshot(self.binary)
        self.assert_success(self.apply(PATCHELF_EXECUTABLE=self.missing_tool))
        self.assertEqual(before, snapshot(self.binary))

    def test_inspection_failure_is_fatal(self):
        invalid = self.root / "invalid.so"
        invalid.write_text("not an ELF file")
        for binary in (self.root / "absent.so", invalid):
            with self.subTest(binary=binary):
                self.assertNotEqual(0, self.apply(binary=str(binary)).returncode)
        self.assertNotEqual(0, self.apply(CMAKE_READELF=self.missing_tool).returncode)

    def test_patching_failure_is_fatal(self):
        self.prepare_path("/original")
        self.assertNotEqual(0, self.apply(PATCHELF_EXECUTABLE=self.missing_tool).returncode)
        self.use_executable()
        self.assertNotEqual(0, self.apply(PATCHELF_EXECUTABLE=self.missing_tool).returncode)

    def test_read_paths_distinguishes_empty_and_absent(self):
        driver = self.root / "read.cmake"
        driver.write_text('''
cmake_minimum_required(VERSION 3.14.6)
include("${exeutils}")
set(current_rpath stale)
set(current_runpath stale)
get_binary_rpaths(BINARY "${binary}" OUT_RPATH current_rpath OUT_RUNPATH current_runpath)
foreach(tag rpath runpath)
  if(DEFINED current_${tag})
    file(WRITE "${root}/${tag}" "${current_${tag}}")
  endif()
endforeach()
''')
        for force in (False, True):
            for value in (None, "", "/original/path with spaces"):
                with self.subTest(force=force, value=value):
                    shutil.copy2(self.baseline, self.binary)
                    if value is not None:
                        self.prepare_path("/initial", force)
                        self.prepare_path(value, force)
                    before = snapshot(self.binary)
                    for tag in ("rpath", "runpath"):
                        if (self.root / tag).exists():
                            (self.root / tag).unlink()
                    run(OPTIONS.cmake, f"-Dexeutils={OPTIONS.exeutils}",
                        f"-Droot={self.root}", f"-Dbinary={self.binary}",
                        f"-DCMAKE_READELF={OPTIONS.readelf}", "-P", str(driver))
                    tags = {tag: (self.root / tag).read_text()
                            for tag in ("rpath", "runpath") if (self.root / tag).exists()}
                    expected = {} if value is None else {"rpath" if force else "runpath": value}
                    self.assertEqual(expected, tags)
                    self.assertEqual(before, snapshot(self.binary))

    def test_source_package_skips_missing_binaries(self):
        project = self.root / "source package"
        project.mkdir()
        (project / "CMakeLists.txt").write_text('''
cmake_minimum_required(VERSION 3.14.6)
project(source_package_fixture NONE)
set(LINUX TRUE)
set(BUILD_SOURCE_PACKAGE ON)
set(CONFIG_BINARY_DIR "${CMAKE_BINARY_DIR}")
set(INSTALL_LIBDIR lib/mysqlsh)
include("${exeutils}")
add_custom_target(bundle ALL)
install_bundled_binaries(DESCRIPTION helpers
  BINARIES "${CMAKE_SOURCE_DIR}/missing/mysqlbinlog"
           "${CMAKE_SOURCE_DIR}/missing/mysql_config_editor"
  DESTINATION libexec/mysqlsh TARGET bundle)
install_bundled_binaries(DESCRIPTION libraries
  BINARIES "${CMAKE_SOURCE_DIR}/missing/libfixture.so"
  DESTINATION lib/mysqlsh TARGET bundle)
get_property(targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
if(NOT "${targets}" STREQUAL "bundle")
  message(FATAL_ERROR "Unexpected binary copy targets: ${targets}")
endif()
get_property(dependencies DIRECTORY PROPERTY CMAKE_CONFIGURE_DEPENDS)
if(dependencies)
  message(FATAL_ERROR "Unexpected binary configuration dependencies: ${dependencies}")
endif()
''')
        for generator in ("Ninja", "Unix Makefiles"):
            with self.subTest(generator=generator):
                build = self.root / generator
                output = run(OPTIONS.cmake, "-S", str(project), "-B", str(build),
                             "-G", generator, f"-Dexeutils={OPTIONS.exeutils}",
                             f"-DCMAKE_READELF={OPTIONS.readelf}")
                self.assertNotIn("Bundling", output)
                run(OPTIONS.cmake, "--build", str(build))
                prefix = self.root / (generator + " install")
                run(OPTIONS.cmake, "--install", str(build), "--prefix", str(prefix))
                self.assertFalse(prefix.exists())

    def test_copy_policy_with_both_generators(self):
        project = self.root / "source tree"
        project.mkdir()
        (project / "cmake").mkdir()
        for name in ("exeutils.cmake", "copy_file.cmake"):
            shutil.copy2(OPTIONS.exeutils.parent / name, project / "cmake" / name)
        lib = project / "libfixture.so.1"
        run(OPTIONS.cc, "-shared", "-fPIC", str(self.root / "main.c"), "-o", str(lib))
        run(OPTIONS.patchelf, "--set-rpath", "/original/library", str(lib))
        (project / "libfixture.so").symlink_to(lib.name)
        shutil.copy2(self.binary, project / "helper")
        shutil.copy2(self.binary, project / "python-fixture")
        # Server build-tree helpers can contain only CMake's RPATH padding.
        run(OPTIONS.patchelf, "--set-rpath", ":" * 30, str(project / "helper"))
        sources = {name: snapshot(project / name) for name in
                   ("libfixture.so.1", "helper", "python-fixture")}
        (project / "CMakeLists.txt").write_text('''
cmake_minimum_required(VERSION 3.14.6)
project(copy_fixture NONE)
set(LINUX TRUE)
set(CONFIG_BINARY_DIR "${CMAKE_BINARY_DIR}")
set(INSTALL_LIBDIR lib/mysqlsh)
set(PYTHON_EXECUTABLE "${CMAKE_SOURCE_DIR}/python-fixture")
include(cmake/exeutils.cmake)
add_custom_target(bundle ALL)
install_bundled_binaries(DESCRIPTION libraries
  BINARIES "${CMAKE_SOURCE_DIR}/libfixture.so.1" "${CMAKE_SOURCE_DIR}/libfixture.so"
  DESTINATION lib/mysqlsh TARGET bundle)
install_bundled_binaries(DESCRIPTION helper BINARIES "${CMAKE_SOURCE_DIR}/helper"
  DESTINATION libexec/mysqlsh TARGET bundle)
install_bundled_binaries(DESCRIPTION python BINARIES "${PYTHON_EXECUTABLE}"
  DESTINATION lib/mysqlsh/bin TARGET bundle)
''')
        for generator in ("Ninja", "Unix Makefiles"):
            for bundled_ssl in ("ON", "OFF"):
                with self.subTest(generator=generator, bundled_ssl=bundled_ssl):
                    build = self.root / (generator + " " + bundled_ssl)
                    run(OPTIONS.cmake, "-S", str(project), "-B", str(build),
                        "-G", generator, f"-DBUNDLED_OPENSSL={bundled_ssl}",
                        f"-DPATCHELF_EXECUTABLE={OPTIONS.patchelf}",
                        "-DPATCHELF_PAGE_SIZE_ARGS=--page-size;4096",
                        f"-DCMAKE_READELF={OPTIONS.readelf}")
                    run(OPTIONS.cmake, "--build", str(build))
                    self.assertEqual([], self.tags(build / "lib/mysqlsh/libfixture.so.1"))
                    self.assertTrue((build / "lib/mysqlsh/libfixture.so").is_symlink())
                    self.assertEqual([("RPATH", "$ORIGIN/..")],
                                     self.tags(build / "lib/mysqlsh/bin/python-fixture"))
                    helper = build / "libexec/mysqlsh/helper"
                    expected = ([("RPATH", "$ORIGIN/../../lib/mysqlsh")]
                                if bundled_ssl == "ON"
                                else [])
                    self.assertEqual(expected, self.tags(helper))
                    before = snapshot(helper)
                    run(OPTIONS.cmake, "--build", str(build))
                    self.assertEqual(before, snapshot(helper))
                    for name, before in sources.items():
                        self.assertEqual(before, snapshot(project / name))

                    # Source tag changes must refresh configuration automatically.
                    copied_lib = build / "lib/mysqlsh/libfixture.so.1"
                    run(OPTIONS.patchelf, "--remove-rpath", str(lib))
                    sources["libfixture.so.1"] = snapshot(lib)
                    run(OPTIONS.cmake, "--build", str(build))
                    self.assertEqual(sources["libfixture.so.1"][0], snapshot(copied_lib)[0])
                    run(OPTIONS.patchelf, "--set-rpath", "/changed", str(lib))
                    sources["libfixture.so.1"] = snapshot(lib)
                    run(OPTIONS.cmake, "--build", str(build))
                    self.assertEqual([], self.tags(copied_lib))
                    for name, before in sources.items():
                        self.assertEqual(before, snapshot(project / name))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("cmake", "cc", "readelf", "patchelf"):
        executable = shutil.which(name)
        parser.add_argument("--" + name, default=executable,
                            required=not executable)
    parser.add_argument("--exeutils", type=Path,
                        default=Path(__file__).resolve().parents[1] / "exeutils.cmake")
    OPTIONS, remaining = parser.parse_known_args()
    OPTIONS.exeutils = OPTIONS.exeutils.resolve()
    unittest.main(argv=[__file__, *remaining])
