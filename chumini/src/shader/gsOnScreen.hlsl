struct In
{
};

struct Out
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
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
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
    float2(0.0f, 1.0f),
};

[maxvertexcount(vertexCount)]
void main(point In input[1], inout TriangleStream<Out> output)
{
    Out ou[6];
    for (int i = 0; i < vertexCount; i++)
    {
        
        ou[i].pos = position[i];
        ou[i].uv = uv[i];
    }
    
    output.Append(ou[0]);
    output.Append(ou[1]);
    output.Append(ou[2]);
    
    output.RestartStrip();
    
    output.Append(ou[3]);
    output.Append(ou[4]);
    output.Append(ou[5]);

}