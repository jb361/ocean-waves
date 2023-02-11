/*!
  @file Ocean.h @author Joel Barrett @date 01/01/12 @brief An ocean surface.
*/

#pragma once

#include <complex>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dcsx.h>
#include <xnamath.h>

#include "fftw3.h"
#include "Settings.h"
#include "Utilities.h"
#include "Vertices.h"

namespace OceanWaves
{
  /*!
    An implementation of Tessendorf's model of ocean surface waves.
  */
  class Ocean
  {
    struct VSConstants
    {
      XMFLOAT4X4 world;
      XMFLOAT4X4 worldViewProjection;
      XMFLOAT3 camPos;
      XMFLOAT3 camView;
    };

  public:
    Ocean() : device_(NULL), immediateContext_(NULL), vertexShader_(NULL), solidPixelShader_(NULL),
      wireframePixelShader_(NULL), vertexLayout_(NULL), vertexBuffer_(NULL), indexBuffer_(NULL),
      vsConstants_(NULL), vertices_(NULL), indices_(NULL), gravity_(9.81f), h0k_(NULL), wk_(NULL) {}
    ~Ocean();

    void Init(ID3D11Device* device, const OceanSettings& settings);
    void Update(const XMFLOAT4X4& world, const XMFLOAT4X4& worldViewProjection, const XMFLOAT3& cp, const XMFLOAT3& cv);
    void UpdateHeightmap(float elapsedTime);
    void Render(bool wireframe);

  private:
    HRESULT InitShaders();
    HRESULT InitBuffers();
    HRESULT InitTextures();

    float Phillips(const XMFLOAT2& k);

    void InitFFTW();
    void InitHeightmap();
    void ComputeNormalsFFT();
    void ComputeNormalsSobel();

  private:
    OceanSettings settings_;
    const float gravity_;
    VertexPosNor* vertices_;
    WORD* indices_;
    unsigned int numVertices_, numIndices_;
    unsigned int fftSize_;
    XMFLOAT2* h0k_;
    float* wk_;

    // FFTW plans and input and output buffers
    fftwf_complex* hktIn_, * DxtIn_, * DztIn_, * nxIn_, * nzIn_;
    float* hktOut_, * DxtOut_, * DztOut_, * nxOut_, * nzOut_;
    fftwf_plan hktPlan_, DxtPlan_, DztPlan_, nxPlan_, nzPlan_;

    ID3D11Device* device_;
    ID3D11DeviceContext* immediateContext_;
    ID3D11VertexShader* vertexShader_;
    ID3D11PixelShader* solidPixelShader_;
    ID3D11PixelShader* wireframePixelShader_;
    ID3D11InputLayout* vertexLayout_;
    ID3D11Buffer* vertexBuffer_;
    ID3D11Buffer* indexBuffer_;
    ID3D11Buffer* vsConstants_;
    ID3D11ShaderResourceView* skyReflectionSRV_;
    ID3D11SamplerState* skyReflectionSampler_;
  };
}
