#include "Effect_Manager.h"
#include "GameInstance.h"
#include "Effect.h"
#include "Effect_Emitter.h"
#include "Effect_Trail.h"
#include "Effect_Serializer.h"
#include "Model.h"
#include "Decal.h"
#include "Texture.h"

CEffect_Manager::CEffect_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice { pDevice }
	, m_pContext { pContext }
	, m_pGameInstance { CGameInstance::Get_Instance() }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CEffect_Manager::Initialize()
{
	if (FAILED(CEffect_Emitter::Build_SharedShaders(m_pDevice, m_pContext, m_tEmitterShaders)))
		return E_FAIL;

	m_pGameInstance->Create_Pool<CEffect, EFFECT_POOL_SIZE>();
	m_pGameInstance->Create_Pool<CEffect_Emitter, EMITTER_POOL_SIZE>();
	m_pGameInstance->Create_Pool<CEffect_Trail, TRAIL_POOL_SIZE>();
	m_pGameInstance->Create_Pool<CDecal, DECAL_POOL_SIZE>();

	return S_OK;
}

void CEffect_Manager::Register_Effect(const string& strName, const EffectDesc& tDesc)
{
	m_DescCache[strName] = tDesc;
}

HRESULT CEffect_Manager::Load_Effect_File(const string& strName, const string& strPath)
{
	EffectDesc tDesc;
	if (FAILED(Load_Effect(tDesc, strPath)))
		return E_FAIL;

	m_DescCache[strName] = tDesc;

	Preload_Effect_Textures(m_DescCache[strName]);

	return S_OK;
}

CEffect* CEffect_Manager::Play(const string& strName, const _float3& vWorldPos, const _float3& vAngle)
{
	auto it = m_DescCache.find(strName);
	if (it == m_DescCache.end())
		return nullptr;

	EFFECT_PLAY_DESC tArg;
	tArg.pDesc = &it->second;
	tArg.vWorldPos = vWorldPos;
	tArg.vFollowAngle = vAngle;
	tArg.pShaderBundle = &m_tEmitterShaders;

	CEffect* pEffect = m_pGameInstance->Get_PoolObject<CEffect>(&tArg);
	if (pEffect == nullptr)
		return nullptr;

	m_ActivateEffects.push_back(pEffect);

	return pEffect;
}

CEffect* CEffect_Manager::Play(const string& strName, const _float4x4* pFollowBoneMat,
	const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	auto it = m_DescCache.find(strName);
	if (it == m_DescCache.end())
		return nullptr;

	EFFECT_PLAY_DESC tArg;
	tArg.pDesc = &it->second;
	tArg.pFollowBoneMat = pFollowBoneMat;
	tArg.pOwnerWorld = pOwnerWorld;
	tArg.vFollowOffset = vOffset;
	tArg.vFollowAngle = vAngle;
	tArg.pShaderBundle = &m_tEmitterShaders;

	CEffect* pEffect = m_pGameInstance->Get_PoolObject<CEffect>(&tArg);
	if (pEffect == nullptr)
		return nullptr;

	m_ActivateEffects.push_back(pEffect);

	return pEffect;
}

CEffect* CEffect_Manager::Play(const string& strName, CModel* pModel, const _char* pBoneName,
	const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	const _float4x4* pBoneMat = (pModel != nullptr) ? pModel->Get_Bone_CombinedMatPtr(pBoneName) : nullptr;
	return Play(strName, pBoneMat, pOwnerWorld, vOffset, vAngle);
}

CEffect* CEffect_Manager::Play(const string& strName, CModel* pModel, const _char* pBoneName,
	const _char* pBoneName2, const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	auto it = m_DescCache.find(strName);
	if (it == m_DescCache.end())
		return nullptr;

	EFFECT_PLAY_DESC tArg;
	tArg.pDesc = &it->second;
	tArg.pFollowBoneMat = (pModel != nullptr) ? pModel->Get_Bone_CombinedMatPtr(pBoneName) : nullptr;
	tArg.pFollowBoneMat2 = (pModel != nullptr) ? pModel->Get_Bone_CombinedMatPtr(pBoneName2) : nullptr;
	tArg.pOwnerWorld = pOwnerWorld;
	tArg.vFollowOffset = vOffset;
	tArg.vFollowAngle = vAngle;
	tArg.pShaderBundle = &m_tEmitterShaders;

	CEffect* pEffect = m_pGameInstance->Get_PoolObject<CEffect>(&tArg);
	if (pEffect == nullptr)
		return nullptr;

	m_ActivateEffects.push_back(pEffect);

	return pEffect;
}

