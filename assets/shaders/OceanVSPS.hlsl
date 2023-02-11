/*!
  @file OceanVSPS.hlsl @author Joel Barrett @date 01/01/12 @brief Ocean vertex and pixel shaders.
*/

TextureCube	textureSkyReflection : register(t0);
SamplerState samplerSkyReflection : register(s0);

cbuffer VSConstants : register(cb0)
{
  float4x4 world : packoffset(c0);
  float4x4 worldViewProjection : packoffset(c4);
  float3 camPos : packoffset(c8);
  float3 camView : packoffset(c9);
};

struct VS_INPUT
{
  float4 position : POSITION;
  float3 normal : NORMAL;
};

struct PS_INPUT
{
  float4 position : SV_POSITION;
  float3 normal : TEXCOORD0;
  float3 wsPos : TEXCOORD1;
};

float Fresnel(float3 I, float3 N, float bias, float scale, float power)
{
  return bias + scale * pow(saturate(1.0f - dot(I, N)), power);
}

PS_INPUT OceanVS(VS_INPUT input)
{
  PS_INPUT output;

  output.position = mul(input.position, worldViewProjection);
  output.normal = mul(input.normal, world);
  output.wsPos = mul(input.position, world);

  return output;
}

float4 OceanSolidPS(PS_INPUT input) : SV_Target
{
  float4 shallowWaterColour = float4(0.065f, 0.15f, 0.15f, 1.0f);
  float4 deepWaterColour = float4(0.04f, 0.04f, 0.065f, 1.0f);
  float3 sunDirection = float3(-1.0f, 0.0f, 0.5f);
  float4 sunColour = float4(1.0f, 1.0f, 0.6f, 1.0f);
  float waterShininess = 80.0f;

  // Compute view and blend between two colours to simulate light scattered by water
  float3 view = normalize(camPos - input.wsPos);
  float4 waterColour = lerp(shallowWaterColour, deepWaterColour, saturate(dot(view, input.normal)));

  // Reflect view through normal and sample the environment map
  float3 reflectivity = reflect(-view, input.normal);
  float4 skyReflection = textureSkyReflection.Sample(samplerSkyReflection, reflectivity);

  // Blend the water and sky reflection colours using Fresnel term
  float fresnel = Fresnel(view, input.normal, 0.02037f, 0.97963f, 5.0f);
  float4 fragmentColour = lerp(waterColour, skyReflection, fresnel);

  // Add specular light from the sun
  float4 sunSpecular = pow(saturate(dot(reflectivity, normalize(sunDirection))), waterShininess) * sunColour;
  fragmentColour += sunSpecular;

  return fragmentColour * 0.7f;
}

float4 OceanWireframePS(PS_INPUT input) : SV_Target
{
  return float4(0.207f, 0.388f, 0.380f, 1.0f);
}
