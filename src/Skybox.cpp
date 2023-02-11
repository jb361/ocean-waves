/*!
  @file Skybox.cpp @author Joel Barrett @date 01/01/12 @brief A basic skybox.
*/

#include <assert.h>

#include "Utilities.h"
#include "Vertices.h"
#include "Skybox.h"

namespace OceanWaves
{
  Skybox::~Skybox()
  {
    SafeRelease(linearSampler_);
    SafeRelease(textureResourceView_);
    SafeRelease(rasterizerState_);
    SafeRelease(blendState_);
    SafeRelease(depthStencilState_);
    SafeRelease(vsConstants_);
    SafeRelease(indexBuffer_);
    SafeRelease(vertexBuffer_);
    SafeRelease(vertexLayout_);
    SafeRelease(pixelShader_);
    SafeRelease(vertexShader_);
  }

  void Skybox::Init(ID3D11Device* device, std::string textureFilename)
  {
    // Get device and immediate context
    device_ = device;
    assert(device_);
    device_->GetImmediateContext(&immediateContext_);
    assert(immediateContext_);

    textureFilename_ = textureFilename;

    InitShaders();
    InitBuffers();
    InitStates();
    InitTextures();
  }

  HRESULT Skybox::InitShaders()
  {
    HRESULT hr;

    // Create vertex shader
    ID3DBlob* vsBlob = NULL;
    DXCALL(CompileShaderFromFile("Assets/Shaders/SkyboxVSPS.hlsl", "SkyboxVS", "vs_4_0", &vsBlob));
    DXCALL(device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
      NULL, &vertexShader_));

    // Create pixel shader
    ID3DBlob* psBlob = NULL;
    DXCALL(CompileShaderFromFile("Assets/Shaders/SkyboxVSPS.hlsl", "SkyboxPS", "ps_4_0", &psBlob));
    DXCALL(device_->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(),
      NULL, &pixelShader_));

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    DXCALL(device_->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(),
      vsBlob->GetBufferSize(), &vertexLayout_));

    SafeRelease(vsBlob);
    SafeRelease(psBlob);

    return S_OK;
  }

  HRESULT Skybox::InitBuffers()
  {
    HRESULT hr;

    // Create vertex buffer
    VertexPos vertices[] =
    {
        VertexPos(-1.0f, 1.0f, 1.0f),
        VertexPos(1.0f, 1.0f, 1.0f),
        VertexPos(1.0f, -1.0f, 1.0f),
        VertexPos(-1.0f, -1.0f, 1.0f),
        VertexPos(1.0f, 1.0f, -1.0f),
        VertexPos(-1.0f, 1.0f, -1.0f),
        VertexPos(-1.0f, -1.0f, -1.0f),
        VertexPos(1.0f, -1.0f, -1.0f)
    };
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(vertices);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA srd;
    ZeroMemory(&srd, sizeof(srd));
    srd.pSysMem = vertices;
    DXCALL(device_->CreateBuffer(&bd, &srd, &vertexBuffer_));

    // Create index buffer
    unsigned short indices[] =
    {
        0, 1, 2, 2, 3, 0, // Front
        4, 5, 6, 6, 7, 4, // Back
        5, 0, 3, 3, 6, 5, // Left
        1, 4, 7, 7, 2, 1, // Right
        5, 4, 1, 1, 0, 5, // Top
        3, 2, 7, 7, 6, 3  // Bottom
    };
    bd.Usage = D3D11_USAGE_IMMUTABLE;
    bd.ByteWidth = sizeof(indices);
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    srd.pSysMem = indices;
    DXCALL(device_->CreateBuffer(&bd, &srd, &indexBuffer_));

    // Create constant buffer
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    bd.ByteWidth = sizeof(VSConstants);
    DXCALL(device_->CreateBuffer(&bd, NULL, &vsConstants_));

    return S_OK;
  }

  HRESULT Skybox::InitStates()
  {
    HRESULT hr;

    // Create depth-stencil state
    D3D11_DEPTH_STENCIL_DESC dsd;
    dsd.DepthEnable = true;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    dsd.StencilEnable = false;
    dsd.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    dsd.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
    dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    dsd.BackFace = dsd.FrontFace;
    DXCALL(device_->CreateDepthStencilState(&dsd, &depthStencilState_));

    // Create blend state
    D3D11_BLEND_DESC bd;
    bd.AlphaToCoverageEnable = false;
    bd.IndependentBlendEnable = false;
    for (UINT i = 0; i < 8; ++i)
    {
      bd.RenderTarget[i].BlendEnable = false;
      bd.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
      bd.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
      bd.RenderTarget[i].DestBlend = D3D11_BLEND_ONE;
      bd.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
      bd.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
      bd.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
      bd.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
    }
    DXCALL(device_->CreateBlendState(&bd, &blendState_));

    // Create rasterizer state
    D3D11_RASTERIZER_DESC rd;
    rd.AntialiasedLineEnable = FALSE;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthBias = 0;
    rd.DepthBiasClamp = 0.0f;
    rd.DepthClipEnable = TRUE;
    rd.FillMode = D3D11_FILL_SOLID;
    rd.FrontCounterClockwise = false;
    rd.MultisampleEnable = true;
    rd.ScissorEnable = false;
    rd.SlopeScaledDepthBias = 0.0f;
    DXCALL(device_->CreateRasterizerState(&rd, &rasterizerState_));

    return S_OK;
  }

  HRESULT Skybox::InitTextures()
  {
    HRESULT hr;

    // Create texture
    DXCALL(D3DX11CreateShaderResourceViewFromFile(device_, textureFilename_.c_str(),
      NULL, NULL, &textureResourceView_, NULL));

    // Create sampler
    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sd.MinLOD = 0.0f;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    sd.MaxAnisotropy = 1;
    sd.MipLODBias = 0.0f;
    sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 0.0f;
    DXCALL(device_->CreateSamplerState(&sd, &linearSampler_));

    return S_OK;
  }

  void Skybox::Update(const XMFLOAT4X4& view, const XMFLOAT4X4& projection)
  {
    VSConstants vsc;

    // Send view and projection matrix transposes to GPU
    XMStoreFloat4x4(&vsc.view, XMMatrixTranspose(XMLoadFloat4x4(&view)));
    XMStoreFloat4x4(&vsc.projection, XMMatrixTranspose(XMLoadFloat4x4(&projection)));

    immediateContext_->UpdateSubresource(vsConstants_, 0, NULL, &vsc, 0, 0);
  }

  void Skybox::Render()
  {
    static UINT stride = sizeof(VertexPos);
    static UINT offset = 0;

    static float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    immediateContext_->IASetInputLayout(vertexLayout_);
    immediateContext_->IASetVertexBuffers(0, 1, &vertexBuffer_, &stride, &offset);
    immediateContext_->IASetIndexBuffer(indexBuffer_, DXGI_FORMAT_R16_UINT, 0);
    immediateContext_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    immediateContext_->VSSetShader(vertexShader_, NULL, 0);
    immediateContext_->VSSetConstantBuffers(0, 1, &vsConstants_);

    immediateContext_->RSSetState(rasterizerState_);

    immediateContext_->PSSetShader(pixelShader_, NULL, 0);
    immediateContext_->PSSetShaderResources(0, 1, &textureResourceView_);
    immediateContext_->PSSetSamplers(0, 1, &linearSampler_);

    immediateContext_->OMSetBlendState(blendState_, blendFactor, 0xFFFFFFFF);
    immediateContext_->OMSetDepthStencilState(depthStencilState_, 0);

    immediateContext_->DrawIndexed(36, 0, 0);
  }
}
