#include "CutScene_Actor.h"

#include "Animator.h"
#include "GameInstance.h"
#include "Model.h"

static _matrix CutScene_ActorBlendWorldMatrix(_fmatrix StartMatrix, _fmatrix TargetMatrix, _float fWeight)
{
	_vector vStartScale = {};
	_vector vStartRotation = {};
	_vector vStartTranslation = {};
	_vector vTargetScale = {};
	_vector vTargetRotation = {};
	_vector vTargetTranslation = {};

	if (false == XMMatrixDecompose(&vStartScale, &vStartRotation, &vStartTranslation, StartMatrix) || false == XMMatrixDecompose(&vTargetScale, &vTargetRotation, &vTargetTranslation, TargetMatrix))
		return TargetMatrix;

	const _float fClampedWeight = max(0.f, min(fWeight, 1.f));
	const _float fSmoothedWeight = fClampedWeight * fClampedWeight * (3.f - 2.f * fClampedWeight);
	vStartRotation = XMQuaternionNormalize(vStartRotation);
	vTargetRotation = XMQuaternionNormalize(vTargetRotation);

	// 같은 회전을 나타내는 Quaternion의 부호가 반대여도 짧은 방향으로 회전하도록 맞춘다.
	if (XMVectorGetX(XMVector4Dot(vStartRotation, vTargetRotation)) < 0.f)
		vTargetRotation = XMVectorNegate(vTargetRotation);

	const _vector vScale = XMVectorLerp(vStartScale, vTargetScale, fSmoothedWeight);
	const _vector vRotation = XMQuaternionNormalize(XMQuaternionSlerp(vStartRotation, vTargetRotation, fSmoothedWeight));
	const _vector vTranslation = XMVectorLerp(vStartTranslation, vTargetTranslation, fSmoothedWeight);
	return XMMatrixScalingFromVector(vScale) * XMMatrixRotationQuaternion(vRotation) * XMMatrixTranslationFromVector(vTranslation);
}

CCutScene_Actor::CCutScene_Actor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CMapGameplayObject{ pDevice, pContext }
{
	XMStoreFloat4x4(&m_RootMotionMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_LocalWorldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_AttachmentBlendStartWorld, XMMatrixIdentity());
	XMStoreFloat4x4(&m_ChildAttachmentReferenceLocal, XMMatrixIdentity());
}

CCutScene_Actor::CCutScene_Actor(const CCutScene_Actor& Prototype)
	: CMapGameplayObject{ Prototype }
{
	XMStoreFloat4x4(&m_RootMotionMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_LocalWorldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&m_AttachmentBlendStartWorld, XMMatrixIdentity());
	XMStoreFloat4x4(&m_ChildAttachmentReferenceLocal, XMMatrixIdentity());
}

HRESULT CCutScene_Actor::Initialize_Prototype()
{
	return __super::Initialize_Prototype();
}

HRESULT CCutScene_Actor::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	CUTSCENE_ACTOR_DESC* pDesc = static_cast<CUTSCENE_ACTOR_DESC*>(pArg);

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	Set_MapTransform(pDesc->InitialTransform.vScale, pDesc->InitialTransform.vRotationDegree, pDesc->InitialTransform.vPosition);

	return S_OK;
}

void CCutScene_Actor::Update(_float fTimeDelta)
{
	// CMapGameplayObject::Update는 Animator를 자동 진행하므로 컷신 액터는 CGameObject 경로만 호출한다.
	CGameObject::Update(fTimeDelta);
	m_TriggeredNotifies.clear();

	if (m_bEvaluationQueued)
		Apply_QueuedEvaluation();
}

void CCutScene_Actor::Late_Update(_float fTimeDelta)
{
	// 런타임은 Object Update 뒤에 Level의 컷신 시간이 갱신되므로, 같은 프레임에 들어온 평가 Queue를 렌더 등록 전에 적용한다.
	if (m_bEvaluationQueued)
		Apply_QueuedEvaluation();

	if (Is_InActive() || false == m_bCutSceneVisible)
	{
		CGameObject::Late_Update(fTimeDelta);
		return;
	}

	// 현재 Pose의 자식 Bone 행렬을 먼저 만든 뒤 부모 Bone과 정렬한다.
	__super::Late_Update(fTimeDelta);
	Apply_AttachmentTransform();
	Dispatch_EffectNotifies();
}

