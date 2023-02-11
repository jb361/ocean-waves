/*!
  @file Scene.h @author Joel Barrett @date 01/01/12 @brief Ocean simulation scene.
*/

#pragma once

#include "AntTweakBar.h"
#include "Camera.h"
#include "Direct3DApp.h"
#include "Ocean.h"
#include "Resource.h"
#include "Settings.h"
#include "Skybox.h"

namespace OceanWaves
{
  /*
    Variables used by the tweak bar(s).
  */
  struct CallBackData
  {
    bool fullscreen, wireframe;

    UINT width, height;
    ID3D11Device* device;
    ID3D11DeviceContext* immediateContext;
    IDXGISwapChain* swapChain;
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ID3D11RasterizerState* rasterizerState;
    D3D11_VIEWPORT viewPort;
    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    IDXGIOutput* output;
  };

  /*!
    Class for arranging the various the scene elements.
  */
  class Scene : public Direct3DApp
  {
  public:
    Scene();
    ~Scene();

    void Init();
    void Execute();

    int GetExitCode() const { return exitCode_; }

  private:
    void InitWindow();
    void InitEntities();
    void InitAntTweakBar();

    HRESULT InitDirect3D();
    HRESULT ResizeWindow();

    void Update();
    void Render();

    //! Override the message router in the window class
    LRESULT CALLBACK MsgRouter(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

  private:
    Settings settings_;
    TwBar* settingsBar_;
    FirstPersonCamera camera_;
    Ocean ocean_;
    Skybox skybox_;
    CallBackData cbd_;

    XMFLOAT4X4 world_, worldView_, worldViewProjection_;
    float backgroundColour_[4];
    int fps_, exitCode_;
    bool paused_;

    ID3D11RenderTargetView* renderTargetView_;
    D3D11_TEXTURE2D_DESC depthStencilDesc_;
    ID3D11Texture2D* depthStencil_;
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc_;
    ID3D11DepthStencilView* depthStencilView_;
    D3D11_RASTERIZER_DESC rasterizerDesc_;
    ID3D11RasterizerState* rasterizerState_;
    D3D11_VIEWPORT viewPort_;
  };
}
