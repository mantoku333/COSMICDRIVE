SamplerState mySampler : register(s0); //サンプラー
Texture2D skyBoxTexture : register(t0); //スカイボックステクスチャ
struct In
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(In input) : SV_TARGET
{
    float4 color = skyBoxTexture.Sample(mySampler, input.uv);
    
    return color;
}