/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#include "mitkIGTLMessageFactory.h"

// IGT message types
#include "igtlImageMessage.h"
#include "igtlTransformMessage.h"
#include "igtlPositionMessage.h"
#include "igtlStatusMessage.h"
#include "igtlImageMetaMessage.h"
#include "igtlPointMessage.h"
#include "igtlTrajectoryMessage.h"
#include "igtlStringMessage.h"
#include "igtlSensorMessage.h"
#include "igtlBindMessage.h"
#include "igtlPolyDataMessage.h"
#include "igtlQuaternionTrackingDataMessage.h"
#include "igtlCapabilityMessage.h"
#include "igtlNDArrayMessage.h"
#include "igtlTrackingDataMessage.h"
#include "igtlColorTableMessage.h"
#include "igtlLabelMetaMessage.h"

//own types
#include "mitkIGTLDummyMessage.h"

#include "mitkIGTLMessageCommon.h"

#include "itksys/SystemTools.hxx"

namespace
{
  template <typename TMessage>
  igtl::MessageBase::Pointer NewMessage()
  {
    return TMessage::New().GetPointer();
  }
}

//------------------------------------------------------------
// Define message clone classes
// igtlMessageHandlerClassMacro() defines a child class of
// igtl::MessageHandler to handle OpenIGTLink messages for
// the message type specified as the first argument. The
// second argument will be used for the name of this
// message handler class, while the third argument specifies
// a type of data that will be shared with the message functions
// of this handler class.

mitkIGTMessageCloneClassMacro(igtl::TransformMessage, TransformMsgCloneHandler)

/**
 * \brief Clones the original message interpreted as transform message
 * \param original_ The original message that will be interpreted as transform
 * message
 * \return The clone of the input message
 */
igtl::MessageBase::Pointer TransformMsgCloneHandler::Clone(igtl::MessageBase* original_)
{
  bool copySuccess = false;
  igtl::TransformMessage::Pointer clone_ = igtl::TransformMessage::New();

  //initialize the clone
  //  clone = igtl::MessageBase::New();

  igtl::TransformMessage* original = (igtl::TransformMessage*)original_;

  //copy all meta data
  copySuccess = clone_->Copy(original);

  if (!copySuccess)
    return nullptr;

  //copy all data that is important for this class
  //copy the matrix
  igtl::Matrix4x4 mat;
  original->GetMatrix(mat);
  clone_->SetMatrix(mat);

  //copy the normals
  float normals[3][3];
  original->GetNormals(normals);
  clone_->SetNormals(normals);

  //copy the position
  float position[3];
  original->GetPosition(position);
  clone_->SetPosition(position);

  return igtl::MessageBase::Pointer(clone_.GetPointer());
}

