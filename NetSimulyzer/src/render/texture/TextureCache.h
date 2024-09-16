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

#pragma once

#include "texture.h"
#include <QDir>
#include <QImage>
#include <QOpenGLFunctions_3_3_Core>
#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace netsimulyzer {

/**
 * ID for outside code to access a given texture
 */
using texture_id = std::size_t;

class TextureCache : protected QOpenGLFunctions_3_3_Core {
  std::unordered_map<std::string, std::size_t> indexMap;
  std::vector<Texture> textures;
  texture_id fallbackTexture;
  QDir resourceDirectory;

public:
  struct CubeMap {
    QImage right;
    QImage left;
    QImage top;
    QImage bottom;
    QImage back;
    QImage front;
  };

  ~TextureCache() override;

  bool init();

  void setResourceDirectory(const QDir &value);
  texture_id load(const std::string &filename);
  unsigned int load(const CubeMap &cubeMap);
  texture_id loadInternal(const std::string &path, GLint filter = GL_LINEAR, GLint repeat = GL_REPEAT);
  [[nodiscard]] const Texture &get(texture_id index);

  [[nodiscard]] texture_id getFallbackTexture() const;

  const Texture &operator[](texture_id index) {
    return get(index);
  }

  void clear();

  void use(texture_id index);

  // TODO: Track the same way as normal textures
  void useCubeMap(unsigned int id);
};

} // namespace netsimulyzer
