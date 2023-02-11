/*!
  @file Ocean.cpp @author Joel Barrett @date 01/01/12 @brief An ocean surface.
*/

#include "Ocean.h"

namespace OceanWaves
{
// Convert from frequency domain to image domain (-pi...pi to N...M)
#define freqToImage(p) ((XM_2PI * (p - settings_.fftDim * 0.5f)) / settings_.patchLength)

// Return the height at point (z, x) for computing normals
#define height(z, x) (hktOut_[(((z) + settings_.fftDim) % settings_.fftDim) + (settings_.fftDim) * (((x) + settings_.fftDim) % settings_.fftDim)])

  Ocean::~Ocean()
  {
    // Release COM objects
    SafeRelease(skyReflectionSampler_);
    SafeRelease(skyReflectionSRV_);
    SafeRelease(vsConstants_);
    SafeRelease(indexBuffer_);
    SafeRelease(vertexBuffer_);
    SafeRelease(vertexLayout_);
    SafeRelease(wireframePixelShader_);
    SafeRelease(solidPixelShader_);
    SafeRelease(vertexShader_);

    // Release FFTW plans
    fftwf_destroy_plan(nzPlan_);
    fftwf_destroy_plan(nxPlan_);
    fftwf_destroy_plan(DztPlan_);
    fftwf_destroy_plan(DxtPlan_);
    fftwf_destroy_plan(hktPlan_);

    // Release FFTW output buffers
    SafeDeleteArray(nzOut_);
    SafeDeleteArray(nxOut_);
    SafeDeleteArray(DztOut_);
    SafeDeleteArray(DxtOut_);
    SafeDeleteArray(hktOut_);

    // Release FFTW input buffers
    SafeDeleteArray(nzIn_);
    SafeDeleteArray(nxIn_);
    SafeDeleteArray(DztIn_);
    SafeDeleteArray(DxtIn_);
    SafeDeleteArray(hktIn_);

    // Release arrays
    SafeDeleteArray(wk_);
    SafeDeleteArray(h0k_);
    SafeDeleteArray(indices_);
    SafeDeleteArray(vertices_);
  }

  void Ocean::Init(ID3D11Device* device, const OceanSettings& settings)
  {
    // Get device and immediate context
    device_ = device;
    assert(device_);
    device_->GetImmediateContext(&immediateContext_);
    assert(immediateContext_);

    // Initialise ocean variables
    settings_ = settings;
    fftSize_ = settings_.fftDim * settings_.fftDim;
    h0k_ = new XMFLOAT2[fftSize_ * sizeof(XMFLOAT2)];
    wk_ = new float[fftSize_ * sizeof(float)];

    InitShaders();
    InitBuffers();
    InitHeightmap();
    InitFFTW();
    InitTextures();
  }

  HRESULT Ocean::InitShaders()
  {
    HRESULT hr;

    // Create vertex shader
    ID3DBlob* vsBlob = NULL;
    DXCALL(CompileShaderFromFile("assets/shaders/OceanVSPS.hlsl", "OceanVS", "vs_4_0", &vsBlob));
    DXCALL(device_->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
      NULL, &vertexShader_));

    // Create solid pixel shader
    ID3DBlob* spsBlob = NULL;
    DXCALL(CompileShaderFromFile("assets/shaders/OceanVSPS.hlsl", "OceanSolidPS", "ps_4_0", &spsBlob));
    DXCALL(device_->CreatePixelShader(spsBlob->GetBufferPointer(), spsBlob->GetBufferSize(),
      NULL, &solidPixelShader_));

    // Create wireframe pixel shader
    ID3DBlob* wpsBlob = NULL;
    DXCALL(CompileShaderFromFile("assets/shaders/OceanVSPS.hlsl", "OceanWireframePS", "ps_4_0", &wpsBlob));
    DXCALL(device_->CreatePixelShader(wpsBlob->GetBufferPointer(), wpsBlob->GetBufferSize(),
      NULL, &wireframePixelShader_));

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    DXCALL(device_->CreateInputLayout(layout, ARRAYSIZE(layout), vsBlob->GetBufferPointer(),
      vsBlob->GetBufferSize(), &vertexLayout_));

    SafeRelease(vsBlob);
    SafeRelease(spsBlob);
    SafeRelease(wpsBlob);

