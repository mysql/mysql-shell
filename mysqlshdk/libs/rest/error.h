/*
 * Copyright (c) 2019, 2023, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_ERROR_H_
#define MYSQLSHDK_LIBS_REST_ERROR_H_

#include <stdexcept>
#include <string>
#include <string_view>

namespace mysqlshdk {
namespace rest {

enum class Error_code {
  OK = 0,
  UNSUPPORTED_PROTOCOL,     /* 1 */
  FAILED_INIT,              /* 2 */
  URL_MALFORMAT,            /* 3 */
  NOT_BUILT_IN,             /* 4 */
  COULDNT_RESOLVE_PROXY,    /* 5 */
  COULDNT_RESOLVE_HOST,     /* 6 */
  COULDNT_CONNECT,          /* 7 */
  WEIRD_SERVER_REPLY,       /* 8 */
  REMOTE_ACCESS_DENIED,     /* 9 */
  FTP_ACCEPT_FAILED,        /* 10 */
  FTP_WEIRD_PASS_REPLY,     /* 11 */
  FTP_ACCEPT_TIMEOUT,       /* 12 */
  FTP_WEIRD_PASV_REPLY,     /* 13 */
  FTP_WEIRD_227_FORMAT,     /* 14 */
  FTP_CANT_GET_HOST,        /* 15 */
  HTTP2,                    /* 16 */
  FTP_COULDNT_SET_TYPE,     /* 17 */
  PARTIAL_FILE,             /* 18 */
  FTP_COULDNT_RETR_FILE,    /* 19 */
  OBSOLETE20,               /* 20 */
  QUOTE_ERROR,              /* 21 */
  HTTP_RETURNED_ERROR,      /* 22 */
  WRITE_ERROR,              /* 23 */
  OBSOLETE24,               /* 24 */
  UPLOAD_FAILED,            /* 25 */
  READ_ERROR,               /* 26 */
  OUT_OF_MEMORY,            /* 27 */
  OPERATION_TIMEDOUT,       /* 28 */
  OBSOLETE29,               /* 29 */
  FTP_PORT_FAILED,          /* 30 */
  FTP_COULDNT_USE_REST,     /* 31 */
  OBSOLETE32,               /* 32 */
  RANGE_ERROR,              /* 33 */
  HTTP_POST_ERROR,          /* 34 */
  SSL_CONNECT_ERROR,        /* 35 */
  BAD_DOWNLOAD_RESUME,      /* 36 */
  FILE_COULDNT_READ_FILE,   /* 37 */
  LDAP_CANNOT_BIND,         /* 38 */
  LDAP_SEARCH_FAILED,       /* 39 */
  OBSOLETE40,               /* 40 */
  FUNCTION_NOT_FOUND,       /* 41 */
  ABORTED_BY_CALLBACK,      /* 42 */
  BAD_FUNCTION_ARGUMENT,    /* 43 */
  OBSOLETE44,               /* 44 */
  INTERFACE_FAILED,         /* 45 */
  OBSOLETE46,               /* 46 */
  TOO_MANY_REDIRECTS,       /* 47 */
  UNKNOWN_OPTION,           /* 48 */
  SETOPT_OPTION_SYNTAX,     /* 49 */
  OBSOLETE50,               /* 50 */
  OBSOLETE51,               /* 51 */
  GOT_NOTHING,              /* 52 */
  SSL_ENGINE_NOTFOUND,      /* 53 */
  SSL_ENGINE_SETFAILED,     /* 54 */
  SEND_ERROR,               /* 55 */
  RECV_ERROR,               /* 56 */
  OBSOLETE57,               /* 57 */
  SSL_CERTPROBLEM,          /* 58 */
  SSL_CIPHER,               /* 59 */
  PEER_FAILED_VERIFICATION, /* 60 */
  BAD_CONTENT_ENCODING,     /* 61 */
  OBSOLETE62,               /* 62 */
  FILESIZE_EXCEEDED,        /* 63 */
  USE_SSL_FAILED,           /* 64 */
  SEND_FAIL_REWIND,         /* 65 */
  SSL_ENGINE_INITFAILED,    /* 66 */
  LOGIN_DENIED,             /* 67 */
  TFTP_NOTFOUND,            /* 68 */
  TFTP_PERM,                /* 69 */
  REMOTE_DISK_FULL,         /* 70 */
  TFTP_ILLEGAL,             /* 71 */
  TFTP_UNKNOWNID,           /* 72 */
  REMOTE_FILE_EXISTS,       /* 73 */
  TFTP_NOSUCHUSER,          /* 74 */
  OBSOLETE75,               /* 75 */
  OBSOLETE76,               /* 76 */
  SSL_CACERT_BADFILE,       /* 77 */
  REMOTE_FILE_NOT_FOUND,    /* 78 */
  SSH,                      /* 79 */
  SSL_SHUTDOWN_FAILED,      /* 80 */
  AGAIN,                    /* 81 */
  SSL_CRL_BADFILE,          /* 82 */
  SSL_ISSUER_ERROR,         /* 83 */
  FTP_PRET_FAILED,          /* 84 */
  RTSP_CSEQ_ERROR,          /* 85 */
  RTSP_SESSION_ERROR,       /* 86 */
  FTP_BAD_FILE_LIST,        /* 87 */
  CHUNK_FAILED,             /* 88 */
  NO_CONNECTION_AVAILABLE,  /* 89 */
  SSL_PINNEDPUBKEYNOTMATCH, /* 90 */
  SSL_INVALIDCERTSTATUS,    /* 91 */
  HTTP2_STREAM,             /* 92 */
  RECURSIVE_API_CALL,       /* 93 */
  AUTH_ERROR,               /* 94 */
  HTTP3,                    /* 95 */
  QUIC_CONNECT_ERROR,       /* 96 */
  PROXY,                    /* 97 */
  SSL_CLIENTCERT,           /* 98 */
  UNRECOVERABLE_POLL,       /* 99 */
};

/**
 * Detailed information on connection error.
 */
class Connection_error : public std::runtime_error {
 public:
  Connection_error(std::string_view msg, Error_code curl_code)
      : std::runtime_error(std::string(msg) + " (CURLcode = " +
                           std::to_string(static_cast<int>(curl_code)) + ")"),
        m_curl_code(curl_code) {}

  Connection_error(std::string_view msg, int curl_code)
      : Connection_error(msg, static_cast<Error_code>(curl_code)) {}

  Error_code code() const noexcept { return m_curl_code; }

 private:
  Error_code m_curl_code;
};

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_ERROR_H_
