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

PS2D_Out main(In input)
{
    PS2D_Out output;
    output.baseColor = input.color;

    // UV: 0..1
    // Map to -1..1
    float2 uv = input.uv * 2.0f - 1.0f;
    
    // Hardcoded Aspect Ratio for 450x20 Gauge: 22.5 : 1
    // But wait, the Foreground bar CHANGES width.
    // If I use UVs on a shrinking mesh, the UVs usually compress?
    // In IngameCanvas::DrawScoreGauge, I am scaling the MESH.
    // So standard UVs (0..1) will STRETCH content if I just draw a scaled quad.
    // IF gsScoreGauge outputs 0..1 UVs regardless of scaling, 
    // AND I treat uv.x as stretched... 
    // The "Roundness" will also be stretched (ellipses).
    
    // To fix this, I need to know the Actual Aspect Ratio in the Pixel Shader.
    // Or I need to correct UVs in GS. 
    // Simpler hack: I assume the 'radius' in UV space is very small for X and 0.5 for Y.
    // But resizing makes it dynamic.
    
    // Hack 2: Just alpha-mask the corners? 
    // If the bar is long, the corners are small.
    // If width decreases, aspect ratio changes.
    // The gauge gets shorter.
    
    // Improved Approach:
    // Render the gauge as a 'ProgressBar' where geometry is fixed full width, 
    // and we cut off the drawing using logical 'ratio' passed in color.a or similar?
    // BUT I am already scaling geometry in C++.
    
    // If I scale geometry, I must fix UVs or SDF logic.
    // Let's rely on simple Circle SDF at the ends?
    // "Capsule" shape is just: anything where abs(y) > 0.5 is transparent? No.
    // Distance from centerline < 1.0.
    // Distance from endpoints.
    
    // If I just want a simple "Rounded Rect" and dont mind slight stretching of corners when near 0:
    // Actually, at Width 450, Height 20. Ratio 22.5.
    // If reduced to Width 20, Height 20. Ratio 1.
    // A Ratio 1 quad with Capsule SDF = Circle. Correct.
    // A Ratio 22.5 quad with Capsule SDF = Long Capsule. Correct.
    // BUT I need to compensate for the non-uniform scaling 'uv.x *= Aspect'.
    // Aspect = Width / Height.
    
    // How to get Width in PS?
    // I can pass it in 'Start' or 'Emission' or something.
    // Let's use 'emissionColor.x' (b2) to pass Aspect Ratio (Width/Height).
    // Default emissionColor is 0,0,0,0.
    // I can set it in IngameCanvas.
    
    float aspect = emissionColor.x;
    if (aspect <= 0.001) aspect = 1.0f; // Safety
    
    // Adjusted Plane
    float2 p = uv; 
    p.x *= aspect; // -aspect .. +aspect
    
    // Capsule SDF
    // Length from center line segment (-aspect+1 to +aspect-1)?
    // Radius = 1.0 (since y is -1..1)
    
    // Segment extent along X
    // Width in "Radius units" is 'aspect'. 
    // Circle ends are at distance 1.0 from edge.
    // So segment goes from -(aspect - 1.0) to +(aspect - 1.0).
    
    float segHalfLen = aspect - 1.0f;
    if (segHalfLen < 0) segHalfLen = 0; // If width < height
    
    float2 clampedP = p;
    clampedP.x = clamp(p.x, -segHalfLen, segHalfLen);
    
    float dist = length(p - clampedP) - 1.0f; // Radius 1.0 (relative to Y=1) (Actually Y=1 means edge)
    
    // SDF: dist <= 0 means inside.
    // Use Alpha Anti-Aliasing
    float alpha = 1.0f - smoothstep(-0.05, 0.05, dist);
    
    output.baseColor.a *= alpha;
    
    float alphaOut = output.baseColor.a;
    output.alphaBuffer = float4(alphaOut, alphaOut, alphaOut, alphaOut);
    
    if (alphaOut < 0.001) discard;
    
    return output;
}
