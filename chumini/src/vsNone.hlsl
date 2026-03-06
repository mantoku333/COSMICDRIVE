//ジオメトリシェーダーインスタンシングを想定してIDを渡す
uint main(uint instanceId : SV_InstanceID) : SV_InstanceID
{
    return instanceId;
}