void CCutScene_Actor::Queue_Evaluation(const CUTSCENE_TRANSFORM_VALUE& Transform, const vector<CUTSCENE_ANIMATION_SECTION>& Sections, _float fCutSceneTime, _bool bSeek)
{
	m_QueuedTransform = Transform;
	m_pQueuedSections = &Sections;
	m_fQueuedCutSceneTime = fCutSceneTime;
	m_bSeekQueued = bSeek;
	m_bEvaluationQueued = true;
}

void CCutScene_Actor::Reset_CutScene()
{
	// 사전 복제된 액터를 다음 재생에도 재사용할 수 있도록 재생 상태만 초기화하고 객체는 제거하지 않는다.
	m_pQueuedSections = nullptr;
	m_bEvaluationQueued = false;
	m_bSeekQueued = false;
	m_bCutSceneAttachmentActive = false;
	XMStoreFloat4x4(&m_AttachmentBlendStartWorld, XMMatrixIdentity());
	m_fAttachmentBlendWeight = 1.f;
	m_bAttachmentBlendActive = false;
	m_bInheritParentRotation = true;
	m_fParentRotationBlendWeight = 1.f;
	m_vLockedAttachmentRotation = _float4(0.f, 0.f, 0.f, 1.f);
	XMStoreFloat4x4(&m_ChildAttachmentReferenceLocal, XMMatrixIdentity());
	m_bChildAttachmentReferenceValid = false;
	m_fQueuedCutSceneTime = 0.f;
	m_fLastEvaluatedCutSceneTime = 0.f;
	m_bCutSceneVisible = false;
	m_fDissolveAmount = 0.f;
	m_vDissolveColor = _float4(0.05f, 0.45f, 1.f, 6.f);
	m_TriggeredNotifies.clear();
	Reset_AnimationState();
}

void CCutScene_Actor::Set_CutSceneAttachment(CCutScene_Actor* pParentActor, const string& strParentBoneName, const string& strChildBoneName, _float fAttachmentBlendWeight, _bool bInheritParentRotation, _float fParentRotationBlendWeight)
{
	const _bool bWasAttached = m_bCutSceneAttachmentActive && nullptr != m_pParentActor && false == m_strAttachBoneName.empty();
	const _bool bAttachmentTargetChanged = pParentActor != m_pParentActor || strParentBoneName != m_strAttachBoneName || strChildBoneName != m_strChildAttachBoneName;

	if (bAttachmentTargetChanged)
	{
		if (bWasAttached && nullptr != m_pTransformCom && fAttachmentBlendWeight < 1.f)
		{
			// Bone이 바뀌는 첫 프레임의 World를 보존해 새 Bone 및 자식 Pivot 위치로 순간 이동하지 않게 한다.
			m_AttachmentBlendStartWorld = *m_pTransformCom->Get_WorldMatrixPtr();
			m_bAttachmentBlendActive = true;
		}
		else
			m_bAttachmentBlendActive = false;

		// 부모 또는 부착 Bone이 바뀌면 새 부착 순간의 자식 Bone 기준 행렬을 다시 저장한다.
		XMStoreFloat4x4(&m_ChildAttachmentReferenceLocal, XMMatrixIdentity());
		m_bChildAttachmentReferenceValid = false;
	}

	if (m_bInheritParentRotation && false == bInheritParentRotation && nullptr != m_pTransformCom)
	{
		_vector vScale = {};
		_vector vRotation = {};
		_vector vTranslation = {};

		// 회전 상속을 끄는 프레임에는 직전 Bone 부착 결과의 World 회전을 잠근다.
		if (XMMatrixDecompose(&vScale, &vRotation, &vTranslation, XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr())))
			XMStoreFloat4(&m_vLockedAttachmentRotation, XMQuaternionNormalize(vRotation));
	}

	m_pParentActor = pParentActor;
	m_strAttachBoneName = strParentBoneName;
	m_strChildAttachBoneName = strChildBoneName;
	m_fAttachmentBlendWeight = max(0.f, min(fAttachmentBlendWeight, 1.f));
	m_bInheritParentRotation = bInheritParentRotation;
	m_fParentRotationBlendWeight = bInheritParentRotation ? max(0.f, min(fParentRotationBlendWeight, 1.f)) : 0.f;
	m_bCutSceneAttachmentActive = nullptr != m_pParentActor && false == m_strAttachBoneName.empty();
}