mitk::IGTLMessageFactory::IGTLMessageFactory()
{
  //create clone handlers
  //  mitk::IGTLMessageCloneHandler::Pointer tmch = ;

  this->AddMessageNewMethod("NONE", nullptr);

  // OpenIGTLink Types V1
  this->AddMessageNewMethod("IMAGE", &NewMessage<igtl::ImageMessage>);
  this->AddMessageNewMethod("TRANSFORM", &NewMessage<igtl::TransformMessage>);
  this->AddMessageNewMethod("POSITION", &NewMessage<igtl::PositionMessage>);
  this->AddMessageNewMethod("STATUS", &NewMessage<igtl::StatusMessage>);
  this->AddMessageNewMethod("CAPABILITY", &NewMessage<igtl::CapabilityMessage>);

  this->AddMessageNewMethod("GET_IMAGE", &NewMessage<igtl::GetImageMessage>);
  this->AddMessageNewMethod("GET_TRANS", &NewMessage<igtl::GetTransformMessage>);
  //this->AddMessageNewMethod("GET_POS", &NewMessage<igtl::GetPositionMessage>); //not available???
  this->AddMessageNewMethod("GET_STATUS", &NewMessage<igtl::GetStatusMessage>);

  // OpenIGTLink Types V2
  this->AddMessageNewMethod("IMGMETA", &NewMessage<igtl::ImageMetaMessage>);
  this->AddMessageNewMethod("LBMETA", &NewMessage<igtl::LabelMetaMessage>);
  this->AddMessageNewMethod("COLORT", &NewMessage<igtl::ColorTableMessage>);
  this->AddMessageNewMethod("POINT", &NewMessage<igtl::PointMessage>);
  this->AddMessageNewMethod("TRAJ", &NewMessage<igtl::TrajectoryMessage>);
  this->AddMessageNewMethod("TDATA", &NewMessage<igtl::TrackingDataMessage>);
  this->AddMessageNewMethod("QTDATA", &NewMessage<igtl::QuaternionTrackingDataMessage>);
  this->AddMessageNewMethod("SENSOR", &NewMessage<igtl::SensorMessage>);
  this->AddMessageNewMethod("STRING", &NewMessage<igtl::StringMessage>);
  this->AddMessageNewMethod("NDARRAY", &NewMessage<igtl::NDArrayMessage>);
  this->AddMessageNewMethod("BIND", &NewMessage<igtl::BindMessage>);
  this->AddMessageNewMethod("POLYDATA", &NewMessage<igtl::PolyDataMessage>);

  this->AddMessageNewMethod("GET_IMGMETA", &NewMessage<igtl::GetImageMetaMessage>);
  this->AddMessageNewMethod("GET_LBMETA", &NewMessage<igtl::GetLabelMetaMessage>);
  this->AddMessageNewMethod("GET_COLORT", &NewMessage<igtl::GetColorTableMessage>);
  this->AddMessageNewMethod("GET_POINT", &NewMessage<igtl::GetPointMessage>);
  this->AddMessageNewMethod("GET_TRAJ", &NewMessage<igtl::GetTrajectoryMessage>);
  //  this->AddMessageNewMethod("GET_TDATA",      &NewMessage<igtl::GetTrackingDataMessage>); //not available???
  //  this->AddMessageNewMethod("GET_QTDATA",     &NewMessage<igtl::GetQuaternionTrackingDataMessage>); //not available???
  //  this->AddMessageNewMethod("GET_SENSOR",     &NewMessage<igtl::GetSensorMessage>); //not available???
  //  this->AddMessageNewMethod("GET_STRING",     &NewMessage<igtl::GetStringMessage>); //not available???
  //  this->AddMessageNewMethod("GET_NDARRAY",    &NewMessage<igtl::GetNDArrayMessage>); //not available???
  this->AddMessageNewMethod("GET_BIND", &NewMessage<igtl::GetBindMessage>);
  this->AddMessageNewMethod("GET_POLYDATA", &NewMessage<igtl::GetPolyDataMessage>);

  this->AddMessageNewMethod("RTS_BIND", &NewMessage<igtl::RTSBindMessage>);
  this->AddMessageNewMethod("RTS_QTDATA", &NewMessage<igtl::RTSQuaternionTrackingDataMessage>);
  this->AddMessageNewMethod("RTS_TDATA", &NewMessage<igtl::RTSTrackingDataMessage>);
  //todo: check if there are more RTS messages

  this->AddMessageNewMethod("STT_BIND", &NewMessage<igtl::StartBindMessage>);
  this->AddMessageNewMethod("STT_TDATA", &NewMessage<igtl::StartTrackingDataMessage>);
  this->AddMessageNewMethod("STT_QTDATA", &NewMessage<igtl::StartQuaternionTrackingDataMessage>);
  //todo: check if there are more STT messages

  this->AddMessageNewMethod("STP_BIND", &NewMessage<igtl::StopBindMessage>);
  this->AddMessageNewMethod("STP_TDATA", &NewMessage<igtl::StopTrackingDataMessage>);
  this->AddMessageNewMethod("STP_QTDATA", &NewMessage<igtl::StopQuaternionTrackingDataMessage>);
  //todo: check if there are more STP messages

  //Own Types
  this->AddMessageNewMethod("DUMMY", &NewMessage<mitk::IGTLDummyMessage>);
}

mitk::IGTLMessageFactory::~IGTLMessageFactory()
{
}

