#include "SectorBall.h"
#include "GameInstance.h"
#include "Effect.h"

CSectorBall::CSectorBall(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CProjectile{ pDevice, pContext }
{
}

CSectorBall::CSectorBall(const CSectorBall& Prototype)
	: CProjectile{ Prototype }
{
}

HRESULT CSectorBall::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CSectorBall::Initialize(void* pArg)
{
	return __super::Initialize(pArg);
}

void CSectorBall::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CSectorBall::Update(_float fTimeDelta)
{
	Update_Standard(fTimeDelta);
}

void CSectorBall::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CSectorBall::Late_Update(_float fTimeDelta)
{
	if (true == Is_InActive())
		return;

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CSectorBall::Render()
{
	return __super::Render();
}

void CSectorBall::Activate(void* pArg)
{
	FIRE_DESC* pDesc = static_cast<FIRE_DESC*>(pArg);
	m_vScale = pDesc->vScale;
	m_pTransformCom->Set_Scale(m_vScale);
	m_pEffectFx = m_pGameInstance->Play_Effect("Effect_Raxasia_GroundBall", m_pModelCom->Get_Bone_CombinedMatPtr("BoneC"), m_pTransformCom->Get_WorldMatrixPtr());
	m_pTrailFx = m_pGameInstance->Play_Effect("Effect_Raxasia_GroundBall_Trail", m_pModelCom->Get_Bone_CombinedMatPtr("BoneC"), m_pTransformCom->Get_WorldMatrixPtr());

	__super::Activate(pArg);
}

void CSectorBall::InActivate()
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

void CSectorBall::Collect_Trails(vector<TRAIL_SPEC>& Out) const
{
	const char* pKey = m_strTrailKey.empty() ? nullptr : m_strTrailKey.c_str();

	Out.push_back({ "BoneL", "BoneR", pKey });
}

CSectorBall* CSectorBall::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CSectorBall* pInstance = new CSectorBall(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CSectorBall");
		Safe_Release(pInstance);
	}
	return pInstance;
}
