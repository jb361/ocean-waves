/*!
  @file Scene.cpp @author Joel Barrett @date 01/01/12 @brief Ocean simulation scene.
*/

#include "Scene.h"

namespace OceanWaves
{
  void TW_CALL SetFullscreenCB(const void* value, void* clientData)
  {
    CallBackData* cbd = static_cast<CallBackData*>(clientData);
    cbd->fullscreen = *static_cast<const bool*>(value);

    DXGI_MODE_DESC md;

    if (cbd->fullscreen)
    {
      md.Format = cbd->swapChainDesc.BufferDesc.Format;
      md.Width = GetSystemMetrics(SM_CXSCREEN);
      md.Height = GetSystemMetrics(SM_CYSCREEN);
      md.RefreshRate.Numerator = 0;
      md.RefreshRate.Denominator = 0;
      md.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
      md.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    }
    else {
      md.Format = cbd->swapChainDesc.BufferDesc.Format;
      md.Width = 800;
      md.Height = 600;
      md.RefreshRate.Numerator = 0;
      md.RefreshRate.Denominator = 0;
      md.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
      md.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    }

    cbd->swapChain->ResizeTarget(&md);
    cbd->swapChain->SetFullscreenState(cbd->fullscreen, NULL);
  }

  void TW_CALL GetFullscreenCB(void* value, void* clientData)
  {
    CallBackData* cbd = static_cast<CallBackData*>(clientData);
    *static_cast<bool*>(value) = cbd->fullscreen;
  }

  void TW_CALL SetWireframeCB(const void* value, void* clientData)
  {
    CallBackData* cbd = static_cast<CallBackData*>(clientData);

    cbd->wireframe = *static_cast<const bool*>(value);
    cbd->rasterizerDesc.FillMode = D3D11_FILL_WIREFRAME;
    cbd->device->CreateRasterizerState(&cbd->rasterizerDesc, &cbd->rasterizerState);
    cbd->immediateContext->RSSetState(cbd->rasterizerState);
  }

  void TW_CALL GetWireframeCB(void* value, void* clientData)
  {
    CallBackData* cbd = static_cast<CallBackData*>(clientData);
    *static_cast<bool*>(value) = cbd->wireframe;
  }

  Scene::Scene() : renderTargetView_(NULL), depthStencil_(NULL), depthStencilView_(NULL),
    rasterizerState_(NULL), paused_(false), exitCode_(1)
  {
    XMStoreFloat4x4(&world_, XMMatrixIdentity());
    XMStoreFloat4x4(&worldView_, XMMatrixIdentity());
    XMStoreFloat4x4(&worldViewProjection_, XMMatrixIdentity());

    backgroundColour_[0] = 0.0f;
    backgroundColour_[1] = 0.0f;
    backgroundColour_[2] = 0.0f;
    backgroundColour_[3] = 1.0f;
  }

  Scene::~Scene()
  {
    // Release the tweak bar(s)
    TwTerminate();

    // Release COM objects
    SafeRelease(rasterizerState_);
    SafeRelease(depthStencilView_);
    SafeRelease(depthStencil_);
    SafeRelease(renderTargetView_);
  }

  void Scene::Init()
  {
    settings_.Load("Assets/Settings.xml");

    InitWindow();
    InitDirect3D();
    InitEntities();
    InitAntTweakBar();

    // Initialise the scene elements
    skybox_.Init(device_, settings_.skyboxTexture_);
    ocean_.Init(device_, settings_.ocean_);
  }

