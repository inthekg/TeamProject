#include "BoltMine.h"
#include "GameInstance.h"
#include "Transform.h"
#include "HitUtil.h"

#ifdef _DEBUG
#include "DebugCapsule.h"
#endif

static const string	BOLTMINE_EFFECT = "Effect_Bolt_Mine";
static const string	BOLTMINE_IMPACT_SOUND = "SE_NPC_Raxasia_SK_Impact_Ground_Heavy_Cue";
static const string	BOLTMINE_SOUND = "SE_NPC_Raxasia_SK_Spark_Explo_Cue";


CBoltMine::CBoltMine(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CPoolableObject{ pDevice, pContext }
{
}

CBoltMine::CBoltMine(const CBoltMine& Prototype)
    : CPoolableObject{ Prototype }
{
}

HRESULT CBoltMine::Initialize_Prototype()
{
    m_pTransformCom = CTransform::Create(m_pDevice, m_pContext);
    if (nullptr == m_pTransformCom)
        return E_FAIL;

    Set_InActive();

    return S_OK;
}

HRESULT CBoltMine::Initialize(void* pArg)
{
    return S_OK;
}

void CBoltMine::Priority_Update(_float fTimeDelta)
{
}

void CBoltMine::Update(_float fTimeDelta)
{

}

void CBoltMine::Physical_Update(_float fFixedTime)
{
}

void CBoltMine::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

#ifdef _DEBUG
	if (true == m_pGameInstance->Get_DebugFlag())
	{
		if (nullptr == m_pDbg)
		{
			CDebugCapsule::DEBUGCAPSULE_OBJ_DESC DbgDesc{};
			DbgDesc.fRadius = m_fRadius;
			DbgDesc.fHalfHeight = 0.f;
			DbgDesc.iSegment = 12;
			DbgDesc.vColor = _float4{ 0.4f, 0.6f, 1.f, 1.f };
			m_pDbg = CDebugCapsule::Create(m_pDevice, m_pContext, DbgDesc);
		}
		if (nullptr != m_pDbg)
		{
			m_pDbg->Update_Pose(m_pTransformCom->Get_State(STATE::POSITION), XMQuaternionIdentity());
			m_pDbg->Late_Update(fTimeDelta);
		}
	}
#endif

	if (false == m_bDetonated)
	{
		m_fDelay -= fTimeDelta;
		if (m_fDelay <= 0.f)
			Detonate();
		return;
	}

	m_fLingerDelay -= fTimeDelta;
	if (m_fLingerDelay <= 0.f)
		InActivate();
}

HRESULT CBoltMine::Render()
{
    return S_OK;
}

void CBoltMine::Activate(void* pArg)
{
	__super::Activate(pArg);

	auto pDesc = static_cast<ARM_DESC*>(pArg);
	if (nullptr == pDesc)
		return;

	m_pOwner = pDesc->pOwner;
	m_bDetonated = false;
	m_fDelay = pDesc->fDelay;
	m_fLingerDelay = pDesc->fLingerDelay;
	m_fRadius = pDesc->fRadius;
	m_fDamage = pDesc->fDamage;
	m_eHitType = pDesc->eHitType;

	m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat3(&pDesc->vPosition));

	m_pGameInstance->Play_Effect(BOLTMINE_EFFECT, pDesc->vPosition);
	m_pGameInstance->Play_Cue(BOLTMINE_IMPACT_SOUND, SOUND_MONSTER, pDesc->vPosition);
}

void CBoltMine::InActivate()
{
	__super::InActivate();
}

void CBoltMine::Detonate()
{
	m_bDetonated = true;

	_float3 vWorldPos;
	XMStoreFloat3(&vWorldPos, m_pTransformCom->Get_State(STATE::POSITION));

	m_pGameInstance->Play_Cue(BOLTMINE_SOUND, SOUND_MONSTER, vWorldPos);

	HitInfo tHit{};
	tHit.fDamage = m_fDamage;
	tHit.pOtherActor = nullptr;
	tHit.pDamangeCauser = this;
	tHit.eHitType = m_eHitType;

	unordered_set<CGameObject*> Targets;
	CHitUtil::Overlap_Sphere(m_pGameInstance, m_fRadius, m_pTransformCom->Get_State(STATE::POSITION),
		tHit, Targets, FILTER_GROUP::PLAYER);
}


CBoltMine* CBoltMine::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBoltMine* pInstance = new CBoltMine(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBoltMine");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CBoltMine::Clone(void* pArg)
{
    return nullptr;
}

void CBoltMine::Free()
{
#ifdef _DEBUG
	Safe_Release(m_pDbg);
#endif

	__super::Free();
}