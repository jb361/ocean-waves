/*!
  @file Settings.h @author Joel Barrett @date 01/01/12 @brief Settings for the application.
*/

#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <xnamath.h>

#define TIXML_USE_STL
#include "TinyXML.h"

namespace OceanWaves
{
  struct WindowSettings
  {
    std::string title;
    int width, height;
  };

  struct OceanSettings
  {
    std::string skyboxTexture;
    int fftDim, heightmapDim, patchLength, wireframe;
    float w, V, A, S, choppiness, wavePeriod, smallestWave;
  };

  struct CameraSettings
  {
    XMFLOAT3 position, lookAt;
  };

  class Settings
  {
  public:
    void Load(const char* pFilename);

  public:
    std::string name_;

    WindowSettings window_;
    std::string skyboxTexture_;
    OceanSettings ocean_;
    CameraSettings camera_;
  };
}
