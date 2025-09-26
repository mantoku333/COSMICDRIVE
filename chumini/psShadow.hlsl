#include "Constant.hlsli"
float4 main(PS3D_In input) : SV_TARGET
{
    float d = input.pos.z / input.pos.w;
    
    return float4(d, d, d, 1);
}