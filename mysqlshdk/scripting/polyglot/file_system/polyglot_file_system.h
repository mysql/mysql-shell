/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_SCRIPTING_POLYGLOT_FILE_SYSTEM_POLYGLOT_FILE_SYSTEM_H_
#define MYSQLSHDK_SCRIPTING_POLYGLOT_FILE_SYSTEM_POLYGLOT_FILE_SYSTEM_H_

#include <memory>
#include <string>

#include "mysqlshdk/scripting/polyglot/file_system/file_system_exceptions.h"

namespace shcore {
namespace polyglot {

class ISeekable_channel {
 public:
  virtual ~ISeekable_channel() = default;
  /**
   * Tells whether or not this channel is open.
   */
  virtual bool is_open() = 0;

  /**
   * Closes this channel.
   *
   * <p> After a channel is closed, any further attempt to invoke I/O
   * operations upon it will cause a {@link ClosedChannelException} to be
   * thrown.
   *
   * <p> If this channel is already closed then invoking this method has no
   * effect.
   *
   * <p> This method may be invoked at any time.  If some other thread has
   * already invoked it, however, then another invocation will block until
   * the first invocation is complete, after which it will return without
   * effect. </p>
   *
   * @throws  IOException  If an I/O error occurs
   */
  virtual void close() = 0;

  /**
   * Reads a sequence of bytes from this channel into the given buffer.
   *
   * <p> Bytes are read starting at this channel's current position, and
   * then the position is updated with the number of bytes actually read.
   * Otherwise this method behaves exactly as specified in the {@link
   * ReadableByteChannel} interface.
   *
   * @throws  ClosedChannelException      {@inheritDoc}
   * @throws  AsynchronousCloseException  {@inheritDoc}
   * @throws  ClosedByInterruptException  {@inheritDoc}
   * @throws  NonReadableChannelException {@inheritDoc}
   */
  virtual int64_t read(void *buffer, size_t size) = 0;

  /**
   * Writes a sequence of bytes to this channel from the given buffer.
   *
   * <p> Bytes are written starting at this channel's current position, unless
   * the channel is connected to an entity such as a file that is opened with
   * the {@link java.nio.file.StandardOpenOption#APPEND APPEND} option, in
   * which case the position is first advanced to the end. The entity to which
   * the channel is connected is grown, if necessary, to accommodate the
   * written bytes, and then the position is updated with the number of bytes
   * actually written. Otherwise this method behaves exactly as specified by
   * the {@link WritableByteChannel} interface.
   *
   * @throws  ClosedChannelException      {@inheritDoc}
   * @throws  AsynchronousCloseException  {@inheritDoc}
   * @throws  ClosedByInterruptException  {@inheritDoc}
   * @throws  NonWritableChannelException {@inheritDoc}
   */
  virtual int64_t write(const char *buffer, size_t size) = 0;

  /**
   * Returns this channel's position.
   *
   * @return  This channel's position,
   *          a non-negative integer counting the number of bytes
   *          from the beginning of the entity to the current position
   *
   * @throws  ClosedChannelException
   *          If this channel is closed
   * @throws  IOException
   *          If some other I/O error occurs
   */
  virtual int64_t position() = 0;

  /**
   * Sets this channel's position.
   *
   * <p> Setting the position to a value that is greater than the current size
   * is legal but does not change the size of the entity.  A later attempt to
   * read bytes at such a position will immediately return an end-of-file
   * indication.  A later attempt to write bytes at such a position will cause
   * the entity to grow to accommodate the new bytes; the values of any bytes
   * between the previous end-of-file and the newly-written bytes are
   * unspecified.
   *
   * <p> Setting the channel's position is not recommended when connected to
   * an entity, typically a file, that is opened with the {@link
   * java.nio.file.StandardOpenOption#APPEND APPEND} option. When opened for
   * append, the position is first advanced to the end before writing.
   *
   * @param  newPosition
   *         The new position, a non-negative integer counting
   *         the number of bytes from the beginning of the entity
   *
   * @return  This channel
   *
   * @throws  ClosedChannelException
   *          If this channel is closed
   * @throws  IllegalArgumentException
   *          If the new position is negative
   * @throws  IOException
   *          If some other I/O error occurs
   */
  virtual ISeekable_channel &set_position(int64_t new_position) = 0;

  /**
   * Returns the current size of entity to which this channel is connected.
   *
   * @return  The current size, measured in bytes
   *
   * @throws  ClosedChannelException
   *          If this channel is closed
   * @throws  IOException
   *          If some other I/O error occurs
   */
  virtual int64_t size() = 0;

