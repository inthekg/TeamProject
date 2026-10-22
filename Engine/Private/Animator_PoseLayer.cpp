#include "Animator.h"
#include "Model.h"
#include "Animation.h"
#include "GameInstance.h"
#include "Parser.h"
#include "Channel.h"

// PoseLayer runtime은 rule 개수와 1:1로 맞춘다.
// 각 rule은 base clip 시간과 별개의 local time을 가지므로,
// 걷기 중 회복/수리 같은 overlay가 0프레임부터 독립 재생될 수 있다.
void CAnimator::Ready_PoseLayerRunTime()
{
	const size_t iRuleCount = m_EditData.Controller.PoseLayerRules.size();

	m_PoseLayerRuntimes.clear();
	m_PoseLayerRuntimes.resize(iRuleCount);

	m_PoseLayerSamplePose = m_BasePose;
	m_PoseLayerKeyFrameIndices.clear();
}

void CAnimator::Reset_PoseLayerRunTime()
{
	for (auto& Runtime : m_PoseLayerRuntimes)
		Runtime = {};

	m_PoseLayerSamplePose = m_BasePose;
	m_PoseLayerKeyFrameIndices.clear();
}

// base pose가 이미 m_CurPose에 계산된 뒤 호출된다.
// active rule마다 overlay clip을 별도 시간으로 sampling하고,
// mask root bone과 그 하위 bone만 m_CurPose에 덮어쓴다.
void CAnimator::Update_PoseLayers(_float fTimeDelta)
{
	const vector<ANIM_POSE_LAYER_RULE>& Rules = m_EditData.Controller.PoseLayerRules;

	if (Rules.empty())
		return;

	if (m_PoseLayerRuntimes.size() != Rules.size())
		Ready_PoseLayerRunTime();

	if (!Is_PoseLayerAllowed())
	{
		Reset_PoseLayerRunTime();
		return;
	}

	for (_uint i = 0; i < Rules.size(); i++)
	{
		const ANIM_POSE_LAYER_RULE& Rule = Rules[i];
		POSE_LAYER_RUNTIME& Runtime = m_PoseLayerRuntimes[i];

		const _bool bConditionActive = Is_PoseLayerRuleActive(Rule);

		if (!bConditionActive)
		{
			if (Runtime.bBlendOut)
			{
				// 이미 base pose로 빠지는 중이면 그대로 진행한다.
			}
			else if (Rule.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE && Runtime.bWasActive && !Runtime.bEnded && Rule.iMotionIndex >= 0 &&
				Rule.iMotionIndex < m_EditData.Sequences.size())
			{
				const ANIM_SEQUENCE& Sequence = m_EditData.Sequences[Rule.iMotionIndex];

				// OneShot overlay는 트리거성 입력이 한 프레임만 들어와도 Start->Active->End를 완주한다.
				// Sustained overlay는 Start 클립의 CANCEL window 안에서만 즉시 취소하고, 이후에는 End로 보낸다.
				if (Should_KeepPoseLayerSequenceOnConditionLost(Sequence, Runtime))
				{
					// 현재 step을 유지해서 OneShot 흐름을 계속 진행
				}
				else if (Can_CancelPoseLayerSequenceStart(Sequence, Runtime))
				{
					const ANIM_SEQUENCE_STEP& CurStep = Sequence.Steps[Runtime.iSequenceStepIndex];

					if (Begin_PoseLayerBlendOut(Rule, Sequence, Runtime, CurStep.iClipIndex, Runtime.fLocalTime))
					{
						// Start cancel도 base pose로 부드럽게 빠진다.
					}
					else
					{
						Runtime = {};
						continue;
					}
				}
				else
				{
					Runtime.bEndRequested = true;
				}
			}
			else
			{
				Runtime = {};
				continue;
			}
		}

		if (!Runtime.bWasActive)
		{
			Runtime.bWasActive = true;
			Runtime.bPlaying = true;
			Runtime.bEnded = false;
			Runtime.fLocalTime = 0.f;
			Runtime.iSequenceStepIndex = -1;
			Runtime.bEndRequested = false;
		}

		if (Rule.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::CLIP)
			Update_PoseLayerClip(Rule, Runtime, fTimeDelta);
		else if (Rule.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)
			Update_PoseLayerSequence(Rule, Runtime, fTimeDelta, bConditionActive);
	}
}

