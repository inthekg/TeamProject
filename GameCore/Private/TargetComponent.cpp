#include "TargetComponent.h"
#include "GameObject.h"
#include "Transform.h"
#include "GameInstance.h"
#include "AIObject.h"
#include "Engine_Utility.h"


CTargetComponent::CTargetComponent(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CComponent{ pDevice, pContext }
{
}

CTargetComponent::CTargetComponent(const CTargetComponent& Prototype)
	:CComponent{ Prototype }
{
}

HRESULT CTargetComponent::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CTargetComponent::Initialize(void* pArg)
{
	if (pArg == nullptr) return E_FAIL;

	TARGET_DESC* pDesc = static_cast<TARGET_DESC*>(pArg);

	m_pOwner = pDesc->pOwner;
	m_fFOV = pDesc->fFOV;
	m_fSearchRange = pDesc->fSearchRange;
	m_iMyFilterGroup = pDesc->iMyFilter;
	m_iTargetFilterGroup = pDesc->iTargetFilter;


	m_pOwnerTransform = m_pOwner->Get_Transform();
	return S_OK;
}



_bool CTargetComponent::Has_LineOfSight(_fvector vFrom, _fvector vTo)
{
	_vector vDeltaDir = vTo - vFrom;
	_float fDist = XMVectorGetX(XMVector3Length(vDeltaDir));
	if (IsEqual(fDist, 0.f)) return true;

	_float3 vOrigin, vDir;
	XMStoreFloat3(&vOrigin, vFrom);
	XMStoreFloat3(&vDir, XMVector3Normalize(vDeltaDir));

	CollisionResult tResult{};
	FILTER_GROUP eFilterGroup = static_cast<FILTER_GROUP>(ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT));
	_bool bCollision = m_pGameInstance->Raycast(vOrigin, vDir, fDist, eFilterGroup, tResult, PxQueryFlag::eSTATIC);
	return bCollision == false;
}

void CTargetComponent::Update(_float fTimeDelta)
{
	if (m_pCurrentTarget)
	{
		if (m_pCurrentTarget->Is_InActive() || m_pCurrentTarget->Is_Dead()) 
			m_pCurrentTarget = nullptr;
	}
}

CAIObject* CTargetComponent::Find_LockOnTarget()
{
	m_pCurrentTarget = nullptr;

	_vector vOrigin = m_pOwnerTransform->Get_State(STATE::POSITION);
	const _float4* pCamLook = m_pGameInstance->Get_CamLook();
	if (pCamLook == nullptr) return nullptr;

	_vector vCamForward = XMVectorSet(pCamLook->x, 0.f, pCamLook->z, 0.f);
	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vCamForward)), 0.f)) return nullptr;

	vCamForward = XMVector3Normalize(vCamForward);

	vector<CGameObject*> Targets;
	if (m_pGameInstance->Overlap_Sphere(m_fSearchRange, vOrigin, m_iTargetFilterGroup, Targets))
	{
		_float fBestDot = -1.f;
		_vector vEyeOffset = XMVectorSet(0.f, LOS_HEIGHT_OFFSET, 0.f, 0.f);

		for (auto& pTarget : Targets)
		{
			CAIObject* pAITarget = dynamic_cast<CAIObject*>(pTarget);
			if (nullptr == pAITarget || pAITarget->Is_Dead() || false == pAITarget->Is_Lockable()) continue;

			CTransform* pTargetTransform = pAITarget->Get_Transform();
			if (pTargetTransform == nullptr) continue;

			_vector vTargetPos = pTargetTransform->Get_State(STATE::POSITION);
			_vector vDirToTarget = XMVector3Normalize(vTargetPos - vOrigin);

			_float fDot = XMVectorGetX(XMVector3Dot(vCamForward, vDirToTarget));
			if (fDot > fBestDot && fDot >= cosf(m_fFOV * 0.5f))
			{
				// 사이에 장애물이 있는지 체크 
				if (false == Has_LineOfSight(vOrigin + vEyeOffset, vTargetPos + vEyeOffset))
					continue;

				fBestDot = fDot;
				m_pCurrentTarget = pAITarget;
			}
		}
	}

	return m_pCurrentTarget;
}

CAIObject* CTargetComponent::Find_FatalTarget()
{
	_vector vOrigin = m_pOwnerTransform->Get_State(STATE::POSITION);

	vector<CGameObject*> Targets;
	if (false == m_pGameInstance->Overlap_Sphere(FATAL_DETECT_RANGE, vOrigin, m_iTargetFilterGroup, Targets))
		return nullptr;

	_vector vEyeOffset = XMVectorSet(0.f, LOS_HEIGHT_OFFSET, 0.f, 0.f);

	CAIObject* pBest = nullptr;
	_float fBestDistSq = FLT_MAX;
	
	for (auto& pTarget : Targets)
	{
		if (nullptr == pTarget || pTarget->Is_Dead()) continue;

		CAIObject* pAIObject = static_cast<CAIObject*>(pTarget);
		if (false == pAIObject->Can_FatalFront() && false == pAIObject->Can_FatalBack()) continue;

		_vector vTargetPos = pAIObject->Get_Transform()->Get_State(STATE::POSITION);
		_vector vToTarget = vTargetPos - vOrigin;

		_float  fDistSq = XMVectorGetX(XMVector3LengthSq(vToTarget));

		if (fDistSq < fBestDistSq) 
		{ 
			// 사이에 장애물이 있는지 체크 
			if (false == Has_LineOfSight(vOrigin + vEyeOffset, vTargetPos + vEyeOffset))
				continue;

			fBestDistSq = fDistSq;
			pBest = pAIObject;
		}
	}
	return pBest;
}

CTargetComponent* CTargetComponent::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CTargetComponent* pInstance = new CTargetComponent(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created CTargetComponent");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CComponent* CTargetComponent::Clone(void* pArg)
{
	CComponent* pInstance = new CTargetComponent(*this);
	if(FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone CTargetComponent");
		Safe_Release(pInstance);
	}
	return pInstance;	
}

void CTargetComponent::Free()
{
	__super::Free();
}
