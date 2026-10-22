#include "ProjectileEmitter.h"
#include "GameInstance.h"
#include "Transform.h"
#include "Monster_Include.h"

static const string	EMITTER_IMPACT_SOUND = "SE_NPC_Raxasia_SK_Impact_Ground_Heavy_Cue";
static const string	EMITTER_FIRE_SOUND = "SE_NPC_Raxasia_SK_PJ_Spark_Ground_Explo_Cue";

CProjectileEmitter::CProjectileEmitter(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPoolableObject{ pDevice, pContext }
{
}

CProjectileEmitter::CProjectileEmitter(const CProjectileEmitter& Prototype)
	: CPoolableObject{ Prototype }
{
}

HRESULT CProjectileEmitter::Initialize_Prototype()
{
	m_pTransformCom = CTransform::Create(m_pDevice, m_pContext);
	if (nullptr == m_pTransformCom)
		return E_FAIL;

	Set_InActive();

	return S_OK;
}

HRESULT CProjectileEmitter::Initialize(void* pArg)
{
	return S_OK;
}

void CProjectileEmitter::Priority_Update(_float fTimeDelta)
{
}

void CProjectileEmitter::Update(_float fTimeDelta)
{
}

void CProjectileEmitter::Physical_Update(_float fFixedTime)
{
}

void CProjectileEmitter::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	if (m_fDelay > 0.f)
	{
		m_fDelay -= fTimeDelta;
		return;
	}

	m_pGameInstance->Play_Cue(EMITTER_FIRE_SOUND, SOUND_MONSTER, m_vOrigin, 1.f, false);

	Fire_Spread();
	InActivate();
}


HRESULT CProjectileEmitter::Render()
{
	return S_OK;
}

void CProjectileEmitter::Activate(void* pArg)
{
	__super::Activate(pArg);

	auto pDesc = static_cast<ARM_DESC*>(pArg);
	if (nullptr == pDesc || nullptr == pDesc->pOwner)
		return;

	m_pOwner = pDesc->pOwner;
	m_pTarget = pDesc->pTarget;
	m_vOrigin = pDesc->vOrigin;
	m_vAimDir = pDesc->vAimDir;
	m_pTelegraphFX = pDesc->pTelegraphFX;
	m_eMode = pDesc->eMode;
	m_iCount = pDesc->iCount;
	m_fStepDeg = pDesc->fStepDeg;
	m_bFullCircle = pDesc->bFullCircle;
	m_fDelay = pDesc->fDelay;
	m_fSpeed = pDesc->fSpeed;
	m_fAccel = pDesc->fAccel;
	m_fRange = pDesc->fRange;
	m_fRadius = pDesc->fRadius;
	m_fDamage = pDesc->fDamage;
	m_eHitType = pDesc->eHitType;

	m_iCurrentLevel = m_pGameInstance->Get_CurrentLevelIndex();

	m_pTransformCom->Set_State(STATE::POSITION, XMLoadFloat3(&m_vOrigin));

	if (nullptr != m_pTelegraphFX)
		m_pGameInstance->Play_Effect(m_pTelegraphFX, m_vOrigin);

	m_pGameInstance->Play_Cue(EMITTER_IMPACT_SOUND, SOUND_MONSTER, m_vOrigin);
}

void CProjectileEmitter::InActivate()
{
	__super::InActivate();
}

void CProjectileEmitter::Fire_Spread()
{
	_vector vBase = XMVectorSetY(XMLoadFloat3(&m_vAimDir), 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vBase)) < 1e-6f)
		vBase = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	vBase = XMVector3Normalize(vBase);

	const _float fStepUse = (true == m_bFullCircle && m_iCount > 0) ? (360.f / m_iCount) : m_fStepDeg;
	const _float fStart = (true == m_bFullCircle) ? 0.f : -0.5f * (m_iCount - 1) * fStepUse;

	shared_ptr<bool> pVolleyHit = make_shared<bool>(false);

	for (_int i = 0; i < m_iCount; ++i)
	{
		const _float fDeg = fStart + i * fStepUse;
		const _vector q = XMQuaternionRotationRollPitchYaw(0.f, XMConvertToRadians(fDeg), 0.f);
		const _vector vDir = XMVector3Rotate(vBase, q);

		CProjectile::FIRE_DESC fd{};
		fd.eMode = m_eMode;
		fd.vOrigin = m_vOrigin;
		XMStoreFloat3(&fd.vDir, vDir);
		fd.fSpeed = m_fSpeed;
		fd.fAccel = m_fAccel;
		fd.fRange = m_fRange;
		fd.fRadius = m_fRadius;
		fd.fDamage = m_fDamage;
		fd.eHitType = m_eHitType;
		fd.pOwner = m_pOwner;
		fd.pTarget = m_pTarget;
		fd.pVolleyHit = pVolleyHit;

		CSectorBall* pProjectile = m_pGameInstance->Get_PoolObject<CSectorBall>(&fd);
		if (nullptr != pProjectile)
			m_pGameInstance->Add_GameObject(ETOUI(m_iCurrentLevel), TEXT("Layer_Projectile"), pProjectile);
	}
}

CProjectileEmitter* CProjectileEmitter::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CProjectileEmitter* pInstance = new CProjectileEmitter(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CProjectileEmitter");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CProjectileEmitter::Clone(void* pArg)
{
	return nullptr;
}

void CProjectileEmitter::Free()
{
	__super::Free();
}
