/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/

#ifndef mitkUSNavigationLoggingBackend_h
#define mitkUSNavigationLoggingBackend_h

#include <fstream>
#include <vector>

#include <mitkLogBackendText.h>
#include <mitkLogMessage.h>
#include <mitkLogLevel.h>

namespace mitk {
  /**
   *  \brief Backend for the MITK log mechanism. This backend writes all messages to the given file.
   */
  class USNavigationLoggingBackend : public LogBackendText
  {
    public:

      USNavigationLoggingBackend();
      ~USNavigationLoggingBackend() override;

      /** \brief Set file path und name for the output file.
       * The file will be opened and all log messages will be directed there from then on.
       *
       * \throws mitk::Exception if the file cannot be opened
       */
      void SetOutputFileName(std::string filename);

      /** \brief This method is called by the mbi logging mechanism if the object is registered in
       *         the mbi logging mechanism and a logging message is emitted. The method formats the
       *         logging messages to a normal string (depending on formatting mode) and writes it to std::cout.
       *
       *  \param logMessage Logging message.
       */
      void ProcessMessage(const LogMessage &logMessage ) override;

      /** @return Returns all messages of the category "USNavigationLogging" since the last call of ClearNavigationMessages(). */
      std::vector<std::string> GetNavigationMessages();

      /** Writes a CSV file containing all stored navigation messages to the given output filename.
       */
      void WriteCSVFileWithNavigationMessages(std::string filename);

      /** Clears the internally stored navigation messages of the category "USNavigationLogging". */
      void ClearNavigationMessages();

      OutputType GetOutputType() const override;

    private:
      std::ofstream m_OutputStream;
      std::vector<std::string> m_lastNavigationMessages;
      std::vector<std::string> m_allNavigationMessages;
  };
} // namespace mitk


#endif
