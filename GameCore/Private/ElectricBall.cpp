#include "ElectricBall.h"
#include "GameInstance.h"
#include "Effect.h"

CElectricBall::CElectricBall(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CProjectile{ pDevice, pContext }
{
}

CElectricBall::CElectricBall(const CElectricBall& Prototype)
	: CProjectile{ Prototype }
{
}

HRESULT CElectricBall::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CElectricBall::Initialize(void* pArg)
{
	return __super::Initialize(pArg);
}

void CElectricBall::Priority_Update(_float fTimeDelta)
{
	__super::Priority_Update(fTimeDelta);
}

void CElectricBall::Update(_float fTimeDelta)
{
	Update_Standard(fTimeDelta);
}

void CElectricBall::Physical_Update(_float fFixedTime)
{
	__super::Physical_Update(fFixedTime);
}

void CElectricBall::Late_Update(_float fTimeDelta)	
{
	if (true == Is_InActive())
		return;

	m_pGameInstance->Add_RenderGroup(RENDERID::NONBLEND, this);
}

HRESULT CElectricBall::Render()
{
	return __super::Render();
}

void CElectricBall::Activate(void* pArg)
{
	FIRE_DESC* pDesc = static_cast<FIRE_DESC*>(pArg);
	m_vScale = pDesc->vScale;
	m_pTransformCom->Set_Scale(m_vScale);
	m_bUseReflectFx = false;
	m_pEffectFx = m_pGameInstance->Play_Effect_Model("Effect_Raxasia_ElectricBall", m_pModelCom, m_pTransformCom->Get_WorldMatrixPtr());
	m_pTrailFx = m_pGameInstance->Play_Effect("Effect_Raxasia_Projectile_Trail", m_pModelCom->Get_Bone_CombinedMatPtr("BoneC"), m_pTransformCom->Get_WorldMatrixPtr());
	
	__super::Activate(pArg);
}

void CElectricBall::InActivate()
{
	if (m_pEffectFx != nullptr)
	{
		m_pEffectFx->Stop_Immediate();
		m_pEffectFx = nullptr;
	}

	if (m_pTrailFx != nullptr)
	{
		m_pTrailFx->Stop_Immediate();
		m_pTrailFx = nullptr;
	}
	__super::InActivate();
}

void CElectricBall::On_Reflect()
{
	if (m_pEffectFx != nullptr)
	{
		m_pEffectFx->Stop_Immediate();
		m_pEffectFx = nullptr;
	}
	if (m_pTrailFx != nullptr)
	{
		m_pTrailFx->Stop_Immediate();
		m_pTrailFx = nullptr;
	}	m_pEffectFx = m_pGameInstance->Play_Effect_Model("Effect_Raxasia_ElectricBall_Reflect", m_pModelCom, m_pTransformCom->Get_WorldMatrixPtr());
	
	_float3 vPos;
	XMStoreFloat3(&vPos, m_pTransformCom->Get_State(STATE::POSITION));
	m_pGameInstance->Play_Effect("Effect_Raxasia_ElectricBall_Reflect_Success", vPos);
	m_pTrailFx = m_pGameInstance->Play_Effect("Effect_Raxasia_Projectile_Reflect_Trail", m_pModelCom->Get_Bone_CombinedMatPtr("BoneC"), m_pTransformCom->Get_WorldMatrixPtr());
	m_bUseReflectFx = true;

	Spawn_Trails();
}

void CElectricBall::Collect_Trails(vector<TRAIL_SPEC>& Out) const
{
	const string& strKey = (true == m_bUseReflectFx) ? m_strReflectFxKey : m_strFxKey;
	const char* pKey = strKey.empty() ? nullptr : strKey.c_str();

	Out.push_back({ "BoneL", "BoneR", pKey });
}

CElectricBall* CElectricBall::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CElectricBall* pInstance = new CElectricBall(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CElectricBall");
		Safe_Release(pInstance);
	}
	return pInstance;
}
