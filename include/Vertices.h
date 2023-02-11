/*!
  @file Vertices.h @author Joel Barrett @date 19/02/12 @brief Custom vertex types.
*/

#pragma once

#include <xnamath.h>

namespace OceanWaves
{
  struct VertexPos
  {
    XMFLOAT3 Pos;

    VertexPos(float x = 0.0f, float y = 0.0f, float z = 0.0f) : Pos(x, y, z) {}
    VertexPos(const XMFLOAT3& p) : Pos(p) {}
  };

  struct VertexPosCol
  {
    XMFLOAT3 Pos;
    XMFLOAT4 Col;

    VertexPosCol(float x = 0.0f, float y = 0.0f, float z = 0.0f, float r = 0.0f, float g = 0.0f,
      float b = 0.0f, float a = 0.0f) : Pos(x, y, z), Col(r, g, b, a) {}

    VertexPosCol(const XMFLOAT3& p, const XMFLOAT4& c) : Pos(p), Col(c) {}
  };

  struct VertexPosTex
  {
    XMFLOAT3 Pos;
    XMFLOAT2 Tex;

    VertexPosTex(float x = 0.0f, float y = 0.0f, float z = 0.0f, float u = 0.0f, float v = 0.0f)
      : Pos(x, y, z), Tex(u, v) {}

    VertexPosTex(const XMFLOAT3& p, const XMFLOAT2& t) : Pos(p), Tex(t) {}
  };

  struct VertexPosNor
  {
    XMFLOAT3 Pos;
    XMFLOAT3 Nor;

    VertexPosNor(float x = 0.0f, float y = 0.0f, float z = 0.0f, float nx = 0.0f, float ny = 0.0f,
      float nz = 0.0f) : Pos(x, y, z), Nor(nx, ny, nz) {}

    VertexPosNor(const XMFLOAT3& p, const XMFLOAT3& n)
      : Pos(p), Nor(n) {}
  };

  struct VertexPosNorCol
  {
    XMFLOAT3 Pos;
    XMFLOAT3 Nor;
    XMFLOAT4 Col;

    VertexPosNorCol(float x = 0.0f, float y = 0.0f, float z = 0.0f, float nx = 0.0f, float ny = 0.0f,
      float nz = 0.0f, float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f)
      : Pos(x, y, z), Nor(nx, ny, nz), Col(r, g, b, a) {}

    VertexPosNorCol(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT4& c)
      : Pos(p), Nor(n), Col(c) {}
  };

  struct VertexPosNorTex
  {
    XMFLOAT3 Pos;
    XMFLOAT3 Nor;
    XMFLOAT2 Tex;

    VertexPosNorTex(float x = 0.0f, float y = 0.0f, float z = 0.0f, float nx = 0.0f,
      float ny = 0.0f, float nz = 0.0f, float u = 0.0f, float v = 0.0f)
      : Pos(x, y, z), Nor(nx, ny, nz), Tex(u, v) {}

    VertexPosNorTex(const XMFLOAT3& p, const XMFLOAT3& n, const XMFLOAT2& t)
      : Pos(p), Nor(n), Tex(t) {}
  };
}