void CAnimator::Update_PoseLayerClip(const ANIM_POSE_LAYER_RULE& Rule, POSE_LAYER_RUNTIME& Runtime, _float fTimeDelta)
{
	if (Rule.iMotionIndex < 0 || Rule.iMotionIndex >= static_cast<_int>(m_EditData.ClipSettings.size()))
		return;

	const ANIM_CLIP_SETTING& Clip = m_EditData.ClipSettings[Rule.iMotionIndex];

	if (Runtime.bPlaying)
	{
		const ANIM_TIME_ADVANCE_RESULT Result = Advance_ClipLocalTime(Clip, fTimeDelta, Rule.fPlayRate, Rule.bLoop, Runtime.fLocalTime);

		if (Result.bEnded)
		{
			Runtime.bPlaying = false;
			Runtime.bEnded = true;
		}
	}

	if (Runtime.bEnded && !Rule.bLoop)
		return;

	Apply_PoseLayerClip(Rule, Runtime, Clip, Runtime.fLocalTime);
}

void CAnimator::Update_PoseLayerSequence(const ANIM_POSE_LAYER_RULE& Rule, POSE_LAYER_RUNTIME& Runtime, _float fTimeDelta, _bool bConditionActive)
{
	if (Rule.iMotionIndex < 0 || Rule.iMotionIndex >= static_cast<_int>(m_EditData.Sequences.size()))
		return;

	const ANIM_SEQUENCE& Sequence = m_EditData.Sequences[Rule.iMotionIndex];

	if (Sequence.Steps.empty())
		return;

	// End step이 끝난 뒤에는 마지막 overlay pose의 weight만 줄여 base pose로 자연스럽게 빠진다.
	if (Runtime.bBlendOut)
	{
		Runtime.fBlendOutElapsed += fTimeDelta * 1000.f;

		_float fWeightScale = 0.f;
		if (Runtime.fBlendOutDuration > 0.f)
			fWeightScale = 1.f - min(Runtime.fBlendOutElapsed / Runtime.fBlendOutDuration, 1.f);

		if (Runtime.iBlendOutClipIndex >= 0 && Runtime.iBlendOutClipIndex < m_EditData.ClipSettings.size() && fWeightScale > 0.f)
		{
			Apply_PoseLayerClip(Rule, Runtime, m_EditData.ClipSettings[Runtime.iBlendOutClipIndex], Runtime.fBlendOutLocalTime, fWeightScale);
		}

		if (Runtime.fBlendOutElapsed >= Runtime.fBlendOutDuration)
		{
			Runtime.bBlendOut = false;
			Runtime.bPlaying = false;
			Runtime.bEnded = true;
			Runtime.iBlendOutClipIndex = -1;
			Runtime.fBlendOutLocalTime = 0.f;
			Runtime.fBlendOutElapsed = 0.f;
			Runtime.fBlendOutDuration = 0.f;
		}

		return;
	}

	if (Runtime.bEnded)
		return;

	// 처음 활성화된 sequence는 첫 step부터 시작한다.
	if (Runtime.iSequenceStepIndex < 0)
	{
		if (!Enter_PoseLayerSequenceStep(Rule, Runtime, 0))
			return;
	}

	const ANIM_SEQUENCE_STEP& Step = Sequence.Steps[Runtime.iSequenceStepIndex];

	// 입력이 끊긴 sustained sequence는 현재 step을 버리고 End step으로 전환한다.
	if (Runtime.bEndRequested && Step.eStepType != ANIM_SEQUENCE_STEP_TYPE::END)
	{
		const _int iEndStep = Find_PoseLayerSequenceEndStepIndex(Sequence, Runtime.iSequenceStepIndex);
		if (iEndStep >= 0)
		{
			Enter_PoseLayerSequenceStep(Rule, Runtime, iEndStep);
		}
		else
		{
			Runtime.bPlaying = false;
			Runtime.bEnded = true;
			return;
		}
	}

	const ANIM_SEQUENCE_STEP& CurStep = Sequence.Steps[Runtime.iSequenceStepIndex];

	if (CurStep.iClipIndex < 0 || CurStep.iClipIndex >= m_EditData.ClipSettings.size())
		return;

	const ANIM_CLIP_SETTING& Clip = m_EditData.ClipSettings[CurStep.iClipIndex];

	const _bool bLoopCurStep = CurStep.eStepType == ANIM_SEQUENCE_STEP_TYPE::LOOP && bConditionActive && !Runtime.bEndRequested;

	const ANIM_TIME_ADVANCE_RESULT TimeResult = Advance_ClipLocalTime(Clip, fTimeDelta, Rule.fPlayRate, bLoopCurStep, Runtime.fLocalTime);

	if (TimeResult.bEnded)
	{
		if (CurStep.eStepType == ANIM_SEQUENCE_STEP_TYPE::LOOP)
		{
			const _int iEndStep = Find_PoseLayerSequenceEndStepIndex(Sequence, Runtime.iSequenceStepIndex);

			if (iEndStep >= 0)
				Enter_PoseLayerSequenceStep(Rule, Runtime, iEndStep);
			else
			{
				Runtime.bPlaying = false;
				Runtime.bEnded = true;
				return;
			}
		}
		else
		{
			const _int iNextStep = Runtime.iSequenceStepIndex + 1;

			if (iNextStep < Sequence.Steps.size())
			{
				Enter_PoseLayerSequenceStep(Rule, Runtime, iNextStep);
			}
			else
			{
				Runtime.bPlaying = false;

				// End 끝난 뒤 blend out 시작 프레임 튐 제거
				if (Begin_PoseLayerBlendOut(Rule, Sequence, Runtime, CurStep.iClipIndex, Runtime.fLocalTime))
					return;

				Runtime.bEnded = true;
				return;
			}
		}
	}

	const ANIM_SEQUENCE_STEP& ApplyStep = Sequence.Steps[Runtime.iSequenceStepIndex];

	if (ApplyStep.iClipIndex < 0 || ApplyStep.iClipIndex >= m_EditData.ClipSettings.size())
		return;

	Apply_PoseLayerClip(Rule, Runtime, m_EditData.ClipSettings[ApplyStep.iClipIndex], Runtime.fLocalTime);
}

