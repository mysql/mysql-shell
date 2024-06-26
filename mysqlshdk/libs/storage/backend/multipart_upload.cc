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

#include "mysqlshdk/libs/storage/backend/multipart_upload.h"

#include "mysqlshdk/libs/rest/error.h"
#include "mysqlshdk/libs/rest/error_codes.h"
#include "mysqlshdk/libs/rest/response.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlshdk {
namespace storage {
namespace backend {

Multipart_uploader::~Multipart_uploader() {
  // if there's a pending multipart upload (in other words multipart upload was
  // started, but close() was not called before writer has been destroyed),
  // attempt to cancel it
  maybe_abort("unexpected inner state");
}

void Multipart_uploader::append(const void *data, std::size_t length) {
  auto to_send = m_buffer.size() + length;

  // Initializes the multipart as soon as `m_part_size` data is provided
  if (!m_multipart_upload->is_active() && to_send > m_part_size) {
    try {
      m_multipart_upload->create();
    } catch (const rest::Response_error &error) {
      throw rest::to_exception(error);
    }
  }

  std::size_t incoming_offset = 0;
  const auto incoming = reinterpret_cast<const char *>(data);

  // This loops handles the upload of N number of chunks of size `m_part_size`,
  // including the buffered data and the incoming data
  while (to_send > m_part_size) {
    const char *part = nullptr;

    if (!m_buffer.empty()) {
      // BUFFERED DATA: fills the buffer and sends it
      const auto buffer_space = m_part_size - m_buffer.size();
      m_buffer.append(incoming + incoming_offset, buffer_space);

      part = m_buffer.data();
      incoming_offset += buffer_space;
    } else {
      // NO BUFFERED DATA: sends the data directly from the incoming buffer
      part = incoming + incoming_offset;
      incoming_offset += m_part_size;
    }

    try {
      m_multipart_upload->upload_part(part, m_part_size);
    } catch (const rest::Response_error &error) {
      maybe_abort("failure uploading part", error.format());
      throw rest::to_exception(error);
    }

    m_buffer.clear();
    to_send -= m_part_size;
  }

  // REMAINING DATA: gets buffered again
  if (const auto remaining_input = length - incoming_offset) {
    m_buffer.append(incoming + incoming_offset, remaining_input);
  }
}

void Multipart_uploader::commit() {
  try {
    if (m_multipart_upload->is_active()) {
      // MULTIPART UPLOAD STARTED: Sends last part if any and commits the upload
      if (!m_buffer.empty()) {
        m_multipart_upload->upload_part(m_buffer.data(), m_buffer.size());
      }

      m_multipart_upload->commit();
      m_multipart_upload->cleanup();
    } else {
      // NO UPLOAD STARTED: Sends whatever buffered data in a single upload
      // BUG#34891382: need to upload the object, even if it's empty
      m_multipart_upload->upload(m_buffer.data(), m_buffer.size());
    }

    m_buffer.clear();
  } catch (const rest::Response_error &error) {
    maybe_abort("failure completing the upload", error.format());
    throw rest::to_exception(error);
  } catch (const rest::Connection_error &error) {
    throw shcore::Exception::runtime_error(error.what());
  }
}

void Multipart_uploader::maybe_abort(const char *context,
                                     const std::string &error) {
  if (!m_multipart_upload->is_active()) {
    return;
  }

  std::string maybe_error;

  if (!error.empty()) {
    maybe_error = ", error: ";
    maybe_error += error;
  }

  log_info("Cancelling multipart upload after %s%s\nobject: %s\nupload id: %s",
           context, maybe_error.c_str(), m_multipart_upload->name().c_str(),
           m_multipart_upload->upload_id().c_str());

  try {
    // always call cleanup(), if abort() throws it's not going to be attempted
    // again
    shcore::on_leave_scope cleanup([this]() { m_multipart_upload->cleanup(); });
    m_multipart_upload->abort();
  } catch (const rest::Response_error &inner_error) {
    log_error(
        "Error cancelling multipart upload after %s, error: %s\nobject: "
        "%s\nupload id: %s",
        context, inner_error.format().c_str(),
        m_multipart_upload->name().c_str(),
        m_multipart_upload->upload_id().c_str());
  }
}

}  // namespace backend
}  // namespace storage
}  // namespace mysqlshdk