void CCutScene_Actor::Set_CutSceneAttachmentActive(_bool bActive)
{
	m_bCutSceneAttachmentActive = bActive && nullptr != m_pParentActor && false == m_strAttachBoneName.empty();

	if (m_bCutSceneAttachmentActive || nullptr == m_pTransformCom)
		return;

	m_bAttachmentBlendActive = false;
	m_fAttachmentBlendWeight = 1.f;
	m_bInheritParentRotation = true;
	m_fParentRotationBlendWeight = 1.f;
	m_vLockedAttachmentRotation = _float4(0.f, 0.f, 0.f, 1.f);
	XMStoreFloat4x4(&m_ChildAttachmentReferenceLocal, XMMatrixIdentity());
	m_bChildAttachmentReferenceValid = false;

	// 분리되는 프레임에는 마지막 Bone 행렬을 제거하고 Track의 월드 Transform으로 즉시 복원한다.
	const _matrix LocalWorldMatrix = XMLoadFloat4x4(&m_LocalWorldMatrix);
	m_pTransformCom->Set_State(STATE::RIGHT, LocalWorldMatrix.r[0]);
	m_pTransformCom->Set_State(STATE::UP, LocalWorldMatrix.r[1]);
	m_pTransformCom->Set_State(STATE::LOOK, LocalWorldMatrix.r[2]);
	m_pTransformCom->Set_State(STATE::POSITION, LocalWorldMatrix.r[3]);
}

void CCutScene_Actor::Clear_CutSceneAttachment()
{
	m_pParentActor = nullptr;
	m_strAttachBoneName.clear();
	m_strChildAttachBoneName.clear();
	m_bCutSceneAttachmentActive = false;
	m_bAttachmentBlendActive = false;
	m_fAttachmentBlendWeight = 1.f;
	m_bInheritParentRotation = true;
	m_fParentRotationBlendWeight = 1.f;
	m_vLockedAttachmentRotation = _float4(0.f, 0.f, 0.f, 1.f);
	XMStoreFloat4x4(&m_ChildAttachmentReferenceLocal, XMMatrixIdentity());
	m_bChildAttachmentReferenceValid = false;
}

_bool CCutScene_Actor::Get_CutSceneBoneWorld(const string& strBoneName, _float4x4* pOutWorldMatrix) const
{
	if (nullptr == pOutWorldMatrix || nullptr == m_pTransformCom || strBoneName.empty())
		return false;

	// Unreal Attach Track에 Socket이 없고 Component만 지정된 경우에는 Bone이 아니라 부모 액터의 World를 사용한다.
	if ("ActorRoot" == strBoneName)
	{
		*pOutWorldMatrix = *m_pTransformCom->Get_WorldMatrixPtr();
		return true;
	}

	if (nullptr == m_pModelCom)
		return false;

	const _float4x4* pBoneMatrix = m_pModelCom->Get_Bone_CombinedMatPtr(strBoneName.c_str());

	if (nullptr == pBoneMatrix)
		return false;

	_matrix AttachmentBoneMatrix = XMLoadFloat4x4(pBoneMatrix);
	const _matrix PreTransformMatrix = XMLoadFloat4x4(&m_pModelCom->Get_PreTransformMatrix());
	const _float fPreScaleX = XMVectorGetX(XMVector3Length(PreTransformMatrix.r[0]));
	const _float fPreScaleY = XMVectorGetX(XMVector3Length(PreTransformMatrix.r[1]));
	const _float fPreScaleZ = XMVectorGetX(XMVector3Length(PreTransformMatrix.r[2]));
	const _float fUniformScaleEpsilon = 0.0001f;

	// 부모 모델의 균일 Import Scale이 자식 모델에 다시 적용되는 이중 축소를 제거한다.
	if (fPreScaleX > fUniformScaleEpsilon && fabsf(fPreScaleX - fPreScaleY) <= fUniformScaleEpsilon && fabsf(fPreScaleX - fPreScaleZ) <= fUniformScaleEpsilon)
	{
		const _float fInverseImportScale = 1.f / fPreScaleX;
		AttachmentBoneMatrix.r[0] = XMVectorScale(AttachmentBoneMatrix.r[0], fInverseImportScale);
		AttachmentBoneMatrix.r[1] = XMVectorScale(AttachmentBoneMatrix.r[1], fInverseImportScale);
		AttachmentBoneMatrix.r[2] = XMVectorScale(AttachmentBoneMatrix.r[2], fInverseImportScale);
	}

	// 부모 액터의 월드 Transform은 그대로 곱해 컷신 Track Scale을 유지한다.
	XMStoreFloat4x4(pOutWorldMatrix, AttachmentBoneMatrix * XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr()));
	return true;
}