_bool CAnimator::Enter_PoseLayerSequenceStep(const ANIM_POSE_LAYER_RULE& Rule, POSE_LAYER_RUNTIME& Runtime, _int iStepIndex)
{
	if (Rule.iMotionIndex < 0 || Rule.iMotionIndex >= m_EditData.Sequences.size())
		return false;

	const ANIM_SEQUENCE& Sequence = m_EditData.Sequences[Rule.iMotionIndex];

	if (iStepIndex < 0 || iStepIndex >= Sequence.Steps.size())
		return false;

	const ANIM_SEQUENCE_STEP& Step = Sequence.Steps[iStepIndex];

	if (Step.iClipIndex < 0 || Step.iClipIndex >= m_EditData.ClipSettings.size())
		return false;

	Runtime.iSequenceStepIndex = iStepIndex;
	Runtime.fLocalTime = 0.f;
	Runtime.bPlaying = true;
	Runtime.bEnded = false;
	Runtime.bBlendOut = false;
	Runtime.fBlendOutElapsed = 0.f;
	Runtime.fBlendOutDuration = 0.f;
	Runtime.iBlendOutClipIndex = -1;
	Runtime.fBlendOutLocalTime = 0.f;

	return true;
}

void CAnimator::Apply_PoseLayerClip(const ANIM_POSE_LAYER_RULE& Rule, POSE_LAYER_RUNTIME& Runtime, const ANIM_CLIP_SETTING& Clip, _float fLocalTime, _float fWeightScale)
{
	_float fWeight = Rule.fWeight * fWeightScale;

	if (fWeight <= 0.f)
		return;
	if (fWeight > 1.f)
		fWeight = 1.f;

	const _bool bAdditive = Rule.eBlendType == ANIM_POSE_LAYER_BLEND_TYPE::ADDITIVE;

	if (bAdditive)
	{
		if (!Runtime.bHasAdditiveBasePose || Runtime.iAdditiveBaseClipHash != Clip.iClipHash)
		{
			// Additive 기준 포즈는 해당 additive clip의 0프레임을 사용한다
			if (!Sample_PoseLayerPose(Clip, 0.f, Runtime.AdditiveBasePose))
				return;

			Runtime.bHasAdditiveBasePose = true;
			Runtime.iAdditiveBaseClipHash = Clip.iClipHash;
		}
	}

	if (!Sample_PoseLayerPose(Clip, fLocalTime, m_PoseLayerSamplePose))
		return;

	_int iPoseCount = static_cast<_int>(min(m_CurPose.size(), m_PoseLayerSamplePose.size()));

	if (bAdditive)
		iPoseCount = static_cast<_int>(min(static_cast<size_t>(iPoseCount), Runtime.AdditiveBasePose.size()));

	// mask root bone과 그 자식 bone에만 overlay pose를 섞는다.
	for (_int iBone = 0; iBone < iPoseCount; iBone++)
	{
		if (!Is_BoneInMask(iBone, Rule.iMaskRootBoneIndex))
			continue;

		if (bAdditive)
		{
			Additive_LocalTransform(m_CurPose[iBone], Runtime.AdditiveBasePose[iBone], m_PoseLayerSamplePose[iBone], fWeight);
		}
		else
		{
			Blend_LocalTransform(m_CurPose[iBone], m_PoseLayerSamplePose[iBone], fWeight);
		}
	}
}

