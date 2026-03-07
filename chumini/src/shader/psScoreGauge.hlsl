#include "Constant.hlsli"

// Unused texture slot might be used for Aspect Ratio if needed, but we can assume or hardcode for now
// Or reuse 'emissionColor' from Material buffer if passed.
// For now, let's hardcode 'radius' relative to height (0.5 for full semi-circle ends).

struct In
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};

// Rounded Box SDF
float sdRoundedBox(float2 p, float2 b, float4 r)
{
    r.xy = (p.x > 0.0) ? r.xy : r.zw;
    r.x = (p.y > 0.0) ? r.x : r.y;
    float2 q = abs(p) - b + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

// HSV to RGB Helper
float3 HsvToRgb(float3 c)
{
    float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

PS2D_Out main(In input)
{
    PS2D_Out output;
    output.baseColor = input.color;

    // UV: 0..1 -> -1..1
    float2 uv = input.uv * 2.0f - 1.0f;
    
    // Check for Rainbow Mode
    // emissionColor is used as parameter buffer
    // x: Aspect Ratio
    // y: Rainbow Flag (> 0.5 = On)
    // z: Time
    
    bool isRainbow = (emissionColor.y > 0.5f);
    
    if (isRainbow) {
        float time = emissionColor.z;
        // Rainbow effect based on UV.x and Time
        // UV.x goes from 0 to 1 (passed from input.uv)
        float hue = input.uv.x - time * 0.5f; // Moving speed
        
        // HSV: Hue, Saturation=0.8, Value=1.0
        float3 rainbowColor = HsvToRgb(float3(hue, 0.6f, 1.0f));
        
        // Blend with white/tint? Or just replace?
        // Let's replace RGB but keep Alpha from input.color and SDF
        output.baseColor.rgb = rainbowColor;
    }
    
    float aspect = emissionColor.x;
    if (aspect <= 0.001) aspect = 1.0f; // Safety
    
    // Adjusted Plane
    float2 p = uv; 
    p.x *= aspect; // -aspect .. +aspect
    
    // Capsule SDF
    float segHalfLen = aspect - 1.0f;
    if (segHalfLen < 0) segHalfLen = 0; // If width < height
    
    float2 clampedP = p;
    clampedP.x = clamp(p.x, -segHalfLen, segHalfLen);
    
    float dist = length(p - clampedP) - 1.0f; // Radius 1.0
    
    // SDF Alpha (Anti-Aliasing)
    float alpha = 1.0f - smoothstep(-0.05, 0.05, dist);
    
    output.baseColor.a *= alpha;
    
    float alphaOut = output.baseColor.a;
    output.alphaBuffer = float4(alphaOut, alphaOut, alphaOut, alphaOut);
    
    if (alphaOut < 0.001) discard;
    
    return output;
}
