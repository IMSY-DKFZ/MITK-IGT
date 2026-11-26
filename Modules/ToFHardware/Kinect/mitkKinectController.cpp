/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#include "mitkKinectController.h"

#include <libfreenect.h>

#include <mitkExceptionMacro.h>
#include <mitkLog.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
  constexpr unsigned int Width = 640;
  constexpr unsigned int Height = 480;

  // In FREENECT_RESOLUTION_MEDIUM the IR stream is 640x488, eight rows more
  // than the depth image it is aligned with.
  constexpr unsigned int IRHeight = 488;
  constexpr unsigned int IRFirstRow = 0;

  constexpr bool MirrorHorizontally = false;

  constexpr auto FrameTimeout = std::chrono::milliseconds(100);
  constexpr long EventTimeoutMicroseconds = 10000;

  unsigned int TargetIndex(unsigned int x, unsigned int y)
  {
    return (MirrorHorizontally ? Width - 1 - x : x) + y * Width;
  }
}

namespace mitk
{
class KinectController::KinectControllerPrivate
{
public:
  static void OnDepth(freenect_device* device, void* depth, uint32_t timestamp);
  static void OnVideo(freenect_device* device, void* video, uint32_t timestamp);

  void ProcessEvents();
  void Close();

  freenect_context* m_Context = nullptr;
  freenect_device* m_Device = nullptr;

  std::thread m_EventThread;
  std::atomic<bool> m_Running = false;

  std::mutex m_FrameMutex; /**< guards the m_Latest* buffers and the flags below */
  std::condition_variable m_FrameArrived;
  std::vector<uint16_t> m_LatestDepth;
  std::vector<unsigned char> m_LatestVideo; /**< RGB24, or 16 bit IR stored as bytes */
  bool m_NewDepth = false;
  bool m_NewVideo = false;
  bool m_DeviceLost = false;

  // Front buffers, filled by UpdateCamera() and read by the getters.
  std::vector<uint16_t> m_Depth;
  std::vector<unsigned char> m_Video;

  bool m_Connected = false;
  bool m_UseIR = false;
  bool m_StreamingIR = false; /**< the mode fixed at connect time; m_UseIR may change while connected */
};

void KinectController::KinectControllerPrivate::OnDepth(freenect_device* device, void* depth, uint32_t)
{
  auto* self = static_cast<KinectControllerPrivate*>(freenect_get_user(device));
  const auto* source = static_cast<const uint16_t*>(depth);
  {
    std::lock_guard<std::mutex> lock(self->m_FrameMutex);
    self->m_LatestDepth.assign(source, source + self->m_LatestDepth.size());
    self->m_NewDepth = true;
  }
  self->m_FrameArrived.notify_all();
}

void KinectController::KinectControllerPrivate::OnVideo(freenect_device* device, void* video, uint32_t)
{
  auto* self = static_cast<KinectControllerPrivate*>(freenect_get_user(device));
  const auto* source = static_cast<const unsigned char*>(video);
  {
    std::lock_guard<std::mutex> lock(self->m_FrameMutex);
    self->m_LatestVideo.assign(source, source + self->m_LatestVideo.size());
    self->m_NewVideo = true;
  }
  self->m_FrameArrived.notify_all();
}

void KinectController::KinectControllerPrivate::ProcessEvents()
{
  while (m_Running)
  {
    timeval timeout = {0, EventTimeoutMicroseconds};
    if (freenect_process_events_timeout(m_Context, &timeout) < 0)
    {
      {
        std::lock_guard<std::mutex> lock(m_FrameMutex);
        m_DeviceLost = true;
      }
      m_FrameArrived.notify_all();
      return;
    }
  }
}

void KinectController::KinectControllerPrivate::Close()
{
  // libfreenect is not thread safe, so the event thread must be gone before
  // the device is stopped and closed.
  m_Running = false;
  if (m_EventThread.joinable())
    m_EventThread.join();

  if (m_Device != nullptr)
  {
    freenect_stop_depth(m_Device);
    freenect_stop_video(m_Device);
    freenect_close_device(m_Device);
    m_Device = nullptr;
  }
  if (m_Context != nullptr)
  {
    freenect_shutdown(m_Context);
    m_Context = nullptr;
  }
  m_Connected = false;
}

KinectController::KinectController(): d(new KinectControllerPrivate)
{
}

KinectController::~KinectController()
{
  d->Close();
  delete d;
}

bool KinectController::OpenCameraConnection()
{
  if (d->m_Connected)
    return true;

  if (freenect_init(&d->m_Context, nullptr) < 0)
  {
    d->m_Context = nullptr;
    mitkThrow() << "Kinect: libfreenect could not be initialised";
  }
  freenect_set_log_level(d->m_Context, FREENECT_LOG_WARNING);
  // Motor and audio are not used. Leaving them out also avoids the audio
  // firmware upload that newer Kinect models require for those subdevices.
  freenect_select_subdevices(d->m_Context, FREENECT_DEVICE_CAMERA);

  if (freenect_num_devices(d->m_Context) <= 0)
  {
    d->Close();
    mitkThrow() << "Kinect: No Kinect found";
  }
  if (freenect_open_device(d->m_Context, &d->m_Device, 0) < 0)
  {
    d->m_Device = nullptr;
    d->Close();
    mitkThrow() << "Kinect: The Kinect could not be opened. Is the libusbK driver installed?";
  }
  freenect_set_user(d->m_Device, d);
  d->m_StreamingIR = d->m_UseIR;

  // In RGB mode the depth is registered to the RGB camera. In IR mode it
  // stays in the IR camera's view, which is the camera that measures depth.
  const auto depthMode = freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM,
    d->m_StreamingIR ? FREENECT_DEPTH_MM : FREENECT_DEPTH_REGISTERED);
  const auto videoMode = freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM,
    d->m_StreamingIR ? FREENECT_VIDEO_IR_10BIT : FREENECT_VIDEO_RGB);
  if (!depthMode.is_valid || !videoMode.is_valid
    || freenect_set_depth_mode(d->m_Device, depthMode) < 0
    || freenect_set_video_mode(d->m_Device, videoMode) < 0)
  {
    d->Close();
    mitkThrow() << "Kinect: The requested stream modes are not supported";
  }

  d->m_LatestDepth.assign(Width * Height, 0);
  d->m_Depth.assign(Width * Height, 0);
  const std::size_t videoBytes = d->m_StreamingIR ? Width * IRHeight * sizeof(uint16_t) : Width * Height * 3;
  d->m_LatestVideo.assign(videoBytes, 0);
  d->m_Video.assign(videoBytes, 0);
  d->m_NewDepth = false;
  d->m_NewVideo = false;
  d->m_DeviceLost = false;

  freenect_set_depth_callback(d->m_Device, &KinectControllerPrivate::OnDepth);
  freenect_set_video_callback(d->m_Device, &KinectControllerPrivate::OnVideo);
  if (freenect_start_depth(d->m_Device) < 0 || freenect_start_video(d->m_Device) < 0)
  {
    d->Close();
    mitkThrow() << "Kinect: The streams could not be started";
  }

  d->m_Running = true;
  d->m_EventThread = std::thread(&KinectControllerPrivate::ProcessEvents, d);
  d->m_Connected = true;
  MITK_INFO << "Kinect connected (" << (d->m_StreamingIR ? "depth and IR" : "depth and RGB") << ")";
  return true;
}

