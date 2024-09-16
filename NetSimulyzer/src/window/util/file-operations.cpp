/*
 * NIST-developed software is provided by NIST as a public service. You may use,
 * copy and distribute copies of the software in any medium, provided that you
 * keep intact this entire notice. You may improve,modify and create derivative
 * works of the software or any portion of the software, and you may copy and
 * distribute such modifications or works. Modified works should carry a notice
 * stating that you changed the software and should note the date and nature of
 * any such change. Please explicitly acknowledge the National Institute of
 * Standards and Technology as the source of the software.
 *
 * NIST-developed software is expressly provided "AS IS." NIST MAKES NO
 * WARRANTY OF ANY KIND, EXPRESS, IMPLIED, IN FACT OR ARISING BY OPERATION OF
 * LAW, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTY OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT
 * AND DATA ACCURACY. NIST NEITHER REPRESENTS NOR WARRANTS THAT THE
 * OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED OR ERROR-FREE, OR THAT
 * ANY DEFECTS WILL BE CORRECTED. NIST DOES NOT WARRANT OR MAKE ANY
 * REPRESENTATIONS REGARDING THE USE OF THE SOFTWARE OR THE RESULTS THEREOF,
 * INCLUDING BUT NOT LIMITED TO THE CORRECTNESS, ACCURACY, RELIABILITY,
 * OR USEFULNESS OF THE SOFTWARE.
 *
 * You are solely responsible for determining the appropriateness of using and
 * distributing the software and you assume all risks associated with its use,
 * including but not limited to the risks and costs of program errors,
 * compliance with applicable laws, damage to or loss of data, programs or
 * equipment, and the unavailability or interruption of operation. This
 * software is not intended to be used in any situation where a failure could
 * cause risk of injury or damage to property. The software developed by NIST
 * employees is not subject to copyright protection within the United States.
 *
 * Author: Evan Black <evan.black@nist.gov>
 */

#include "file-operations.h"
#include "src/settings/SettingsManager.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QString>

namespace netsimulyzer {
QString getExistingDirectory(const QString &caption, QWidget *parent) {
  return QFileDialog::getExistingDirectory(parent, caption,
                                           ""
#ifdef __linux__
                                           // Disable native dialogs on linux,
                                           // as some distros have poor performance with them
                                           ,
                                           QFileDialog::DontUseNativeDialog
#endif
  );
}

QString getScenarioFile(QWidget *parent) {

  SettingsManager settings;
  auto lastPath =
      settings.get<QString>(SettingsManager::Key::LastLoadPath, SettingsManager::RetrieveMode::DisallowDefault);

  // Use either the last loaded path, or the current working directory
  QString startingDirectory = ".";
  if (lastPath && QFileInfo{lastPath.value()}.exists())
    startingDirectory = lastPath.value();

  auto selected = QFileDialog::getOpenFileName(parent, "Open Scenario File", startingDirectory, "JSON Files (*.json)",
                                               nullptr
#ifdef __linux__
                                               // Disable native dialogs on linux,
                                               // as some distros have poor performance with them
                                               ,
                                               QFileDialog::DontUseNativeDialog
#endif
  );

  // Save the current path if a file was selected
  QFileInfo selectedFile{selected};
  if (!selected.isEmpty() && selectedFile.exists())
    settings.set(SettingsManager::Key::LastLoadPath, selectedFile.absolutePath());

  return selected;
}

std::string getModelFile(QWidget *parent) {
  SettingsManager settings;
  auto lastPath =
      settings.get<QString>(SettingsManager::Key::LastLoadPath, SettingsManager::RetrieveMode::DisallowDefault);

  // Use either the last loaded path, or the current working directory
  QString startingDirectory = ".";
  if (lastPath && QFileInfo::exists(lastPath.value()))
    startingDirectory = lastPath.value();

  const auto selected = QFileDialog::getOpenFileName(parent, "Open Model File", startingDirectory, "Model Files (*)",
                                                     nullptr
#ifdef __linux__
                                                     // Disable native dialogs on linux,
                                                     // as some distros have poor performance with them
                                                     ,
                                                     QFileDialog::DontUseNativeDialog
#endif
  );

  // Save the current path if a file was selected
  QFileInfo selectedFile{selected};
  if (!selected.isEmpty() && selectedFile.exists() && selectedFile.isFile() && selectedFile.isReadable()) {
    settings.set(SettingsManager::Key::LastLoadPath, selectedFile.absolutePath());
    return selected.toStdString();
  }

  return {};
}

} // namespace netsimulyzer