void CCutScene_Actor::Apply_QueuedEvaluation()
{
	m_bEvaluationQueued = false;

	if (nullptr == m_pQueuedSections)
	{
		Reset_AnimationState();
		Apply_FinalTransform();
		return;
	}

	const vector<CUTSCENE_ANIMATION_SECTION>& Sections = *m_pQueuedSections;

	if (m_bSeekQueued || m_fQueuedCutSceneTime < m_fLastEvaluatedCutSceneTime)
	{
		Rebuild_AnimationTimeline(Sections, m_fQueuedCutSceneTime);
	}
	else
	{
		const _int iTargetSectionIndex = Find_LatestStartedSectionIndex(Sections, m_fQueuedCutSceneTime);
		_int iActiveSectionIndex = Find_ActiveSectionIndex(Sections);

		if (iTargetSectionIndex < 0)
		{
			Reset_AnimationState();
		}
		else if (iActiveSectionIndex > iTargetSectionIndex)
		{
			Rebuild_AnimationTimeline(Sections, m_fQueuedCutSceneTime);
		}
		else
		{
			if (iActiveSectionIndex < 0)
			{
				Reset_AnimationState();
				iActiveSectionIndex = 0;
				Begin_AnimationSection(Sections[iActiveSectionIndex]);
				Advance_AnimationSection(Sections[iActiveSectionIndex], Sections[iActiveSectionIndex].fStartTime, min(m_fQueuedCutSceneTime, Sections[iActiveSectionIndex].fEndTime), true);
			}
			else
			{
				Advance_AnimationSection(Sections[iActiveSectionIndex], m_fLastEvaluatedCutSceneTime, min(m_fQueuedCutSceneTime, Sections[iActiveSectionIndex].fEndTime), true);
			}

			for (_int i = iActiveSectionIndex + 1; i <= iTargetSectionIndex; ++i)
			{
				Begin_AnimationSection(Sections[i]);
				Advance_AnimationSection(Sections[i], Sections[i].fStartTime, min(m_fQueuedCutSceneTime, Sections[i].fEndTime), true);
			}
		}
	}

	m_fLastEvaluatedCutSceneTime = m_fQueuedCutSceneTime;
	Apply_FinalTransform();
}

_int CCutScene_Actor::Find_LatestStartedSectionIndex(const vector<CUTSCENE_ANIMATION_SECTION>& Sections, _float fCutSceneTime) const
{
	_int iResult = -1;

	for (_int i = 0; i < static_cast<_int>(Sections.size()); ++i)
	{
		if (Sections[i].fStartTime > fCutSceneTime)
			break;

		iResult = i;
	}

	return iResult;
}

_int CCutScene_Actor::Find_ActiveSectionIndex(const vector<CUTSCENE_ANIMATION_SECTION>& Sections) const
{
	if (false == m_bHasActiveSection)
		return -1;

	for (_int i = 0; i < static_cast<_int>(Sections.size()); ++i)
	{
		if (Sections[i].iSectionID == m_iActiveSectionID)
			return i;
	}

	return -1;
}

