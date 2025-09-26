#include "Motion.h"
#include "DirectX11.h"
#include "Mesh.h"
#include "GUI.h"
#include "Time.h"

sf::motion::MotionBone::MotionBone(MotionBone* _parent) :parent(_parent)
{
}

sf::motion::MotionBone::~MotionBone()
{
	for (auto& i : children) {
		delete i;
	}
}
void sf::motion::MotionBone::Create(RootMotionBone* _root, const std::map<std::string, Bone>& maps, const Node* node)
{
	boneName = node->boneName;

	offsetMatrix = maps.at(boneName).offsetMatrix;
	idx = maps.at(boneName).index;
	root = _root;

	root->bones[boneName] = this;


	for (auto& i : node->children) {

		MotionBone* newMBone = new MotionBone(this);
		newMBone->Create(root, maps, i);
		newMBone->parent = this;
		children.push_back(newMBone);
	}

}

void sf::motion::MotionBone::SetBoneMatrix(const DirectX::XMMATRIX& wMtx)
{
	matrix = offsetMatrix * animationMatrix * wMtx;
}

void sf::motion::MotionBone::SetMotionIdForChild(int id)
{
	motionId = id;

	for (auto& i : children) {
		i->SetMotionIdForChild(id);
	}
}

void sf::motion::MotionBone::SetTimeForChild(float _t)
{
	time = _t;

	for (auto& i : children) {
		i->SetTimeForChild(_t);
	}
}

void sf::motion::MotionBone::SetMotionSpeed(float _speed)
{
	motionSpeed = _speed;

	for (auto& i : children) {
		i->SetTimeForChild(_speed);
	}
}

void sf::motion::MotionBone::UpdateBoneMotion(int duration, float _motionSpeed)
{
	time += sf::Time::DeltaTime() * motionSpeed * 60.0f * _motionSpeed;

	if (loop)
	{
		if (time >= duration)
		{
			time = 0.0f;
		}
	}
	else
	{
		time = duration - 1;
	}
}

sf::motion::RootMotionBone::RootMotionBone() :MotionBone(nullptr)
{

}


void sf::motion::RootMotionBone::Create(const std::map<std::string, Bone>& maps, const Node* node)
{
	boneName = node->boneName;

	offsetMatrix = maps.at(boneName).offsetMatrix;
	idx = maps.at(boneName).index;

	for (auto& i : node->children) {
		MotionBone* newMBone = new MotionBone(this);
		newMBone->Create(this, maps, i);
		newMBone->parent = this;
		children.push_back(newMBone);
	}
}

void sf::motion::Motion::Begin()
{
	Mesh* mesh = actorRef.Target()->GetComponent<Mesh>();
	if (mesh == nullptr)
	{
		throw std::runtime_error("モーションがありますがメッシュが存在しません。\nモーションを使用する時はメッシュが必須です。");
		return;
	}
	mesh->SetMotion(this);
}

void sf::motion::Motion::Activate()
{
	motionCommand.Bind(std::bind(&Motion::Update, this));
}

void sf::motion::Motion::DeActivate()
{
	motionCommand.UnBind();
}

void sf::motion::Motion::SetMotion(int id, float _t)
{
	motionId = id;
	rootMotionBone.SetMotionIdForChild(id);

	SetTime(_t);
}

void sf::motion::Motion::SetTime(float _t)
{
	time = _t;
	rootMotionBone.SetTimeForChild(_t);
}

void sf::motion::Motion::SetGeomeyryMotion(const geometry::GeometryFbx& _geomeyry, const MotionArray& _motionArray)
{
	motionArray = &_motionArray;

	geomtryFbx = &_geomeyry;

	for (auto& i : motionArray->GetMotionDatas().front().GetData().front()) {
		nodeTree.push_back(i.first);
	}

	mtx.resize(_geomeyry.GetBoneMap().size(), DirectX::XMMatrixIdentity());

	rootMotionBone.Create(geomtryFbx->GetBoneMap(), &geomtryFbx->GetRootNode());

	motionBone_Head = rootMotionBone.bones.find("mixamorig:Head")->second;
	motionBone_Spine = rootMotionBone.bones.find("mixamorig:Spine")->second;

	motionBone_LeftUpLeg = rootMotionBone.bones.find("mixamorig:LeftUpLeg")->second;
	motionBone_LeftLeg = rootMotionBone.bones.find("mixamorig:LeftLeg")->second;
	motionBone_LeftToeBase = rootMotionBone.bones.find("mixamorig:LeftToeBase")->second;
	motionBone_LeftShoulder = rootMotionBone.bones.find("mixamorig:LeftShoulder")->second;
	motionBone_LeftArm = rootMotionBone.bones.find("mixamorig:LeftArm")->second;
	motionBone_LeftForeArm = rootMotionBone.bones.find("mixamorig:LeftForeArm")->second;
	motionBone_LeftHand = rootMotionBone.bones.find("mixamorig:LeftHand")->second;

	motionBone_RightUpLeg = rootMotionBone.bones.find("mixamorig:RightUpLeg")->second;
	motionBone_RightLeg = rootMotionBone.bones.find("mixamorig:RightLeg")->second;
	motionBone_RightToeBase = rootMotionBone.bones.find("mixamorig:RightToeBase")->second;
	motionBone_RightShoulder = rootMotionBone.bones.find("mixamorig:RightShoulder")->second;
	motionBone_RightArm = rootMotionBone.bones.find("mixamorig:RightArm")->second;
	motionBone_RightForeArm = rootMotionBone.bones.find("mixamorig:RightForeArm")->second;
	motionBone_RightHand = rootMotionBone.bones.find("mixamorig:RightHand")->second;


	auto add = [this](MotionBone* a)
		{
			motionBones.push_back(a);
		};
	add(motionBone_Spine);

	add(motionBone_LeftUpLeg);
	add(motionBone_LeftLeg);
	add(motionBone_LeftToeBase);
	add(motionBone_LeftShoulder);
	add(motionBone_LeftArm);
	add(motionBone_LeftForeArm);
	add(motionBone_LeftHand);

	add(motionBone_RightUpLeg);
	add(motionBone_RightLeg);
	add(motionBone_RightToeBase);
	add(motionBone_RightShoulder);
	add(motionBone_RightArm);
	add(motionBone_RightForeArm);
	add(motionBone_RightHand);
}

