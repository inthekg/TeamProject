#include "CutScene_Camera.h"

CCutScene_Camera::CCutScene_Camera(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CCamera{ pDevice, pContext }
{
}

CCutScene_Camera::CCutScene_Camera(const CCutScene_Camera& Prototype)
	: CCamera{ Prototype }
{
}

HRESULT CCutScene_Camera::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CCutScene_Camera::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CCutScene_Camera::Priority_Update(_float fTimeDelta)
{
	// 가상 컷신 카메라는 입력을 직접 처리하지 않는다.
}

void CCutScene_Camera::Update(_float fTimeDelta)
{
	// 컷신 매니저가 현재 프레임의 Pose를 계산해 Set_CameraPose로 전달한다.
}

void CCutScene_Camera::Late_Update(_float fTimeDelta)
{
	// CCamera::Late_Update를 호출하면 가상 카메라가 Pipeline을 덮어쓰므로 호출하지 않는다.
}

HRESULT CCutScene_Camera::Render()
{
	return S_OK;
}

void CCutScene_Camera::Set_CameraPose(_fvector vPosition, _fvector vQuaternion, _float fFovy)
{
	if (nullptr == m_pTransformCom)
		return;

	// 컷신 카메라 위치는 월드 절대 좌표로 적용한다.
	m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(vPosition, 1.f));

	// Euler 회전 튐을 방지하기 위해 정규화한 Quaternion을 사용한다.
	m_pTransformCom->Set_Quat(XMQuaternionNormalize(vQuaternion));

	// CCamera는 세로 FOV Radian을 사용한다.
	if (0.f < fFovy)
		Set_Fovy(fFovy);
}

void CCutScene_Camera::Update_PipeLine()
{
	// 최종 View/Projection 출력은 CCamera_Manager의 m_pMainCamera만 담당한다.
}

CCutScene_Camera* CCutScene_Camera::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CCutScene_Camera* pInstance = new CCutScene_Camera(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Create : CCutScene_Camera");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CCutScene_Camera::Clone(void* pArg)
{
	CCutScene_Camera* pInstance = new CCutScene_Camera(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone : CCutScene_Camera");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CCutScene_Camera::Free()
{
	__super::Free();
}