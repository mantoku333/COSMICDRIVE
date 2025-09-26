
Texture2D myTexture : register(t0);

SamplerState mySampler : register(s0); //ƒTƒ“ƒvƒ‰[

struct In
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};
float4 main(In input) : SV_TARGET
{
#if 1
    return myTexture.Sample(mySampler, input.uv);
#else
    
    float2 unitx = float2(1.0f / 1920, 0);
    float2 unity = float2(0, 1.0f / 1080);
    
    float4 color1 = myTexture.Sample(mySampler, input.uv);
    float4 color2 = myTexture.Sample(mySampler, input.uv - unitx);
    float4 color3 = myTexture.Sample(mySampler, input.uv + unitx);
    
    float l2 = length(color2);
    float l3 = length(color3);
    

    if (distance(l2, l3) > 0.001f)
    {
        return float4(0, 0, 0, 1);
    }
    
    float4 color4 = myTexture.Sample(mySampler, input.uv - unity);
    float4 color5 = myTexture.Sample(mySampler, input.uv + unity);
    
    float l4 = length(color4);
    float l5 = length(color5);
    

    if (distance(l4, l5) > 0.001f)
    {
        return float4(0, 0, 0, 1);
    }
    
    return color1;
#endif
}