void sf::motion::Motion::SetGPU() const
{
	cmotion mot;

	std::copy(mtx.begin(), mtx.end(), mot.matrix);

	dx::DirectX11::Instance()->motionBuffer.SetGPU(mot, dx::DirectX11::Instance()->GetMainDevice());
}

bool sf::motion::Motion::EndMotion() const
{
	int duration = motionArray->GetMotionDatas()[motionId].Duration();

	return time >= (duration - 1);
}

void sf::motion::Motion::Update()
{
	time += Time::DeltaTime() * 60.0f * motionSpeed;

	int timeInt = int(time);

	const auto& dat = motionArray->GetMotionDatas();

	//ImGui::Begin("HierarchyMotionGUI");

	//HierarchyMotionGUI(&rootMotionBone, 0);

	//ImGui::End();

	int duration = dat[motionId].Duration();

	if (timeInt >= duration)
	{
		if (loop)
		{
			time = 0.0f;
		}
		else
		{
			time = duration - 1;
		}
	}

	//モーションデータを取得
	const std::vector<MotionData>& motionDatas = motionArray->GetMotionDatas();

	for (auto& i : rootMotionBone.bones) {
		i.second->UpdateBoneMotion(motionDatas[i.second->GetMotionId()].Duration(), motionSpeed);
	}

	for (auto& i : nodeTree) {

		//ボーン取得
		MotionBone* bone = rootMotionBone.bones[i];


		//対象のモーションデータを取得
		const MotionData& motionData = motionDatas[bone->GetMotionId()];

		bone->animationMatrix = motionData.GetMatrix(bone->GetTime(), bone->boneName);
	}

	UpdateBoneMatrix(&rootMotionBone, DirectX::XMMatrixIdentity());

	ConvertMtx(&rootMotionBone);

	//転置
	/*for (auto& i : mtx) {
		i = DirectX::XMMatrixTranspose(i);
	}*/
}

void sf::motion::Motion::ConvertMtx(const MotionBone* bone)
{
	DirectX::XMMATRIX pre = mtx[bone->idx];
	DirectX::XMMATRIX current = DirectX::XMMatrixTranspose(bone->GetBoneMatrix());

	//単純な線形補間
	mtx[bone->idx] = pre * 0.5f + current * 0.5f;

	for (auto& i : bone->children) {
		ConvertMtx(i);
	}
}

void sf::motion::Motion::HierarchyMotionGUI(MotionBone* bone, int n)
{
	std::string tx = std::to_string(n) + ":";

	int id = bone->GetMotionId();

	auto it = std::find(nodeTree.begin(), nodeTree.end(), bone->boneName);

	//ノードツリーにあるボーンのみGUI描画
	if (it != nodeTree.end())
	{
		if (ImGui::SliderInt((tx + bone->boneName).c_str(), &id, 0, 4))
		{
			bone->SetMotionIdForChild(id);
		}
		n++;
	}

	for (auto& i : bone->children) {
		HierarchyMotionGUI(i, n);
	}
}

void sf::motion::Motion::UpdateBoneMatrix(MotionBone* bone, DirectX::XMMATRIX _mtx)
{
	bone->SetBoneMatrix(_mtx);

	// 自分の姿勢を表す行列を作成
	DirectX::XMMATRIX mybonemtx = bone->animationMatrix * _mtx;

	for (auto& i : bone->children) {

		UpdateBoneMatrix(i, mybonemtx);
	}
}

