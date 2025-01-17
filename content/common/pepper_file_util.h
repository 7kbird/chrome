// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_PEPPER_FILE_UTIL_H_
#define CONTENT_COMMON_PEPPER_FILE_UTIL_H_

#include "ppapi/c/pp_file_info.h"
#include "webkit/common/fileapi/file_system_types.h"

namespace content {

storage::FileSystemType PepperFileSystemTypeToFileSystemType(
    PP_FileSystemType type);

}  // namespace content

#endif  // CONTENT_COMMON_PEPPER_FILE_UTIL_H_
