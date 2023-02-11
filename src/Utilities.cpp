/*!
  @file Utilities.cpp @author Joel Barrett @date 01/01/12 @brief Utility functions.
*/

#include <windows.h>
#include <d3dcompiler.h>
#include <D3Dcommon.h>
#include <xnamath.h>

#include "Utilities.h"

namespace OceanWaves
{
  float GaussRand()
  {
    // Uniform random numbers
    float u1 = rand() / (float)RAND_MAX;
    float u2 = rand() / (float)RAND_MAX;

    // Box-Muller transform
    if (u1 < 1e-6f) {
      u1 = 1e-6f;
    }
    return sqrtf(-2.0f * logf(u1)) * cosf(XM_2PI * u2);
  }

  unsigned int GenerateVertices(VertexPosNor** vertices, int dimensions, float stride)
  {
    unsigned int numVertices = dimensions * dimensions;
    *vertices = new VertexPosNor[numVertices];

    float halfDim = (dimensions - 1.0f) / 2.0f;

    for (int z = 0; z < dimensions; ++z)
    {
      for (int x = 0; x < dimensions; ++x)
      {
        (*vertices)[z * dimensions + x] = VertexPosNor((x - halfDim) * stride, 0.0f,
          (z - halfDim) * stride, 0.0f, 1.0f, 0.0f);
      }
    }
    return numVertices;
  }

  unsigned int GenerateIndices(WORD** indices, int dimensions)
  {
    unsigned int numIndices = (dimensions * 2) * (dimensions - 1) + (dimensions - 2);
    *indices = new WORD[numIndices];

    unsigned int index = 0;
    for (int z = 0; z < dimensions - 1; ++z)
    {
      // Even rows move left to right, odd rows move right to left
      if (z % 2 == 0)
      {
        // Even row
        int x;
        for (x = 0; x < dimensions; ++x)
        {
          (*indices)[index++] = x + (z * dimensions);
          (*indices)[index++] = x + (z * dimensions) + dimensions;
        }
        // Insert degenerate vertex if this isn't the last row
        if (z != dimensions - 2)
        {
          (*indices)[index++] = --x + (z * dimensions);
        }
      }
      else {
        // Odd row
        int x;
        for (x = dimensions - 1; x >= 0; --x)
        {
          (*indices)[index++] = x + (z * dimensions);
          (*indices)[index++] = x + (z * dimensions) + dimensions;
        }
        // Insert degenerate vertex if this isn't the last row
        if (z != dimensions - 2)
        {
          (*indices)[index++] = ++x + (z * dimensions);
        }
      }
    }
    return numIndices;
  }

  HRESULT CompileShaderFromFile(char* fileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** blob)
  {
    HRESULT hr;

    DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG) || defined(_DEBUG)
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    shaderFlags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* errorBlob;
    hr = D3DX11CompileFromFile(fileName, NULL, NULL, entryPoint, shaderModel,
      shaderFlags, 0, NULL, blob, &errorBlob, NULL);
    if (FAILED(hr))
    {
      OutputDebugStringA((char*)errorBlob->GetBufferPointer());
      SafeRelease(errorBlob);
      return hr;
    }
    SafeRelease(errorBlob);
    return S_OK;
  }
}
