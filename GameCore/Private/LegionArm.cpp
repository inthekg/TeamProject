#include "LegionArm.h"
#include "Player.h"
#include "MasterModel.h"
#include "PartsModel.h"
#include "RenderStruct.h"
#include "GameInstance.h"
#include "BodyPart.h"
#include "AnimDriver.h"
#include "SpringArm.h"
#include "Transform.h"
#include "AimComponent.h"
#include "AIObject.h"
#include "CrosshairHUDData.h"

CLegionArm::CLegionArm(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	:CBodyPart{ pDevice, pContext }
{
}

HRESULT CLegionArm::Initialize(void* pArg)
{
	if (pArg == nullptr)
	{
		MSG_BOX("CLegionArm : Arg is nullptr");
		return E_FAIL;
	}

	LEGIONARM_DESC* pDesc = (LEGIONARM_DESC*)pArg;
	m_pOwner = pDesc->pOwner;

	if(m_pOwner == nullptr)
	{
		MSG_BOX("CLegionArm : Owner is nullptr");
		return E_FAIL;
	}

	m_pAnimDriver = m_pOwner->Get_AnimDriverComponent();
	m_pSpringArm = m_pOwner->Get_SpringArmComponent();
	m_pStatusCom = m_pOwner->Get_StatusComponent();
	m_pOwnerTransform = m_pOwner->Get_Transform();
	m_pAimCom = m_pOwner->Get_AimComponent();

	if (m_pAnimDriver == nullptr || m_pSpringArm == nullptr ||
		m_pStatusCom == nullptr || m_pOwnerTransform == nullptr || m_pAimCom == nullptr)
	{
		MSG_BOX("CLegionArm : Owner Component is nullptr");
		return E_FAIL;
	}

	if(FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CLegionArm::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void CLegionArm::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);	
}

void CLegionArm::Compute_RayToTarget(CAIObject* pTarget, _fvector vStartOffset, _vector& vStart, _vector& vDir) const
{
	vStart = m_pOwnerTransform->Get_State(STATE::POSITION) + vStartOffset;
	_vector vTargetPos = {};

	if (pTarget->Get_LockOnWorldPosition(vTargetPos) == false)
		vTargetPos = pTarget->Get_Transform()->Get_State(STATE::POSITION) + vStartOffset;

	vDir = XMVector3Normalize(vTargetPos - vStart);
}

// 카메라 시선에서 카메라-플레이어의 구간을 제외하여, 사이의 물체를 제외함
void CLegionArm::Compute_CameraRay(_vector& vStart, _vector& vDir) const
{
	CameraPose CamPose = m_pSpringArm->Get_FinalCameraPose();
	_vector vCamPos = XMLoadFloat4(&CamPose.vPosition);

	vDir = XMVector3Rotate(XMVectorSet(0.f, 0.f, 1.f, 0.f), XMLoadFloat4(&CamPose.vQuat));

	_vector vOwnerPos = m_pOwnerTransform->Get_State(STATE::POSITION);
	// 시선 벡터에 사이 벡터를 투영한 거리 
	_float fToOwnerDist = max(XMVectorGetX(XMVector3Dot(vOwnerPos - vCamPos, vDir)), 0.f);

	vStart = vCamPos + vDir * fToOwnerDist;
}

void CLegionArm::Set_CrosshairVisible(_bool bVisible)
{
	if (m_bCrosshairVisible == bVisible)
		return;

	m_bCrosshairVisible = bVisible;

	CROSSHAIR_VISIBILITY_DESC Desc{};
	Desc.bVisible = bVisible;
	m_pGameInstance->Broadcast(EVENT_TYPE_UI::CROSSHAIR_VISIBILITY_CHANGED, &Desc);
}

void CLegionArm::Free()
{
	__super::Free();
}