  /**
   * Truncates the entity, to which this channel is connected, to the given
   * size.
   *
   * <p> If the given size is less than the current size then the entity is
   * truncated, discarding any bytes beyond the new end. If the given size is
   * greater than or equal to the current size then the entity is not modified.
   * In either case, if the current position is greater than the given size
   * then it is set to that size.
   *
   * <p> An implementation of this interface may prohibit truncation when
   * connected to an entity, typically a file, opened with the {@link
   * java.nio.file.StandardOpenOption#APPEND APPEND} option.
   *
   * @param  size
   *         The new size, a non-negative byte count
   *
   * @return  This channel
   *
   * @throws  NonWritableChannelException
   *          If this channel was not opened for writing
   * @throws  ClosedChannelException
   *          If this channel is closed
   * @throws  IllegalArgumentException
   *          If the new size is negative
   * @throws  IOException
   *          If some other I/O error occurs
   */
  virtual ISeekable_channel &truncate(int64_t size) = 0;
};

class IDirectory_stream {};

class IFile_system {
 public:
  virtual ~IFile_system() = default;

  /**
   * Parses a path from an {@link URI}.
   *
   * @param uri the {@link URI} to be converted to {@link Path}
   * @return the {@link Path} representing given {@link URI}
   * @throws UnsupportedOperationException when {@link URI} scheme is not
   * supported
   * @throws IllegalArgumentException if preconditions on the {@code uri} do not
   * hold. The format of the URI is {@link FileSystem} specific.
   */
  virtual std::string parse_uri_path(const std::string &uri) = 0;

  /**
   * Parses a path from a {@link String}. This method is called only on the
   * {@link FileSystem} with {@code file} scheme.
   *
   * @param path the string path to be converted to {@link Path}
   * @return the {@link Path}
   * @throws UnsupportedOperationException when the {@link FileSystem} supports
   * only {@link URI}
   * @throws IllegalArgumentException if the {@code path} string cannot be
   * converted to a
   *             {@link Path}
   */
  virtual std::string parse_string_path(const std::string &path) = 0;

  /**
   * Checks existence and accessibility of a file.
   *
   * @param path the path to the file to check
   * param modes the access modes to check, possibly empty to check existence
   * only.
   * param linkOptions options determining how the symbolic links should be
   * handled
   * @throws NoSuchFileException if the file denoted by the path does not exist
   * @throws IOException in case of IO error
   * @throws SecurityException if this {@link FileSystem} denied the operation
   */
  virtual void check_access(const std::string &path, int64_t flags) = 0;

  /**
   * Creates a directory.
   *
   * param dir the directory to create
   * param attrs the optional attributes to set atomically when creating the
   * directory
   * @throws FileAlreadyExistsException if a file on given path already exists
   * @throws IOException in case of IO error
   * @throws UnsupportedOperationException if the attributes contain an
   * attribute which cannot be set atomically
   * @throws SecurityException if this {@link FileSystem} denied the operation
   */
  virtual void create_directory(const std::string &path) = 0;

  /**
   * Deletes a file.
   *
   * @param path the path to the file to delete
   * @throws NoSuchFileException if a file on given path does not exist
   * @throws DirectoryNotEmptyException if the path denotes a non empty
   * directory
   * @throws IOException in case of IO error
   * @throws SecurityException if this {@link FileSystem} denied the operation
   */
  virtual void remove(const std::string &path) = 0;

  /**
   * Opens or creates a file returning a {@link SeekableByteChannel} to access
   * the file content.
   *
   * @param path the path to the file to open
   * @param options the options specifying how the file should be opened
   * @param attrs the optional attributes to set atomically when creating the
   * new file
   * @return the created {@link SeekableByteChannel}
   * @throws FileAlreadyExistsException if {@link StandardOpenOption#CREATE_NEW}
   * option is set and a file already exists on given path
   * @throws IOException in case of IO error
   * @throws UnsupportedOperationException if the attributes contain an
   * attribute which cannot be set atomically
   * @throws IllegalArgumentException in case of invalid options combination
   * @throws SecurityException if this {@link FileSystem} denied the operation
   */
  virtual std::shared_ptr<ISeekable_channel> new_byte_channel(
      const std::string &path) = 0;

  /**
   * Returns directory entries.
   *
   * param dir the path to the directory to iterate entries for
   * param filter the filter
   * @return the new {@link DirectoryStream}
   * @throws NotDirectoryException when given path does not denote a directory
   * @throws IOException in case of IO error
   * @throws SecurityException if this {@link FileSystem} denied the operation
   */
  virtual std::shared_ptr<IDirectory_stream> new_directory_stream(
      const std::string &path) = 0;

  /**
   * Resolves given path to an absolute path.
   *
   * @param path the path to resolve, may be a non normalized path
   * @return an absolute {@link Path}
   * @throws SecurityException if this {@link FileSystem} denied the operation
   */
  virtual std::string to_absolute_path(const std::string &path) = 0;

  /**
   * Returns the real (canonical) path of an existing file.
   *
   * @param path the path to resolve, may be a non normalized path
   * param linkOptions options determining how the symbolic links should be
   * handled
   * @return an absolute canonical path
   * @throws IOException in case of IO error
   * @throws SecurityException if this {@link FileSystem} denied the operation
   */
  virtual std::string to_real_path(const std::string &path) = 0;
};

}  // namespace polyglot
}  // namespace shcore

#endif  // MYSQLSHDK_SCRIPTING_POLYGLOT_FILE_SYSTEM_POLYGLOT_FILE_SYSTEM_H_
