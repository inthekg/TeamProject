#include "HitBox_Helper.h"
#include "GameInstance.h"
#include "Model.h"
#include "Character.h"
#include "Weapon_Monster.h"
#include "HitUtil.h"
#ifdef _DEBUG
#include "DebugCapsule.h"
#endif // _DEBUG


void CHitBox_Helper::Add_BoneCapsule(const _wstring& strId, const string& strTopBone, const string& strBotBone, _float fRadius, _float fHalfHeight, _float fDamage)
{
	HITBOX hb;
	hb.strId = strId;
	hb.eType = HITBOX::TYPE::BONE_CAPSULE;
	hb.strTopBone = strTopBone;
	hb.strBotBone = strBotBone;
	hb.fRadius = fRadius;
	hb.fHalfHeight = fHalfHeight;
	hb.fDamage = fDamage;
	m_HitBoxes.push_back(hb);
}

void CHitBox_Helper::Add_BoneSphere(const _wstring& strId, const string& strBone, _float fRadius, _float fDamage)
{
	HITBOX hb;
	hb.strId = strId;
	hb.eType = HITBOX::TYPE::BONE_SPHERE;
	hb.strTopBone = strBone;
	hb.fRadius = fRadius;
	hb.fDamage = fDamage;
	m_HitBoxes.push_back(hb);
}

void CHitBox_Helper::Add_Weapon(const _wstring& strId, CWeapon_Monster* pWeapon, _float fDamage, const string& strProbeBone)
{
	HITBOX hb;
	hb.strId = strId;
	hb.eType = HITBOX::TYPE::WEAPON;
	hb.pWeapon = pWeapon;
	hb.fDamage = fDamage;
	hb.strTopBone = strProbeBone;
	m_HitBoxes.push_back(hb);
}

HRESULT CHitBox_Helper::Build(CModel* pModel, ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	m_pDevice = pDevice;
	m_pContext = pContext;

	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);

	for (auto& hb : m_HitBoxes)
	{
		if (nullptr != pModel)
		{
			if (false == hb.strTopBone.empty())	hb.iTopBoneIndex = pModel->Get_BoneIndex(hb.strTopBone.c_str());
			if (false == hb.strBotBone.empty())	hb.iBotBoneIndex = pModel->Get_BoneIndex(hb.strBotBone.c_str());
		}
#ifdef _DEBUG
		if (HITBOX::TYPE::WEAPON != hb.eType && hb.iTopBoneIndex < 0)
			MSG_BOX("CHitBoxSet : HitBox bone not found (check bone names)");
#endif
	}


	for (_int i = 0; i < static_cast<_int>(m_HitBoxes.size()); ++i)
	{
		const _wstring strOn = L"HitOn_" + m_HitBoxes[i].strId;
		const _wstring strOff = L"HitOff_" + m_HitBoxes[i].strId;
		const _wstring strFx = L"Effect_" + m_HitBoxes[i].strId;

		m_HitCommands.emplace(Hash_String(strOn.c_str()), HIT_COMMAND{ true,  i });
		m_HitCommands.emplace(Hash_String(strOff.c_str()), HIT_COMMAND{ false, i });

		m_HitBoxes[i].iFxNameHash = Hash_String(strFx.c_str());
	}
	m_iHashHitOffAll = Hash_String(L"HitOff_All");

#ifdef _DEBUG
	m_DebugCapsules.assign(m_HitBoxes.size(), nullptr);
	for (_int i = 0; i < static_cast<_int>(m_HitBoxes.size()); ++i)
		Rebuild_DebugCapsule(i);
#endif

	return S_OK;
}

_bool CHitBox_Helper::Handle_Notify(_uint64 iHashKey)
{
	if (iHashKey == m_iHashHitOffAll)
	{
		Disarm_All();
		return true;
	}

	auto range = m_HitCommands.equal_range(iHashKey);
	if (range.first == range.second)
		return false;

	for (auto it = range.first; it != range.second; ++it)
	{
		if (true == it->second.bOn)	Arm(it->second.iIndex);
		else						Disarm(it->second.iIndex);
	}
	return true;
}

void CHitBox_Helper::Arm(_int iIndex)
{
	if (iIndex < 0 || iIndex >= static_cast<_int>(m_HitBoxes.size()))
		return;

	HITBOX& hb = m_HitBoxes[iIndex];

	hb.bActive = true;

	if (HITBOX::TYPE::WEAPON == hb.eType)
	{
		if (nullptr != hb.pWeapon)	hb.pWeapon->Attack_Begin(hb.fDamage, m_eHitType);
		return;
	}

	hb.bPrevValid = false;
	hb.Targets.clear();
}

