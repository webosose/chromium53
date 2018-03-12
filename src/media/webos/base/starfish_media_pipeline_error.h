// Copyright (c) 2015-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MEDIA_WEBOS_BASE_STARFISH_MEDIA_PIPELINE_ERROR_H_
#define MEDIA_WEBOS_BASE_STARFISH_MEDIA_PIPELINE_ERROR_H_

enum StarfishMediaPipelineStatus {
  SMP_PLAYING_ERROR = 100,
  SMP_COMMAND_NOT_SUPPORTED = 101,
  SMP_NEED_TO_RELOAD_A_PIPELINE = 102,
  SMP_TAKE_SNAPSHOT_ERROR = 103,
  SMP_FAILED_TO_LOAD = 104,
  SMP_AUDIO_CODEC_NOT_SUPPORTED = 200,
  SMP_VIDEO_CODEC_NOT_SUPPORTED = 201,
  SMP_MEDIA_NOT_FOUND = 202,
  SMP_AV_TYPE_NOT_FOUNDED = 203,
  SMP_FAILED_TO_DEMULTIPLEX = 204,
  SMP_UNKNOWN_SUBTITLE = 210,
  SMP_NETWORK_ERROR = 300,
  SMP_UNSECURED_WIFI = 301,
  SMP_HLS_SERVER_ERROR_302 = 400,
  SMP_HLS_SERVER_ERROR_404 = 401,
  SMP_HLS_SERVER_ERROR_503 = 402,
  SMP_HLS_SERVER_ERROR_504 = 403,
  SMP_DRM_RELATED_ERROR = 500,
  SMP_RM_RELATED_ERROR = 600,
  SMP_RESOURCE_ALLOCATION_ERROR = 601,
  SMP_SEEK_FAILURE = 700,
  SMP_SETPLAYRATE_FAILURE = 701,
  SMP_STREAMING_PROTOCOL_RELATED_ERROR = 40000,
  SMP_MEDIA_API_PLAYING_ERROR = 61440,
  SMP_BUFFER_FULL = 61441,
  SMP_BUFFER_LOW = 61442,
};

#define SMP_STATUS_IS_100_GENERAL_ERROR(status) \
  ((status) >= SMP_PLAYING_ERROR && (status) < SMP_AUDIO_CODEC_NOT_SUPPORTED)

#define SMP_STATUS_IS_200_PLAYBACK_ERROR(status) \
  ((status) >= SMP_AUDIO_CODEC_NOT_SUPPORTED && (status) < SMP_NETWORK_ERROR)

#define SMP_STATUS_IS_300_NETWORK_ERROR(status) \
  ((status) >= SMP_NETWORK_ERROR && (status) < SMP_HLS_SERVER_ERROR_302)

#define SMP_STATUS_IS_400_SERVER_ERROR(status) \
  ((status) >= SMP_HLS_SERVER_ERROR_302 && (status) < SMP_DRM_RELATED_ERROR)

#define SMP_STATUS_IS_500_DRM_ERROR(status) \
  ((status) >= SMP_DRM_RELATED_ERROR && (status) < SMP_RM_RELATED_ERROR)

#define SMP_STATUS_IS_600_RM_ERROR(status) \
  ((status) >= SMP_RM_RELATED_ERROR && (status) < SMP_SEEK_FAILURE)

#define SMP_STATUS_IS_700_API_ERROR(status) \
  ((status) >= SMP_SEEK_FAILURE && (status) < 800)

#define SMP_STATUS_IS_40000_STREAMING_ERROR(status) \
  ((status) >= SMP_STREAMING_PROTOCOL_RELATED_ERROR && (status) < 50000)

#endif  // MEDIA_WEBOS_BASE_STARFISH_MEDIA_PIPELINE_ERROR_H_