void CCutScene_Actor::Begin_AnimationSection(const CUTSCENE_ANIMATION_SECTION& Section)
{
	m_ActiveSection = Section;
	m_bHasActiveSection = true;
	m_iActiveSectionID = Section.iSectionID;

	// 서로 다른 독립 소품 Clip 사이에는 이전 Clip의 Root Motion 누적값을 넘기지 않는다.
	if (Section.bResetRootMotion)
		XMStoreFloat4x4(&m_RootMotionMatrix, XMMatrixIdentity());

	if (nullptr == m_pAnimator || Section.strClipName.empty())
		return;

	const _float fClipStartTime = max(0.f, Section.fClipStartTime);
	const _bool bBlendFromPreviousSection = Section.fBlendInTime > 0.f && nullptr != m_pAnimator->Get_CurClip();
	HRESULT hResult = E_FAIL;

	if (bBlendFromPreviousSection)
	{
		// 이전 Section의 마지막 Pose를 유지한 채 새 Clip의 실제 시작 Pose로 보간한다.
		hResult = m_pAnimator->Blend_ToClip(Hash_String(Section.strClipName.c_str()), fClipStartTime * 1000.f, Section.fBlendInTime * 1000.f, Section.bUseRootMotion);
	}
	else
	{
		// 독립 컷신 액터의 이전 Clip과 Pose 블렌드 상태를 함께 정리한다.
		m_pAnimator->Reset_PlaybackState();
		hResult = m_pAnimator->Play_Clip(Hash_String(Section.strClipName.c_str()));
	}

	// 에디터에서 지정한 Clip 이름을 실제 Animator Clip으로 연결한다.
	if (FAILED(hResult))
	{
		m_pAnimator->Reset_PlaybackState();
		const _wstring strMessage = L"[CutScene Actor] Animation clip not found: " + Section.strClipName + L"\n";
		OutputDebugStringW(strMessage.c_str());
		return;
	}

	// 컷신 시간은 매니저가 적용하므로 빌드 구성과 관계없이 Animator 내부 배율은 1로 고정한다.
	m_pAnimator->Set_Loop(Section.bLoop);
	m_pAnimator->Set_PlayRate(1.f);

	if (bBlendFromPreviousSection)
		return;

	m_pAnimator->Set_UseRootMotion(Section.bUseRootMotion);

	const _float fFixedStep = 1.f / 60.f;
	_float fAdvancedTime = {};
	const _bool bUseRootMotion = m_ActiveSection.bUseRootMotion;

	// Clip 시작 지점까지는 pose만 이동하고 컷신 위치에 이전 root motion을 더하지 않는다.
	m_ActiveSection.bUseRootMotion = false;

	while (fAdvancedTime + fFixedStep < fClipStartTime)
	{
		Advance_Animator(fFixedStep, false);
		fAdvancedTime += fFixedStep;
	}

	if (fAdvancedTime < fClipStartTime)
		Advance_Animator(fClipStartTime - fAdvancedTime, false);

	m_ActiveSection.bUseRootMotion = bUseRootMotion;
}

void CCutScene_Actor::Rebuild_AnimationTimeline(const vector<CUTSCENE_ANIMATION_SECTION>& Sections, _float fCutSceneTime)
{
	Reset_AnimationState();

	for (const CUTSCENE_ANIMATION_SECTION& Section : Sections)
	{
		if (Section.fStartTime > fCutSceneTime)
			break;

		Begin_AnimationSection(Section);
		Advance_AnimationSection(Section, Section.fStartTime, min(fCutSceneTime, Section.fEndTime), false);
	}

	m_TriggeredNotifies.clear();
}

void CCutScene_Actor::Advance_AnimationSection(const CUTSCENE_ANIMATION_SECTION& Section, _float fPreviousCutSceneTime, _float fCurrentCutSceneTime, _bool bCollectNotifies)
{
	const _float fPlayRate = max(Section.fPlayRate, 0.f);
	_float fMaximumLocalTime = max(0.f, Section.fEndTime - Section.fStartTime) * fPlayRate;

	if (Section.fClipEndTime > Section.fClipStartTime)
		fMaximumLocalTime = min(fMaximumLocalTime, Section.fClipEndTime - Section.fClipStartTime);

	const _float fPreviousLocalTime = min(max(0.f, fPreviousCutSceneTime - Section.fStartTime) * fPlayRate, fMaximumLocalTime);
	const _float fCurrentLocalTime = min(max(0.f, fCurrentCutSceneTime - Section.fStartTime) * fPlayRate, fMaximumLocalTime);
	const _float fAdvanceTime = max(0.f, fCurrentLocalTime - fPreviousLocalTime);

	Advance_Animator(fAdvanceTime, bCollectNotifies);
}

