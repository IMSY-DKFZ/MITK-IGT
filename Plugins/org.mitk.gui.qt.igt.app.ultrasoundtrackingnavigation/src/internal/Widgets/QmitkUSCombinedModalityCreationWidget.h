/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#ifndef QmitkUSCombinedModalityCreationWidget_h
#define QmitkUSCombinedModalityCreationWidget_h

#include <QWidget>

#include "mitkUSCombinedModality.h"

namespace Ui {
class QmitkUSCombinedModalityCreationWidget;
}

/**
 * \brief Widget that enables the user to create a mitk::USCombinedModality of a mitk::NavigationDataSource and a mitk::USDevice.
 * A mitk::NavigationDataSource and a mitk::USDevice can be picked from two lists, showing
 * the corresponding micro service objects. The combined modality is registered as a micro
 * service as well during the creation process.
 */
class QmitkUSCombinedModalityCreationWidget : public QWidget
{
  Q_OBJECT

signals:
  /** \brief Emitted when the user clicks the "Cancel" button. */
  void SignalAborted();

  /** \brief Emitted when the user clicked the "Create" button and the creation is successful. */
  void SignalCreated(mitk::USCombinedModality::Pointer combinedModality);

  /** \brief Emitted when the user clicked the "Create" button and the creation is successful. */
  void SignalCreated();

protected slots:
  /** \brief Creates a combined modility of the selected mitk::NavigationDataSource and mitk::USDevice. **/
  void OnCreation();

  /** \brief Handles the enabled state of the "Create" button. **/
  void OnSelectedUltrasoundOrTrackingDevice();

public:
  explicit QmitkUSCombinedModalityCreationWidget(QWidget *parent = nullptr);
  ~QmitkUSCombinedModalityCreationWidget() override;

private:
  Ui::QmitkUSCombinedModalityCreationWidget *ui;

  itk::SmartPointer<mitk::AbstractUltrasoundTrackerDevice> m_CombinedModality;
};

#endif
