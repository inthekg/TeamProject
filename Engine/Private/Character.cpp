#include "Character.h"
#include "GameInstance.h"
#include "PartObject.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CCharacter, CContainerObject)
END_REFLECTION(CCharacter)
#endif

CCharacter::CCharacter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CContainerObject{ pDevice, pContext }
{
}

CCharacter::CCharacter(const CCharacter& Prototype)
	:CContainerObject{ Prototype }
	, m_strRightHand_BoneName(Prototype.m_strRightHand_BoneName)
	, m_strLeftHand_BoneName(Prototype.m_strLeftHand_BoneName)
	, m_eActorType{ Prototype.m_eActorType }
	, m_fGravityWeight{ Prototype.m_fGravityWeight }
{
}

HRESULT CCharacter::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CCharacter::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CCharacter::Priority_Update(_float fTimeDelta)
{
	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Priority_Update(fTimeDelta);
	}
}

void CCharacter::Update(_float fTimeDelta)
{
	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Update(fTimeDelta);
	}
}

void CCharacter::Physical_Update(_float fFixedTime)
{
	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Physical_Update(fFixedTime);
	}
}

void CCharacter::Late_Update(_float fTimeDelta)
{
	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Late_Update(fTimeDelta);
	}
}

HRESULT CCharacter::Get_BoneWorldMatrix(_uint iBoneIndex, _matrix& OutWorldMatrix)
{
	const _float4x4* pBoneCombinedMatrix = Get_Bone_CombinedMatPtr(iBoneIndex);
	if (pBoneCombinedMatrix == nullptr) return E_FAIL;

	const _float4x4* pWorldMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	OutWorldMatrix = XMMatrixMultiply(XMLoadFloat4x4(pBoneCombinedMatrix), XMLoadFloat4x4(pWorldMatrix));
	return S_OK;
}

const _float4x4* CCharacter::Get_Bone_CombinedMatPtr(_uint iBoneIndex)
{
	if (m_pModelCom == nullptr) return nullptr;
	return m_pModelCom->Get_Bone_CombinedMatPtr(iBoneIndex);
}

const _float4x4* CCharacter::Get_Bone_CombinedMatPtr(const _char* pBoneName)
{
	if (m_pModelCom == nullptr) return nullptr;
	return m_pModelCom->Get_Bone_CombinedMatPtr(pBoneName);
}

HRESULT CCharacter::Render()
{
	return S_OK;
}

HRESULT CCharacter::Get_LockOnWorldMatrix(_matrix& OutWorld)
{
	if (nullptr == m_pModelCom)
		return E_FAIL;

	const _int iBoneIndex = m_pModelCom->Get_BoneIndex("LockOn_Base");
	if (iBoneIndex < 0)
		return E_FAIL;

	return Get_BoneWorldMatrix(ETOUI(iBoneIndex), OutWorld);
}

_bool CCharacter::Get_LockOnWorldPosition(_vector& OutPos)
{
	_matrix LockOnWorld;
	if (FAILED(Get_LockOnWorldMatrix(LockOnWorld)))
		return false;
	OutPos = LockOnWorld.r[3];
	return true;
}

HRESULT CCharacter::Get_HeadWorldMatrix(_matrix& OutWorld)
{
	if (nullptr == m_pModelCom)
		return E_FAIL;

	const _int iHeadIndex = m_pModelCom->Get_BoneIndex("Bip001-Head");
	if (iHeadIndex < 0)
	{
		const _int iBoneIndex = m_pModelCom->Get_BoneIndex("LockOn_Base");
		if (iBoneIndex < 0)
			return E_FAIL;

		return Get_BoneWorldMatrix(ETOUI(iBoneIndex), OutWorld);
	}	

	return Get_BoneWorldMatrix(ETOUI(iHeadIndex), OutWorld);
	
}

_bool CCharacter::Get_HeadWorldPosition(_vector& OutPos)
{
	_matrix HeadWorld;
	if (FAILED(Get_HeadWorldMatrix(HeadWorld)))
		return false;
	OutPos = HeadWorld.r[3];
	return true;
}

void CCharacter::Set_InActive()
{
	if (Is_InActive()) return;
	ActorDespawn_EventDesc Desc = {};
	Desc.pActor = this;
	Desc.eActorType = m_eActorType;
	m_pGameInstance->Broadcast(EVENT_TYPE::ACTOR_DESPAWN, &Desc);
	
	__super::Set_InActive();	
	
}

void CCharacter::Free()
{
	//디스폰 이벤트 발생 
	Set_InActive();

	__super::Free();
	Safe_Release(m_pModelCom);
}