_bool CAnimator::Sample_PoseLayerPose(const ANIM_CLIP_SETTING& Clip, _float fLocalTime, POSE& OutPose)
{
	return Sample_ClipPose(Clip, fLocalTime, OutPose, m_PoseLayerKeyFrameIndices, false);
}

void CAnimator::Blend_LocalTransform(TransformData& Dst, const TransformData& Src, _float fWeight) const
{
	if (fWeight <= 0.f)
		return;

	if (fWeight > 1.f)
		fWeight = 1.f;

	_vector vDstT = XMLoadFloat4(&Dst.vTranslation);
	_vector vSrcT = XMLoadFloat4(&Src.vTranslation);

	_vector vDstS = XMLoadFloat4(&Dst.vScale);
	_vector vSrcS = XMLoadFloat4(&Src.vScale);

	_vector vDstR = XMLoadFloat4(&Dst.vRot);
	_vector vSrcR = XMLoadFloat4(&Src.vRot);

	XMStoreFloat4(&Dst.vTranslation, XMVectorLerp(vDstT, vSrcT, fWeight));
	XMStoreFloat4(&Dst.vScale, XMVectorLerp(vDstS, vSrcS, fWeight));
	XMStoreFloat4(&Dst.vRot, XMQuaternionSlerp(vDstR, vSrcR, fWeight));
}

