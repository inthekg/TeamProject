#include "Monster_Training.h"
#include "GameInstance.h"
#include "BehaviorTree.h"
#include "BT_BlackBoard.h"
#include "PartObject.h"

#include "Model.h"
#include "Animator.h"
#include "DebugCapsule.h"
#include "StatComponent_AI.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CMonster_Training, CAIObject)
ADD_PROPERTY(CMonster_Training, m_bFront, Bool)
ADD_PROPERTY_SETTER(CMonster_Training, m_fFistSweepRadius, Float, Set_FistSweepRadius, _float)
ADD_PROPERTY_SETTER(CMonster_Training, m_fFistSweepHalfHeight, Float, Set_FistSweepHalfHeight, _float)
END_REFLECTION(CMonster_Training)
#endif

CMonster_Training::CMonster_Training(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CAIObject{ pDevice, pContext }
{
}

CMonster_Training::CMonster_Training(const CMonster_Training& Prototype)
	: CAIObject{ Prototype }	
{
	m_strBTPath = Prototype.m_strBTPath;
}

HRESULT CMonster_Training::Initialize_Prototype()
{	
	if (FAILED(__super::Initialize_Prototype()))
		return E_FAIL;

	m_NotifyKeys[Hash_String(TEXT("REVIVE"))] = NOTIFY_KEY::REVIVE;
	m_strBTPath = wstring(SOLUTION_DIR) + L"GameCore/Bin/Resources/BT/Monster_Training.json";
	return S_OK;
}

HRESULT CMonster_Training::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	CAIObject::CHARACTER_SETUP_DESC SetupDesc{};
	SetupDesc.strModelPrototypeTag = TEXT("Prototype_Component_Model_Training");
	SetupDesc.strAnimFolderTag = TEXT("Training");
	SetupDesc.strAnimTag = TEXT("Training");
	SetupDesc.pRootBoneName = "Root";
	SetupDesc.bUseCCT = true;
	SetupDesc.fCCTRadius = 0.4f;
	SetupDesc.fCCTHeight = 1.5f;
	//SetupDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);
	//추가 : 모델과 객체가 같은 챕터 레벨에 있다는 전제하에 진행 
	if (pArg != nullptr)
	{
		auto pDesc = static_cast<AIOBJECT_DESC*>(pArg);
		SetupDesc.iModelLevel = pDesc->iProtoLevel;
	}
	else SetupDesc.iModelLevel = ETOUI(LEVEL::GAMEPLAY);

	if (FAILED(Ready_Character(SetupDesc)))
		return E_FAIL;

	Ready_FistBones();

#ifdef _DEBUG
	Rebuild_FistDebugCapsule();
#endif

	return S_OK;
}

void CMonster_Training::Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	__super::Update(fTimeDelta);
}

void CMonster_Training::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CMonster_Training::Late_Update(_float fTimeDelta)
{
	if (true == Is_AISleep())
		return;

	__super::Late_Update(fTimeDelta);

	Drive_FistCollision(fTimeDelta);

#ifdef _DEBUG
	if (true == m_pGameInstance->Get_DebugFlag())
	{
		if (m_fFistSweepRadius != m_fFistLastDbgRadius || m_fFistSweepHalfHeight != m_fFistLastDbgHalf)
			Rebuild_FistDebugCapsule();

		_vector vCenter, vQuat;
		if (nullptr != m_pFistDebugCapsule &&
			true == Make_BoneCapsule(m_iFistTopBoneIndex, m_iFistBotBoneIndex, vCenter, vQuat))
		{
			m_pFistDebugCapsule->Update_Pose(vCenter, vQuat);
			m_pFistDebugCapsule->Late_Update(fTimeDelta);
		}
	}
#endif

}

HRESULT CMonster_Training::Render()
{
	return __super::Render();
}

void CMonster_Training::On_Freeze(_bool bFreeze)
{
	if (true == bFreeze)
		m_bFistActive = false;
}

void CMonster_Training::Action_Notify(_uint64 iHashKey)
{
	NOTIFY_KEY eKey = m_NotifyKeys.find(iHashKey) != m_NotifyKeys.end() ? static_cast<NOTIFY_KEY>(m_NotifyKeys[iHashKey]) : NOTIFY_KEY::END;

	switch (eKey)
	{
	case NOTIFY_KEY::REVIVE:
		Spawn();
		break;
	}
}

void CMonster_Training::Spawn()
{
	__super::Spawn();
	if (Is_Dead() == true)
	{
		m_pStatCom->Revive();
		m_pStatCom->Set_Staggerable(false);
		m_pStatCom->Set_SinceHit(10.f);
		m_bDetected = false;
	}
	m_pStatCom->Reset_Groggy();

	// 이전 위치가 남아 있으면 리스폰 직후 죽은 자리에서 여기까지 한 번에 Sweep 되어 헛히트가 난다.
	m_bFistActive = false;
	m_bPrevHitActive = false;
	m_bFistPrevValid = false;
}
void CMonster_Training::Despawn()
{
	__super::Despawn();
}
void CMonster_Training::On_Groggy(_bool bGroggy)
{
	if (true == bGroggy)
		m_bFistActive = false;
}


#ifdef _DEBUG
void CMonster_Training::Rebuild_FistDebugCapsule()
{
	Safe_Release(m_pFistDebugCapsule);

	CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DbgDesc{};
	DbgDesc.fRadius = m_fFistSweepRadius;
	DbgDesc.fHalfHeight = m_fFistSweepHalfHeight;
	DbgDesc.iSegment = 10;
	DbgDesc.vColor = _float4{ 1.f, 0.4f, 0.f, 1.f };

	m_pFistDebugCapsule = CDebugCapsule::Create(m_pDevice, m_pContext, DbgDesc);

	m_fFistLastDbgRadius = m_fFistSweepRadius;
	m_fFistLastDbgHalf = m_fFistSweepHalfHeight;
}

