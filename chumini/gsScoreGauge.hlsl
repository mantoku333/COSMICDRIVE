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

// Reuse HPBar buffer slot (b6) just to match layout, even if unused or use it for custom data
cbuffer HPBar : register(b6)
{
    float4 position;
};

static const int vertexCount = 6;
static const float4 positions[vertexCount] =
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
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f),
};

// Rainbow Function
float3 Rainbow(float t)
{
    float3 c;
    c.r = sin(t) * 0.5 + 0.5;
    c.g = sin(t + 2.094) * 0.5 + 0.5;
    c.b = sin(t + 4.188) * 0.5 + 0.5;
    return c;
}

[maxvertexcount(vertexCount)]
void main(point In input[1], inout TriangleStream<Out> output)
{
    Out ou[vertexCount];
    
    // Fallback screen size if 0
    float2 scr = screenSize;
    if (scr.x <= 1.0f) scr = float2(1920.0f, 1080.0f);

    for (uint i = 0; i < vertexCount; i++)
    {
        ou[i].pos = positions[i];
        
        // 1. Pixel Space Transform
        ou[i].pos = mul(ou[i].pos, worldMatrix);
        
        // 2. Clip Space Projection
        ou[i].pos.xy = ou[i].pos.xy / (scr * 0.5f);
        ou[i].pos.w = 1.0f; 
        
        ou[i].uv = uv[i];
        
        float2 screenUV = float2(ou[i].pos.xy * 0.5f + 0.5f);
        
        // Dynamic Color
        // Use diffuseColor passed from C++ (Rank Color)
        float3 baseColor = diffuseColor.rgb;
        
        float t = time * 2.0f + screenUV.x * 5.0f;
        // Subtle shine/wave effect on top of base color
        float shine = sin(t) * 0.1f + 0.1f;
        
        baseColor += shine; // Add shine
        
        float brightness = 1.0f + (1.0f - ou[i].uv.y) * 0.5f;
        
        ou[i].color = float4(baseColor * brightness, 1.0f);
    }
    
    output.Append(ou[0]);
    output.Append(ou[1]);
    output.Append(ou[2]);
    
    output.RestartStrip();
    
    output.Append(ou[3]);
    output.Append(ou[4]);
    output.Append(ou[5]);
}