void CAnimator::Additive_LocalTransform(TransformData& Dst, const TransformData& Base, const TransformData& AdditivePose, _float fWeight) const
{
	if (fWeight <= 0.f)
		return;

	if (fWeight > 1.f)
		fWeight = 1.f;

	// Translation: additive pose가 기준 pose에서 움직인 양만 현재 pose에 더한다.
	_vector vDstT = XMLoadFloat4(&Dst.vTranslation);
	_vector vBaseT = XMLoadFloat4(&Base.vTranslation);
	_vector vAddT = XMLoadFloat4(&AdditivePose.vTranslation);
	_vector vDeltaT = XMVectorSubtract(vAddT, vBaseT);

	XMStoreFloat4(&Dst.vTranslation, XMVectorAdd(vDstT, XMVectorScale(vDeltaT, fWeight)));

	// Scale: 기준 scale 대비 변화 비율만 현재 scale에 곱한다.
	const _float fEpsilon = 0.0001f;

	_float4 vDstScale = Dst.vScale;
	_float4 vBaseScale = Base.vScale;
	_float4 vAddScale = AdditivePose.vScale;

	if (fabsf(vBaseScale.x) > fEpsilon)
		vDstScale.x *= 1.f + ((vAddScale.x / vBaseScale.x) - 1.f) * fWeight;

	if (fabsf(vBaseScale.y) > fEpsilon)
		vDstScale.y *= 1.f + ((vAddScale.y / vBaseScale.y) - 1.f) * fWeight;

	if (fabsf(vBaseScale.z) > fEpsilon)
		vDstScale.z *= 1.f + ((vAddScale.z / vBaseScale.z) - 1.f) * fWeight;

	vDstScale.w = 1.f;
	Dst.vScale = vDstScale;

	// Rotation: additive pose와 기준 pose의 회전 차이만 현재 회전에 더한다.
	_vector qDst = XMQuaternionNormalize(XMLoadFloat4(&Dst.vRot));
	_vector qBase = XMQuaternionNormalize(XMLoadFloat4(&Base.vRot));
	_vector qAdd = XMQuaternionNormalize(XMLoadFloat4(&AdditivePose.vRot));

	_vector qDelta = XMQuaternionMultiply(qAdd, XMQuaternionInverse(qBase));
	qDelta = XMQuaternionNormalize(qDelta);

	_vector qWeightedDelta = XMQuaternionSlerp(XMQuaternionIdentity(), qDelta, fWeight);
	_vector qResult = XMQuaternionMultiply(qWeightedDelta, qDst);

	XMStoreFloat4(&Dst.vRot, XMQuaternionNormalize(qResult));
}

_bool CAnimator::Begin_PoseLayerBlendOut(const ANIM_POSE_LAYER_RULE& Rule, const ANIM_SEQUENCE& Sequence, POSE_LAYER_RUNTIME& Runtime, _int iClipIndex, _float fLocalTime)
{
	if (Sequence.fEndBlendOutTime <= 0.f)
		return false;

	if (iClipIndex < 0 || iClipIndex >= static_cast<_int>(m_EditData.ClipSettings.size()))
		return false;

	Runtime.bBlendOut = true;
	Runtime.fBlendOutElapsed = 0.f;
	Runtime.fBlendOutDuration = Sequence.fEndBlendOutTime;
	Runtime.iBlendOutClipIndex = iClipIndex;
	Runtime.fBlendOutLocalTime = fLocalTime;

	Runtime.bPlaying = false;
	Runtime.bEnded = false;

	// blend out 시작 프레임에도 마지막 overlay pose를 100% 적용해서 한 프레임 튐을 막는다.
	Apply_PoseLayerClip(Rule, Runtime, m_EditData.ClipSettings[iClipIndex], fLocalTime, 1.f);

	return true;
}

// 현재 controller state가 pose layer를 허용하는지 검사한다.
// 특정 state에서 상하체 분리를 막고 싶을 때 bAllowPoseLayers=false를 사용한다.
_bool CAnimator::Is_PoseLayerAllowed() const
{
	const ANIM_CONTROLLER_STATE* pState = Get_CurControllerState();

	if (pState != nullptr && !pState->bAllowPoseLayers)
		return false;

	return true;
}