#endif // _DEBUG

void CMonster_Training::Update_BlackBoard(_float fTimeDelta)
{
	__super::Update_BlackBoard(fTimeDelta);

	m_bFront = false;
	if (nullptr != m_pTarget && nullptr != m_pTransformCom)
	{
		CTransform* pTargetTransform = m_pTarget->Get_Transform();
		if (nullptr != pTargetTransform)
		{
			_vector vTargetPos = pTargetTransform->Get_State(STATE::POSITION);
			_vector vMyPos = m_pTransformCom->Get_State(STATE::POSITION);
			_vector vTo = XMVectorSubtract(vTargetPos, vMyPos);
			_float  fDist = XMVectorGetX(XMVector3Length(vTo));
			if (fDist > 0.0001f && fDist <= m_fDetectRange)
			{
				_vector vDir = XMVector3Normalize(XMVectorSetY(vTo, 0.f));
				_vector vFwd = XMVector3Normalize(XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));
				m_bFront = (XMVectorGetX(XMVector3Dot(vFwd, vDir)) >= m_fFrontCos);
				m_bDetected = true;
			}
		}
	}

	const _bool bDead = m_pStatCom->Is_Dead();
	if (bDead)
	{
		m_bDetected = false;
	}
	m_pBlackBoard->Set<_bool>(L"bDead", bDead);
	m_pBlackBoard->Set<_bool>(L"bPlayerInFront", m_bFront);


	Set_AnimBool(L"Dead", bDead);
	Set_AnimBool(L"Groggy", m_pBlackBoard->Get<_bool>(L"bGroggy"));
	Set_AnimBool(L"Fatal", m_pBlackBoard->Get<_bool>(L"bFatal"));
	Set_AnimBool(L"InFront", m_bFront);
	Set_AnimBool(L"Damaged", m_pStatCom->Get_SinceHit() < 0.3f);
}

void CMonster_Training::On_Perceive_Attacker(_bool bPreFatal)
{
	if (true == bPreFatal)
		m_bDetected = true;
}

_bool CMonster_Training::Uses_Full_Hit_Reaction() const
{
	return false;
}

void CMonster_Training::Apply_ChargeHit(_float fDamage, _float fGroggy)
{
	m_pStatCom->Apply_Damage(fDamage);
	if (Is_Dead())
		return;

	m_pStatCom->Set_SinceHit(0.f);

	if (true == m_pStatCom->Is_Staggerable())
	{
		m_pStatCom->Set_Staggerable(false);
		m_pStatCom->Reset_Groggy();
		Set_Groggy(true);
	}
	else
	{
		m_pStatCom->Add_Groggy(fGroggy);
		if (m_pStatCom->Is_GroggyFull())
			m_pStatCom->Set_Staggerable(true);
	}
}


HRESULT CMonster_Training::Ready_FistBones()
{
	if (nullptr == m_pModelCom)
		return E_FAIL;

	m_iFistTopBoneIndex = m_pModelCom->Get_BoneIndex("Bip001-R-Forearm");
	m_iFistBotBoneIndex = m_pModelCom->Get_BoneIndex("Bip001-R-Finger0");

#ifdef _DEBUG
	if (m_iFistTopBoneIndex < 0 || m_iFistBotBoneIndex < 0)
		MSG_BOX("CMonster_Training : Fist bone not found (check bone names)");
#endif

	return S_OK;
}

void CMonster_Training::Drive_FistCollision(_float fTimeDelta)
{
	const _bool bHit = Is_HitActive();

	if (true == bHit && false == m_bPrevHitActive)
	{
		m_bFistActive = true;
		m_bFistPrevValid = false;
		m_FistTargets.clear();
	}
	else if (false == bHit && true == m_bPrevHitActive)
	{
		m_bFistActive = false;
	}
	m_bPrevHitActive = bHit;

	if (true == m_bFistActive)
		Sweep_RightFist();
}

void CMonster_Training::Sweep_RightFist()
{
	_vector vCenter, vQuat;
	if (false == Make_BoneCapsule(m_iFistTopBoneIndex, m_iFistBotBoneIndex, vCenter, vQuat))
		return;

	if (false == m_bFistPrevValid)
	{
		XMStoreFloat3(&m_vFistPrevCenter, vCenter);
		m_bFistPrevValid = true;
		return;
	}

	const _vector vPrev = XMLoadFloat3(&m_vFistPrevCenter);
	XMStoreFloat3(&m_vFistPrevCenter, vCenter);

	HitInfo tHit;
	tHit.fDamage = m_fFistDamage;
	tHit.eHitType = HIT_TYPE::Heavy;

	Hit_SweepCapsule(m_fFistSweepRadius, m_fFistSweepHalfHeight,
		vPrev, vCenter, vQuat, tHit, m_FistTargets);
}

CMonster_Training* CMonster_Training::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMonster_Training* pInstance = new CMonster_Training(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMonster_Training");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CMonster_Training::Clone(void* pArg)
{
	CMonster_Training* pInstance = new CMonster_Training(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonster_Training");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMonster_Training::Free()
{
	__super::Free();

#ifdef _DEBUG
	Safe_Release(m_pFistDebugCapsule);
#endif
}
