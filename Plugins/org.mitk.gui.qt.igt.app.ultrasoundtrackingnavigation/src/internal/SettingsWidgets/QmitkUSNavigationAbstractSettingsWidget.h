/*============================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center (DKFZ)
All rights reserved.

Use of this source code is governed by a 3-clause BSD license that can be
found in the LICENSE file.

============================================================================*/
#ifndef QmitkUSNavigationAbstractSettingsWidget_h
#define QmitkUSNavigationAbstractSettingsWidget_h

#include <QWidget>

#include "mitkDataNode.h"

/**
 * \brief Abstract class of settings widgets used by the QmitkUSNavigationProcessWidget.
 * This class handles the emitting of Saved(), Canceled() and SettingsChanged()
 * signals and provides slots for save and cancel buttons.
 *
 * The actual saving and loading of settings is delegated to concrete subclasses
 * which should therefore implement OnSetSettingsNode(), OnSaveProcessing(),
 * OnCancelProcessing(), OnLoadSettingsProcessing().
 */
class QmitkUSNavigationAbstractSettingsWidget : public QWidget
{
  Q_OBJECT

signals:
  void Saved();
  void Canceled();
  void SettingsChanged(itk::SmartPointer<mitk::DataNode>);

protected slots:
  /**
   * \brief Triggers the saving of the current settings.
   * OnSaveProcessing() is called and the Saved() signal is emitted afterwards.
   */
  void OnSave();

  /**
   * \brief Ends the settings changing process without saving the changes.
   * OnCancelProcessing() is called and the Canceled() signal is emitted
   * afterwards.
   */
  void OnCancel();

public:
  explicit QmitkUSNavigationAbstractSettingsWidget(QWidget *parent = nullptr);
  ~QmitkUSNavigationAbstractSettingsWidget() override;

  /**
   * \brief Loads the settings.
   * The actual loading is done by calling the OnLoadSettingsProcessing()
   * method.
   */
  void LoadSettings();

  /**
   * \brief Setter for the data node in which the settings are loaded.
   * The settings are made available as properties of this data node.
   */
  void SetSettingsNode(itk::SmartPointer<mitk::DataNode> settingsNode, bool overwriteValues = false);
  itk::SmartPointer<mitk::DataNode> GetSettingsNode();

protected:
  /**
   * \brief Method for handling the setting of a new settings node.
   * This method has to be implemented by a concrete subclass.
   */
  virtual void OnSetSettingsNode(itk::SmartPointer<mitk::DataNode> settingsNode, bool overwriteValues) = 0;

  /**
   * \brief Method for actually saving the settings.
   * This method has to be implemented by a concrete subclass.
   */
  virtual void OnSaveProcessing() = 0;

  /**
   * \brief Method for handling the cancel process.
   * This method can be implemented by a concrete subclass. An empty default
   * implementation exists.
   */
  virtual void OnCancelProcessing() {}

  /**
   * \brief Method for actually loading the settings.
   * This method has to be implemented by a concrete subclass.
   */
  virtual void OnLoadSettingsProcessing() = 0;

private:
  mitk::DataNode::Pointer m_SettingsNode;
};

#endif
