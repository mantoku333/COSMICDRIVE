#include "Constant.hlsli"

cbuffer UVRectBuffer : register(b6)
{
    float2 uvScale;
    float2 uvOffset;
}

struct In
{
    uint instanceId : SV_InstanceID;
};

struct Out
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

// UV座標にスケールとオフセットを適用する関数
float2 SetUV(float2 uv, float2 scale, float2 offset)
{
    return offset + uv * scale;
}

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
    Out gout[vertexCount];
    
    for (int i = 0; i < vertexCount; i++)
    {        
        gout[i].pos = positions[i];
        gout[i].pos = mul(gout[i].pos, worldMatrix);
        gout[i].color = diffuseColor;
        gout[i].uv = uvOffset + uv[i] * uvScale;
    }
    
    output.Append(gout[0]);
    output.Append(gout[1]);
    output.Append(gout[2]);
    
    output.RestartStrip();
    
    output.Append(gout[3]);
    output.Append(gout[4]);
    output.Append(gout[5]);
}