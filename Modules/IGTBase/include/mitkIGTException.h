/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef mitkIGTException_h
#define mitkIGTException_h

#include <MitkIGTBaseExports.h>
#include <mitkException.h>
#include "mitkExceptionMacro.h"

namespace mitk {
/**Documentation
* \brief An object of this class represents an exception of the MITK-IGT module.
*
* \ingroup IGT
*/  class MITKIGTBASE_EXPORT IGTException /** \cond */ : public mitk::Exception /** \endcond */
{
  public:
    mitkExceptionClassMacro(IGTException, mitk::Exception)
  };
} // namespace mitk
#endif