void CHitBox_Helper::Disarm(_int iIndex)
{
	if (iIndex < 0 || iIndex >= static_cast<_int>(m_HitBoxes.size()))
		return;

	HITBOX& hb = m_HitBoxes[iIndex];

	hb.bActive = false;

	if (HITBOX::TYPE::WEAPON == hb.eType)
	{
		if (nullptr != hb.pWeapon)	hb.pWeapon->Attack_End();
		return;
	}
}


void CHitBox_Helper::Disarm_All()
{
	for (_int i = 0; i < static_cast<_int>(m_HitBoxes.size()); ++i)
		Disarm(i);
}

void CHitBox_Helper::Update(CGameInstance* pGI, CCharacter* pOwner, _float fTimeDelta)
{
	if (nullptr == pGI || nullptr == pOwner)
		return;

	for (_int i = 0; i < static_cast<_int>(m_HitBoxes.size()); ++i)
	{
		if (HITBOX::TYPE::WEAPON == m_HitBoxes[i].eType)
			continue;
		if (false == m_HitBoxes[i].bActive)
			continue;

		Sweep_One(i, pGI, pOwner, fTimeDelta);
	}

#ifdef _DEBUG
	Debug_DrawAll(pGI, pOwner, fTimeDelta);
#endif
}

_bool CHitBox_Helper::Get_HitBoxPose(const HITBOX& hb, CCharacter* pOwner, _vector& OutCenter, _vector& OutQuat)
{
	if (hb.iTopBoneIndex < 0)
		return false;

	_matrix TopWorld;
	if (FAILED(pOwner->Get_BoneWorldMatrix(static_cast<_uint>(hb.iTopBoneIndex), TopWorld)))
		return false;
	const _vector vTop = TopWorld.r[3];

	if (HITBOX::TYPE::BONE_CAPSULE == hb.eType && hb.iBotBoneIndex >= 0)
	{
		_matrix BotWorld;
		if (FAILED(pOwner->Get_BoneWorldMatrix(static_cast<_uint>(hb.iBotBoneIndex), BotWorld)))
			return false;
		CHitUtil::Make_CapsulePose(vTop, BotWorld.r[3], OutCenter, OutQuat);
	}
	else
	{
		OutCenter = vTop;
		OutQuat = XMQuaternionIdentity();
	}
	return true;
}

#ifdef _DEBUG
void CHitBox_Helper::Debug_DrawAll(CGameInstance* pGI, CCharacter* pOwner, _float fTimeDelta)
{
	if (false == pGI->Get_DebugFlag())
		return;

	for (_int i = 0; i < static_cast<_int>(m_HitBoxes.size()); ++i)
	{
		if (i >= static_cast<_int>(m_DebugCapsules.size()) || nullptr == m_DebugCapsules[i])
			continue;

		_vector vCenter, vQuat;
		if (false == Get_HitBoxPose(m_HitBoxes[i], pOwner, vCenter, vQuat))
			continue;

		m_DebugCapsules[i]->Update_Pose(vCenter, vQuat);
		m_DebugCapsules[i]->Late_Update(fTimeDelta);
	}
}

void CHitBox_Helper::Rebuild_DebugCapsule(_int iIndex)
{
	if (iIndex < 0 || iIndex >= static_cast<_int>(m_DebugCapsules.size()))
		return;

	Safe_Release(m_DebugCapsules[iIndex]);

	const HITBOX& hb = m_HitBoxes[iIndex];
	if (HITBOX::TYPE::WEAPON == hb.eType || nullptr == m_pDevice)
		return;

	CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DbgDesc{};
	DbgDesc.fRadius = hb.fRadius;
	DbgDesc.fHalfHeight = (HITBOX::TYPE::BONE_CAPSULE == hb.eType) ? hb.fHalfHeight : 0.f;
	DbgDesc.iSegment = 10;
	DbgDesc.vColor = _float4{ 1.f, 0.25f, 0.f, 1.f };
	m_DebugCapsules[iIndex] = CDebugCapsule::Create(m_pDevice, m_pContext, DbgDesc);
}