void mitk::IGTLMessageFactory::AddMessageType(std::string messageTypeName,
  IGTLMessageFactory::PointerToMessageBaseNew messageTypeNewPointer,
  mitk::IGTLMessageCloneHandler::Pointer cloneHandler)
{
  this->AddMessageNewMethod(messageTypeName, messageTypeNewPointer);
  this->AddMessageCloneHandler(messageTypeName, cloneHandler);
}

void mitk::IGTLMessageFactory::AddMessageNewMethod(std::string messageTypeName,
  IGTLMessageFactory::PointerToMessageBaseNew messageTypeNewPointer)
{
  this->m_NewMethods[messageTypeName] = messageTypeNewPointer;
}

void mitk::IGTLMessageFactory::AddMessageCloneHandler(std::string msgTypeName,
  mitk::IGTLMessageCloneHandler::Pointer cloneHandler)
{
  this->m_CloneHandlers[msgTypeName] = cloneHandler;
}

mitk::IGTLMessageCloneHandler::Pointer
mitk::IGTLMessageFactory::GetCloneHandler(std::string messageTypeName)
{
  if (this->m_CloneHandlers.find(messageTypeName) !=
    this->m_CloneHandlers.end())
  {
    return m_CloneHandlers[messageTypeName];
  }

  MITK_ERROR("IGTLMessageFactory") << messageTypeName <<
    " message type is not registered to factory!";

  mitkThrow() << messageTypeName << " message type is not registered to factory!";

  return nullptr;
}

igtl::MessageBase::Pointer
mitk::IGTLMessageFactory::Clone(igtl::MessageBase::Pointer msg)
{
  return this->GetCloneHandler(msg->GetDeviceType())->Clone(msg);
}

mitk::IGTLMessageFactory::PointerToMessageBaseNew
mitk::IGTLMessageFactory::GetMessageTypeNewPointer(std::string messageTypeName)
{
  if (this->m_NewMethods.find(messageTypeName) != this->m_NewMethods.end())
  {
    return m_NewMethods[messageTypeName];
  }

  MITK_ERROR("IGTLMessageFactory") << messageTypeName <<
    " message type is not registered to factory!";
  return nullptr;
}

igtl::MessageBase::Pointer
mitk::IGTLMessageFactory::CreateInstance(std::string messageTypeName)
{
  mitk::IGTLMessageFactory::PointerToMessageBaseNew newPointer =
    this->GetMessageTypeNewPointer(messageTypeName);
  if (newPointer != nullptr)
  {
    return newPointer();
  }
  else
  {
    return nullptr;
  }
}

std::list<std::string>
mitk::IGTLMessageFactory::GetAvailableMessageRequestTypes()
{
  std::list<std::string> allGetMessages;
  for (std::map<std::string, PointerToMessageBaseNew>::const_iterator it =
    this->m_NewMethods.begin();
    it != this->m_NewMethods.end(); ++it)
  {
    if (it->first.find("GET_") != std::string::npos ||
      it->first.find("STT_") != std::string::npos ||
      it->first.find("STP_") != std::string::npos ||
      it->first.find("RTS_") != std::string::npos)
    {
      allGetMessages.push_back(it->first);
    }
  }

  return allGetMessages;
}

igtl::MessageBase::Pointer
mitk::IGTLMessageFactory::CreateInstance(igtl::MessageHeader::Pointer msgHeader)
{
  std::string messageType;

  //check the header
  if (msgHeader.IsNull())
  {
    messageType = "NONE";
  }
  else
  {
    messageType = msgHeader->GetDeviceType();
  }

  //make message type uppercase
  messageType = itksys::SystemTools::UpperCase(messageType);

  //find the according new method
  if (this->m_NewMethods.find(messageType)
    != this->m_NewMethods.end())
  {
    if (this->m_NewMethods[messageType] != nullptr)
    {
      // Call tracker New() function if tracker not nullptr
      return (*this->m_NewMethods[messageType])();
    }
    else
      return nullptr;
  }
  else
  {
    MITK_ERROR("IGTLMessageFactory") << "Unknown IGT message type: "
      << messageType;
    return nullptr;
  }
}