void CCutScene_Actor::Advance_Animator(_float fTimeDelta, _bool bCollectNotifies)
{
	if (nullptr == m_pAnimator || fTimeDelta < 0.f)
		return;

	// Animator 내부에서 초를 ms로 변환하므로 컷신의 초 단위 Delta를 그대로 전달한다.
	m_pAnimator->Update(fTimeDelta);

	if (bCollectNotifies)
	{
		const vector<const ANIM_NOTIFY*>& TriggeredNotifies = m_pAnimator->Get_TriggeredNotifies();
		m_TriggeredNotifies.insert(m_TriggeredNotifies.end(), TriggeredNotifies.begin(), TriggeredNotifies.end());
	}

	if (m_ActiveSection.bUseRootMotion)
		Accumulate_RootMotion(m_pAnimator->Get_RootMotionDelta());
}

void CCutScene_Actor::Dispatch_EffectNotifies()
{
	for (const ANIM_NOTIFY* pNotify : m_TriggeredNotifies)
	{
		if (nullptr == pNotify || ANIM_NOTIFY_TYPE::EFFECT != pNotify->eType || true == pNotify->strPayload.empty() || nullptr == m_pModelCom)
			continue;

		if (true == pNotify->bUseSlot && false == pNotify->Slot.strBoneName.empty())
		{
			// Bone Pose와 최종 컷신 Transform이 반영된 상태에서 Notify Slot을 따라간다.
			m_pGameInstance->Play_Effect(pNotify->strPayload, m_pModelCom, pNotify->Slot.strBoneName.c_str(), m_pTransformCom->Get_WorldMatrixPtr(), pNotify->Slot.vOffsetPosition);
			continue;
		}

		// Slot이 없는 Effect Notify는 컷신 액터의 현재 World 위치에서 한 번 재생한다.
		_float3 vWorldPosition = {};
		XMStoreFloat3(&vWorldPosition, m_pTransformCom->Get_State(STATE::POSITION));
		m_pGameInstance->Play_Effect(pNotify->strPayload, vWorldPosition);
	}
}

void CCutScene_Actor::Reset_AnimationState()
{
	m_bHasActiveSection = false;
	m_iActiveSectionID = UINT_MAX;
	XMStoreFloat4x4(&m_RootMotionMatrix, XMMatrixIdentity());

	if (nullptr != m_pAnimator)
		m_pAnimator->Reset_PlaybackState();
}

void CCutScene_Actor::Configure_RenderCommand(RenderCommand* pCommand) const
{
	__super::Configure_RenderCommand(pCommand);

	if (nullptr == pCommand)
		return;

	// 컷신 모델 하나에만 적용하며 공유 Material과 다른 객체에는 영향을 주지 않는다.
	pCommand->fDissolveAmount = m_fDissolveAmount;
	pCommand->vDissolveColor = m_vDissolveColor;
}

void CCutScene_Actor::Accumulate_RootMotion(const ANIM_ROOT_MOTION_DELTA& RootMotion)
{
	_vector vRootRotation = XMQuaternionNormalize(XMLoadFloat4(&RootMotion.vRotation));
	_vector vAxis = XMVectorZero();
	_float fAngle = {};

	XMQuaternionToAxisAngle(&vAxis, &fAngle, vRootRotation);

	const _float fYaw = fAngle * XMVectorGetY(vAxis);
	_matrix DeltaMatrix = XMMatrixScaling(RootMotion.vScale.x, RootMotion.vScale.y, RootMotion.vScale.z) * XMMatrixRotationY(fYaw) * XMMatrixTranslation(RootMotion.vTranslation.x, RootMotion.vTranslation.y, RootMotion.vTranslation.z);
	_matrix RootMatrix = XMLoadFloat4x4(&m_RootMotionMatrix);

	// 새 delta를 누적 local root 앞에 곱해 현재 진행 방향을 유지한다.
	XMStoreFloat4x4(&m_RootMotionMatrix, DeltaMatrix * RootMatrix);
}

