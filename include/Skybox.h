/*!
  @file Skybox.h @author Joel Barrett @date 01/01/12 @brief A basic skybox.
*/

#pragma once

#include <d3d11.h>
#include <d3dx11.h>
#include <string>

namespace OceanWaves
{
  /*!
    A basic skybox that renders a cubemap texture.
  */
  class Skybox
  {
    struct VSConstants
    {
      XMFLOAT4X4 view, projection;
    };

  public:
    Skybox() : device_(NULL), immediateContext_(NULL), vertexShader_(NULL), pixelShader_(NULL),
      vertexLayout_(NULL), vertexBuffer_(NULL), indexBuffer_(NULL), vsConstants_(NULL),
      depthStencilState_(NULL), blendState_(NULL), rasterizerState_(NULL),
      textureResourceView_(NULL), linearSampler_(NULL) {}
    ~Skybox();

    void Init(ID3D11Device* device, std::string textureFilename);
    void Update(const XMFLOAT4X4& view, const XMFLOAT4X4& projection);
    void Render();

  private:
    HRESULT InitShaders();
    HRESULT InitBuffers();
    HRESULT InitStates();
    HRESULT InitTextures();

  private:
    std::string textureFilename_;

    ID3D11Device* device_;
    ID3D11DeviceContext* immediateContext_;
    ID3D11VertexShader* vertexShader_;
    ID3D11PixelShader* pixelShader_;
    ID3D11InputLayout* vertexLayout_;
    ID3D11Buffer* vertexBuffer_;
    ID3D11Buffer* indexBuffer_;
    ID3D11Buffer* vsConstants_;
    ID3D11DepthStencilState* depthStencilState_;
    ID3D11BlendState* blendState_;
    ID3D11RasterizerState* rasterizerState_;
    ID3D11ShaderResourceView* textureResourceView_;
    ID3D11SamplerState* linearSampler_;
  };
}
