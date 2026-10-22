#include "Bullet.h"
#include "GameInstance.h"

CBullet::CBullet(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CProjectile{ pDevice, pContext }
{
}

CBullet::CBullet(const CBullet& Prototype)
	: CProjectile{ Prototype }
{
}

HRESULT CBullet::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CBullet::Initialize(void* pArg)
{
	return __super::Initialize(pArg);
}

void CBullet::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CBullet::Update(_float fTimeDelta)
{
	Update_Standard(fTimeDelta);
}

void CBullet::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CBullet::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	//m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CBullet::Render()
{
	return __super::Render();
}

void CBullet::Activate(void* pArg)
{
	__super::Activate(pArg);

	m_pTransformCom->Set_Scale(0.01f, 0.01f, 0.01f);
	m_fSpeed = 120.f;

}

void CBullet::InActivate()
{
	__super::InActivate();
}

void CBullet::Collect_Trails(vector<TRAIL_SPEC>& Out) const
{
	const char* pKey = m_strTrailKey.empty() ? nullptr : m_strTrailKey.c_str();

	Out.push_back({ "BoneL", "BoneR", pKey });
}

CBullet* CBullet::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBullet* pInstance = new CBullet(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBullet");
		Safe_Release(pInstance);
	}
	return pInstance;
}
