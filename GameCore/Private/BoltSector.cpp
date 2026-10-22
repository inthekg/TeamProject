#include "BoltSector.h"
#include "GameInstance.h"
#include "Transform.h"
#include "HitUtil.h"

#ifdef _DEBUG
#include "DebugCapsule.h"
#endif

static const string	BOLTSECTOR_EFFECT = "Effect_Bolt_Sector";
static const string	BOLTRANGE_EFFECT = "Effect_Bolt_Range";

CBoltSector::CBoltSector(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPoolableObject{ pDevice, pContext }
{
}

CBoltSector::CBoltSector(const CBoltSector& Prototype)
	: CPoolableObject{ Prototype }
{
}

HRESULT CBoltSector::Initialize_Prototype()
{
	m_pTransformCom = CTransform::Create(m_pDevice, m_pContext);
	if (nullptr == m_pTransformCom)
		return E_FAIL;

	Set_InActive();

	return S_OK;
}

HRESULT CBoltSector::Initialize(void* pArg)
{
	return S_OK;
}

void CBoltSector::Priority_Update(_float fTimeDelta)
{
}

void CBoltSector::Update(_float fTimeDelta)
{
}

void CBoltSector::Physical_Update(_float fFixedTime)
{
}

void CBoltSector::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	if (m_fDelay > 0.f)
	{
		m_fDelay -= fTimeDelta;
		return;
	}

	m_fTimer += fTimeDelta;
	const _float fRatio = m_fTimer / m_fDuration;
	const _float fCurRadius = m_fRadius * ((fRatio < 1.f) ? fRatio : 1.f);

	const _vector vArmPos = m_pTransformCom->Get_State(STATE::POSITION);
	const _vector vArmLook = m_pTransformCom->Get_State(STATE::LOOK);

#ifdef _DEBUG
	if (true == m_pGameInstance->Get_DebugFlag())
	{
		Safe_Release(m_pDbg);

		CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DbgDesc{};
		DbgDesc.fRadius = fCurRadius;
		DbgDesc.fHalfHeight = 0.f;
		DbgDesc.iSegment = 16;
		DbgDesc.vColor = _float4{ 1.f, 0.5f, 0.f, 1.f };
		m_pDbg = CDebugCapsule::Create(m_pDevice, m_pContext, DbgDesc);

		if (nullptr != m_pDbg)
		{
			m_pDbg->Update_Pose(vArmPos, XMQuaternionIdentity());
			m_pDbg->Late_Update(fTimeDelta);
		}
	}
#endif

	if (false == m_bHit && nullptr != m_pTarget)
	{
		CTransform* pTargetTransform = m_pTarget->Get_Transform();

		if (nullptr != pTargetTransform)
		{
			const _vector vTargetPos = pTargetTransform->Get_State(STATE::POSITION);
			const _vector vToTarget = XMVectorSetY(vTargetPos - vArmPos, 0.f);
			const _float fDist = XMVectorGetX(XMVector3Length(vToTarget));

			if (fDist <= fCurRadius && fDist > 1e-4f)
			{
				const _vector vDir = XMVector3Normalize(vToTarget);

				if (XMVectorGetX(XMVector3Dot(vArmLook, vDir)) >= m_fHalfAngleCos)
				{
					HitInfo tHit{};
					tHit.fDamage = m_fDamage;
					tHit.pOtherActor = nullptr;
					tHit.pDamangeCauser = this;
					tHit.eHitType = m_eHitType;

					unordered_set<CGameObject*> Targets;
					if (true == CHitUtil::Apply_HitOnce(m_pTarget, tHit, vTargetPos, XMVectorSet(0.f, 1.f, 0.f, 0.f), Targets))
						m_bHit = true;
				}
			}
		}
	}

	if (fRatio >= 1.f)
		InActivate();
}

HRESULT CBoltSector::Render()
{
	return S_OK;
}

void CBoltSector::Activate(void* pArg)
{
	__super::Activate(pArg);

	auto pDesc = static_cast<ARM_DESC*>(pArg);
	if (nullptr == pDesc || nullptr == pDesc->pOwner)
		return;

	m_pTarget = pDesc->pTarget;
	m_bHit = false;
	m_fDelay = pDesc->fDelay;
	m_fTimer = 0.f;
	m_fDuration = pDesc->fDuration;
	m_fRadius = pDesc->fRadius;
	m_fHalfAngleCos = pDesc->fHalfAngleCos;
	m_fDamage = pDesc->fDamage;
	m_eHitType = pDesc->eHitType;
	m_iType = pDesc->iType;

	CTransform* pOwnerTransform = pDesc->pOwner->Get_Transform();
	if (nullptr == pOwnerTransform)
		return;

	const _vector vArmPos = (true == pDesc->bUsePosition)
		? XMLoadFloat3(&pDesc->vPosition)
		: pOwnerTransform->Get_State(STATE::POSITION);

	_vector vArmLook = XMVectorSetY(pOwnerTransform->Get_State(STATE::LOOK), 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vArmLook)) > 1e-6f)
		vArmLook = XMVector3Normalize(vArmLook);

	m_pTransformCom->Set_State(STATE::POSITION, vArmPos);
	m_pTransformCom->Set_State(STATE::LOOK, vArmLook);

	_float3 vEffectPos;
	XMStoreFloat3(&vEffectPos, vArmPos);
	vEffectPos.y += 0.05f;

	m_pGameInstance->Play_Effect(m_iType == 0 ? BOLTSECTOR_EFFECT : BOLTRANGE_EFFECT, vEffectPos);
}


void CBoltSector::InActivate()
{
	__super::InActivate();
}

CBoltSector* CBoltSector::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBoltSector* pInstance = new CBoltSector(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBoltSector");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CBoltSector::Clone(void* pArg)
{
	return nullptr;
}

void CBoltSector::Free()
{
#ifdef _DEBUG
	Safe_Release(m_pDbg);
#endif

	__super::Free();
}
