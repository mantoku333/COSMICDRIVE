#include "Constant.hlsli"
struct In
{
	
};

struct Out
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

[maxvertexcount(2)]
void main(point In input[1], inout LineStream<Out> output)
{
    Out gout[2];
    
    gout[0].pos = float4(p1.xyz, 1.0f);
    gout[0].pos = mul(gout[0].pos, view);
    gout[0].pos = mul(gout[0].pos, proj);
    gout[0].color = color;
    
    
    gout[1].pos = float4(p2.xyz, 1.0f);
    gout[1].pos = mul(gout[1].pos, view);
    gout[1].pos = mul(gout[1].pos, proj);
    gout[1].color = color;
    
    output.Append(gout[0]);
    output.Append(gout[1]);
}