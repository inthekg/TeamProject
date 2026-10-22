#include "Weapon_Range.h"
#include "GameInstance.h"
#include "Monster_Include.h"
#include "AIObject.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CWeapon_Range, CWeapon_Monster)
END_REFLECTION(CWeapon_Range)
#endif

CWeapon_Range::CWeapon_Range(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CWeapon_Monster{ pDevice, pContext }
{
}

CWeapon_Range::CWeapon_Range(const CWeapon_Range& Prototype)
	: CWeapon_Monster{ Prototype }
{
}

HRESULT CWeapon_Range::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CWeapon_Range::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	auto pDesc = static_cast<RANGE_DESC*>(pArg);
	m_fRange = pDesc->fRange;
	m_fBulletSpeed = pDesc->fBulletSpeed;
	m_fBulletAccel = pDesc->fBulletAccel;
	m_fBulletRadius = pDesc->fBulletRadius;
	m_eFireMode = pDesc->eFireMode;

	m_iCurrentLevel = m_pGameInstance->Get_CurrentLevelIndex();

	return S_OK;
}

void CWeapon_Range::Priority_Update(_float fTimeDelta)
{
}

void CWeapon_Range::Update(_float fTimeDelta)
{
}

void CWeapon_Range::Physical_Update(_float fFixedTime)
{
}

void CWeapon_Range::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
}

HRESULT CWeapon_Range::Render()
{
	return __super::Render();
}

void CWeapon_Range::Fire(_fvector vOrigin, _fvector vDir, _float fDamage, HIT_TYPE eType)
{
	if (FIRE_MODE::PROJECTILE != m_eFireMode)
		return;

	m_pGameInstance->Play_Effect("Effect_Shotgun_Muzzle",
		m_pModelCom->Get_Bone_CombinedMatPtr("FX_WP_Muzzle_R"),
		Get_CombinedWorldMatrixPtr(),
		_float3(0.f, 0.f, 0.f));

	_float3 vFirePos;
	XMStoreFloat3(&vFirePos, vOrigin);
	m_pGameInstance->Play_Cue("SE_NPC_Buttler_SK_FX_Gun_Firing_L_Cue", CHANNEL_ID::SOUND_MONSTER, vFirePos);

	CProjectile::FIRE_DESC fd{};
	fd.eMode = CProjectile::MODE::SHOT;
	XMStoreFloat3(&fd.vOrigin, vOrigin);
	XMStoreFloat3(&fd.vDir, vDir);
	fd.vScale = _float3(1.f, 1.f, 1.f);
	fd.fSpeed = m_fBulletSpeed;
	fd.fAccel = m_fBulletAccel;
	fd.fRange = m_fRange;
	fd.fRadius = m_fBulletRadius;
	fd.fDamage = (fDamage >= 0.f) ? fDamage : m_fDamage;
	fd.eHitType = (eType != HIT_TYPE::Normal) ? eType : m_eHitType;
	fd.pOwner = m_pOwner;
	fd.pTarget = (nullptr != m_pOwner) ? m_pOwner->Get_Target() : nullptr;

	CBullet* pProjectile = m_pGameInstance->Get_PoolObject<CBullet>(&fd);
	if (nullptr == pProjectile)
		return;

	m_pGameInstance->Add_GameObject(ETOUI(m_iCurrentLevel), TEXT("Layer_Projectile"), pProjectile);
}


void CWeapon_Range::Update_Attack(_float fTimeDelta)
{
}

CWeapon_Range* CWeapon_Range::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CWeapon_Range* pInstance = new CWeapon_Range(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CWeapon_Range");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CWeapon_Range::Clone(void* pArg)
{
	CWeapon_Range* pInstance = new CWeapon_Range(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CWeapon_Range");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CWeapon_Range::Free()
{
	__super::Free();
}
