/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef mitkIGTHardwareException_h
#define mitkIGTHardwareException_h

#include "mitkIGTException.h"
#include "mitkExceptionMacro.h"

namespace mitk {
  /**Documentation
  * \brief An object of this class represents an exception of the MITK-IGT module which are releated to the hardware (e.g. connection problems, etc.).
  *
  * \ingroup IGT
  */  class MITKIGTBASE_EXPORT IGTHardwareException /** \cond */ : public mitk::IGTException /** \endcond */
  {
  public:
    mitkExceptionClassMacro(IGTHardwareException,mitk::IGTException);
  };
} // namespace mitk
#endif
