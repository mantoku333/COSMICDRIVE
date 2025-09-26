#include "Constant.hlsli"

struct In
{
	
};

struct Out
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};


#if 1
static const int vertexCount = 36;

//座標情報
static const float4 positions[vertexCount] =
{
    // Front face
    float4(-0.5f, +0.5f, -0.5f, 1.0f),
    float4(+0.5f, +0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, -0.5f, 1.0f),
    float4(+0.5f, +0.5f, -0.5f, 1.0f),
    float4(+0.5f, -0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, -0.5f, 1.0f),

    // Back face
    float4(-0.5f, +0.5f, +0.5f, 1.0f),
    float4(-0.5f, -0.5f, +0.5f, 1.0f),
    float4(+0.5f, +0.5f, +0.5f, 1.0f),
    float4(+0.5f, +0.5f, +0.5f, 1.0f),
    float4(-0.5f, -0.5f, +0.5f, 1.0f),
    float4(+0.5f, -0.5f, +0.5f, 1.0f),

    // Left face
    float4(-0.5f, +0.5f, +0.5f, 1.0f),
    float4(-0.5f, +0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, +0.5f, 1.0f),
    float4(-0.5f, +0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, -0.5f, 1.0f),
    float4(-0.5f, -0.5f, +0.5f, 1.0f),

    // Right face
    float4(+0.5f, +0.5f, +0.5f, 1.0f),
    float4(+0.5f, -0.5f, +0.5f, 1.0f),
    float4(+0.5f, +0.5f, -0.5f, 1.0f),
    float4(+0.5f, +0.5f, -0.5f, 1.0f),
    float4(+0.5f, -0.5f, +0.5f, 1.0f),
    float4(+0.5f, -0.5f, -0.5f, 1.0f),

    // Top face
    float4(-0.5f, +0.5f, +0.5f, 1.0f),
    float4(+0.5f, +0.5f, +0.5f, 1.0f),
    float4(-0.5f, +0.5f, -0.5f, 1.0f),
    float4(+0.5f, +0.5f, +0.5f, 1.0f),
    float4(+0.5f, +0.5f, -0.5f, 1.0f),
    float4(-0.5f, +0.5f, -0.5f, 1.0f),

    // Bottom face
    float4(-0.5f, -0.5f, +0.5f, 1.0f),
    float4(-0.5f, -0.5f, -0.5f, 1.0f),
    float4(+0.5f, -0.5f, +0.5f, 1.0f),
    float4(+0.5f, -0.5f, +0.5f, 1.0f),
    float4(-0.5f, -0.5f, -0.5f, 1.0f),
    float4(+0.5f, -0.5f, -0.5f, 1.0f),
};
static const float yoko = 1.0f / 4.0f;
static const float tate = 1.0f / 3.0f;
// UV座標
static const float2 uv[vertexCount] =
{
    // Front face
    float2(yoko * 1, tate * 1),
    float2(yoko * 2, tate * 1),
    float2(yoko * 1, tate * 2),
    float2(yoko * 2, tate * 1),
    float2(yoko * 2, tate * 2),
    float2(yoko * 1, tate * 2),

    // Back face
    float2(yoko * 4, tate * 1),
    float2(yoko * 4, tate * 2),
    float2(yoko * 3, tate * 1),
    float2(yoko * 3, tate * 1),
    float2(yoko * 4, tate * 2),
    float2(yoko * 3, tate * 2),

    // Left face
    float2(yoko * 0, tate * 1),
    float2(yoko * 1, tate * 1),
    float2(yoko * 0, tate * 2),
    float2(yoko * 1, tate * 1),
    float2(yoko * 1, tate * 2),
    float2(yoko * 0, tate * 2),

    // Right face
    float2(yoko * 3, tate * 1),
    float2(yoko * 3, tate * 2),
    float2(yoko * 2, tate * 1),
    float2(yoko * 2, tate * 1),
    float2(yoko * 3, tate * 2),
    float2(yoko * 2, tate * 2),

    // Top face
    float2(yoko * 1, tate * 0),
    float2(yoko * 2, tate * 0),
    float2(yoko * 1, tate * 1),
    float2(yoko * 2, tate * 0),
    float2(yoko * 2, tate * 1),
    float2(yoko * 1, tate * 1),

    // Bottom face
    float2(yoko * 1, tate * 2),
    float2(yoko * 2, tate * 2),
    float2(yoko * 1, tate * 3),
    float2(yoko * 2, tate * 3),
    float2(yoko * 2, tate * 2),
    float2(yoko * 1, tate * 3),
};
#endif

[maxvertexcount(vertexCount)]
void main(point In input[1], inout TriangleStream<Out> output)
{
    //0,1,2で形成されている図形を2,1,0の順で形成すれば良い
    int offset[3] =
    {
        +2, +0, -2
        //0,0,0
    };
    
    for (int i = 0; i < vertexCount; i++)
    {
        Out ou = (Out) 0;
        
        int n = i % 3;
        ou.pos = positions[i + offset[n]];
        ou.pos.x += cameraPos.x;
        ou.pos.y += cameraPos.y;
        ou.pos.z += cameraPos.z;
        
        //ou.pos = mul(ou.pos, worldMatrix);
        ou.pos = mul(ou.pos, view);
        ou.pos = mul(ou.pos, proj);
        
        ou.uv = uv[i + offset[n]];
        
        if (!(i % 3))
        {
            output.RestartStrip();
        }
        output.Append(ou);
    }
}