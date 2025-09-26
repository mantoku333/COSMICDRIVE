#include "Constant.hlsli"
struct In
{
    float4 pos : POSITION;
    float3 nor : NORMAL;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    uint instanceId : SV_InstanceID;
};

SamplerState mySampler : register(s0); //ÉTÉìÉvÉâÅ[

PS3D_In main(In input)
{
    PS3D_In output;
    
    output.wpos = mul(input.pos, worldMatrix);
    output.pos = mul(output.wpos, view);
    output.pos = mul(output.pos, proj);
    output.shadowSpacePos = mul(float4(output.wpos.xyz, 1.0f), shadowViewProj);
    
    
    output.normal = input.nor;    
    output.normal = mul(output.normal, (float3x3) rotMtx);
    
    
    output.color = input.color * diffuseColor;
    
    output.uv = input.uv;
    
    return output;
}
