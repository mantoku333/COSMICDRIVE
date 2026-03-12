struct PS3D_In
{
    float4 pos : SV_POSITION; //スクリーン座標
    float4 wpos : POSITION; //ワールド座標
    float2 uv : TEXCOORD; //UV座標
    float3 normal : NORMAL; //法線
    float4 color : COLOR; //頂点カラー
    float4 shadowSpacePos : LIGHTPOS; //影用ライトから見たスクリーン座標
};

struct PS3D_Out
{
    float4 baseColor : SV_Target0; //ベースカラー
    float4 normalColor : SV_Target1;//法線カラー
    float4 alpha : SV_Target2;      //アルファ深度
    float4 luminance : SV_Target3;  //輝度
};

struct PS2D_Out
{
    float4 baseColor : SV_Target0;
    float4 alphaBuffer : SV_Target1;
};

#if 1   //固定レジスタ(自由レジスタは6から)
cbuffer World : register(b0)
{
    matrix worldMatrix;
    matrix rotMtx;
}

cbuffer Camera : register(b1)
{
    float4 cameraPos;
    matrix view;
    matrix proj;
    matrix shadowViewProj;
}

cbuffer Material : register(b2)
{
    float4 diffuseColor;
    float4 emissionColor;
    float4 textureEnable;
}

cbuffer MotionBuffer : register(b3)
{
    matrix motions[400];
}

cbuffer Line : register(b4)
{
    float4 p1;
    float4 p2;
    float4 color;
}

cbuffer System : register(b5)
{
    float time;
    float2 screenSize;
}

#endif