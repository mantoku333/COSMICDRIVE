Texture2D luminance : register(t0);
SamplerState mySampler : register(s0); //サンプラー

static const float v0 = 0.124450631;
static const float v1 = 0.116910554;
static const float v2 = 0.0969222561;
static const float v3 = 0.0709098354;
static const float v4 = 0.0457828306;
static const float v5 = 0.0260862708;
static const float v6 = 0.0131170005;
static const float v7 = 0.00582063338;

struct In
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
float4 main(In In) : SV_TARGET
{
    // step-15 横、縦ブラー用のピクセルシェーダーを実装
    float4 Color = float4(0, 0, 0, 0);
    float4 weight[2];
    weight[0] = float4(v0, v1, v2, v3);
    weight[1] = float4(v4, v5, v6, v7);
    
    
    // 基準テクセルからプラス方向に8テクセル、重み付きでサンプリング
    Color += weight[0].x * luminance.Sample(mySampler, In.tex0.xy);
    Color += weight[0].y * luminance.Sample(mySampler, In.tex1.xy);
    Color += weight[0].z * luminance.Sample(mySampler, In.tex2.xy);
    Color += weight[0].w * luminance.Sample(mySampler, In.tex3.xy);
    Color += weight[1].x * luminance.Sample(mySampler, In.tex4.xy);
    Color += weight[1].y * luminance.Sample(mySampler, In.tex5.xy);
    Color += weight[1].z * luminance.Sample(mySampler, In.tex6.xy);
    Color += weight[1].w * luminance.Sample(mySampler, In.tex7.xy);

    // 基準テクセルにマイナス方向に8テクセル、重み付きでサンプリング
    Color += weight[0].x * luminance.Sample(mySampler, In.tex0.zw);
    Color += weight[0].y * luminance.Sample(mySampler, In.tex1.zw);
    Color += weight[0].z * luminance.Sample(mySampler, In.tex2.zw);
    Color += weight[0].w * luminance.Sample(mySampler, In.tex3.zw);
    Color += weight[1].x * luminance.Sample(mySampler, In.tex4.zw);
    Color += weight[1].y * luminance.Sample(mySampler, In.tex5.zw);
    Color += weight[1].z * luminance.Sample(mySampler, In.tex6.zw);
    Color += weight[1].w * luminance.Sample(mySampler, In.tex7.zw);

    return float4(Color.xyz, 1.0f);
}