  void Scene::Execute()
  {
    MSG msg = { 0 };

    while (true)
    {
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        if (msg.message == WM_QUIT) {
          break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      else {
        Update();
        Render();
      }
    }
    exitCode_ = msg.wParam;
  }

  void Scene::InitWindow()
  {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    Initialise("OceanWindowClass", 0, LoadIcon(hInst_, (LPCTSTR)IDI_DIRECTX_ICON), LoadCursor(NULL, IDC_ARROW),
      (HBRUSH)GetStockObject(WHITE_BRUSH), NULL);

    Display(settings_.window_.title.c_str(), WS_OVERLAPPEDWINDOW | WS_VISIBLE, (screenWidth - settings_.window_.width) / 2,
      (screenHeight - settings_.window_.height) / 2, settings_.window_.width, settings_.window_.height);
  }

  HRESULT Scene::InitDirect3D()
  {
    HRESULT hr;

    // Set window dimensions
    RECT rc;
    GetClientRect(hWnd_, &rc);
    width_ = rc.right - rc.left;
    height_ = rc.bottom - rc.top;

    // Setup device and swap chain
    ZeroMemory(&swapChainDesc_, sizeof(swapChainDesc_));
    swapChainDesc_.BufferDesc.Width = width_;
    swapChainDesc_.BufferDesc.Height = height_;
    swapChainDesc_.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc_.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc_.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc_.SampleDesc.Count = 4;
    swapChainDesc_.SampleDesc.Quality = 0;
    swapChainDesc_.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc_.BufferCount = 1;
    swapChainDesc_.OutputWindow = hWnd_;
    swapChainDesc_.Windowed = TRUE;
    swapChainDesc_.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    CreateDeviceAndSwapChain();

    // Setup render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    DXCALL(swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer));
    DXCALL(device_->CreateRenderTargetView(pBackBuffer, NULL, &renderTargetView_));
    SafeRelease(pBackBuffer);

    // Setup depth stencil texture
    ZeroMemory(&depthStencilDesc_, sizeof(depthStencilDesc_));
    depthStencilDesc_.Width = width_;
    depthStencilDesc_.Height = height_;
    depthStencilDesc_.MipLevels = 1;
    depthStencilDesc_.ArraySize = 1;
    depthStencilDesc_.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc_.SampleDesc = swapChainDesc_.SampleDesc;
    depthStencilDesc_.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc_.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc_.CPUAccessFlags = 0;
    depthStencilDesc_.MiscFlags = 0;
    DXCALL(device_->CreateTexture2D(&depthStencilDesc_, NULL, &depthStencil_));

    // Setup depth stencil view
    ZeroMemory(&depthStencilViewDesc_, sizeof(depthStencilViewDesc_));
    depthStencilViewDesc_.Format = depthStencilDesc_.Format;
    depthStencilViewDesc_.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
    depthStencilViewDesc_.Texture2D.MipSlice = 0;
    DXCALL(device_->CreateDepthStencilView(depthStencil_, &depthStencilViewDesc_, &depthStencilView_));
    immediateContext_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);

    // Setup rasterizer state
    ZeroMemory(&rasterizerDesc_, sizeof(rasterizerDesc_));
    rasterizerDesc_.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc_.CullMode = D3D11_CULL_BACK;
    rasterizerDesc_.MultisampleEnable = (swapChainDesc_.SampleDesc.Count > 0);
    DXCALL(device_->CreateRasterizerState(&rasterizerDesc_, &rasterizerState_));
    immediateContext_->RSSetState(rasterizerState_);

    // Setup viewport
    viewPort_.TopLeftX = 0;
    viewPort_.TopLeftY = 0;
    viewPort_.Width = static_cast<float>(width_);
    viewPort_.Height = static_cast<float>(height_);
    viewPort_.MinDepth = 0.0f;
    viewPort_.MaxDepth = 1.0f;
    immediateContext_->RSSetViewports(1, &viewPort_);

    return S_OK;
  }

  void Scene::InitEntities()
  {
    // Initialise camera
    camera_.SetRotateButtons(true, false, false);
    camera_.SetScalers(0.003f, 0.05f);
    camera_.SetViewMatrix(settings_.camera_.position, settings_.camera_.lookAt);
    camera_.SetProjectionMatrix(XM_PIDIV4, width_ / (float)height_, 0.1f, 1000.0f);

    // Initialise variables used by the tweak bar(s)
    cbd_.fullscreen = false;
    cbd_.wireframe = false;
    cbd_.width = width_;
    cbd_.height = height_;
    cbd_.device = device_;
    cbd_.immediateContext = immediateContext_;
    cbd_.swapChain = swapChain_;
    cbd_.rasterizerState = rasterizerState_;
    cbd_.rasterizerDesc = rasterizerDesc_;
    cbd_.viewPort = viewPort_;
    cbd_.swapChainDesc = swapChainDesc_;
  }

