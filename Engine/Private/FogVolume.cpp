#include "FogVolume.h"
#include "GameInstance.h"

#ifdef _DEBUG
BEGIN_REFLECTION(CFogVolume, CGameObject)
ADD_PROPERTY(CFogVolume, m_tVolume.fRadius, Float)
ADD_PROPERTY_COLOR(CFogVolume, m_tVolume.vColor, Float3)
ADD_PROPERTY(CFogVolume, m_tVolume.fDensity, Float)
ADD_PROPERTY(CFogVolume, m_tVolume.fHeightFalloff, Float)
ADD_PROPERTY(CFogVolume, m_tVolume.fHeightFalloffDirection, Float)
END_REFLECTION(CFogVolume)
#endif // _DEBUG


CFogVolume::CFogVolume(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject{ pDevice, pContext }
	, m_tVolume{}
{
}

CFogVolume::CFogVolume(const CFogVolume& Prototype)
	: CGameObject{ Prototype }
	, m_tVolume{ Prototype.m_tVolume }
{
}

HRESULT CFogVolume::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CFogVolume::Initialize(void* pArg)
{
	FOGVOLUME_DESC* pDesc = static_cast<FOGVOLUME_DESC*>(pArg);

	if (nullptr != pDesc)
	{
		m_tVolume.iShapeType = static_cast<_int>(pDesc->iShapeType);
		m_tVolume.fRadius = pDesc->fRadius;
		m_tVolume.vExtents = pDesc->vExtents;
		m_tVolume.vColor = pDesc->vColor;
		m_tVolume.fDensity = pDesc->fDestiny;
		m_tVolume.fHeightFalloff = pDesc->fHeightFalloff;
		m_tVolume.fHeightFalloffDirection = pDesc->bHeightFalloffBottomToTop ? 1.f : -1.f; // 셰이더 적분이 사용할 월드 Y 방향 부호다.

		Bake_RotationAxis(pDesc->vRotation);
	}
	else
	{
		Bake_RotationAxis(_float3(0.f, 0.f, 0.f));
	}

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CFogVolume::Bake_RotationAxis(const _float3& vRotation)
{
	// 회전은 배치 후 고정이므로 한 번만 축으로 굽는다.
	// 셰이더는 이 세 축을 행으로 하는 행렬로 월드 광선을 볼륨 로컬로 옮긴다.
	_matrix matRotation = XMMatrixRotationRollPitchYaw(vRotation.x, vRotation.y, vRotation.z);

	XMStoreFloat3(&m_tVolume.vAxisX, matRotation.r[0]);
	XMStoreFloat3(&m_tVolume.vAxisY, matRotation.r[1]);
	XMStoreFloat3(&m_tVolume.vAxisZ, matRotation.r[2]);
}

void CFogVolume::Update(_float fTimeDelta)
{

}

void CFogVolume::Physical_Update(_float fFixedTime)
{
}

void CFogVolume::Late_Update(_float fTimeDelta)
{
	if (false == m_bFogEnable)
		return;

	_float4 vWorld = {};
	XMStoreFloat4(&vWorld, m_pTransformCom->Get_State(STATE::POSITION));
	m_tVolume.vCenter = _float3(vWorld.x, vWorld.y, vWorld.z);
#ifdef _DEBUG
	if (m_bAdd == false)
		return;
#endif // _DEBUG

	m_pGameInstance->Add_FogVolume(m_tVolume);
}

HRESULT CFogVolume::Render()
{
	return S_OK;
}

CFogVolume* CFogVolume::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CFogVolume* pInstance = new CFogVolume(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CFogVolume");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CFogVolume::Clone(void* pArg)
{
	CFogVolume* pInstance = new CFogVolume(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CFogVolume");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CFogVolume::Free()
{
	__super::Free();
}
