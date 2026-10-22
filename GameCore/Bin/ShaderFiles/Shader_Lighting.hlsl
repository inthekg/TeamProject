#include "Shader_FullScreen.hlsli"
#include "Shader_GBuffer.hlsli"
#include "Shader_BRDF.hlsli"
#include "Shader_LightCommon.hlsli"
#include "Shader_Spot.hlsli"
#include "Shader_IBL.hlsli"
#include "Shader_CSM.hlsli"
#include "Shader_Point.hlsli"
#include "Shader_RenderTuning.hlsli"
#include "Shader_RimProfile.hlsli"

Texture2D   g_DiffuseTexture    : register(t20);
Texture2D   g_NormalTexture     : register(t21);
Texture2D   g_ARMTexture        : register(t22);
Texture2D   g_DepthTexture      : register(t23);
TextureCube g_IrradianceCube    : register(t25);
TextureCube g_PrefilterCube     : register(t26);
Texture2D   g_BRDFLUT           : register(t27);
Texture2D   g_SSAOTexture       : register(t28);

#define DEBUG_RIM 0
#define RIM_EDGE_EXP  2.0f
#define RIM_FILL_EXP  1.2f

GBuffer_Surface Sample_GBuffer(float2 vTexcoord)
{
    GBuffer_Surface Out = (GBuffer_Surface) 0;
    float4 vDepthDesc = g_DepthTexture.Sample(g_PointSampler, vTexcoord);
    Out.bValid = (vDepthDesc.x > 0.f && vDepthDesc.x < 1.f);
    Out.vWorldPos = Reconstruct_WorldPos(vTexcoord, vDepthDesc);
    Out.vNormal = Decode_Normal(g_NormalTexture.Sample(g_PointSampler, vTexcoord));
    float4 vARM = g_ARMTexture.Sample(g_PointSampler, vTexcoord);
    Out.fAO = vARM.r;
    Out.fRoughness = vARM.g;
    Out.fMetallic = vARM.b;
    Out.vAlbedo = g_DiffuseTexture.Sample(g_PointSampler, vTexcoord).rgb;
            
    return Out;
}

PS_OUT_LIGHT PS_LIGHT_DIRECTIONAL(PS_IN In)
{
    PS_OUT_LIGHT Out = (PS_OUT_LIGHT) 0;
    GBuffer_Surface buffer = Sample_GBuffer(In.vTexcoord);
    if (!buffer.bValid)
        discard;

    float3 vLook = normalize(vCamPosition.xyz - buffer.vWorldPos);
    float3 vLight = normalize(-vLight_Direction.xyz);

    float3 vShade, vSpecular;

    Compute_PBR(buffer.vNormal, vLook, vLight, buffer.vAlbedo, buffer.fAO,
                buffer.fRoughness, buffer.fMetallic, vLight_Diffuse.rgb, vShade, vSpecular);

    float fViewZ = mul(float4(buffer.vWorldPos, 1.f), ViewMatrix).z;
    float fShadow = Compute_Shadow_CSM(buffer.vWorldPos, fViewZ, In.vPosition.xy);

    //Out.vLight = float4((vShade + vSpecular) * fShadow, 1.f);
    Out.vLight = float4(max(vShade + vSpecular, 0.f) * fShadow, 1.f);
    return Out;
}

PS_OUT_LIGHT PS_LIGHT_POINT(PS_IN In)
{
    PS_OUT_LIGHT Out = (PS_OUT_LIGHT) 0;
    GBuffer_Surface buffer = Sample_GBuffer(In.vTexcoord);
    if (!buffer.bValid)
        discard;
    
    float fDist;
    float3 vLight = Get_SurfaceToLight(buffer.vWorldPos, fDist);
    float fAtt = Distance_Attenuation(fDist, fLight_Range);
    if(fAtt <= 0.f)
        discard;
    
    fAtt *= Compute_Shadow_Point(buffer.vWorldPos, iShadowIndex, In.vPosition.xy);
    
    float3 vLook = normalize(vCamPosition.xyz - buffer.vWorldPos);
    float3 fRadiance = vLight_Diffuse.rgb * fAtt;
    
    float3 vShade, vSpecular;
    Compute_PBR(buffer.vNormal, vLook, vLight, buffer.vAlbedo, buffer.fAO, buffer.fRoughness, buffer.fMetallic, fRadiance, vShade, vSpecular);
    Out.vLight = float4(max(vShade + vSpecular, 0.f), 1.f);
    return Out;
}