void CCutScene_Actor::Apply_FinalTransform()
{
	_matrix BaseMatrix = XMMatrixScaling(m_QueuedTransform.vScale.x, m_QueuedTransform.vScale.y, m_QueuedTransform.vScale.z) * XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_QueuedTransform.vRotationDegree.x), XMConvertToRadians(m_QueuedTransform.vRotationDegree.y), XMConvertToRadians(m_QueuedTransform.vRotationDegree.z)) * XMMatrixTranslation(m_QueuedTransform.vPosition.x, m_QueuedTransform.vPosition.y, m_QueuedTransform.vPosition.z);
	_matrix AnimationCorrectionMatrix = XMMatrixIdentity();

	if (m_bHasActiveSection)
	{
		// Section 보정은 애니메이션/Root Motion의 local 결과와 Track Transform 사이에 적용한다.
		AnimationCorrectionMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_ActiveSection.vRotationCorrectionDegree.x), XMConvertToRadians(m_ActiveSection.vRotationCorrectionDegree.y), XMConvertToRadians(m_ActiveSection.vRotationCorrectionDegree.z)) * XMMatrixTranslation(m_ActiveSection.vPositionCorrection.x, m_ActiveSection.vPositionCorrection.y, m_ActiveSection.vPositionCorrection.z);
	}

	_matrix FinalMatrix = XMLoadFloat4x4(&m_RootMotionMatrix) * AnimationCorrectionMatrix * BaseMatrix;

	XMStoreFloat4x4(&m_LocalWorldMatrix, FinalMatrix);

	m_pTransformCom->Set_State(STATE::RIGHT, FinalMatrix.r[0]);
	m_pTransformCom->Set_State(STATE::UP, FinalMatrix.r[1]);
	m_pTransformCom->Set_State(STATE::LOOK, FinalMatrix.r[2]);
	m_pTransformCom->Set_State(STATE::POSITION, FinalMatrix.r[3]);
}

void CCutScene_Actor::Apply_AttachmentTransform()
{
	if (false == m_bCutSceneAttachmentActive || nullptr == m_pParentActor || m_strAttachBoneName.empty() || nullptr == m_pTransformCom || nullptr == m_pModelCom)
		return;

	_float4x4 ParentBoneWorld = {};
	_float4x4 ChildAttachmentLocal = {};

	if (false == m_pParentActor->Get_CutSceneBoneWorld(m_strAttachBoneName, &ParentBoneWorld))
		return;

	if (false == m_bChildAttachmentReferenceValid)
	{
		if (false == Get_CutSceneAttachmentLocal(m_strChildAttachBoneName, &m_ChildAttachmentReferenceLocal))
			return;

		// 현재 Pose는 부착 시작점의 기준으로 한 번만 저장하고 이후 자식 Animation에서는 갱신하지 않는다.
		m_bChildAttachmentReferenceValid = true;
	}

	ChildAttachmentLocal = m_ChildAttachmentReferenceLocal;

	// 고정된 ChildBone^-1 * LocalOffset * ParentBoneWorld로 정렬해 자식 Animation의 Bone 변화는 그대로 보존한다.
	_matrix FinalMatrix = XMMatrixInverse(nullptr, XMLoadFloat4x4(&ChildAttachmentLocal)) * XMLoadFloat4x4(&m_LocalWorldMatrix) * XMLoadFloat4x4(&ParentBoneWorld);

	if (false == m_bInheritParentRotation || m_fParentRotationBlendWeight < 1.f)
	{
		_vector vScale = {};
		_vector vTargetRotation = {};
		_vector vTranslation = {};

		if (XMMatrixDecompose(&vScale, &vTargetRotation, &vTranslation, FinalMatrix))
		{
			_vector vLockedRotation = XMQuaternionNormalize(XMLoadFloat4(&m_vLockedAttachmentRotation));
			vTargetRotation = XMQuaternionNormalize(vTargetRotation);

			// Quaternion 부호가 반대인 같은 회전은 짧은 방향으로 보간되도록 맞춘다.
			if (XMVectorGetX(XMVector4Dot(vLockedRotation, vTargetRotation)) < 0.f)
				vTargetRotation = XMVectorNegate(vTargetRotation);

			const _vector vBlendedRotation = XMQuaternionNormalize(XMQuaternionSlerp(vLockedRotation, vTargetRotation, m_fParentRotationBlendWeight));
			FinalMatrix = XMMatrixScalingFromVector(vScale) * XMMatrixRotationQuaternion(vBlendedRotation) * XMMatrixTranslationFromVector(vTranslation);
		}
	}

	if (m_bAttachmentBlendActive)
	{
		// 부착 대상이 바뀐 순간의 World에서 새 Bone 기준 World까지 BlendTime 동안 위치·회전·크기를 함께 보간한다.
		FinalMatrix = CutScene_ActorBlendWorldMatrix(XMLoadFloat4x4(&m_AttachmentBlendStartWorld), FinalMatrix, m_fAttachmentBlendWeight);

		if (m_fAttachmentBlendWeight >= 1.f)
			m_bAttachmentBlendActive = false;
	}

	m_pTransformCom->Set_State(STATE::RIGHT, FinalMatrix.r[0]);
	m_pTransformCom->Set_State(STATE::UP, FinalMatrix.r[1]);
	m_pTransformCom->Set_State(STATE::LOOK, FinalMatrix.r[2]);
	m_pTransformCom->Set_State(STATE::POSITION, FinalMatrix.r[3]);
}

