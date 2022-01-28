/*
 * Copyright (c) 2021, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_REST_ERROR_CODES_H_
#define MYSQLSHDK_LIBS_REST_ERROR_CODES_H_

#include <string>

#include "mysqlshdk/include/scripting/types.h"

#define SHERR_NETWORK_MIN 54100

#define SHERR_NETWORK_FIRST SHERR_NETWORK_MIN

#define SHERR_NETWORK_CONTINUE 54100
#define SHERR_NETWORK_SWITCHING_PROTOCOLS 54101
#define SHERR_NETWORK_PROCESSING 54102
#define SHERR_NETWORK_EARLY_HINTS 54103
#define SHERR_NETWORK_OK 54200
#define SHERR_NETWORK_CREATED 54201
#define SHERR_NETWORK_ACCEPTED 54202
#define SHERR_NETWORK_NON_AUTHORITATIVE_INFORMATION 54203
#define SHERR_NETWORK_NO_CONTENT 54204
#define SHERR_NETWORK_RESET_CONTENT 54205
#define SHERR_NETWORK_PARTIAL_CONTENT 54206
#define SHERR_NETWORK_MULTI_STATUS 54207
#define SHERR_NETWORK_ALREADY_REPORTED 54208
#define SHERR_NETWORK_IM_USED 54226
#define SHERR_NETWORK_MULTIPLE_CHOICES 54300
#define SHERR_NETWORK_MOVED_PERMANENTLY 54301
#define SHERR_NETWORK_FOUND 54302
#define SHERR_NETWORK_SEE_OTHER 54303
#define SHERR_NETWORK_NOT_MODIFIED 54304
#define SHERR_NETWORK_USE_PROXY 54305
#define SHERR_NETWORK_SWITCH_PROXY 54306
#define SHERR_NETWORK_TEMPORARY_REDIRECT 54307
#define SHERR_NETWORK_PERMANENT_REDIRECT 54308
#define SHERR_NETWORK_BAD_REQUEST 54400
#define SHERR_NETWORK_UNAUTHORIZED 54401
#define SHERR_NETWORK_PAYMENT_REQUIRED 54402
#define SHERR_NETWORK_FORBIDDEN 54403
#define SHERR_NETWORK_NOT_FOUND 54404
#define SHERR_NETWORK_METHOD_NOT_ALLOWED 54405
#define SHERR_NETWORK_NOT_ACCEPTABLE 54406
#define SHERR_NETWORK_PROXY_AUTHENTICATION_REQUIRED 54407
#define SHERR_NETWORK_REQUEST_TIMEOUT 54408
#define SHERR_NETWORK_CONFLICT 54409
#define SHERR_NETWORK_GONE 54410
#define SHERR_NETWORK_LENGTH_REQUIRED 54411
#define SHERR_NETWORK_PRECONDITION_FAILED 54412
#define SHERR_NETWORK_PAYLOAD_TOO_LARGE 54413
#define SHERR_NETWORK_URI_TOO_LONG 54414
#define SHERR_NETWORK_UNSUPPORTED_MEDIA_TYPE 54415
#define SHERR_NETWORK_RANGE_NOT_SATISFIABLE 54416
#define SHERR_NETWORK_EXPECTATION_FAILED 54417
#define SHERR_NETWORK_IM_A_TEAPOT 54418
#define SHERR_NETWORK_MISDIRECTED_REQUEST 54421
#define SHERR_NETWORK_UNPROCESSABLE_ENTITY 54422
#define SHERR_NETWORK_LOCKED 54423
#define SHERR_NETWORK_FAILED_DEPENDENCY 54424
#define SHERR_NETWORK_TOO_EARLY 54425
#define SHERR_NETWORK_UPGRADE_REQUIRED 54426
#define SHERR_NETWORK_PRECONDITION_REQUIRED 54428
#define SHERR_NETWORK_TOO_MANY_REQUESTS 54429
#define SHERR_NETWORK_REQUEST_HEADER_FIELDS_TOO_LARGE 54431
#define SHERR_NETWORK_UNAVAILABLE_FOR_LEGAL_REASONS 54451
#define SHERR_NETWORK_INTERNAL_SERVER_ERROR 54500
#define SHERR_NETWORK_NOT_IMPLEMENTED 54501
#define SHERR_NETWORK_BAD_GATEWAY 54502
#define SHERR_NETWORK_SERVICE_UNAVAILABLE 54503
#define SHERR_NETWORK_GATEWAY_TIMEOUT 54504
#define SHERR_NETWORK_HTTP_VERSION_NOT_SUPPORTED 54505
#define SHERR_NETWORK_VARIANT_ALSO_NEGOTIATES 54506
#define SHERR_NETWORK_INSUFFICIENT_STORAGE 54507
#define SHERR_NETWORK_LOOP_DETECTED 54508
#define SHERR_NETWORK_NOT_EXTENDED 54510
#define SHERR_NETWORK_NETWORK_AUTHENTICATION_REQUIRED 54511

#define SHERR_NETWORK_LAST 54511

#define SHERR_NETWORK_MAX SHERR_NETWORK_LAST

namespace mysqlshdk {
namespace rest {

class Response_error;

/**
 * Converts the given response error to the shcore::Exception with the
 * corresponding SHERR_NETWORK error code. Optionally prepends the context
 * information.
 *
 * @param error The response error to convert.
 * @param context Optional context.
 *
 * @returns Corresponding shcore::Exception.
 */
shcore::Exception to_exception(const Response_error &error,
                               const std::string &context = {});

/**
 * Rethrows current exception, translating it to shcore::Exception, including
 * information about the context.
 *
 * Does nothing if currently there is no exception.
 *
 * @param context Additional context.
 */
void translate_current_exception(const std::string &context);

}  // namespace rest
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_REST_ERROR_CODES_H_