CEffect* CEffect_Manager::Play(const string& strName, CModel* pModel,
	const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	auto it = m_DescCache.find(strName);
	if (it == m_DescCache.end())
		return nullptr;

	EFFECT_PLAY_DESC tArg;
	tArg.pDesc = &it->second;
	tArg.pFollowModel = pModel;
	tArg.pOwnerWorld = pOwnerWorld;
	tArg.vFollowOffset = vOffset;
	tArg.vFollowAngle = vAngle;
	tArg.pShaderBundle = &m_tEmitterShaders;

	CEffect* pEffect = m_pGameInstance->Get_PoolObject<CEffect>(&tArg);
	if (pEffect == nullptr)
		return nullptr;

	m_ActivateEffects.push_back(pEffect);

	return pEffect;
}

CEffect* CEffect_Manager::Play(const string& strName, const vector<CModel*>& Parts,
	const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	auto it = m_DescCache.find(strName);
	if (it == m_DescCache.end())
		return nullptr;

	EFFECT_PLAY_DESC tArg;
	tArg.pDesc = &it->second;
	tArg.pFollowModel = Parts.empty() ? nullptr : Parts.front();
	tArg.pFollowParts = &Parts;
	tArg.pOwnerWorld = pOwnerWorld;
	tArg.vFollowOffset = vOffset;
	tArg.vFollowAngle = vAngle;
	tArg.pShaderBundle = &m_tEmitterShaders;

	CEffect* pEffect = m_pGameInstance->Get_PoolObject<CEffect>(&tArg);
	if (pEffect == nullptr)
		return nullptr;

	m_ActivateEffects.push_back(pEffect);

	return pEffect;
}

CEffect* CEffect_Manager::Play(const string& strName, const _float4x4* pFollowBoneMat, const _float4x4* pFollowBoneMat2,
	const _float4x4* pOwnerWorld, const _float3& vOffset, const _float3& vAngle)
{
	auto it = m_DescCache.find(strName);
	if (it == m_DescCache.end())
		return nullptr;

	EFFECT_PLAY_DESC tArg;
	tArg.pDesc = &it->second;
	tArg.pFollowBoneMat = pFollowBoneMat;
	tArg.pFollowBoneMat2 = pFollowBoneMat2;
	tArg.pOwnerWorld = pOwnerWorld;
	tArg.vFollowOffset = vOffset;
	tArg.vFollowAngle = vAngle;
	tArg.pShaderBundle = &m_tEmitterShaders;

	CEffect* pEffect = m_pGameInstance->Get_PoolObject<CEffect>(&tArg);
	if (pEffect == nullptr)
		return nullptr;

	m_ActivateEffects.push_back(pEffect);

	return pEffect;
}

void CEffect_Manager::Tick(_float fTimeDelta)
{
	for (auto it = m_ActivateEffects.begin(); it != m_ActivateEffects.end();)
	{
		(*it)->Update(fTimeDelta);

		if ((*it)->Is_Finished())
		{
			Safe_Release(*it);
			it = m_ActivateEffects.erase(it);
		}
		else
			++it;
	}
}

void CEffect_Manager::Clear()
{
	for (CEffect* pEffect : m_ActivateEffects)
		Safe_Release(pEffect);

	m_ActivateEffects.clear();
}

void CEffect_Manager::Preload_Effect_Textures(const EffectDesc& tDesc)
{
	if (m_pGameInstance == nullptr)
		return;

	auto Preload = [&](const string& strPath)
		{
			if (strPath.empty())
				return;

			int iLen = MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), -1, nullptr, 0);
			wstring strWPath(iLen > 0 ? iLen - 1 : 0, L'\0');
			if (iLen > 0)
				MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), -1, &strWPath[0], iLen);

			wstring strName = Get_FileNameFromPath(strWPath);
			CTexture* pTex = nullptr;
			if (FAILED(m_pGameInstance->Get_Texture(strName, &pTex)))
				m_pGameInstance->Register_TextureFromFile(m_pDevice, m_pContext, strWPath);
			else
				Safe_Release(pTex);
		};

	for (const auto& e : tDesc.Emitters)
	{
		Preload(e.tMaterial.strDiffusePath);
		Preload(e.tUVDistort.strNoiseAPath);
		Preload(e.tUVDistort.strNoiseBPath);
		Preload(e.tDistortion.strNoisePath);
		Preload(e.tEmissive.strEmissiveBPath);
		Preload(e.tMask.strMaskPath);
		Preload(e.tDissolve.strDissolvePath);
		Preload(e.tFresnel.strCrackPath);
		Preload(e.tVectorField.strFieldPath);
		Preload(e.tTrail.strTexturePath);
		Preload(e.tTrail.strNoiseTexPath);
	}
}


CEffect_Manager* CEffect_Manager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CEffect_Manager* pInstance = new CEffect_Manager(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CEffect_Manager");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CEffect_Manager::Free()
{
	Clear();

	CEffect_Emitter::Free_SharedShaders(m_tEmitterShaders);

	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);
}