#include "Constant.hlsli"
struct In
{
    uint id : SV_InstanceID;
};

struct Out
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

cbuffer HPBar : register(b6)
{
    float4 position;
};

static const int vertexCount = 6;
static const float4 positions[vertexCount] =
{
    // Front face
    float4(-1.0f, +1.0f, 0.0f, 1.0f),
    float4(+1.0f, +1.0f, 0.0f, 1.0f),
    float4(-1.0f, -1.0f, 0.0f, 1.0f),
    float4(+1.0f, +1.0f, 0.0f, 1.0f),
    float4(+1.0f, -1.0f, 0.0f, 1.0f),
    float4(-1.0f, -1.0f, 0.0f, 1.0f),
};

static const float2 uv[vertexCount] =
{
    // Front face
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f),
};

[maxvertexcount(vertexCount)]
void main(point In input[1], inout TriangleStream<Out> output)
{
    Out ou[vertexCount];
    for (uint i = 0; i < vertexCount; i++)
    {
        
        ou[i].pos = positions[i];
        ou[i].pos = mul(ou[i].pos, worldMatrix);
        
        ou[i].uv = uv[i];
        
        float2 screenUV = float2(ou[i].pos.xy * 0.5f + 0.5f);
        
        ou[i].color = float4(
        (sin(screenUV.x + time) * 0.5f + 0.5f),
        (cos(screenUV.x + time) * 0.5f + 0.5f),
        1,
        1);

    }
    
    output.Append(ou[0]);
    output.Append(ou[1]);
    output.Append(ou[2]);
    
    output.RestartStrip();
    
    output.Append(ou[3]);
    output.Append(ou[4]);
    output.Append(ou[5]);
}