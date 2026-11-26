/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#include "mitkKinectDevice.h"
#include <mitkRealTimeClock.h>

#include <mitkExceptionMacro.h>


namespace mitk
{

KinectDevice::KinectDevice()
{
  m_Controller = mitk::KinectController::New();
}

KinectDevice::~KinectDevice()
{
  if (m_Thread.joinable())
    this->StopCamera();
}

bool KinectDevice::OnConnectCamera()
{
  bool ok = false;
  if (m_Controller)
  {
    ok = m_Controller->OpenCameraConnection();
    if (ok)
    {
      this->m_CaptureWidth = m_Controller->GetCaptureWidth();
      this->m_CaptureHeight = m_Controller->GetCaptureHeight();
      this->m_PixelNumber = this->m_CaptureWidth * this->m_CaptureHeight;

      this->m_RGBImageWidth = m_CaptureWidth;
      this->m_RGBImageHeight = m_CaptureHeight;
      this->m_RGBPixelNumber = this->m_RGBImageWidth * this->m_RGBImageHeight;

      this->SetBoolProperty("HasAmplitudeImage", m_Controller->GetUseIR());
      this->SetBoolProperty("HasRGBImage", !m_Controller->GetUseIR());

      // allocate buffer
      this->m_DistanceArray = new float[this->m_PixelNumber];
      for(int i=0; i<this->m_PixelNumber; i++) {this->m_DistanceArray[i]=0.0;}
      this->m_AmplitudeArray = new float[this->m_PixelNumber];
      for(int i=0; i<this->m_PixelNumber; i++) {this->m_AmplitudeArray[i]=0.0;}

      this->m_DistanceDataBuffer = new float*[this->m_MaxBufferSize];
      for(int i=0; i<this->m_MaxBufferSize; i++)
      {
        this->m_DistanceDataBuffer[i] = new float[this->m_PixelNumber];
      }
      this->m_AmplitudeDataBuffer = new float*[this->m_MaxBufferSize];
      for(int i=0; i<this->m_MaxBufferSize; i++)
      {
        this->m_AmplitudeDataBuffer[i] = new float[this->m_PixelNumber];
      }
      this->m_RGBDataBuffer = new unsigned char*[this->m_MaxBufferSize];
      for (int i=0; i<this->m_MaxBufferSize; i++)
      {
        this->m_RGBDataBuffer[i] = new unsigned char[this->m_PixelNumber*3];
      }

      m_CameraConnected = true;
    }
  }
  return ok;
}

bool KinectDevice::DisconnectCamera()
{
  // The acquisition thread writes into the buffers freed below, and the
  // ToF image grabber disconnects without stopping first.
  this->StopCamera();

  bool ok = false;
  if (m_Controller)
  {
    ok =  m_Controller->CloseCameraConnection();

    // clean-up only if camera was connected
    if (m_CameraConnected)
    {
      delete [] m_DistanceArray;
      delete [] m_AmplitudeArray;

      for(int i=0; i<this->m_MaxBufferSize; i++)
      {
        delete[] this->m_DistanceDataBuffer[i];
        delete[] this->m_AmplitudeDataBuffer[i];
        delete[] this->m_RGBDataBuffer[i];
      }
      delete[] this->m_DistanceDataBuffer;
      delete[] this->m_AmplitudeDataBuffer;
      delete[] this->m_RGBDataBuffer;

      m_CameraConnected = false;
    }

  }
  return ok;
}

void KinectDevice::StartCamera()
{
  if (this->IsCameraActive())
    return;
  // A thread that ended on its own, e.g. after the camera was lost, must be
  // joined before a new one can be assigned.
  if (m_Thread.joinable())
    m_Thread.join();

  if (m_CameraConnected)
  {
    // Readers expect an image once the camera is started. The first frames
    // can take longer than one UpdateCamera() timeout after connecting.
    bool hasFirstImage = false;
    for (int attempt = 0; attempt < 20 && !hasFirstImage; ++attempt)
      hasFirstImage = this->m_Controller->UpdateCamera();
    if (!hasFirstImage)
    {
      MITK_WARN << "Kinect: No image within 2 s after starting the camera";
    }
    else
    {
      std::lock_guard<std::mutex> lock(this->m_ImageMutex);
      this->m_Controller->GetAllData(this->m_DistanceDataBuffer[this->m_FreePos],this->m_AmplitudeDataBuffer[this->m_FreePos],this->m_RGBDataBuffer[this->m_FreePos]);
      this->m_FreePos = (this->m_FreePos+1) % this->m_BufferSize;
      this->m_CurrentPos = (this->m_CurrentPos+1) % this->m_BufferSize;
      this->m_ImageSequence++;
    }

    this->m_CameraActiveMutex.lock();
    this->m_CameraActive = true;
    this->m_CameraActiveMutex.unlock();
    this->m_Thread = std::thread(&KinectDevice::Acquire, this);
  }
  else
  {
    MITK_INFO<<"Camera not connected";
  }
}

void KinectDevice::StopCamera()
{
  m_CameraActiveMutex.lock();
  m_CameraActive = false;
  m_CameraActiveMutex.unlock();
  // The acquisition loop wakes up at least every 100 ms (the controller's
  // frame timeout), so joining is bounded and the thread never outlives the
  // buffers it writes to.
  if (m_Thread.joinable())
    m_Thread.join();
}

bool KinectDevice::IsCameraActive()
{
  std::lock_guard<std::mutex> lock(m_CameraActiveMutex);
  return m_CameraActive;
}

void KinectDevice::UpdateCamera()
{
  if (m_Controller)
  {
    m_Controller->UpdateCamera();
  }
}

void KinectDevice::Acquire()
{
  mitk::RealTimeClock::Pointer realTimeClock = mitk::RealTimeClock::New();
  double t1 = realTimeClock->GetCurrentStamp();
  const int n = 100;
  while (this->IsCameraActive())
  {
    try
    {
      if (!this->m_Controller->UpdateCamera())
        continue;
    }
    catch (const mitk::Exception& e)
    {
      // An exception must not escape the thread; the camera is reported
      // inactive instead, which the ToF image grabber notices.
      MITK_ERROR << e.GetDescription();
      std::lock_guard<std::mutex> lock(this->m_CameraActiveMutex);
      this->m_CameraActive = false;
      break;
    }

    // With the default buffer size of 1, m_FreePos and m_CurrentPos are the
    // same slot, so the write must be guarded against concurrent readers.
    this->m_ImageMutex.lock();
    this->m_Controller->GetAllData(this->m_DistanceDataBuffer[this->m_FreePos],this->m_AmplitudeDataBuffer[this->m_FreePos],this->m_RGBDataBuffer[this->m_FreePos]);
    this->m_FreePos = (this->m_FreePos+1) % this->m_BufferSize;
    this->m_CurrentPos = (this->m_CurrentPos+1) % this->m_BufferSize;
    this->m_ImageSequence++;
    const int imageSequence = this->m_ImageSequence;
    this->m_ImageMutex.unlock();

    this->Modified();

    if (imageSequence % n == 0)
    {
      const double t2 = realTimeClock->GetCurrentStamp() - t1;
      MITK_INFO << " Framerate (fps): " << n / (t2/1000) << " Sequence: " << imageSequence;
      t1 = realTimeClock->GetCurrentStamp();
    }
  }
}

void KinectDevice::GetAmplitudes(float*, int& imageSequence)
{
  m_ImageMutex.lock();
  if (m_CameraActive)
  {
      // copy the image buffer
    imageSequence = this->m_ImageSequence;
  }
  else
  {
    MITK_WARN("ToF") << "Warning: Data can only be acquired if camera is active.";
  }
  m_ImageMutex.unlock();
}

void KinectDevice::GetIntensities(float *, int &)
{
}

void KinectDevice::GetDistances(float*, int& imageSequence)
{
  m_ImageMutex.lock();
  if (m_CameraActive)
  {
    imageSequence = this->m_ImageSequence;
  }
  else
  {
    MITK_WARN("ToF") << "Warning: Data can only be acquired if camera is active.";
  }
  m_ImageMutex.unlock();
}

void KinectDevice::GetAllImages(float* distanceArray, float* amplitudeArray, float*, char*,
                                int requiredImageSequence, int& capturedImageSequence, unsigned char* rgbDataArray)
{
  std::lock_guard<std::mutex> lock(m_ImageMutex);
  if (m_CameraActive)
  {
    // check for empty buffer
    if (this->m_ImageSequence < 0 || this->m_CurrentPos < 0)
    {
      // buffer empty
      MITK_INFO << "Buffer empty!! ";
      capturedImageSequence = this->m_ImageSequence;
      return;
    }

    // determine position of image in buffer
    int pos = 0;
    if ((requiredImageSequence < 0) || (requiredImageSequence > this->m_ImageSequence))
    {
      capturedImageSequence = this->m_ImageSequence;
      pos = this->m_CurrentPos;
    }
    else if (requiredImageSequence <= this->m_ImageSequence - this->m_BufferSize)
    {
      capturedImageSequence = (this->m_ImageSequence - this->m_BufferSize) + 1;
      pos = (this->m_CurrentPos + 1) % this->m_BufferSize;
    }
    else // (requiredImageSequence > this->m_ImageSequence - this->m_BufferSize) && (requiredImageSequence <= this->m_ImageSequence)
    {
      capturedImageSequence = requiredImageSequence;
      pos = (this->m_CurrentPos + (10-(this->m_ImageSequence - requiredImageSequence))) % this->m_BufferSize;
    }

    memcpy( distanceArray, this->m_DistanceDataBuffer[pos], m_PixelNumber*sizeof(float));
    // The controller streams the mode chosen at connect time, even if IR is
    // switched while connected.
    bool streamingIR = false;
    this->GetBoolProperty("HasAmplitudeImage", streamingIR);
    if (streamingIR)
    {
      memcpy( amplitudeArray, this->m_AmplitudeDataBuffer[pos], m_PixelNumber*sizeof(float));
    }
    else
    {
      memcpy( rgbDataArray, this->m_RGBDataBuffer[pos], m_RGBPixelNumber*3*sizeof(unsigned char));
    }
  }
  else
  {
    MITK_WARN("ToF") << "Warning: Data can only be acquired if camera is active.";
  }
}

KinectController::Pointer KinectDevice::GetController()
{
  return this->m_Controller;
}

void KinectDevice::SetProperty( const char *propertyKey, BaseProperty* propertyValue )
{
  ToFCameraDevice::SetProperty(propertyKey,propertyValue);
  this->m_PropertyList->SetProperty(propertyKey, propertyValue);
  if (strcmp(propertyKey, "RGB") == 0)
  {
    bool rgb = false;
    GetBoolProperty(propertyKey, rgb);
    m_Controller->SetUseIR(!rgb);
  }
  else if (strcmp(propertyKey, "IR") == 0)
  {
    bool ir = false;
    GetBoolProperty(propertyKey, ir);
    m_Controller->SetUseIR(ir);
  }
}

int KinectDevice::GetRGBCaptureWidth()
{
  return this->GetCaptureWidth();
}

int KinectDevice::GetRGBCaptureHeight()
{
  return this->GetCaptureHeight();
}
}
