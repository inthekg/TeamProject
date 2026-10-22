#include "Camera.h"
#include "GameInstance.h"

CCamera::CCamera(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject { pDevice, pContext }
{
}

CCamera::CCamera(const CCamera& Prototype)
	: CGameObject{ Prototype }
{
}

HRESULT CCamera::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CCamera::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CAMERA_DESC*>(pArg);

	D3D11_VIEWPORT			ViewPortDesc{};
	_uint				iNumViewports = { 1 };

	m_pContext->RSGetViewports(&iNumViewports, &ViewPortDesc);

	m_fNear = pDesc->fNear;
	m_fAspect = static_cast<_float>(ViewPortDesc.Width) / ViewPortDesc.Height;
	m_fFar = pDesc->fFar;
	m_fFovy = pDesc->fFovy;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(XMLoadFloat3(&pDesc->vEye), 1.f));
	m_pTransformCom->LookAt(XMVectorSetW(XMLoadFloat3(&pDesc->vAt), 1.f));

	XMStoreFloat4x4(&m_ProjMatrix,
		XMMatrixPerspectiveFovLH(m_fFovy, m_fAspect, m_fNear, m_fFar));

	Update_PipeLine();

	return S_OK;
}

void CCamera::Priority_Update(_float fTimeDelta)
{
}	

void CCamera::Update(_float fTimeDelta)
{

}

void CCamera::Late_Update(_float fTimeDelta)
{
	Update_PipeLine();
}

HRESULT CCamera::Render()
{
	return S_OK;
}


void CCamera::Set_Fovy(_float fFovy)
{
	m_fFovy = fFovy;
	XMStoreFloat4x4(&m_ProjMatrix, XMMatrixPerspectiveFovLH(m_fFovy, m_fAspect, m_fNear, m_fFar));
}

void CCamera::Update_PipeLine()
{
	m_pGameInstance->Set_Transform(D3DTS::VIEW, XMMatrixInverse(nullptr, XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr())));
	m_pGameInstance->Set_Transform(D3DTS::PROJ, XMLoadFloat4x4(&m_ProjMatrix));
}


void CCamera::Free()
{
	__super::Free();
}
