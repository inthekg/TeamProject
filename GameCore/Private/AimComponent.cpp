#include "AimComponent.h"
#include "SpringArm.h"
#include "Transform.h"
#include "Engine_Utility.h"
	
CAimComponent::CAimComponent()
	: CComponent{ nullptr, nullptr }
{
}

HRESULT CAimComponent::Initialize(void* pArg)
{
	const AIM_COMPONENT_DESC* pDesc = static_cast<const AIM_COMPONENT_DESC*>(pArg);
	if (pDesc == nullptr || pDesc->pModel == nullptr || pDesc->pOwnerTransform == nullptr || pDesc->pSpringArm == nullptr)
	{
		MSG_BOX("CAimComponent : Invalid Desc");
		return E_FAIL;
	}
	m_pModel = pDesc->pModel;
	m_pOwnerTransform = pDesc->pOwnerTransform;
	m_pSpringArm = pDesc->pSpringArm;

	return S_OK;
}

void CAimComponent::Clear()
{
	m_iRequestedProfile = INVALID_PROFILE;

	for (auto& pChain : m_AimChains)
		pChain->Reset();
}

_int CAimComponent::Register_AimProfile(const CAimBoneChain::AIM_PROFILE& Profile)
{
	CAimBoneChain* pChain = CAimBoneChain::Create(m_pModel, m_pOwnerTransform, Profile);
	if (pChain == nullptr)
		return INVALID_PROFILE;

	m_AimChains.push_back(pChain);
	//AimChain 삭제하지 않는다는 전제로 현재 인덱스 반환
	return static_cast<_int>(m_AimChains.size()) - 1;
}

void CAimComponent::Request_AimPoseToCamera(_int iProfile)
{
	if (false == Is_ValidProfile(iProfile))
		return;

	_vector vStart = {}, vDir = {};
	Compute_CameraRay(vStart, vDir);

	XMStoreFloat3(&m_vRequestedDir, vDir);
	m_iRequestedProfile = iProfile;
}

void CAimComponent::Request_AimPoseToPoint(_int iProfile, _fvector vWorldPos)
{
	if (false == Is_ValidProfile(iProfile))
		return;

	_vector vDir = vWorldPos - m_pOwnerTransform->Get_State(STATE::POSITION);

	//점이 발밑과 겹치면 방향을 바꾸지 않고 이전 방향 유지
	if (IsEqual(XMVectorGetX(XMVector3LengthSq(vDir)) ,0.f) == false)
		XMStoreFloat3(&m_vRequestedDir, XMVector3Normalize(vDir));

	m_iRequestedProfile = iProfile;
}

//카메라~플레이어 구간은 제외해 사이 물체 제외
void CAimComponent::Compute_CameraRay(_vector& vStart, _vector& vDir) const
{
	CameraPose CamPose = m_pSpringArm->Get_FinalCameraPose();
	_vector vCamPos = XMLoadFloat4(&CamPose.vPosition);

	vDir = XMVector3Rotate(XMVectorSet(0.f, 0.f, 1.f, 0.f), XMLoadFloat4(&CamPose.vQuat));

	_vector vOwnerPos = m_pOwnerTransform->Get_State(STATE::POSITION);
	_float fToOwnerDist = max(XMVectorGetX(XMVector3Dot(vOwnerPos - vCamPos, vDir)), 0.f);

	vStart = vCamPos + vDir * fToOwnerDist;
}

_bool CAimComponent::Update_AimPose(_float fTimeDelta)
{
	const _int iRequested = m_iRequestedProfile;
	m_iRequestedProfile = INVALID_PROFILE;

	_bool bNeedApply = false;

	for (_int i = 0; i < static_cast<_int>(m_AimChains.size()); ++i)
	{
		CAimBoneChain* pChain = m_AimChains[i];
		_bool bRequested = (i == iRequested);

		//요청도 없고 완전 멈춘 ChainBone이면 무시
		if (false == bRequested && false == pChain->Is_Affecting())
			continue;

		pChain->Set_Active(bRequested);

		if (bRequested)
			pChain->Set_TargetDirection(XMLoadFloat3(&m_vRequestedDir));

		pChain->Update(fTimeDelta);
		bNeedApply |= pChain->Is_Affecting();
	}
	return bNeedApply;
}

void CAimComponent::Apply_AimPose(POSE& Pose)
{
	for (auto& pChain : m_AimChains)
	{
		if (pChain->Is_Affecting())
			pChain->Apply(Pose);
	}
}

CAimComponent* CAimComponent::Create(void* pArg)
{
	CAimComponent* pInstance = new CAimComponent();
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Created : CAimComponent");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CAimComponent::Free()
{
	__super::Free();

	for (auto& pChain : m_AimChains)
		Safe_Release(pChain);
	m_AimChains.clear();
}