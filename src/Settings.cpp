/*!
  @file Settings.cpp @author Joel Barrett @date 01/01/12 @brief Settings for the application.
*/

#include "Settings.h"

#pragma warning (push)
#pragma warning (disable : 4244)

namespace OceanWaves
{
  void Settings::Load(const char* pFilename)
  {
    // Load and parse the xml document
    TiXmlDocument doc(pFilename);
    if (!doc.LoadFile()) {
      throw std::runtime_error("The file '" + std::string(pFilename) + "' couldn't be found");
    }
    TiXmlHandle hDoc(&doc);
    TiXmlHandle hRoot(0);

    // Root
    {
      TiXmlElement* pNode = hDoc.FirstChildElement().ToElement();
      if (!pNode) {
        throw std::runtime_error("The file '" + std::string(pFilename) + "' doesn't contain a valid root element");
      }
      name_ = pNode->Value();
      hRoot = TiXmlHandle(pNode);
    }

    // Window
    {
      TiXmlElement* pNode = hRoot.FirstChild("Window").FirstChild().ToElement();

      window_.title = pNode->GetText();
      pNode = pNode->NextSiblingElement();

      window_.width = atoi(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      window_.height = atoi(pNode->GetText());
    }

    // Skybox
    {
      TiXmlElement* pNode = hRoot.FirstChild("Skybox").FirstChild().ToElement();
      skyboxTexture_ = pNode->GetText();
    }

    // Ocean
    {
      TiXmlElement* pNode = hRoot.FirstChild("Ocean").FirstChild().ToElement();

      ocean_.skyboxTexture = skyboxTexture_;

      ocean_.fftDim = atoi(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      ocean_.heightmapDim = atoi(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      ocean_.patchLength = atoi(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      ocean_.w = atof(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      ocean_.V = atof(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      ocean_.A = atof(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      ocean_.S = atof(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      ocean_.choppiness = atof(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      ocean_.wavePeriod = atof(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      ocean_.smallestWave = atof(pNode->GetText());
      pNode = pNode->NextSiblingElement();

      ocean_.wireframe = atoi(pNode->GetText());
    }

    // Camera
    {
      TiXmlElement* pNode = hRoot.FirstChild("Camera").FirstChild("Position").ToElement();

      camera_.position = XMFLOAT3(atof(pNode->FirstChild("X")->ToElement()->GetText()),
        atof(pNode->FirstChild("Y")->ToElement()->GetText()),
        atof(pNode->FirstChild("Z")->ToElement()->GetText()));

      pNode = pNode->NextSiblingElement("LookAt");

      camera_.lookAt = XMFLOAT3(atof(pNode->FirstChild("X")->ToElement()->GetText()),
        atof(pNode->FirstChild("Y")->ToElement()->GetText()),
        atof(pNode->FirstChild("Z")->ToElement()->GetText()));
    }
  }
}

#pragma warning (pop)