PS_OUT_LIGHT PS_LIGHT_SPOT(PS_IN In)
{
    PS_OUT_LIGHT Out = (PS_OUT_LIGHT) 0;
    GBuffer_Surface buffer = Sample_GBuffer(In.vTexcoord);
    if (!buffer.bValid)
        discard;
    
    float fDist;
    float3 vLight = Get_SurfaceToLight(buffer.vWorldPos, fDist);
    float fAtt = Distance_Attenuation(fDist, fLight_Range);
    if (fAtt <= 0.f)
        discard;
    
    float fCon = Spot_ConeFactor(vLight);
    if(fCon <= 0.f)
        discard;
    
    
    float3 vLook = normalize(vCamPosition.xyz - buffer.vWorldPos);
    float3 fRadiance = vLight_Diffuse.rgb * fAtt * fCon;
    
    fRadiance *= Compute_Shadow_Spot(buffer.vWorldPos, iShadowIndex, In.vPosition.xy);
    
    float3 vShade, vSpecular;
    Compute_PBR(buffer.vNormal, vLook, vLight, buffer.vAlbedo, buffer.fAO, buffer.fRoughness, buffer.fMetallic, fRadiance, vShade, vSpecular);
    Out.vLight = float4(max(vShade + vSpecular, 0.f), 1.f);
    return Out;
}



PS_OUT_LIGHT PS_IBL_AMBIENT(PS_IN In)
{
    PS_OUT_LIGHT Out = (PS_OUT_LIGHT) 0;

    GBuffer_Surface buffer = Sample_GBuffer(In.vTexcoord);
    if (!buffer.bValid)
        discard;

    float3 V = normalize(vCamPosition.xyz - buffer.vWorldPos);
    float NdotV = saturate(dot(buffer.vNormal, V));
    float3 R = reflect(-V, buffer.vNormal);
    float3 F0 = lerp(0.04, buffer.vAlbedo, buffer.fMetallic);

    float3 F = _FresnelSchlickRoughness(F0, NdotV, buffer.fRoughness);
    float3 kD = saturate((1.0 - F) * (1.0 - buffer.fMetallic));

    // Diffuse IBL ( desat )
    float3 irradiance = g_IrradianceCube.Sample(g_LinearClampSampler, buffer.vNormal).rgb;
    float lum = dot(irradiance, float3(0.2126, 0.7152, 0.0722));
    irradiance = lerp(irradiance, lum.xxx, g_RenderTuning[0].fAmbientDesat);
    float3 diffuseIBL = irradiance * buffer.vAlbedo * buffer.fAO;

    // SSAO
    float fSSAO = g_SSAOTexture.Sample(g_PointSampler, In.vTexcoord).r;

    float3 ambient = (kD * diffuseIBL * g_IBLData[0].fDiffuseIntensity) * fSSAO
                   + buffer.vAlbedo * g_RenderTuning[0].vAmbientFloor * lerp(0.5, 1.0, buffer.fAO);

    Out.vLight = float4(ambient, 1);
    return Out;
}

PS_OUT_LIGHT PS_RIMLIGHT(PS_IN In)
{
    PS_OUT_LIGHT Out = (PS_OUT_LIGHT) 0;

    float4 vDepthDesc = g_DepthTexture.Sample(g_PointSampler, In.vTexcoord);
    int   iProfile = (int)(vDepthDesc.z + 0.5f);
    float fAmount  = vDepthDesc.w;
    if (iProfile < 1 || fAmount <= 0.f)
        discard;

    float3 vWorldPos = Reconstruct_WorldPos(In.vTexcoord, vDepthDesc);
    float3 N = Decode_Normal(g_NormalTexture.Sample(g_PointSampler, In.vTexcoord));
    float3 V = normalize(vCamPosition.xyz - vWorldPos);
    float  fF = 1.0f - saturate(dot(N, V));

    RimProfile prof = g_RimProfiles[iProfile];

    float fEdge = pow(fF, RIM_EDGE_EXP);
    float fFill = pow(fF, RIM_FILL_EXP) * prof.fFill;
    float fRim = fEdge + fFill;

    float3 vAlbedo = g_DiffuseTexture.Sample(g_PointSampler, In.vTexcoord).rgb;
    float fLum = dot(vAlbedo, float3(0.299f, 0.587f, 0.114f));
    fRim *= lerp(0.35f, 1.0f, fLum);

    float3 vRim = prof.vColor * (fRim * prof.fIntensity * fAmount);
    Out.vLight = float4(max(vRim, 0.f), 1.f);
    return Out;
}