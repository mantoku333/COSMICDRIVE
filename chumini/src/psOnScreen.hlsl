Texture2D rBuffer[4] : register(t0);
Texture2D normalBuffer : register(t4);
Texture2D uiColor : register(t5);
Texture2D uiDraw : register(t6);

SamplerState mySampler : register(s0); //サンプラー

cbuffer SystemBuffer : register(b5)
{
    float4 time;
    float2 screenSize;
    float glitchIntensity;
    float padding;
};

struct In
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float OutLineFromColor(float2 uv);
float OutLineFromNormal(float2 uv);

float4 GetNormal(float2 uv);

float4 DrawUIColor(float4 color, float2 uv);

float4 GetSceneColor(float2 uv)
{
    if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
    {
        return float4(0, 0, 0, 1);
    }
    float4 c = float4(0, 0, 0, 1);
    c += rBuffer[0].Sample(mySampler, uv) * 0.5f;
    c += rBuffer[1].Sample(mySampler, uv) * 0.25f;
    c += rBuffer[2].Sample(mySampler, uv) * 0.125f;
    c += rBuffer[3].Sample(mySampler, uv) * 0.125f;
    return c;
}

// Random noise function
float nrand(float2 uv)
{
    return frac(sin(dot(uv, float2(12.9898, 78.233))) * 43758.5453);
}

float4 main(In input) : SV_Target
{
    float offsetx = 1.0f / 1920.0f;
    float offsety = 1.0f / 1080.0f;
    
    // --- Glitch Effect ---
    float2 uv = input.uv;
    if (glitchIntensity > 0.0f)
    {
        float t = time.x * 20.0f; // Increase speed
        
        // Thin scanline strips instead of blocks
        float splitY = floor(uv.y * 50.0f + t * 0.5f);
        float noise = nrand(float2(splitY, t));
        
        // Scanline offset
        if (noise < glitchIntensity) 
        {
             float shift = (nrand(float2(splitY, splitY)) - 0.5f) * glitchIntensity * 0.1f;
             uv.x += shift;
        }
        
        // High frequency jitter (Biri-Biri effect)
        float jitter = nrand(uv + t) - 0.5f;
        if (abs(jitter) < glitchIntensity) {
            uv.x += jitter * 0.02f * glitchIntensity;
        }
    }

    // --- Chromatic Aberration ---
    float2 dist = uv - 0.5f;
    float r2 = dot(dist, dist);
    float2 caOffset = dist * (r2 * 0.025f); 
    
    // If glitching, increase CA significantly
    if (glitchIntensity > 0.0f)
    {
        float t = time.x * 50.0f;
        float rgbNoise = nrand(float2(t, t));
        caOffset += float2(glitchIntensity * 0.02f * rgbNoise, 0.0f);
    }

    float R = GetSceneColor(uv - caOffset).r;
    float G = GetSceneColor(uv).g;
    float B = GetSceneColor(uv + caOffset).b;

    float4 color = float4(R, G, B, 1.0f);

    color = DrawUIColor(color, input.uv); // UI uses original UV to stay sharp? or glitch UI too? Glitch UI too for immersion
    // To glitch UI, use 'uv' instead of 'input.uv'. 
    // Let's use 'uv' for consistency.
    color = DrawUIColor(float4(R,G,B,1.0f), uv);

    // --- Vignette ---
    float len = length(uv - 0.5f);
    float vignette = 1.0f - smoothstep(0.4f, 1.2f, len); 
    color.rgb *= vignette;

    return color;
}


float4 GetNormal(float2 uv)
{
    float3 nor = normalBuffer.Sample(mySampler, uv);

    //0～1を-1～1に展開
    nor = nor * 2 - float3(1, 1, 1);

    return float4(nor, 1.0f);
}

//カラー情報よりアウトライン描画
float OutLineFromColor(float2 uv)
{
    float offsetx = 1.0f / 1920.0f;
    float offsety = 1.0f / 1080.0f;

    float4 color = rBuffer[0].Sample(mySampler, uv);

    float2 right = uv + float2(offsetx, 0);
    right.x = min(1.0f - offsetx, right.x);

    float2 left = uv + float2(-offsetx, 0);
    left.x = max(offsetx, left.x);

    float2 up = uv + float2(0, offsety);
    up.y = min(1.0f - offsety, up.y);

    float2 down = uv + float2(0, -offsety);
    down.y = max(offsety, down.y);

    float4 rightC = rBuffer[0].Sample(mySampler, right);
    float4 leftC = rBuffer[0].Sample(mySampler, left);
    float4 upC = rBuffer[0].Sample(mySampler, up);
    float4 downC = rBuffer[0].Sample(mySampler, down);

    float l = 0.1f;
    if (distance(color.rgb, rightC.rgb) > l)
    {
        return 1.0f;
    }
    if (distance(color.rgb, leftC.rgb) > l)
    {
        return 1.0f;
    }
    if (distance(color.rgb, upC.rgb) > l)
    {
        return 1.0f;
    }
    if (distance(color.rgb, downC.rgb) > l)
    {
        return 1.0f;
    }

    return 0.0f;
}

//法線情報よりアウトライン描画
float OutLineFromNormal(float2 uv)
{
    float offsetx = 1.0f / 1920.0f;
    float offsety = 1.0f / 1080.0f;

    float2 right = uv + float2(offsetx, 0);
    right.x = min(1.0f - offsetx, right.x);

    float2 left = uv + float2(-offsetx, 0);
    left.x = max(offsetx, left.x);

    float2 up = uv + float2(0, offsety);
    up.y = min(1.0f - offsety, up.y);

    float2 down = uv + float2(0, -offsety);
    down.y = max(offsety, down.y);


    float4 normal = GetNormal(uv);
    float4 rightN = GetNormal(right);
    float4 leftN = GetNormal(left);
    float4 upN = GetNormal(up);
    float4 downN = GetNormal(down);

    //アウトライン描画の閾値
    float n = 1.0f;
    if (distance(normal.rgb, rightN.rgb) > n)
    {
        return 1.0f;
    }
    if (distance(normal.rgb, leftN.rgb) > n)
    {
        return 1.0f;
    }
    if (distance(normal.rgb, upN.rgb) > n)
    {
        return 1.0f;
    }
    if (distance(normal.rgb, downN.rgb) > n)
    {
        return 1.0f;
    }
    return 0.0f;
}

float4 DrawUIColor(float4 color, float2 uv)
{
    float4 draw = uiDraw.Sample(mySampler, uv);

    //UIカラー取得
    float4 uColor = uiColor.Sample(mySampler, uv);

    float alpha = draw.r;
    color = color * (1 - alpha) + uColor * alpha;

    return color;
}
