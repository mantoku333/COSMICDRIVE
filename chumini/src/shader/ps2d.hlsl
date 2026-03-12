#include "Constant.hlsli"
Texture2D diffuseTexture : register(t0);
SamplerState mySampler : register(s0); //サンプラー
struct In
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

PS2D_Out main(In input)
{
    PS2D_Out output;
    
    output.baseColor = input.color;
    
    if (textureEnable.x)
    {
        float4 texColor = diffuseTexture.Sample(mySampler, input.uv);
        output.baseColor *= texColor;
    }
    
    // 非常に透明な部分のみ破棄
    if (output.baseColor.a < 0.001)
    {
        discard;
    }
    
    // アルファ値を正しく伝播させる
    float alpha = output.baseColor.a;
    output.alphaBuffer = float4(alpha, alpha, alpha, alpha);
    
    return output;
}