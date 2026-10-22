#include "Animator.h"
#include "Model.h"
#include "Animation.h"
#include "GameInstance.h"
#include "Parser.h"
#include "Channel.h"

void CAnimator::Update_RootMotion(_float fPrevLocalTime, _float fCurLocalTime, _bool bStepChanged)
{
	Clear_RootMotionDelta();

	const ANIM_CLIP_SETTING* pClip = Get_CurClip();
	if (pClip == nullptr)
		return;

	if (!m_bUseRootMotion)
		return;

	if (bStepChanged)
	{
		// 이전 클립과 새 클립을 직접 비교하면 안 되므로,
		// 새 step 첫 프레임은 루트 모션 기준만 새 클립으로 리셋한다.
		return;
	}

	if (m_iRootBoneIndex < 0)
		return;

	const _float fDuration = Calc_ClipDuration(*pClip);
	if (fDuration <= 0.f)
		return;

	if (m_bLoop && fCurLocalTime < fPrevLocalTime)
	{
		POSE PrevPose;
		POSE EndPose;
		POSE StartPose;
		POSE CurPose;

		if (!Sample_Pose(*pClip, fPrevLocalTime, PrevPose))
			return;

		if (!Sample_Pose(*pClip, fDuration, EndPose))
			return;

		if (!Sample_Pose(*pClip, 0.f, StartPose))
			return;

		if (!Sample_Pose(*pClip, fCurLocalTime, CurPose))
			return;

		_matrix matPrev = Compute_BoneMatrix(PrevPose, m_iRootBoneIndex);
		_matrix matEnd = Compute_BoneMatrix(EndPose, m_iRootBoneIndex);
		_matrix matStart = Compute_BoneMatrix(StartPose, m_iRootBoneIndex);
		_matrix matCur = Compute_BoneMatrix(CurPose, m_iRootBoneIndex);

		// 이동: prev -> end, start -> cur
		_vector vDeltaA = matEnd.r[3] - matPrev.r[3];
		_vector vDeltaB = matCur.r[3] - matStart.r[3];

		_vector vDelta = vDeltaA + vDeltaB;
		XMStoreFloat3(&m_RootMotionDelta.vTranslation, vDelta);

		// 회전: prev -> end, start -> cur
		_vector qDeltaA = Calc_RootRotation(matPrev, matEnd);
		_vector qDeltaB = Calc_RootRotation(matStart, matCur);

		_vector qDelta = XMQuaternionNormalize(XMQuaternionMultiply(qDeltaA, qDeltaB));
		XMStoreFloat4(&m_RootMotionDelta.vRotation, qDelta);

		m_RootMotionDelta.vScale = { 1.f, 1.f, 1.f };
	}
	else
	{
		POSE PrevPose;
		POSE CurPose;

		if (!Sample_Pose(*pClip, fPrevLocalTime, PrevPose))
			return;

		if (!Sample_Pose(*pClip, fCurLocalTime, CurPose))
			return;

		_matrix matPrev = Compute_BoneMatrix(PrevPose, m_iRootBoneIndex);
		_matrix matCur = Compute_BoneMatrix(CurPose, m_iRootBoneIndex);

		// 이동
		_vector vDeltaPos = matCur.r[3] - matPrev.r[3];
		XMStoreFloat3(&m_RootMotionDelta.vTranslation, vDeltaPos);

		// 회전
		_vector qDelta = Calc_RootRotation(matPrev, matCur);
		XMStoreFloat4(&m_RootMotionDelta.vRotation, qDelta);

		m_RootMotionDelta.vScale = { 1.f, 1.f, 1.f };
	}
}

void CAnimator::Strip_RootMotion(POSE& Pose, const ANIM_CLIP_SETTING& Clip) const
{
	if (m_iRootBoneIndex < 0)
		return;

	if (m_iRootBoneIndex >= Pose.size())
		return;

	_matrix matCurModel = Compute_BoneMatrix(Pose, m_iRootBoneIndex);
	_matrix matParentModel = Compute_ParentMatrix(Pose, m_iRootBoneIndex);

	// 모든 in-place 포즈는 클립별 시작 root가 아니라 bind/base pose root를 공통 기준으로 고정한다.
    // Run_Stop처럼 시작 root가 이미 앞으로 밀린 클립이 전환 순간 튀는 것을 막기 위함.
	_matrix matBaseRootModel = Compute_BoneMatrix(m_BasePose, m_iRootBoneIndex);
	_vector vAnchorModelPos = matBaseRootModel.r[3];

	_matrix matDesiredModel = matCurModel;

	const _float fDesiredY = m_bRootYAnchored ? XMVectorGetY(vAnchorModelPos) : XMVectorGetY(matCurModel.r[3]);

	matDesiredModel.r[3] = XMVectorSet(XMVectorGetX(vAnchorModelPos), fDesiredY, XMVectorGetZ(vAnchorModelPos), 1.f);

	_matrix matInvParent = XMMatrixInverse(nullptr, matParentModel);
	_matrix matDesiredLocal = matDesiredModel * matInvParent;

	_vector vScale = XMVectorSet(1.f, 1.f, 1.f, 0.f);
	_vector vRotation = XMQuaternionIdentity();
	_vector vTranslation = XMVectorZero();

	if (!XMMatrixDecompose(&vScale, &vRotation, &vTranslation, matDesiredLocal))
		return;

	XMStoreFloat4(&Pose[m_iRootBoneIndex].vScale, vScale);
	XMStoreFloat4(&Pose[m_iRootBoneIndex].vRot, XMQuaternionNormalize(vRotation));
	XMStoreFloat4(&Pose[m_iRootBoneIndex].vTranslation, vTranslation);
}