  void Scene::InitAntTweakBar()
  {
    float windDir[3];
    if (!TwInit(TW_DIRECT3D11, device_)) {
      throw std::runtime_error(TwGetLastError());
    }
    // Prevent bars from being moved outside the window
    TwDefine("GLOBAL contained=true");

    // Create a settings bar
    settingsBar_ = TwNewBar("Settings");
    TwDefine("Settings size='220 300' position='15 15'");

    // Application settings
    TwAddVarCB(settingsBar_, "Fullscreen mode", TW_TYPE_BOOLCPP, SetFullscreenCB, GetFullscreenCB, &cbd_, "group=Application key=f");
    TwAddVarCB(settingsBar_, "Wireframe mode", TW_TYPE_BOOLCPP, SetWireframeCB, GetWireframeCB, &cbd_, "group=Application key=y");
    TwAddVarRW(settingsBar_, "Pause", TW_TYPE_BOOLCPP, &paused_, "group=Application key=p");

    // Ocean settings
    TwAddVarRO(settingsBar_, "FFT size", TW_TYPE_INT32, &settings_.ocean_.fftDim, "group=Ocean");
    TwAddVarRO(settingsBar_, "Heightmap size", TW_TYPE_INT32, &settings_.ocean_.heightmapDim, "group=Ocean");
    TwAddVarRO(settingsBar_, "Patch length", TW_TYPE_INT32, &settings_.ocean_.patchLength, "group=Ocean");
    TwAddVarRO(settingsBar_, "Wind velocity", TW_TYPE_FLOAT, &settings_.ocean_.V, "group=Ocean");
    TwAddVarRO(settingsBar_, "Choppiness", TW_TYPE_FLOAT, &settings_.ocean_.choppiness, "group=Ocean");
    TwAddVarRO(settingsBar_, "Wave period", TW_TYPE_FLOAT, &settings_.ocean_.wavePeriod, "group=Ocean");
    TwAddVarRO(settingsBar_, "Wind direction", TW_TYPE_DIR3F, &windDir, "opened=true axisz=-z showval=false");
  }

