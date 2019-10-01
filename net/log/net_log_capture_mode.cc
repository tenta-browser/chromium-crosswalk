// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/log/net_log_capture_mode.h"

namespace net {

bool NetLogCaptureIncludesSensitive(NetLogCaptureMode capture_mode) {
  return capture_mode >= NetLogCaptureMode::kIncludeSensitive;
}

bool NetLogCaptureIncludesSocketBytes(NetLogCaptureMode capture_mode) {
  return capture_mode == NetLogCaptureMode::kEverything;
}

NetLogCaptureMode GetNetCaptureModeFromCommandLine(
    const base::CommandLine& command_line,
    base::StringPiece switch_name) {
  if (command_line.HasSwitch(switch_name)) {
    std::string value = command_line.GetSwitchValueASCII(switch_name);

    if (value == "Default")
      return NetLogCaptureMode::Default();
    if (value == "IncludeCookiesAndCredentials")
      return NetLogCaptureMode::IncludeCookiesAndCredentials();
    if (value == "IncludeSocketBytes")
      return NetLogCaptureMode::IncludeSocketBytes();

    LOG(ERROR) << "Unrecognized value for --" << switch_name;
  }

  return net::NetLogCaptureMode::Default();
}

}  // namespace net
