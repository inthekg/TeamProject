#include "Missile.h"
#include "GameInstance.h"
#include "Transform.h"

namespace
{
	static const _wstring MISSILE_MODEL_TAG = TEXT("Prototype_Component_Model_Missile");
}

CMissile::CMissile(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CProjectile{ pDevice, pContext }
{
}

CMissile::CMissile(const CMissile& Prototype)
	: CProjectile{ Prototype }
{
}

HRESULT CMissile::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CMissile::Initialize(void* pArg)
{
	return __super::Initialize(pArg);
}

void CMissile::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CMissile::Update(_float fTimeDelta)
{
	Update_Missile(fTimeDelta);
}

void CMissile::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CMissile::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CMissile::Render()
{
	return __super::Render();
}

void CMissile::Activate(void* pArg)
{
	__super::Activate(pArg);
}

void CMissile::InActivate()
{
	__super::InActivate();
}

const _wstring& CMissile::Get_ModelProtoTag() const
{
	return MISSILE_MODEL_TAG;
}

void CMissile::On_ImpactFx(_fvector vPos)
{
	_float3 vCenter;
	XMStoreFloat3(&vCenter, vPos);

	m_pGameInstance->Play_Effect("Effect_Balista_Fire", vCenter);
}

void CMissile::On_Explode(_fvector vPos)
{
	On_ImpactFx(vPos);

	vector<CGameObject*> Targets;
	if (false == m_pGameInstance->Overlap_Sphere(m_fExplodeRadius, vPos, FILTER_GROUP::PLAYER, Targets))
		return;

	_float3 vCenter;
	XMStoreFloat3(&vCenter, vPos);

	const FILTER_GROUP eBlock = static_cast<FILTER_GROUP>(ETOUI(FILTER_GROUP::GROUND) | ETOUI(FILTER_GROUP::OBJECT));

	for (CGameObject* pTarget : Targets)
	{
		if (nullptr == pTarget)
			continue;

		_vector vTargetPos = pTarget->Get_Transform()->Get_State(STATE::POSITION);
		_vector vTo = XMVectorSubtract(vTargetPos, vPos);
		const _float fDist = XMVectorGetX(XMVector3Length(vTo));

		if (fDist > 1e-4f)
		{
			_float3 vDir;
			XMStoreFloat3(&vDir, XMVectorScale(vTo, 1.f / fDist));

			CollisionResult tBlock{};
			if (true == m_pGameInstance->Raycast(vCenter, vDir, fDist, eBlock, tBlock, PxQueryFlag::eSTATIC))
				continue;
		}

		HitInfo tHit{};
		tHit.fDamage = m_fDamage;
		tHit.pOtherActor = nullptr;
		tHit.pDamangeCauser = this;
		tHit.eHitType = m_eHitType;
		XMStoreFloat3(&tHit.vHitPosition, vTargetPos);

		pTarget->Take_Damage(tHit);
	}
}

void CMissile::Collect_Trails(vector<TRAIL_SPEC>& Out) const
{
	const char* pKey = m_strTrailKey.empty() ? nullptr : m_strTrailKey.c_str();

	Out.push_back({ "Tail_01", "Tail_03", pKey });
	Out.push_back({ "Tail_02", "Tail_04", pKey });
}

CProjectile::MODE CMissile::Get_Mode() const
{
	return CProjectile::MODE::SHOT;
}

CMissile* CMissile::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMissile* pInstance = new CMissile(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMissile");
		Safe_Release(pInstance);
	}
	return pInstance;
}