  HRESULT Scene::ResizeWindow()
  {
    HRESULT hr;

    if (device_)
    {
      // Release render target and depth stencil view
      ID3D11RenderTargetView* nullRTV = NULL;
      immediateContext_->OMSetRenderTargets(1, &nullRTV, NULL);
      SafeRelease(renderTargetView_);
      SafeRelease(depthStencilView_);

      if (swapChain_)
      {
        // Resize swap chain
        swapChainDesc_.BufferDesc.Width = width_;
        swapChainDesc_.BufferDesc.Height = height_;
        DXCALL(swapChain_->ResizeBuffers(swapChainDesc_.BufferCount, swapChainDesc_.BufferDesc.Width,
          swapChainDesc_.BufferDesc.Height, swapChainDesc_.BufferDesc.Format, swapChainDesc_.Flags));

        // Recreate render target and depth stencil view
        ID3D11Texture2D* backBuffer = NULL, * dsBuffer = NULL;
        DXCALL(swapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer));
        DXCALL(device_->CreateRenderTargetView(backBuffer, NULL, &renderTargetView_));
        SafeRelease(backBuffer);
        depthStencilDesc_.Width = swapChainDesc_.BufferDesc.Width;
        depthStencilDesc_.Height = swapChainDesc_.BufferDesc.Height;
        DXCALL(device_->CreateTexture2D(&depthStencilDesc_, NULL, &dsBuffer));
        DXCALL(device_->CreateDepthStencilView(dsBuffer, NULL, &depthStencilView_));
        SafeRelease(dsBuffer);
        immediateContext_->OMSetRenderTargets(1, &renderTargetView_, depthStencilView_);

        // Resize the viewport
        viewPort_.Width = static_cast<float>(swapChainDesc_.BufferDesc.Width);
        viewPort_.Height = static_cast<float>(swapChainDesc_.BufferDesc.Height);
        immediateContext_->RSSetViewports(1, &viewPort_);

        // Alter the aspect ratio of the camera's projection matrix
        camera_.SetProjectionMatrix(XM_PIDIV4, swapChainDesc_.BufferDesc.Width /
          (float)swapChainDesc_.BufferDesc.Height, 0.1f, 1000.0f);
      }
    }
    return S_OK;
  }

  void Scene::Update()
  {
    static float t = 0.0f;

    XMMATRIX world = XMLoadFloat4x4(&world_);
    XMMATRIX view = XMLoadFloat4x4(&camera_.GetViewMatrix());
    XMMATRIX projection = XMLoadFloat4x4(&camera_.GetProjectionMatrix());

    // Compute world-view matrix
    XMMATRIX worldView = XMMatrixMultiply(world, view);
    XMStoreFloat4x4(&worldView_, worldView);

    // Compute world-view-projection matrix
    XMMATRIX worldViewProjection = XMMatrixMultiply(worldView, projection);
    XMStoreFloat4x4(&worldViewProjection_, worldViewProjection);

    // Update the various scene elements
    camera_.Update(1.0f);
    skybox_.Update(camera_.GetViewMatrix(), camera_.GetProjectionMatrix());
    ocean_.Update(world_, worldViewProjection_, camera_.GetPosition(), camera_.GetLookAt());

    if (!paused_) {
      ocean_.UpdateHeightmap(t);
      t += 0.005f;
    }
  }

  void Scene::Render()
  {
    // Clear the back and depth buffers
    immediateContext_->ClearRenderTargetView(renderTargetView_, backgroundColour_);
    immediateContext_->ClearDepthStencilView(depthStencilView_, D3D11_CLEAR_DEPTH, 1.0f, 0);

    if (!cbd_.wireframe) {
      skybox_.Render();
    }
    // TODO Render ocean *before* the skybox for efficiency
    ocean_.Render(cbd_.wireframe);

    // Render the tweak bar(s)
    TwDraw();

    // Present the back buffer to the front buffer
    swapChain_->Present(0, 0);
  }

  LRESULT CALLBACK Scene::MsgRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    PAINTSTRUCT ps;
    HDC hdc;

    // Send message to tweak bar(s) and return if it was handled
    if (TwEventWin(hWnd, uMsg, wParam, lParam)) {
      return 0;
    }
    // Send message to camera so that it can respond to user input
    camera_.HandleMessages(hWnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case WM_PAINT:
      hdc = BeginPaint(hWnd, &ps);
      EndPaint(hWnd, &ps);
      break;

    case WM_KEYDOWN:
      switch (wParam)
      {
      case VK_ESCAPE:
        cbd_.fullscreen = false;

        DXGI_MODE_DESC md;
        md.Format = cbd_.swapChainDesc.BufferDesc.Format;
        md.Width = settings_.window_.width;
        md.Height = settings_.window_.height;
        md.RefreshRate.Numerator = 0;
        md.RefreshRate.Denominator = 0;
        md.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        md.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

        cbd_.swapChain->ResizeTarget(&md);
        cbd_.swapChain->SetFullscreenState(cbd_.fullscreen, NULL);

        PostQuitMessage(0);
        break;
      }
      break;

    case WM_MOVING:
      Render();
      break;

    case WM_SIZING:
      Render();
      break;

    case WM_SIZE:
      width_ = LOWORD(lParam);
      height_ = HIWORD(lParam);

      switch (wParam)
      {
      case SIZE_MAXIMIZED:
        ResizeWindow();
        break;

      case SIZE_RESTORED:
        ResizeWindow();
        break;
      }
      break;

    case WM_CLOSE:
      PostQuitMessage(0);
      break;

    case WM_DESTROY:
      break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
}
