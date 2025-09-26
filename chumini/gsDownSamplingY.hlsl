#include "Constant.hlsli"
static const int downSamplingLevel = 8;
struct In
{
    
};

struct Out
{
    float4 pos : SV_POSITION;
    float4 tex0 : TEXCOORD0;
    float4 tex1 : TEXCOORD1;
    float4 tex2 : TEXCOORD2;
    float4 tex3 : TEXCOORD3;
    float4 tex4 : TEXCOORD4;
    float4 tex5 : TEXCOORD5;
    float4 tex6 : TEXCOORD6;
    float4 tex7 : TEXCOORD7;
};

static const int vertexCount = 6;
static const float4 position[vertexCount] =
{
    float4(-1.0f, +1.0f, 0.0f, 1.0f),
    float4(+1.0f, +1.0f, 0.0f, 1.0f),
    float4(-1.0f, -1.0f, 0.0f, 1.0f),
    float4(+1.0f, +1.0f, 0.0f, 1.0f),
    float4(+1.0f, -1.0f, 0.0f, 1.0f),
    float4(-1.0f, -1.0f, 0.0f, 1.0f),
};
static const float2 uv[vertexCount] =
{
    float2(0.0f, 0.0f),
    float2(downSamplingLevel, 0.0f),
    float2(0.0f, downSamplingLevel),
    float2(downSamplingLevel, 0.0f),
    float2(downSamplingLevel, downSamplingLevel),
    float2(0.0f, downSamplingLevel),
};


[maxvertexcount(vertexCount)]
void main(point In input[1], inout TriangleStream<Out> output)
{
    float2 offset[8];
        //1ピクセルの単位量を計算
    float unit = 1.0f / screenSize.y;
    
    offset[0] = float2(0.0f, 1.0f * unit);
    offset[1] = float2(0.0f, 3.0f * unit);
    offset[2] = float2(0.0f, 5.0f * unit);
    offset[3] = float2(0.0f, 7.0f * unit);
    offset[4] = float2(0.0f, 9.0f * unit);
    offset[5] = float2(0.0f, 11.0f * unit);
    offset[6] = float2(0.0f, 13.0f * unit);
    offset[7] = float2(0.0f, 15.0f * unit);
    
    Out ou[6];
    for (int i = 0; i < vertexCount; i++)
    {
        ou[i].pos = position[i];
        
        ou[i].tex0.xy = uv[i] + offset[0];
        ou[i].tex1.xy = uv[i] + offset[1];
        ou[i].tex2.xy = uv[i] + offset[2];
        ou[i].tex3.xy = uv[i] + offset[3];
        ou[i].tex4.xy = uv[i] + offset[4];
        ou[i].tex5.xy = uv[i] + offset[5];
        ou[i].tex6.xy = uv[i] + offset[6];
        ou[i].tex7.xy = uv[i] + offset[7];
        
        ou[i].tex0.zw = uv[i] - offset[0];
        ou[i].tex1.zw = uv[i] - offset[1];
        ou[i].tex2.zw = uv[i] - offset[2];
        ou[i].tex3.zw = uv[i] - offset[3];
        ou[i].tex4.zw = uv[i] - offset[4];
        ou[i].tex5.zw = uv[i] - offset[5];
        ou[i].tex6.zw = uv[i] - offset[6];
        ou[i].tex7.zw = uv[i] - offset[7];
    }
    
    output.Append(ou[0]);
    output.Append(ou[1]);
    output.Append(ou[2]);
    
    output.RestartStrip();
    
    output.Append(ou[3]);
    output.Append(ou[4]);
    output.Append(ou[5]);
}