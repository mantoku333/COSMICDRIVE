#include "Mesh.h"
#include "GUI.h"

std::vector<sf::Mesh*> sf::Mesh::meshes;
std::mutex sf::Mesh::meshesMtx;

sf::Mesh::~Mesh()
{
	DeActivate();
}

void sf::Mesh::Activate()
{
	std::lock_guard<std::mutex> lock(meshesMtx);
	auto it = std::find(meshes.begin(), meshes.end(), this);
	if (it == meshes.end())
	{
		meshes.push_back(this);
	}
}

void sf::Mesh::DeActivate()
{
	std::lock_guard<std::mutex> lock(meshesMtx);
	auto it = std::remove(meshes.begin(), meshes.end(), this);
	meshes.erase(it, meshes.end());
}

void sf::Mesh::DrawAll()
{
	std::lock_guard<std::mutex> lock(meshesMtx);
	for (auto& i : meshes) {

		try
		{
			i->Draw();
		}
		catch (const std::exception& hoge)
		{
			debug::Debug::LogError(hoge.what());
		}
	}
}

void sf::Mesh::DrawShadowAll()
{
	std::lock_guard<std::mutex> lock(meshesMtx);
	for (auto& i : meshes) {
		if (i->material.shadow)
		{
			try
			{
				i->Draw();
			}
			catch (const std::exception& hoge)
			{
				debug::Debug::LogError(hoge.what());
			}
		}
	}
}

DirectX::XMMATRIX sf::Mesh::WorldMatrix() const
{
	return actorRef.Target()->transform.Matrix();
}

void sf::Mesh::Draw()
{
	sf::dx::DirectX11* dx11 = sf::dx::DirectX11::Instance();

	//ワールド変換行列を取得
	DirectX::XMMATRIX mtx = WorldMatrix();

	//転置
	DirectX::XMMATRIX tMtx = DirectX::XMMatrixTranspose(mtx);

	//定数バッファに転送
	WorldMatrixBuffer buffer;
	buffer.mtx = tMtx;
	buffer.rot = DirectX::XMMatrixTranspose(actorRef.Target()->transform.GetRotationMatrix());
	dx11->wBuffer.SetGPU(buffer, dx11->GetMainDevice());

	if (motion != nullptr)
	{
		//モーションデータをGPUに転送
		motion->SetGPU();
	}
	else
	{
		//単位行列をGPUに転送
		std::vector<DirectX::XMMATRIX> mtx;
		mtx.resize(400, DirectX::XMMatrixIdentity());
		cmotion mot;
		std::copy(mtx.begin(), mtx.end(), mot.matrix);
		dx11->motionBuffer.SetGPU(mot, dx11->GetMainDevice());
	}

	//描画
	geometryRef.Target()->Draw(material);
}

void sf::Mesh::ClearAllRegistered()
{
	std::lock_guard<std::mutex> lock(meshesMtx);
	// リストに残っているポインタを全て忘れさせる
	meshes.clear();
}
