#include "Constant.hlsli"
struct In
{
    uint instanceId : SV_InstanceID;
};

#if 1
static const int vertexCount = 36;

//ç¿ïWèÓïÒ
static const float4 positions[vertexCount] =
{
    // Front face
    float4(-0.5f, +0.5f, -0.5f, 1.0f),
    float4(+0.5f, +0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, -0.5f, 1.0f),
    float4(+0.5f, +0.5f, -0.5f, 1.0f),
    float4(+0.5f, -0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, -0.5f, 1.0f),

    // Back face
    float4(-0.5f, +0.5f, +0.5f, 1.0f),
    float4(-0.5f, -0.5f, +0.5f, 1.0f),
    float4(+0.5f, +0.5f, +0.5f, 1.0f),
    float4(+0.5f, +0.5f, +0.5f, 1.0f),
    float4(-0.5f, -0.5f, +0.5f, 1.0f),
    float4(+0.5f, -0.5f, +0.5f, 1.0f),

    // Left face
    float4(-0.5f, +0.5f, +0.5f, 1.0f),
    float4(-0.5f, +0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, +0.5f, 1.0f),
    float4(-0.5f, +0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, +0.5f, 1.0f),

    // Right face
    float4(+0.5f, +0.5f, +0.5f, 1.0f),
    float4(+0.5f, -0.5f, +0.5f, 1.0f),
    float4(+0.5f, +0.5f, -0.5f, 1.0f),
    float4(+0.5f, +0.5f, -0.5f, 1.0f),
    float4(+0.5f, -0.5f, +0.5f, 1.0f),
    float4(+0.5f, -0.5f, -0.5f, 1.0f),

    // Top face
    float4(-0.5f, +0.5f, +0.5f, 1.0f),
    float4(+0.5f, +0.5f, +0.5f, 1.0f),
    float4(-0.5f, +0.5f, -0.5f, 1.0f),
    float4(+0.5f, +0.5f, +0.5f, 1.0f),
    float4(+0.5f, +0.5f, -0.5f, 1.0f),
    float4(-0.5f, +0.5f, -0.5f, 1.0f),

    // Bottom face
    float4(-0.5f, -0.5f, +0.5f, 1.0f),
    float4(-0.5f, -0.5f, -0.5f, 1.0f),
    float4(+0.5f, -0.5f, +0.5f, 1.0f),
    float4(+0.5f, -0.5f, +0.5f, 1.0f),
    float4(-0.5f, -0.5f, -0.5f, 1.0f),
    float4(+0.5f, -0.5f, -0.5f, 1.0f),
};

// UVç¿ïW
static const float2 uv[vertexCount] =
{
    // Front face
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f),

    // Back face
    float2(0.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),

    // Left face
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f),

    // Right face
    float2(0.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),

    // Top face
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f),

    // Bottom face
    float2(0.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f),
};

// ñ@ê¸èÓïÒ
static const float3 normal[vertexCount] =
{
    // Front face
    float3(0.0f, 0.0f, -1.0f),
    float3(0.0f, 0.0f, -1.0f),
    float3(0.0f, 0.0f, -1.0f),
    float3(0.0f, 0.0f, -1.0f),
    float3(0.0f, 0.0f, -1.0f),
    float3(0.0f, 0.0f, -1.0f),

    // Back face
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 0.0f, 1.0f),
    float3(0.0f, 0.0f, 1.0f),

    // Left face
    float3(-1.0f, 0.0f, 0.0f),
    float3(-1.0f, 0.0f, 0.0f),
    float3(-1.0f, 0.0f, 0.0f),
    float3(-1.0f, 0.0f, 0.0f),
    float3(-1.0f, 0.0f, 0.0f),
    float3(-1.0f, 0.0f, 0.0f),

    // Right face
    float3(1.0f, 0.0f, 0.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(1.0f, 0.0f, 0.0f),
    float3(1.0f, 0.0f, 0.0f),

    // Top face
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),

    // Bottom face
    float3(0.0f, -1.0f, 0.0f),
    float3(0.0f, -1.0f, 0.0f),
    float3(0.0f, -1.0f, 0.0f),
    float3(0.0f, -1.0f, 0.0f),
    float3(0.0f, -1.0f, 0.0f),
    float3(0.0f, -1.0f, 0.0f),
};
#endif

[maxvertexcount(vertexCount)]
void main(point In input[1], inout TriangleStream<PS3D_In> output)
{
    for (int i = 0; i < vertexCount; i++)
    {
        PS3D_In ou = (PS3D_In) 0;
        
        ou.pos = positions[i];
        ou.wpos = mul(ou.pos, worldMatrix);
        ou.pos = mul(ou.wpos, view);
        ou.pos = mul(ou.pos, proj);
        
        
        ou.shadowSpacePos = mul(float4(ou.wpos.xyz, 1.0f), shadowViewProj);
        
        matrix rotMtx = worldMatrix;
        rotMtx._14 = 0.0f;
        rotMtx._24 = 0.0f;
        rotMtx._34 = 0.0f;
        rotMtx._41 = 0.0f;
        rotMtx._42 = 0.0f;
        rotMtx._43 = 0.0f;
        rotMtx._44 = 1.0f;
        
        ou.normal = normal[i];
        ou.normal = mul(ou.normal, rotMtx);
        ou.uv = uv[i];
        ou.color = diffuseColor;
        
        if (!(i % 3))
        {
            output.RestartStrip();
        }
        output.Append(ou);
    }
}