_matrix CAnimator::Compute_BoneMatrix(const POSE& Pose, _int iBoneIndex) const
{
	if (iBoneIndex < 0 || iBoneIndex >= Pose.size() || iBoneIndex >= m_Bones.size())
		return XMMatrixIdentity();

	vector<_int> Chain;

	_int iCurIndex = iBoneIndex;

	while (iCurIndex >= 0)
	{
		if (iCurIndex >= Pose.size())
			return XMMatrixIdentity();

		if (iCurIndex >= m_Bones.size())
			return XMMatrixIdentity();

		Chain.push_back(iCurIndex);
		iCurIndex = m_Bones[iCurIndex].iParentIndex;
	}

	_matrix matCombined = XMLoadFloat4x4(&m_PreTransformMatrix);

	for (_int i = Chain.size() - 1; i >= 0; --i)
	{
		const TransformData& BonePose = Pose[Chain[i]];

		_vector vScale = XMLoadFloat4(&BonePose.vScale);
		_vector vRot = XMLoadFloat4(&BonePose.vRot);
		_vector vTrans = XMLoadFloat4(&BonePose.vTranslation);

		_matrix matLocal = XMMatrixScalingFromVector(vScale) * XMMatrixRotationQuaternion(vRot) * XMMatrixTranslationFromVector(vTrans);

		matCombined = matLocal * matCombined;
	}

	return matCombined;
}

_matrix CAnimator::Compute_ParentMatrix(const POSE& Pose, _int iBoneIndex) const
{
	if (iBoneIndex < 0)
		return XMMatrixIdentity();

	if (iBoneIndex >= m_Bones.size())
		return XMMatrixIdentity();

	const _int iParentIndex = m_Bones[iBoneIndex].iParentIndex;

	if (iParentIndex < 0)
		return XMLoadFloat4x4(&m_PreTransformMatrix);

	return Compute_BoneMatrix(Pose, iParentIndex);
}

_vector CAnimator::Calc_RootRotation(_matrix matPrev, _matrix matCur) const
{
	_vector vPrevScale = {};
	_vector vPrevRot = {};
	_vector vPrevTrans = {};

	_vector vCurScale = {};
	_vector vCurRot = {};
	_vector vCurTrans = {};

	if (!XMMatrixDecompose(&vPrevScale, &vPrevRot, &vPrevTrans, matPrev))
		return XMQuaternionIdentity();

	if (!XMMatrixDecompose(&vCurScale, &vCurRot, &vCurTrans, matCur))
		return XMQuaternionIdentity();

	vPrevRot = XMQuaternionNormalize(vPrevRot);
	vCurRot = XMQuaternionNormalize(vCurRot);

	_vector qDelta = XMQuaternionMultiply(XMQuaternionInverse(vPrevRot), vCurRot);

	return XMQuaternionNormalize(qDelta);
}

_bool CAnimator::Try_GetRootModelY(const POSE& Pose, _float& OutY) const
{
	OutY = 0.f;

	if (m_iRootBoneIndex < 0)
		return false;

	if (m_iRootBoneIndex >= Pose.size())
		return false;

	_matrix matRootModel = Compute_BoneMatrix(Pose, m_iRootBoneIndex);

	OutY = XMVectorGetY(matRootModel.r[3]);

	return true;
}

void CAnimator::Set_RootModelY(POSE& Pose, _float fModelY) const
{
	if (m_iRootBoneIndex < 0)
		return;

	if (m_iRootBoneIndex >= Pose.size())
		return;

	_matrix matCurModel = Compute_BoneMatrix(Pose, m_iRootBoneIndex);
	_matrix matParentModel = Compute_ParentMatrix(Pose, m_iRootBoneIndex);

	_vector vCurModelPos = matCurModel.r[3];

	_matrix matDesiredModel = matCurModel;
	matDesiredModel.r[3] = XMVectorSet(XMVectorGetX(vCurModelPos), fModelY, XMVectorGetZ(vCurModelPos), 1.f);

	_matrix matInvParent = XMMatrixInverse(nullptr, matParentModel);
	_matrix matDesiredLocal = matDesiredModel * matInvParent;

	_vector vScale = XMVectorSet(1.f, 1.f, 1.f, 0.f);
	_vector vRotation = XMQuaternionIdentity();
	_vector vTranslation = XMVectorZero();

	if (!XMMatrixDecompose(&vScale, &vRotation, &vTranslation, matDesiredLocal))
		return;

	XMStoreFloat4(&Pose[m_iRootBoneIndex].vScale, vScale);
	XMStoreFloat4(&Pose[m_iRootBoneIndex].vRot, XMQuaternionNormalize(vRotation));
	XMStoreFloat4(&Pose[m_iRootBoneIndex].vTranslation, vTranslation);
}

void CAnimator::Clear_RootMotionDelta()
{
	m_RootMotionDelta.vTranslation = {};
	m_RootMotionDelta.vRotation = { 0.f, 0.f, 0.f, 1.f };
	m_RootMotionDelta.vScale = { 1.f, 1.f, 1.f };
}
