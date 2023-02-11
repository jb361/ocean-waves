/*!
  @file SkyboxVSPS.hlsl @author Joel Barrett @date 01/01/12 @brief Skybox vertex and pixel shaders.
*/

TextureCube textureCube : register(t0);
SamplerState samplerLinear : register(s0);

cbuffer VSConstants : register(cb0)
{
  float4x4 view : packoffset(c0);
  float4x4 projection : packoffset(c4);
}

struct VS_INPUT
{
  float4 position : POSITION;
};

struct PS_INPUT
{
  float4 position : SV_POSITION;
  float3 texCoord : TEXCOORD;
};

PS_INPUT SkyboxVS(VS_INPUT input)
{
  PS_INPUT output;

  // Transform into view space, centered at the camera position
  float3 vsPos = mul(input.position.xyz, (float3x3)view);

  // Transform into clip space and set texture coordinate
  output.position = mul(float4(vsPos, 1.0f), projection);
  output.texCoord = normalize(input.position);

  return output;
}

float4 SkyboxPS(PS_INPUT input) : SV_Target
{
  return textureCube.Sample(samplerLinear, input.texCoord);
}
