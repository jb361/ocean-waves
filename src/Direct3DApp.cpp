/*!
  @file Direct3DApp.cpp @author Joel Barrett @date 01/01/12 @brief A Direct3D 11 application.
*/

#include <iostream>
#include "Direct3DApp.h"

namespace OceanWaves
{
  Direct3DApp::~Direct3DApp()
  {
    if (immediateContext_)
    {
      immediateContext_->ClearState();
      immediateContext_->Flush();
    }
    SafeRelease(swapChain_);
    SafeRelease(immediateContext_);
    SafeRelease(device_);
  }

  HRESULT Direct3DApp::CreateDeviceAndSwapChain()
  {
    HRESULT hr;

    UINT createDeviceFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

#if defined(DEBUG) | defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE(driverTypes);

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE(featureLevels);

    for (UINT i = 0; i < numDriverTypes; ++i)
    {
      driverType_ = driverTypes[i];
      hr = D3D11CreateDeviceAndSwapChain(NULL, driverType_, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
        D3D11_SDK_VERSION, &swapChainDesc_, &swapChain_, &device_, &featureLevel_, &immediateContext_);
      if (SUCCEEDED(hr)) {
        break;
      }
    }
    if (FAILED(hr)) {
      return DXTRACE_ERR("D3D11CreateDeviceAndSwapChain()", hr);
    }
    return S_OK;
  }
}
