#include "Constant.hlsli"

struct In
{
    float4 pos : POSITION;
    float3 nor : NORMAL;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    // Instance Data - Split into 4 float4 for safety/clarity with Input Layout
    float4 iWorld0 : INSTANCE_WORLD0;
    float4 iWorld1 : INSTANCE_WORLD1;
    float4 iWorld2 : INSTANCE_WORLD2;
    float4 iWorld3 : INSTANCE_WORLD3;
    float4 instanceColor : INSTANCE_COLOR;
    uint instanceId : SV_InstanceID;
};

SamplerState mySampler : register(s0);

PS3D_In main(In input)
{
    PS3D_In output;

    // Construct World Matrix from 4 vectors
    // CPU sends transposed matrix (like Mesh::Draw does for wBuffer)
    // In HLSL default column-major mode, the matrix is then used correctly
    // by mul(vector, matrix) which treats vector as row vector.
    float4x4 instanceWorld;
    instanceWorld[0] = input.iWorld0;
    instanceWorld[1] = input.iWorld1;
    instanceWorld[2] = input.iWorld2;
    instanceWorld[3] = input.iWorld3;

    // Use Instance World Matrix instead of global worldMatrix
    output.wpos = mul(input.pos, instanceWorld);
    output.pos = mul(output.wpos, view);
    output.pos = mul(output.pos, proj);
    output.shadowSpacePos = mul(float4(output.wpos.xyz, 1.0f), shadowViewProj);

    output.normal = input.nor;
    // Rotate normal by instance rotation
    output.normal = mul(output.normal, (float3x3)instanceWorld);

    output.color = input.color * diffuseColor * input.instanceColor;
    output.uv = input.uv;

    return output;
}
