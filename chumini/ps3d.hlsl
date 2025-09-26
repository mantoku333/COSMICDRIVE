#include "Constant.hlsli"

Texture2D diffuseTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D preTexture : register(t2);
Texture2D shadowTexture : register(t3);
SamplerState mySampler : register(s0); //サンプラー

float4 ParallelLighting(float3 normal);
float4 Shadow(float4 shadowSpacePos);
float4 Shadow2(float4 shadowSpacePos);

PS3D_Out main(PS3D_In input) : SV_TARGET
{
    PS3D_Out output;
    
    float4 texColor = float4(1, 1, 1, 1);
    if (textureEnable.x)
    {
        texColor = diffuseTexture.Sample(mySampler, input.uv);
    }
    
    output.baseColor = input.color * texColor;
    output.baseColor *= ParallelLighting(input.normal);
    
    //if (textureEnable.y)
    //{
    //    float3 nor;
    //    nor = normalTexture.Sample(mySampler, input.uv);
    //    //nor = nor * 2 - 1;
        
    //    //nor = mul(nor, (float3x3) worldMatrix);
    //    //nor = nor * 0.5f + 0.5f;
    //    output.normalColor = float4(nor, 1);
    //}
    //else
    {
        output.normalColor = float4((input.normal.xyz + float3(1, 1, 1)) * 0.5f, 1);
    }
    
    
    if (!(textureEnable.z > 0))
    {
        output.baseColor *= Shadow(input.shadowSpacePos);
    }
   
    float alpha = output.baseColor.a;
    float depth = input.pos.z / input.pos.w;
    output.alpha = float4(alpha, depth, 0.0f, 1.0f);
    
    //アルファディザ
    //if (alpha < 1.0f)
    //{
    //    int n = input.pos.x * input.pos.y;
    //    n %= 4;
        
    //    if (n == 0)
    //    {
    //        discard;
    //    }
    //}
    
    output.luminance = float4(output.baseColor.rgb - float3(1, 1, 1), 1.0f);
    
    return output;
}

float4 ParallelLighting(float3 normal)
{
    //光の向きベクトル
    float3 lightVector = float3(0.5f, 1, -0.25f);

    //2つのベクトルを単位ベクトル化する
    lightVector = normalize(lightVector);

    //内積を求める
    float diffusePower = dot(normal, lightVector);
    diffusePower = max(diffusePower, 0.5f);

    float4 resultColor = float4(diffusePower, diffusePower, diffusePower, 1);
    
    return resultColor;
}

float4 Shadow(float4 shadowSpacePos)
{
    float4 shadow = float4(1.0f, 1.0f, 1.0f, 1.0f);

    //ライトビュースクリーン空間からUV空間に座標変換
    float2 shadowMapUV = shadowSpacePos.xy / shadowSpacePos.w;
    shadowMapUV *= float2(0.5f, -0.5f);
    shadowMapUV += 0.5f;
    
    //UVテクスチャの範囲内なら
    if (shadowMapUV.x > 0.0f && shadowMapUV.x < 1.0f
     && shadowMapUV.y > 0.0f && shadowMapUV.y < 1.0f)
    {
        float2 offset = float2(1.0f / 1920.0f, 1.0f / 1080.0f);
        
        //書き込もうとする深度値を計算する
        float zInLVP = shadowSpacePos.z / shadowSpacePos.w;
        
        
        const int nearSampling = 3;
        
        //周囲のピクセルを深度情報をサンプリング
        for (int i = -nearSampling; i <= nearSampling; i++)
        {
            for (int j = -nearSampling; j <= nearSampling; j++)
            {
                float2 uv = float2(0, 0);
                uv.x += offset.x * i;
                uv.y += offset.y * j;
                float zInShadowMap = shadowTexture.Sample(mySampler, shadowMapUV + uv).r;
                
                if (zInShadowMap > 0)
                {
                    //取り出した深度値より奥側なら
                    if (zInLVP > zInShadowMap)
                    {
                    //遮蔽されている
                        shadow.xyz *= 0.99f;
                    }
                }
            }
        }
    }

    return shadow;
}

float4 Shadow2(float4 shadowSpacePos)
{
    float4 color = float4(1, 1, 1, 1);
    
    float2 screenUV = shadowSpacePos.xy / shadowSpacePos.w;
    
    screenUV = screenUV * 0.5f + 0.5f;
    
    screenUV.y = 1 - screenUV.y;
    
    float objDepth = shadowSpacePos.z / shadowSpacePos.w;
    
    screenUV = saturate(screenUV);
    
    float texDepth = shadowTexture.Sample(mySampler, screenUV).r;
    
    if (screenUV.x < 0.0f || screenUV.x > 1.0f ||
        screenUV.y < 0.0f || screenUV.y > 1.0f)
    {
        texDepth = 1.0f;
    }
    
    color.rgb = step(objDepth, texDepth);
    return color;
}