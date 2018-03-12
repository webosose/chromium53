/*
 * Copyright (C) 2017 LG Electrionics, Inc.
 */

#include "components/media_capture_util/devices_dispatcher.h"

#include "base/callback.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/media_capture_devices.h"

using content::BrowserThread;

namespace media_capture_util {

namespace {

inline const content::MediaStreamDevice* GetDevice(
    const content::MediaStreamDevices& devices,
    const std::string& requested_device_id) {
  if (devices.empty())
    return NULL;

  if (!requested_device_id.empty())
    return devices.FindById(requested_device_id);

  return &(*devices.begin());
}

const content::MediaStreamDevice* GetAudioDevice(
    const std::string& requested_audio_device_id) {
  const content::MediaStreamDevices& audio_devices =
      content::MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices();

  return GetDevice(audio_devices, requested_audio_device_id);
}

const content::MediaStreamDevice* GetVideoDevice(
    const std::string& requested_video_device_id) {
  const content::MediaStreamDevices& video_devices =
      content::MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices();
  return GetDevice(video_devices, requested_video_device_id);
}

}  // namespace

DevicesDispatcher* DevicesDispatcher::GetInstance() {
  return base::Singleton<DevicesDispatcher>::get();
}

void DevicesDispatcher::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    bool accepts_video,
    bool accepts_audio,
    const content::MediaResponseCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::MediaStreamDevices devices;
  std::unique_ptr<content::MediaStreamUI> ui;

  switch (request.request_type) {
    case content::MEDIA_OPEN_DEVICE_PEPPER_ONLY: {
      const content::MediaStreamDevice* device = NULL;
      if (request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE &&
          accepts_audio) {
        device = GetAudioDevice(request.requested_audio_device_id);
      } else if (request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE &&
                 accepts_video) {
        device = GetVideoDevice(request.requested_video_device_id);
      }
      if (device)
        devices.push_back(*device);
      break;
    }
    case content::MEDIA_DEVICE_ACCESS:
    case content::MEDIA_GENERATE_STREAM: {
      if (request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE &&
          accepts_audio) {
        const content::MediaStreamDevice* device = NULL;
        device = GetAudioDevice(request.requested_audio_device_id);
        if (device)
          devices.push_back(*device);
      }
      if (request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE &&
          accepts_video) {
        const content::MediaStreamDevice* device = NULL;
        device = GetVideoDevice(request.requested_video_device_id);
        if (device)
          devices.push_back(*device);
      }
      break;
    }
    case content::MEDIA_ENUMERATE_DEVICES: {
      if (request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE &&
          accepts_audio) {
        const content::MediaStreamDevices& audio_devices =
            content::MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices();
        devices.insert(devices.end(), audio_devices.begin(), audio_devices.end());
      }
      if (request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE &&
          accepts_video) {
        const content::MediaStreamDevices& video_devices =
            content::MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices();
        devices.insert(devices.end(), video_devices.begin(), video_devices.end());
      }
      break;
    }
  }

  callback.Run(devices, devices.empty() ? content::MEDIA_DEVICE_NO_HARDWARE
                                        : content::MEDIA_DEVICE_OK,
               std::move(ui));
}

}  // namespace media_capture_util