bool KinectController::CloseCameraConnection()
{
  d->Close();
  return true;
}

bool KinectController::UpdateCamera()
{
  if (!d->m_Connected)
    mitkThrow() << "Kinect: Not connected";

  std::unique_lock<std::mutex> lock(d->m_FrameMutex);
  const bool complete = d->m_FrameArrived.wait_for(lock, FrameTimeout, [this]
  {
    return d->m_DeviceLost || (d->m_NewDepth && d->m_NewVideo);
  });
  if (d->m_DeviceLost)
    mitkThrow() << "Kinect: Lost the connection to the Kinect";
  if (!complete)
    return false;

  d->m_Depth.swap(d->m_LatestDepth);
  d->m_Video.swap(d->m_LatestVideo);
  d->m_NewDepth = false;
  d->m_NewVideo = false;
  return true;
}

void KinectController::GetDistances(float* distances)
{
  if (d->m_Depth.empty())
    return;

  for (unsigned int y = 0; y < Height; ++y)
  {
    for (unsigned int x = 0; x < Width; ++x)
    {
      distances[TargetIndex(x, y)] = static_cast<float>(d->m_Depth[x + y * Width]);
    }
  }
}

void KinectController::GetRgb(unsigned char* rgb)
{
  if (d->m_StreamingIR || d->m_Video.empty())
    return;

  for (unsigned int y = 0; y < Height; ++y)
  {
    for (unsigned int x = 0; x < Width; ++x)
    {
      const unsigned char* source = d->m_Video.data() + (x + y * Width) * 3;
      unsigned char* target = rgb + TargetIndex(x, y) * 3;
      target[0] = source[0];
      target[1] = source[1];
      target[2] = source[2];
    }
  }
}

void KinectController::GetAllData(float* distances, float* amplitudes, unsigned char* rgb)
{
  this->GetDistances(distances);
  if (d->m_StreamingIR)
    this->GetAmplitudes(amplitudes);
  else
    this->GetRgb(rgb);
}

void KinectController::GetAmplitudes(float* amplitudes)
{
  if (!d->m_StreamingIR || d->m_Video.empty())
    return;

  const auto* ir = reinterpret_cast<const uint16_t*>(d->m_Video.data());
  for (unsigned int y = 0; y < Height; ++y)
  {
    for (unsigned int x = 0; x < Width; ++x)
    {
      amplitudes[TargetIndex(x, y)] = static_cast<float>(ir[x + (y + IRFirstRow) * Width]);
    }
  }
}

void KinectController::GetIntensities(float*)
{
}

unsigned int KinectController::GetCaptureWidth() const
{
  return Width;
}

unsigned int KinectController::GetCaptureHeight() const
{
  return Height;
}

bool KinectController::GetUseIR() const
{
  return d->m_UseIR;
}

void KinectController::SetUseIR(bool useIR)
{
  if (d->m_UseIR != useIR)
  {
    d->m_UseIR = useIR;
    this->Modified();
  }
}
}
