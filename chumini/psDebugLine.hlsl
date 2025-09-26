struct In
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

float4 main(In input) : SV_TARGET
{
	return input.color;
}