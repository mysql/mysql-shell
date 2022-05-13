/*
 * Copyright (c) 2019, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_H_
#define MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_H_

#include <memory>
#include <string>

#include "mysqlshdk/libs/storage/idirectory.h"
#include "mysqlshdk/libs/storage/ifile.h"

#include "mysqlshdk/libs/storage/backend/object_storage_bucket.h"

// TODO(rennox): Add handling for read only bucket (public)
// TODO(rennox): Should we support additional content/types on the objects being
// uploaded?

namespace mysqlshdk {
namespace storage {
namespace backend {
namespace object_storage {

class Config;

/**
 * Emulates a directory behaviour in a Bucket.
 */
class Directory : public mysqlshdk::storage::IDirectory {
 public:
  Directory() = delete;

  /**
   * Creates a directory instance which emulates the directory behavior using
   * the following base path:
   *
   *  <bucketName>[/<name>]
   *
   * @param config: the configuration for the bucket.
   * @param name: the directory name.
   *
   * If the name parameter is empty, the emulation will be on the bucket root.
   */
  explicit Directory(const Config_ptr &config, const std::string &name = "");

  Directory(const Directory &other) = delete;
  Directory(Directory &&other) = default;

  Directory &operator=(const Directory &other) = delete;
  Directory &operator=(Directory &&other) = default;

  virtual ~Directory() = default;

  /**
   * Tests for the directory existence.
   */
  bool exists() const override;

  /**
   * Emulates the directory creation.
   */
  void create() override;

  void close() override {}

  /**
   * Returns the full path to the directory.
   */
  Masked_string full_path() const override { return m_name; }

  /**
   * Retrieves a list of files on the directory.
   *
   * NOTE: This function emulates non recursive listing by returning ONLY those
   * object names that reside on the emulated directory but dont have / as part
   * of their name.
   */
  std::unordered_set<File_info> list_files(
      bool hidden_files = false) const override;

  std::unordered_set<File_info> filter_files(
      const std::string &pattern) const override;

  /**
   * Creates a new file handle for for a file contained on this directory.
   *
   * @param name: The name of the file.
   * @param options File backend specific options.
   */
  std::unique_ptr<IFile> file(const std::string &name,
                              const File_options &options = {}) const override;

  bool is_local() const override { return false; }

 protected:
  std::string m_name;
  std::unique_ptr<Bucket> m_bucket;

  // Used to simulate a directory has been created
  bool m_created;
  std::string join_path(const std::string &a,
                        const std::string &b) const override;
};

/**
 * Handle for an Object in a Bucket.
 */
class Object : public mysqlshdk::storage::IFile {
 public:
  Object() = delete;
  /**
   * Creates an handle for an object in the specified bucket.
   *
   * @param config: the name of the bucket where the object is located.
   * @param name: the name of the object to be represented by this handle.
   * @param prefix: a prefix to be appended to the real object name, i.e.
   * containing "folder"
   */
  Object(const Config_ptr &config, const std::string &name,
         const std::string &prefix = "");

  Object(Object &&other) = default;

  Object &operator=(const Object &other) = delete;
  Object &operator=(Object &&other) = delete;
  virtual ~Object() = default;

  /**
   * Opens the object for data operations:
   *
   * @param mode indicates the mode in which the object will be used.
   *
   * Valid modes include:
   *
   * - WRITE: Used to create or overwrite an object.
   * - APPEND: used to continue writing the parts of a multipart object that has
   * not been completed.
   * - READ: Used to read the content of the object.
   */
  void open(mysqlshdk::storage::Mode mode) override;

  /**
   * Verifies if the object has been opened.
   */
  bool is_open() const override;

  int error() const override {
    // this throws exceptions
    return 0;
  }

  /**
   * Finishes the operation being done on the object.
   *
   * If the object was opened in WRITE or APPEND mode this will flush the object
   * data and complete the object.
   */
  void close() override;

  /**
   * Returns the size of the object.
   *
   * If the object was opened in READ mode it returns the total size of the
   * object, otherwise returns the number of bytes written so far.
   */
  size_t file_size() const override;

  /**
   * Returns the full path to the object.
   */
  Masked_string full_path() const override { return m_prefix + m_name; }

  /**
   * Returns the file name of the object within the bucket that contains it.
   */
  std::string filename() const override;

  /**
   * Indicates whether the object already exists in the bucket or not.
   */
  bool exists() const override;

  std::unique_ptr<IDirectory> parent() const override;

  /**
   * Sets an internal offset to the indicated position.
   *
   * @param offset: the position where the internal offset is to be located.
   * @returns the position where the internal offset was located.
   *
   * For READ operations this will cause the next read operation to be done
   * starting on the indicated offset. If the provided offset is bigger than the
   * object size, the offset will be located at the end of the object.
   *
   * For WRITE and APPEND this is a NO OP.
   */
  off64_t seek(off64_t offset) override;

  off64_t tell() const override;

  /**
   * Fills up to length bytes into the buffer starting at the internal offset
   * position.
   *
   * @param buffer: target buffer where the data is to be copied.
   * @param length: amount of bytes to be copied into the buffer.
   *
   * @returns the actual number of bytes copied into the buffer.
   */
  ssize_t read(void *buffer, size_t length) override;

  /**
   * Appends data into the object.
   *
   * @param buffer: buffer containing the data to be appended to the object.
   * @param length: number of bytes to be appended into the object.
   *
   * @returns the number of bytes actually written into the object.
   */
  ssize_t write(const void *buffer, size_t length) override;

  /**
   * Renames this object to the new name.
   */
  void rename(const std::string &new_name) override;

  /**
   * Delets the object.
   */
  void remove() override;

  /**
   * Does NOTHING
   */
  bool flush() override { return true; }

  bool is_local() const override { return false; }

  /**
   * Use this function to custimize the part size for multipart uploads.
   *
   * The default size is 10MiB.
   */
  void set_max_part_size(size_t new_size);

 protected:
  std::string m_name;
  std::string m_prefix;
  std::unique_ptr<Bucket> m_bucket;
  mysqlshdk::utils::nullable<Mode> m_open_mode;
  size_t m_max_part_size;

  /**
   * Base class for the Read and Write Object handlers
   */
  class File_handler {
   public:
    explicit File_handler(Object *owner) : m_object(owner), m_size(0) {}
    virtual ~File_handler() = default;
    size_t size() const { return m_size; }

   protected:
    Object *m_object;
    size_t m_size;
  };

  /**
   * Handler for write operations on an Object
   */
  class Writer : public File_handler {
   public:
    explicit Writer(Object *owner, Multipart_object *object = nullptr);
    ~Writer() override;

    off64_t seek(off64_t offset);
    off64_t tell() const;
    ssize_t write(const void *incoming, size_t length);
    void close();

   private:
    void reset();

    void abort_multipart_upload(const char *context,
                                const std::string &error = {});

    std::string m_buffer;
    bool m_is_multipart;
    Multipart_object m_multipart;
    std::vector<Multipart_object_part> m_parts;
  };

  /**
   * Handler for read operations on an Object
   */
  class Reader : public File_handler {
   public:
    explicit Reader(Object *owner);
    ~Reader() override = default;

    off64_t seek(off64_t offset);
    off64_t tell() const;
    ssize_t read(void *buffer, size_t length);

   private:
    off64_t m_offset;
  };

  std::unique_ptr<Writer> m_writer;
  std::unique_ptr<Reader> m_reader;
};

}  // namespace object_storage
}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_STORAGE_BACKEND_OBJECT_STORAGE_H_
