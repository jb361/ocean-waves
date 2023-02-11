/*!
  @file Utilities.h @author Joel Barrett @date 01/01/12 @brief Utility functions.
*/

#pragma once

#include <windows.h>
#include <d3dx11.h>
#include <dxerr.h>

#include "Vertices.h"

#define PAD16(bytes) (((bytes) + 15) / 16 * 16)
#define DXCALL(function) { hr = (function); if (FAILED(hr)) { return DXTRACE_ERR(#function, hr); } }

namespace OceanWaves
{
  // Delete pointers and arrays of pointers, or release an object
  template < typename T > inline void SafeDelete(T*& p) { delete p; p = NULL; }
  template < typename T > inline void SafeDeleteArray(T*& p) { delete[] p; p = NULL; }
  template < typename T > inline void SafeRelease(T*& p) { if (p) { p->Release(); } p = NULL; }

  // Return a Gaussian random number with mean 0 and standard deviation 1
  float GaussRand();

  // Generate vertices and indices for a heightmap
  unsigned int GenerateVertices(VertexPosNor** vertices, int dimensions, float stride);
  unsigned int GenerateIndices(WORD** indices, int dimensions);

  // Helper function from the DirectX SDK for compiling a shader
  HRESULT CompileShaderFromFile(CHAR* fileName, LPCSTR entryPoint, LPCSTR shaderModel, ID3DBlob** blob);
}