// rule 자체 활성 조건 검사.
// clip/mask가 유효하고, rule condition이 모두 통과해야 overlay가 적용된다.
_bool CAnimator::Is_PoseLayerRuleActive(const ANIM_POSE_LAYER_RULE& Rule) const
{
	if (!Rule.bEnable)
		return false;

	if (Rule.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::CLIP)
	{
		if (Rule.iMotionIndex < 0 || Rule.iMotionIndex >= m_EditData.ClipSettings.size())
			return false;
	}
	else if (Rule.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)
	{
		if (Rule.iMotionIndex < 0 || Rule.iMotionIndex >= m_EditData.Sequences.size())
			return false;
	}
	else
	{
		return false;
	}

	if (Rule.iMaskRootBoneIndex < 0)
		return false;

	for (const auto& Condition : Rule.Conditions)
	{
		if (!Is_ConditionPassed(Condition))
			return false;
	}

	return true;
}

_bool CAnimator::Is_ClipInWindow(const ANIM_CLIP_SETTING& Clip, _float fLocalTime, ANIM_WINDOW_TYPE eType) const
{
	const _uint iFrameCount = Calc_ClipFrameCount(Clip);
	const _float fDuration = Calc_ClipDuration(Clip);

	if (iFrameCount <= 1 || fDuration <= 0.f)
		return false;

	_float fRatio = fLocalTime / fDuration;
	fRatio = max(0.f, min(1.f, fRatio));

	const _uint iFrame = static_cast<_uint>(fRatio * static_cast<_float>(iFrameCount - 1));

	for (const auto& Window : Clip.Windows)
	{
		if (Window.eType == eType && iFrame >= Window.iStartFrame && iFrame <= Window.iEndFrame)
			return true;
	}

	return false;
}

_bool CAnimator::Can_CancelPoseLayerSequenceStart(const ANIM_SEQUENCE& Sequence, const POSE_LAYER_RUNTIME& Runtime) const
{
	if (!Sequence.bStartCancelByWindow)
		return false;

	if (Runtime.iSequenceStepIndex < 0 || Runtime.iSequenceStepIndex >= Sequence.Steps.size())
		return false;

	const ANIM_SEQUENCE_STEP& Step = Sequence.Steps[Runtime.iSequenceStepIndex];
	if (Step.eStepType != ANIM_SEQUENCE_STEP_TYPE::START)
		return false;

	if (Step.iClipIndex < 0 || Step.iClipIndex >= m_EditData.ClipSettings.size())
		return false;

	return Is_ClipInWindow(m_EditData.ClipSettings[Step.iClipIndex], Runtime.fLocalTime, ANIM_WINDOW_TYPE::CANCEL);
}

_bool CAnimator::Should_KeepPoseLayerSequenceOnConditionLost(const ANIM_SEQUENCE& Sequence, const POSE_LAYER_RUNTIME& Runtime) const
{
	if (!Runtime.bWasActive || Runtime.bEnded)
		return false;

	return Sequence.eFlow == ANIM_SEQUENCE_FLOW::ONE_SHOT;
}

_bool CAnimator::Is_BoneInMask(_int iBoneIndex, _int iMaskRootBoneIndex) const
{
	if (iBoneIndex < 0 || iMaskRootBoneIndex < 0)
		return false;

	if (iBoneIndex >= m_Bones.size())
		return false;

	_int iCur = iBoneIndex;

	while (iCur >= 0 && iCur < m_Bones.size())
	{
		if (iCur == iMaskRootBoneIndex)
			return true;

		iCur = m_Bones[iCur].iParentIndex;
	}

	return false;
}

_int CAnimator::Find_PoseLayerSequenceEndStepIndex(const ANIM_SEQUENCE& Sequence, _int iCurStepIndex) const
{
	return Find_SequenceStepIndexByType(Sequence, ANIM_SEQUENCE_STEP_TYPE::END, iCurStepIndex);
}
