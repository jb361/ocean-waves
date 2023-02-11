/*!
  @file Direct3DApp.h @author Joel Barrett @date 01/01/12 @brief A Direct3D 11 application.
*/

#pragma once

#include "Window.h"
#include "Utilities.h"

#include <d3d11.h>
#include <d3dx11.h>
#include <D3DCommon.h>
#include <xnamath.h>

namespace OceanWaves
{
  /*!
    A lightweight Direct3D 11 application wrapper.
  */
  class Direct3DApp : public Window
  {
  protected:
    Direct3DApp() : device_(NULL), immediateContext_(NULL), swapChain_(NULL),
      driverType_(D3D_DRIVER_TYPE_NULL), featureLevel_(D3D_FEATURE_LEVEL_11_0) {}
    ~Direct3DApp();

    HRESULT CheckForSuitableOutput();
    HRESULT CreateDeviceAndSwapChain();

  protected:
    ID3D11Device* device_;
    ID3D11DeviceContext* immediateContext_;
    IDXGISwapChain* swapChain_;
    DXGI_SWAP_CHAIN_DESC swapChainDesc_;

    D3D_DRIVER_TYPE driverType_;
    D3D_FEATURE_LEVEL featureLevel_;
  };
}
