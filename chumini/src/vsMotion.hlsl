#include "Constant.hlsli"
struct In
{
    float4 pos : POSITION;
    float3 nor : NORMAL;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
    int4 boneIndex : BONEINDEX;
    float4 boneWeight : BONEWEIGHT;
    uint instanceId : SV_InstanceID;
};

Texture2D normalTexture : register(t1);
SamplerState mySampler : register(s0); //サンプラー

PS3D_In main(In input)
{
    PS3D_In output;
    
    // ワンスキン頂点ブレンドの処理
    float4x4 comb = (float4x4) 0;
    for (int i = 0; i < 4; i++)
    {
		// 重みを計算しながら行列生成
        comb += motions[input.boneIndex[i]] * input.boneWeight[i];
    }
    
    
    output.pos = mul(input.pos, comb);
    output.wpos = mul(output.pos, worldMatrix);
    
    
    output.pos = mul(output.wpos, view);
    output.pos = mul(output.pos, proj);
    
    output.shadowSpacePos = mul(float4(output.wpos.xyz, 1.0f), shadowViewProj);
    
    
    output.normal = input.nor;
    
    if (textureEnable.y)
    {
        normalTexture.Sample(mySampler, input.uv);
    }
    output.normal = mul(input.nor, rotMtx);
    
    
    output.color = input.color * diffuseColor;
    output.color.rgb *= 2;
    
    output.uv = input.uv;
    
    return output;
}