_bool CCutScene_Actor::Get_CutSceneAttachmentLocal(const string& strChildBoneName, _float4x4* pOutLocalMatrix) const
{
	if (nullptr == pOutLocalMatrix || nullptr == m_pModelCom)
		return false;

	if ("ComponentRoot" == strChildBoneName)
	{
		// Unreal Attach Track이 Socket 없이 Component만 지정한 경우 자식 Component 원점을 그대로 사용한다.
		XMStoreFloat4x4(pOutLocalMatrix, XMMatrixIdentity());
		return true;
	}

	if (strChildBoneName.empty() || "ActorRoot" == strChildBoneName)
	{
		const _matrix ChildPreTransformMatrix = XMLoadFloat4x4(&m_pModelCom->Get_PreTransformMatrix());
		_vector vChildImportScale = {};
		_vector vChildImportRotation = {};
		_vector vChildImportTranslation = {};
		_matrix ChildImportRotationMatrix = XMMatrixIdentity();

		// Child Bone이 없는 모델은 기존과 같이 모델 원점을 사용하되 Import 회전 중복만 제거한다.
		if (XMMatrixDecompose(&vChildImportScale, &vChildImportRotation, &vChildImportTranslation, ChildPreTransformMatrix))
			ChildImportRotationMatrix = XMMatrixRotationQuaternion(XMQuaternionNormalize(vChildImportRotation));

		XMStoreFloat4x4(pOutLocalMatrix, ChildImportRotationMatrix);
		return true;
	}

	const _float4x4* pChildBoneMatrix = m_pModelCom->Get_Bone_CombinedMatPtr(strChildBoneName.c_str());

	if (nullptr == pChildBoneMatrix)
		return false;

	_matrix ChildBoneMatrix = XMLoadFloat4x4(pChildBoneMatrix);
	const _matrix PreTransformMatrix = XMLoadFloat4x4(&m_pModelCom->Get_PreTransformMatrix());
	const _float fPreScaleX = XMVectorGetX(XMVector3Length(PreTransformMatrix.r[0]));
	const _float fPreScaleY = XMVectorGetX(XMVector3Length(PreTransformMatrix.r[1]));
	const _float fPreScaleZ = XMVectorGetX(XMVector3Length(PreTransformMatrix.r[2]));
	const _float fUniformScaleEpsilon = 0.0001f;

	// 자식 Bone 역행렬에 Import Scale이 남아 있으면 부착된 모델이 100배 커지므로 균일 Scale만 제거한다.
	if (fPreScaleX > fUniformScaleEpsilon && fabsf(fPreScaleX - fPreScaleY) <= fUniformScaleEpsilon && fabsf(fPreScaleX - fPreScaleZ) <= fUniformScaleEpsilon)
	{
		const _float fInverseImportScale = 1.f / fPreScaleX;
		ChildBoneMatrix.r[0] = XMVectorScale(ChildBoneMatrix.r[0], fInverseImportScale);
		ChildBoneMatrix.r[1] = XMVectorScale(ChildBoneMatrix.r[1], fInverseImportScale);
		ChildBoneMatrix.r[2] = XMVectorScale(ChildBoneMatrix.r[2], fInverseImportScale);
	}

	XMStoreFloat4x4(pOutLocalMatrix, ChildBoneMatrix);
	return true;
}

CCutScene_Actor* CCutScene_Actor::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CCutScene_Actor* pInstance = new CCutScene_Actor(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Create : CCutScene_Actor");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CCutScene_Actor::Clone(void* pArg)
{
	CCutScene_Actor* pInstance = new CCutScene_Actor(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone : CCutScene_Actor");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CCutScene_Actor::Free()
{
	Clear_CutSceneAttachment();
	m_pQueuedSections = nullptr;
	m_TriggeredNotifies.clear();
	__super::Free();
}