    return S_OK;
  }

  HRESULT Ocean::InitBuffers()
  {
    HRESULT hr;

    // Create vertex buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    numVertices_ = GenerateVertices(&vertices_, settings_.heightmapDim, 0.2f);
    bd.ByteWidth = sizeof(VertexPosNor) * numVertices_;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA srd;
    ZeroMemory(&srd, sizeof(srd));
    srd.pSysMem = vertices_;
    DXCALL(device_->CreateBuffer(&bd, &srd, &vertexBuffer_));

    // Create index buffer
    numIndices_ = GenerateIndices(&indices_, settings_.heightmapDim);
    bd.ByteWidth = sizeof(WORD) * numIndices_;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    srd.pSysMem = indices_;
    DXCALL(device_->CreateBuffer(&bd, &srd, &indexBuffer_));

    // Create constant buffer
    bd.ByteWidth = PAD16(sizeof(VSConstants));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    DXCALL(device_->CreateBuffer(&bd, NULL, &vsConstants_));

    return S_OK;
  }

  void Ocean::InitFFTW()
  {
    hktIn_ = new fftwf_complex[fftSize_ * sizeof(fftwf_complex)];
    hktOut_ = new float[fftSize_ * sizeof(float)];
    hktPlan_ = fftwf_plan_dft_c2r_2d(settings_.fftDim, settings_.fftDim, hktIn_, hktOut_, FFTW_PATIENT);

    DxtIn_ = new fftwf_complex[fftSize_ * sizeof(fftwf_complex)];
    DxtOut_ = new float[fftSize_ * sizeof(float)];
    DxtPlan_ = fftwf_plan_dft_c2r_2d(settings_.fftDim, settings_.fftDim, DxtIn_, DxtOut_, FFTW_PATIENT);

    DztIn_ = new fftwf_complex[fftSize_ * sizeof(fftwf_complex)];
    DztOut_ = new float[fftSize_ * sizeof(float)];
    DztPlan_ = fftwf_plan_dft_c2r_2d(settings_.fftDim, settings_.fftDim, DztIn_, DztOut_, FFTW_PATIENT);

    nxIn_ = new fftwf_complex[fftSize_ * sizeof(fftwf_complex)];
    nxOut_ = new float[fftSize_ * sizeof(float)];
    nxPlan_ = fftwf_plan_dft_c2r_2d(settings_.fftDim, settings_.fftDim, nxIn_, nxOut_, FFTW_PATIENT);

    nzIn_ = new fftwf_complex[fftSize_ * sizeof(fftwf_complex)];
    nzOut_ = new float[fftSize_ * sizeof(float)];
    nzPlan_ = fftwf_plan_dft_c2r_2d(settings_.fftDim, settings_.fftDim, nzIn_, nzOut_, FFTW_PATIENT);
  }

  HRESULT Ocean::InitTextures()
  {
    HRESULT hr;

    // Create texture
    DXCALL(D3DX11CreateShaderResourceViewFromFile(device_, settings_.skyboxTexture.c_str(),
      NULL, NULL, &skyReflectionSRV_, NULL));

    // Create sampler
    D3D11_SAMPLER_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MinLOD = 0.0f;
    sd.MaxLOD = D3D11_FLOAT32_MAX;
    sd.MaxAnisotropy = 1;
    sd.MipLODBias = 0.0f;
    sd.BorderColor[0] = sd.BorderColor[1] = sd.BorderColor[2] = sd.BorderColor[3] = 1.0f;
    DXCALL(device_->CreateSamplerState(&sd, &skyReflectionSampler_));

    return S_OK;
  }

  float Ocean::Phillips(const XMFLOAT2& k)
  {
    if (k.x == 0.0f && k.y == 0.0f) {
      return 0.0f;
    }
    // Largest possible wave from constant wind speed V
    float L = (settings_.V * settings_.V) / gravity_;

    // Smallest possible wave from constant wind speed V
    float l = L / settings_.smallestWave;

    float ksqr = k.x * k.x + k.y * k.y;
    float hcosf = k.x * cosf(settings_.w) + k.y * sinf(settings_.w);
    float retval = settings_.A * (expf(-1.0f / (ksqr * L * L)) / (ksqr * ksqr * ksqr)) * (hcosf * hcosf);

    // Filter out waves moving opposite to wind
    if (hcosf < 0.0f) {
      retval *= settings_.S;
    }
    return retval * expf(-ksqr * l * l);
  }

  void Ocean::InitHeightmap()
  {
    static const float invSqrt2 = 0.7071068f;
    settings_.w = XMConvertToRadians(settings_.w);

    // Seed random number generator
    srand(0);

    XMFLOAT2 k;

    for (int y = 0; y < settings_.fftDim; ++y)
    {
      k.y = freqToImage(y);

      for (int x = 0; x < settings_.fftDim; ++x)
      {
        k.x = freqToImage(x);

        float sqrtPhk = sqrtf(Phillips(k));
        float Er = GaussRand(), Ei = GaussRand();

        // ~h0(k)
        h0k_[y * settings_.fftDim + x].x = invSqrt2 * Er * sqrtPhk;
        h0k_[y * settings_.fftDim + x].y = invSqrt2 * Ei * sqrtPhk;

        // omega(k)
        wk_[y * settings_.fftDim + x] = sqrtf(gravity_ * sqrtf(k.x * k.x + k.y * k.y));
      }
    }
  }

  void Ocean::Update(const XMFLOAT4X4& world, const XMFLOAT4X4& worldViewProjection, const XMFLOAT3& cp, const XMFLOAT3& cv)
  {
    VSConstants vsc;

    // Send world-view-projection matrix transpose to GPU
    XMStoreFloat4x4(&vsc.world, XMMatrixTranspose(XMLoadFloat4x4(&world)));
    XMStoreFloat4x4(&vsc.worldViewProjection, XMMatrixTranspose(XMLoadFloat4x4(&worldViewProjection)));
    vsc.camPos = cp;
    vsc.camView = cv;

    immediateContext_->UpdateSubresource(vsConstants_, 0, NULL, &vsc, 0, 0);
  }

  void Ocean::UpdateHeightmap(float elapsedTime)
  {
    // h0(k) -> h(k,t)
    for (int y = 0; y < settings_.fftDim; ++y)
    {
      for (int x = 0; x < settings_.fftDim; ++x)
      {
        XMFLOAT2 h0k = h0k_[y * settings_.fftDim + x];
        XMFLOAT2 h0cmk = h0k_[y * settings_.fftDim + x];

        float sin = sinf(wk_[y * settings_.fftDim + x] * elapsedTime * settings_.wavePeriod);
        float cos = cosf(wk_[y * settings_.fftDim + x] * elapsedTime * settings_.wavePeriod);

        hktIn_[y * settings_.fftDim + x][0] = (h0k.x + h0cmk.x) * cos - (h0k.y + h0cmk.y) * sin;
        hktIn_[y * settings_.fftDim + x][1] = (h0k.x - h0cmk.x) * sin + (h0k.y - h0cmk.y) * cos;
      }
    }

    XMFLOAT2 k, l;

    // h(k,t) -> Dx(k,t), Dz(k,t)
    for (int y = 0; y < settings_.fftDim; ++y)
    {
      k.y = l.y = freqToImage(y);

      for (int x = 0; x < settings_.fftDim; ++x)
      {
        k.x = l.x = freqToImage(x);

        float ksqr = k.x * k.x + k.y * k.y;
        float krsqr = (ksqr > 1e-12f) ? 1.0f / sqrt(ksqr) : 0.0f;

        k.x *= krsqr;
        k.y *= krsqr;

        DxtIn_[y * settings_.fftDim + x][0] = k.x * hktIn_[y * settings_.fftDim + x][1];
        DxtIn_[y * settings_.fftDim + x][1] = k.x * -hktIn_[y * settings_.fftDim + x][0];

        DztIn_[y * settings_.fftDim + x][0] = k.y * hktIn_[y * settings_.fftDim + x][1];
        DztIn_[y * settings_.fftDim + x][1] = k.y * -hktIn_[y * settings_.fftDim + x][0];

        nxIn_[y * settings_.fftDim + x][0] = l.x * -hktIn_[y * settings_.fftDim + x][1];
        nxIn_[y * settings_.fftDim + x][1] = l.x * hktIn_[y * settings_.fftDim + x][0];

        nzIn_[y * settings_.fftDim + x][0] = l.y * -hktIn_[y * settings_.fftDim + x][1];
        nzIn_[y * settings_.fftDim + x][1] = l.y * hktIn_[y * settings_.fftDim + x][0];
      }
    }

    fftwf_execute(DxtPlan_);
    fftwf_execute(hktPlan_);
    fftwf_execute(DztPlan_);
    fftwf_execute(nxPlan_);
    fftwf_execute(nzPlan_);

    XMFLOAT3 n;

    for (int z = 0; z < settings_.fftDim; z += 2)
    {
      for (int x = 0; x < settings_.fftDim; x += 2)
      {
        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Pos.x += settings_.choppiness * DxtOut_[z * settings_.fftDim + x];
        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Pos.y = hktOut_[z * settings_.fftDim + x];
        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Pos.z += settings_.choppiness * DztOut_[z * settings_.fftDim + x];

        n.x = nxOut_[z * settings_.fftDim + x];
        n.z = nzOut_[z * settings_.fftDim + x];

        float length = sqrt(n.x * n.x + 1.0f + n.z * n.z);

        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Nor.x = n.x / length;
        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Nor.y = 1.0f / length;
        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Nor.z = n.z / length;
      }
    }

    immediateContext_->UpdateSubresource(vertexBuffer_, 0, NULL, vertices_, 0, 0);
  }

  void Ocean::ComputeNormalsFFT()
  {
    XMFLOAT2 k;
    XMFLOAT3 n;

    for (int y = 0; y < settings_.fftDim; ++y)
    {
      k.y = freqToImage(y);

      for (int x = 0; x < settings_.fftDim; ++x)
      {
        k.x = freqToImage(x);

        nxIn_[y * settings_.fftDim + x][0] = k.x * -hktIn_[y * settings_.fftDim + x][1];
        nxIn_[y * settings_.fftDim + x][1] = k.x * hktIn_[y * settings_.fftDim + x][0];

        nzIn_[y * settings_.fftDim + x][0] = k.y * -hktIn_[y * settings_.fftDim + x][1];
        nzIn_[y * settings_.fftDim + x][1] = k.y * hktIn_[y * settings_.fftDim + x][0];
      }
    }

    fftwf_execute(nxPlan_);
    fftwf_execute(nzPlan_);

    for (int z = 0; z < settings_.fftDim; z += 2)
    {
      for (int x = 0; x < settings_.fftDim; x += 2)
      {
        n.x = nxOut_[z * settings_.fftDim + x];
        n.z = nzOut_[z * settings_.fftDim + x];

        float length = sqrt(n.x * n.x + 1.0f + n.z * n.z);

        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Nor.x = n.x / length;
        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Nor.y = 1.0f / length;
        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Nor.z = n.z / length;
      }
    }
  }

  void Ocean::ComputeNormalsSobel()
  {
    static float damp = 0.4f;

    for (int z = 0; z < settings_.fftDim; z += 2)
    {
      for (int x = 0; x < settings_.fftDim; x += 2)
      {
        // Orthogonal neighbours
        float l = damp * height(z - 1, x); // Left
        float t = damp * height(z, x - 1); // Top
        float r = damp * height(z + 1, x); // Right
        float b = damp * height(z, x + 1); // Bottom

        // Diagonal neighbours
        float tl = damp * height(z - 1, x - 1); // Top left
        float tr = damp * height(z + 1, x - 1); // Top right
        float br = damp * height(z + 1, x + 1); // Bottom right
        float bl = damp * height(z - 1, x + 1); // Bottom left

        // Compute dx and dy using Sobel filter
        float dx = -(tl + 2.0f * l + bl) + (tr + 2.0f * r + br);
        float dy = -(tl + 2.0f * t + tr) + (bl + 2.0f * b + br);

        float length = sqrt(dx * dx + dy * dy + 1.0f);

        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Nor.x = -dx / length;
        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Nor.y = 1.0f / length;
        vertices_[(z / 2) * settings_.heightmapDim + (x / 2)].Nor.z = dy / length;
      }
    }
  }

  void Ocean::Render(bool wireframe)
  {
    static UINT stride = sizeof(VertexPosNor);
    static UINT offset = 0;

    immediateContext_->IASetInputLayout(vertexLayout_);
    immediateContext_->IASetVertexBuffers(0, 1, &vertexBuffer_, &stride, &offset);
    immediateContext_->IASetIndexBuffer(indexBuffer_, DXGI_FORMAT_R16_UINT, 0);
    immediateContext_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    immediateContext_->VSSetShader(vertexShader_, NULL, 0);
    immediateContext_->VSSetConstantBuffers(0, 1, &vsConstants_);

    immediateContext_->PSSetShader(wireframe ? wireframePixelShader_ : solidPixelShader_, NULL, 0);
    immediateContext_->PSSetConstantBuffers(0, 1, &vsConstants_);
    immediateContext_->PSSetShaderResources(0, 1, &skyReflectionSRV_);
    immediateContext_->PSSetSamplers(0, 1, &skyReflectionSampler_);

    immediateContext_->DrawIndexed(numIndices_, 0, 0);
  }
}
