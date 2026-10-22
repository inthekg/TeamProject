#include "GroundBall.h"
#include "GameInstance.h"
#include "Effect.h"

CGroundBall::CGroundBall(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CProjectile{ pDevice, pContext }
{
}

CGroundBall::CGroundBall(const CGroundBall& Prototype)
	: CProjectile{ Prototype }
{
}

HRESULT CGroundBall::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CGroundBall::Initialize(void* pArg)
{
	return __super::Initialize(pArg);
}

void CGroundBall::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CGroundBall::Update(_float fTimeDelta)
{
	Update_Standard(fTimeDelta);
}

void CGroundBall::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CGroundBall::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CGroundBall::Render()
{
	return __super::Render();
}

void CGroundBall::Activate(void* pArg)
{
	FIRE_DESC* pDesc = static_cast<FIRE_DESC*>(pArg);
	m_vScale = pDesc->vScale;
	m_pTransformCom->Set_Scale(m_vScale);
	m_pEffectFx = m_pGameInstance->Play_Effect_Model("Effect_Raxasia_GroundBall", m_pModelCom, m_pTransformCom->Get_WorldMatrixPtr());
	m_pTrailFx = m_pGameInstance->Play_Effect("Effect_Raxasia_Projectile_Trail", m_pModelCom->Get_Bone_CombinedMatPtr("BoneC"), m_pTransformCom->Get_WorldMatrixPtr());


	__super::Activate(pArg);
}

void CGroundBall::InActivate()
{
	if (m_pEffectFx != nullptr)
	{
		m_pEffectFx->Stop();
		m_pEffectFx = nullptr;
	}

	if (m_pTrailFx != nullptr)
	{
		m_pTrailFx->Stop();
		m_pTrailFx = nullptr;
	}
	__super::InActivate();
}

void CGroundBall::Collect_Trails(vector<TRAIL_SPEC>& Out) const
{
	const char* pKey = m_strFxKey.empty() ? nullptr : m_strFxKey.c_str();

	Out.push_back({ "BoneL", "BoneR", pKey });
}

CGroundBall* CGroundBall::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CGroundBall* pInstance = new CGroundBall(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CGroundBall");
		Safe_Release(pInstance);
	}
	return pInstance;
}