_int CHitBox_Helper::Get_Count() const
{
	return static_cast<_int>(m_HitBoxes.size());
}

_float CHitBox_Helper::Get_Radius(_int iIndex) const
{
	if (iIndex < 0 || iIndex >= static_cast<_int>(m_HitBoxes.size()))
		return 0.f;
	return m_HitBoxes[iIndex].fRadius;
}

_float CHitBox_Helper::Get_HalfHeight(_int iIndex) const
{
	if (iIndex < 0 || iIndex >= static_cast<_int>(m_HitBoxes.size()))
		return 0.f;
	return m_HitBoxes[iIndex].fHalfHeight;
}

void CHitBox_Helper::Set_Radius(_int iIndex, _float fRadius)
{
	if (iIndex < 0 || iIndex >= static_cast<_int>(m_HitBoxes.size()))
		return;
	m_HitBoxes[iIndex].fRadius = fRadius;
#ifdef _DEBUG
	Rebuild_DebugCapsule(iIndex);
#endif
}

void CHitBox_Helper::Set_HalfHeight(_int iIndex, _float fHalfHeight)
{
	if (iIndex < 0 || iIndex >= static_cast<_int>(m_HitBoxes.size()))
		return;
	m_HitBoxes[iIndex].fHalfHeight = fHalfHeight;
#ifdef _DEBUG
	Rebuild_DebugCapsule(iIndex);
#endif
}
#endif // _DEBUG



void CHitBox_Helper::Sweep_One(_int iIndex, CGameInstance* pGI, CCharacter* pOwner, _float fTimeDelta)
{
	HITBOX& hb = m_HitBoxes[iIndex];

	_vector vCenter, vQuat;
	if (false == Get_HitBoxPose(hb, pOwner, vCenter, vQuat))
		return;
	if (false == hb.bPrevValid)
	{
		XMStoreFloat3(&hb.vPrevCenter, vCenter);
		hb.bPrevValid = true;
		return;
	}

	const _vector vPrev = XMLoadFloat3(&hb.vPrevCenter);
	XMStoreFloat3(&hb.vPrevCenter, vCenter);

	HitInfo tHit;
	tHit.fDamage = hb.fDamage;
	tHit.pOtherActor = pOwner;
	tHit.pDamangeCauser = pOwner;
	tHit.eHitType = m_eHitType;

	const _bool bCapsule = (HITBOX::TYPE::BONE_CAPSULE == hb.eType && hb.iBotBoneIndex >= 0);
	if (true == bCapsule)
		CHitUtil::Sweep_Capsule(pGI, hb.fRadius, hb.fHalfHeight, vPrev, vCenter, vQuat, tHit, hb.Targets, FILTER_GROUP::PLAYER);
	else
		CHitUtil::Sweep_Sphere(pGI, hb.fRadius, vPrev, vCenter, tHit, hb.Targets, FILTER_GROUP::PLAYER);
}



void CHitBox_Helper::Free()
{
	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);

#ifdef _DEBUG
	for (auto pCapsule : m_DebugCapsules)
		Safe_Release(pCapsule);
	m_DebugCapsules.clear();
#endif
}

_bool CHitBox_Helper::Get_GroundProbe(_uint64 iFxNameHash, CCharacter* pOwner, _float3& OutOrigin, _float& OutRadius)
{
	if (0 == iFxNameHash || nullptr == pOwner)
		return false;

	for (const HITBOX& hb : m_HitBoxes)
	{
		if (hb.iFxNameHash != iFxNameHash)
			continue;

		_vector vCenter, vQuat;
		if (false == Get_HitBoxPose(hb, pOwner, vCenter, vQuat))
			return false;

		XMStoreFloat3(&OutOrigin, vCenter);
		OutRadius = hb.fRadius;
		return true;
	}

	return false;
}

_bool CHitBox_Helper::Get_ArmedCenter(const _wstring& strId, CCharacter* pOwner, _float3& OutCenter, _float& OutRadius)
{
	if (nullptr == pOwner)
		return false;

	for (const HITBOX& hb : m_HitBoxes)
	{
		if (hb.strId != strId)
			continue;

		if (false == hb.bActive)
			return false;

		_vector vCenter, vQuat;
		if (false == Get_HitBoxPose(hb, pOwner, vCenter, vQuat))
			return false;

		XMStoreFloat3(&OutCenter, vCenter);
		OutRadius = hb.fRadius;
		return true;
	}

	return false;
}
