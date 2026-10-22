#include "Animator.h"
#include "Model.h"
#include "Animation.h"
#include "GameInstance.h"
#include "Parser.h"
#include "Channel.h"


CAnimator::CAnimator()
{
	m_pGameInstance = CGameInstance::Get_Instance();
}

#pragma region PUBLIC_EDITOR_API
#ifdef _DEBUG
const ANIMATION_EDIT_DATA& CAnimator::Get_EditData() const
{
	return m_EditData;
}

// 에디터에서 clip 설정 목록이 다시 들어오면 현재 재생 상태를 초기화한다.
// 기존 clip index/time은 새 목록에서 의미가 달라질 수 있으므로 유지하지 않는다.
void CAnimator::Set_ClipSettings(const vector<ANIM_CLIP_SETTING>& ClipSettings)
{
	m_EditData.ClipSettings = ClipSettings;

	Reset_PlaybackState();
}

void CAnimator::Set_LocalFrame(_uint iLocalFrame)
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return;

	const _uint iFrameCount = Calc_ClipFrameCount(*pClip);

	if (iFrameCount <= 1)
	{
		Set_LocalTime(0.f);
		return;
	}

	if (iLocalFrame >= iFrameCount)
		iLocalFrame = iFrameCount - 1;

	const _float fDuration = Calc_ClipDuration(*pClip);

	if (fDuration <= 0.f)
		return;

	const _float fRatio = static_cast<_float>(iLocalFrame) / static_cast<_float>(iFrameCount - 1);

	Set_LocalTime(fDuration * fRatio);
}




// combo/cancel 요청은 현재 clip window를 통과해야만 실제 clip 전환으로 이어진다.
// 이후 Controller가 붙어도 최종 허용 판단은 이 window 검사 흐름을 재사용한다.
HRESULT CAnimator::Request_Combo_ByIndex(_uint iClipIndex)
{
	if (!Can_ComboInput())
		return E_FAIL;

	return Play_Clip_ByIndex(iClipIndex);
}

HRESULT CAnimator::Request_Cancel_ByIndex(_uint iClipIndex)
{
	if (!Can_Cancel())
		return E_FAIL;

	return Play_Clip_ByIndex(iClipIndex);
}

void CAnimator::Reset_PoseLayerPreview()
{
	Reset_PoseLayerRunTime();

	Ready_ControllerRuntime();

	Update_CurPose();
}

HRESULT CAnimator::Preview_Sequence_ByIndex(_uint iSequenceIndex, _uint iStepIndex, _float fStepLocalTime)
{
	if (FAILED(Jump_SequenceStep_ForPreview(iSequenceIndex, iStepIndex, fStepLocalTime)))
		return E_FAIL;

	Pause();
	return S_OK;
}
_bool CAnimator::Is_SequenceMode() const
{
	return m_bSequenceMode;
}
_int CAnimator::Get_CurSequenceIndex() const
{
	return m_iCurSequeneIndex;
}
_int CAnimator::Get_CurSequenceStepIndex() const
{
	return m_iCurSequenceStepIndex;
}
_bool CAnimator::Is_StateBlending() const
{
	return m_bStateBlending;
}
_float CAnimator::Get_StateBlendAlpha() const
{
	if (!m_bStateBlending || m_fBlendDuration <= 0.f)
		return 0.f;

	return min(max(m_fBlendElapsed / m_fBlendDuration, 0.f), 1.f);
}
#endif
#pragma endregion

void CAnimator::Set_PlayRate(_float fPlayRate)
{
	if (fPlayRate <= 0.f)
		return;

	m_fPlayRate = fPlayRate;
}

void CAnimator::Set_UseRootMotion(_bool bUseRootMotion)
{
	m_bUseRootMotion = bUseRootMotion;
	Update_CurPose();
}

#pragma region PUBLIC_RUNTIME_API

void CAnimator::Set_Loop(_bool bLoop)
{
	m_bLoop = bLoop;
}

void CAnimator::Set_EditData(const ANIMATION_EDIT_DATA& EditData)
{
	ANIMATION_EDIT_DATA ResolvedEditData = EditData;

	Validate_EditData(ResolvedEditData);
	Resolve_RawAnimationIndices(ResolvedEditData);
	Resolve_EditData(ResolvedEditData);

	m_EditData = ResolvedEditData;

	Ready_ControllerRuntime();
	Ready_PoseLayerRunTime();

	Reset_PlaybackState();
}

const POSE& CAnimator::Get_CurPose() const
{
	return m_CurPose;
}

const ANIM_CLIP_SETTING* CAnimator::Get_CurClip() const
{
	if (m_iCurClipIndex < 0)
		return nullptr;

	if (m_iCurClipIndex >= static_cast<_int>(m_EditData.ClipSettings.size()))
		return nullptr;

	return &m_EditData.ClipSettings[m_iCurClipIndex];
}

const ANIM_GAMEPLAY_FLAGS& CAnimator::Get_GameplayFlags() const
{
	return m_GameplayFlags;
}

const vector<const ANIM_NOTIFY*>& CAnimator::Get_TriggeredNotifies() const
{
	return m_TriggeredNotifies;
}

const ANIM_SEQUENCE* CAnimator::Get_CurSequence() const
{
	if (m_iCurSequeneIndex < 0 || m_iCurSequeneIndex >= m_EditData.Sequences.size())
		return nullptr;

	return &m_EditData.Sequences[m_iCurSequeneIndex];
}

const ANIM_SEQUENCE_STEP* CAnimator::Get_CurSequenceStep() const
{
	const ANIM_SEQUENCE* pSequence = Get_CurSequence();
	if (pSequence == nullptr)
		return nullptr;

	if (m_iCurSequenceStepIndex < 0 || m_iCurSequenceStepIndex >= pSequence->Steps.size())
		return nullptr;

	return &pSequence->Steps[m_iCurSequenceStepIndex];
}

const ANIM_ROOT_MOTION_DELTA& CAnimator::Get_RootMotionDelta() const
{
	return m_RootMotionDelta;
}

const ANIM_CONTROLLER_STATE* CAnimator::Get_CurControllerState() const
{
	if (m_iCurControllerStateIndex < 0 || m_iCurControllerStateIndex >= m_EditData.Controller.States.size())
		return nullptr;

	return &m_EditData.Controller.States[m_iCurControllerStateIndex];
}

void CAnimator::Set_Bool(const _wstring& strParamName, _bool bValue)
{
	_int iIndex = Find_ControllerParamIndex(Hash_String(strParamName.c_str()));

	if (iIndex < 0)
		return;

	if (m_ControllerParamValues[iIndex].eType != ANIM_CONTROLLER_PARAM_TYPE::BOOL)
		return;

	m_ControllerParamValues[iIndex].bBool = bValue;
}

void CAnimator::Set_Int(const _wstring& strParamName, _int iValue)
{
	_int iIndex = Find_ControllerParamIndex(Hash_String(strParamName.c_str()));  

	if (iIndex < 0)
		return;

	if (m_ControllerParamValues[iIndex].eType != ANIM_CONTROLLER_PARAM_TYPE::INT)
		return;

	m_ControllerParamValues[iIndex].iInt = iValue;
}

void CAnimator::Set_Float(const _wstring& strParamName, _float fValue)
{
	_int iIndex = Find_ControllerParamIndex(Hash_String(strParamName.c_str()));

	if (iIndex < 0)
		return;

	if (m_ControllerParamValues[iIndex].eType != ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
		return;

	m_ControllerParamValues[iIndex].fFloat = fValue;
}

void CAnimator::Set_Trigger(const _wstring& strParamName)
{
	_int iIndex = Find_ControllerParamIndex(Hash_String(strParamName.c_str()));

	if (iIndex < 0)
		return;

	if (m_ControllerParamValues[iIndex].eType != ANIM_CONTROLLER_PARAM_TYPE::TRIGGER)
		return;

	m_ControllerParamValues[iIndex].bTriggered = true;
}

void CAnimator::Request_State(const _wstring& strStateName)
{
	Request_State(Hash_String(strStateName.c_str()));
}

void CAnimator::Request_State(_uint64 iStateHash)
{
	if (iStateHash == 0)
		return;

	m_bHasRequestedState = true;
	m_iRequestedStateHash = iStateHash;
}

void CAnimator::Request_SequenceEnd()
{
	if (!m_bSequenceMode)
		return;

	m_bSequenceEndRequested = true;
}

_float CAnimator::Get_LocalTime() const
{
	return m_fLocalTime;
}

// 타임라인 seek처럼 외부에서 local time을 직접 지정하는 경로다.
// notify는 seek 순간에 발생시키지 않고, pose/gameplay flag만 현재 시간에 맞춰 다시 계산한다.
void CAnimator::Set_LocalTime(_float fLocalTime)
{
	Clear_TriggeredNotifies();

	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return;

	const _float fDuration = Calc_ClipDuration(*pClip);

	if (fDuration <= 0.f)
		return;

	if (fLocalTime < 0.f)
		fLocalTime = 0.f;

	if (fLocalTime > fDuration)
		fLocalTime = fDuration;

	m_fLocalTime = fLocalTime;
	m_bEnded = false;

	Update_GameplayFlags();
	Update_CurPose();

	// seek에서는 overlay runtime 시간을 강제로 진행시키지 않고,
	// 현재 active rule이면 현재 저장된 overlay time 기준으로만 다시 합성.
	Update_PoseLayers(0.f);
}

_uint CAnimator::Get_LocalFrame() const
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return 0;

	const _uint iFrameCount = Calc_ClipFrameCount(*pClip);

	if (iFrameCount <= 1)
		return 0;

	const _float fSampleFrame = Calc_SampleFramePosition(*pClip, m_fLocalTime);

	if (fSampleFrame < 0.f)
		return 0;

	const _uint iMaxFrame = iFrameCount - 1;

	if (fSampleFrame > static_cast<_float>(iMaxFrame))
		return iMaxFrame;

	return static_cast<_uint>(fSampleFrame);
}

_float CAnimator::Get_RawTime() const
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return 0.f;

	return Calc_LocalToRawTime(*pClip, m_fLocalTime);
}

_uint CAnimator::Get_RawAnimationIndex() const
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return 0;

	return pClip->iRawAnimationIndex;
}

_float CAnimator::Get_CurClipDuration() const
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return 0.f;

	return Calc_ClipDuration(*pClip);
}

_uint CAnimator::Get_CurClipFrameCount() const
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();


	if (pClip == nullptr)
		return 0;

	return Calc_ClipFrameCount(*pClip);
}

const vector<const ANIM_WINDOW*>& CAnimator::Get_ActiveCustomWindows() const
{
	return m_ActiveCustomWindows;
}

_bool CAnimator::Is_PoseLayerActive(_uint64 iRuleHash) const
{
	const auto& Rules = m_EditData.Controller.PoseLayerRules;
	for (size_t i = 0; i < Rules.size(); ++i)
	{
		if (Rules[i].iNameHash == iRuleHash && i < m_PoseLayerRuntimes.size())
			return m_PoseLayerRuntimes[i].bPlaying && !m_PoseLayerRuntimes[i].bBlendOut;
	}
	return false;
}

_bool CAnimator::Is_PoseLayerLoop(_uint64 iRuleHash) const
{
	const auto& Rules = m_EditData.Controller.PoseLayerRules;
	for (size_t i = 0; i < Rules.size(); ++i)
	{
		const ANIM_POSE_LAYER_RULE& Rule = Rules[i];
		if (Rule.iNameHash != iRuleHash || i >= m_PoseLayerRuntimes.size())
			continue;

		const POSE_LAYER_RUNTIME& Rt = m_PoseLayerRuntimes[i];
		if (!Rt.bPlaying || Rt.bBlendOut)                                   
			return false;
		if (Rule.eMotionType != ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)       
			return false;
		if (Rule.iMotionIndex < 0 || Rule.iMotionIndex >= (_int)m_EditData.Sequences.size()) 
			return false;

		const ANIM_SEQUENCE& Seq = m_EditData.Sequences[Rule.iMotionIndex];
		if (Rt.iSequenceStepIndex < 0 || Rt.iSequenceStepIndex >= (_int)Seq.Steps.size()) return false;

		return Seq.Steps[Rt.iSequenceStepIndex].eStepType == ANIM_SEQUENCE_STEP_TYPE::LOOP;
	}
	return false;
}

_bool CAnimator::Is_Playing() const
{
	return m_bPlaying;
}

_bool CAnimator::Is_Ended() const
{
	return m_bEnded;
}

_bool CAnimator::Is_InWindow(ANIM_WINDOW_TYPE eType) const
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return false;

	const _uint iFrame = Get_LocalFrame();

	for (const ANIM_WINDOW& Window : pClip->Windows)
	{
		if (Window.eType != eType)
			continue;

		if (iFrame >= Window.iStartFrame && iFrame <= Window.iEndFrame)
			return true;
	}

	return false;
}

_bool CAnimator::Is_InCustomWindow(const _wstring strPayload) const
{
	for (const ANIM_WINDOW* pWindow : m_ActiveCustomWindows)
	{
		if (pWindow && pWindow->iPayloadHash == Hash_String(strPayload.c_str()))
			return true;
	}

	return false;
}

_bool CAnimator::Can_ComboInput() const
{
	return Is_InWindow(ANIM_WINDOW_TYPE::COMBO_INPUT);
}

_bool CAnimator::Can_Cancel() const
{
	return Is_InWindow(ANIM_WINDOW_TYPE::CANCEL);
}

void CAnimator::Reset_PlaybackState()
{
	Clear_RootMotionDelta();

	m_bRawMode = false;
	m_iCurRawAnimationIndex = -1;
	m_fRawTime = 0.f;
	m_bRawLoop = false;

	m_bSequenceMode = false;
	m_iCurSequeneIndex = -1;
	m_iCurSequenceStepIndex = -1;
	m_bSequenceEndRequested = false;

	m_iCurClipIndex = -1;
	m_fLocalTime = 0.f;
	m_fPlayRate = 1.f;

	m_bPlaying = false;
	m_bLoop = false;
	m_bEnded = false;

	m_iCurControllerStateIndex = -1;
	m_bHasRequestedState = false;
	m_iRequestedStateHash = 0;

	Stop_StateBlend();
	m_BlendFromPose = m_BasePose;
	m_BlendToPose = m_BasePose;
	m_BlendFromKeyFrameIndices.clear();

	m_GameplayFlags = {};
	Clear_TriggeredNotifies();

	m_CurPose = m_BasePose;

	Reset_PoseLayerRunTime();
}

void CAnimator::Set_RootBoneIndex(_int iRootBoneIndex)
{
	m_iRootBoneIndex = iRootBoneIndex;
}

void CAnimator::Set_ControllerEnabled(_bool bEnable)
{
	m_bControllerEnabled = bEnable;
}

HRESULT CAnimator::Play_Default()
{
	if (!m_EditData.ClipSettings.empty())
		return Play_Clip_ByIndex(0);
	if (!m_Animations.empty())
		return Play_RawAnimation_ByIndex(0);

	return E_FAIL;
}

HRESULT CAnimator::Play_Clip(_uint64 iClipHash)
{
	for (_uint i = 0; i < m_EditData.ClipSettings.size(); i++)
	{
		if (m_EditData.ClipSettings[i].iClipHash == iClipHash)
			return Play_Clip_ByIndex(i);
	}

	return E_FAIL;
}

HRESULT CAnimator::Blend_ToClip(_uint64 iClipHash, _float fTargetLocalTimeMS, _float fBlendDurationMS, _bool bUseRootMotion)
{
	_int iTargetClipIndex = -1;

	for (_uint i = 0; i < m_EditData.ClipSettings.size(); ++i)
	{
		if (m_EditData.ClipSettings[i].iClipHash == iClipHash)
		{
			iTargetClipIndex = static_cast<_int>(i);
			break;
		}
	}

	if (iTargetClipIndex < 0)
		return E_FAIL;

	const _int iFromClipIndex = m_iCurClipIndex;
	const _float fFromLocalTime = m_fLocalTime;

	if (FAILED(Play_Clip_ByIndex(static_cast<_uint>(iTargetClipIndex))))
		return E_FAIL;

	// Root Motion 설정도 Pose를 다시 계산하므로 블렌드 시작 전에 목표 Clip 설정과 시작 Pose를 모두 확정한다.
	Set_UseRootMotion(bUseRootMotion);
	Set_LocalTime(max(0.f, fTargetLocalTimeMS));

	// 이전 Pose를 고정해 전환 중 발과 Root가 흔들리지 않게 한다.
	if (iFromClipIndex >= 0 && iFromClipIndex != iTargetClipIndex && fBlendDurationMS > 0.f)
	{
		Begin_StateBlend(iFromClipIndex, fFromLocalTime, fBlendDurationMS, true);
		// 전환 첫 프레임도 이전 Pose에서 시작하도록 0초 상태를 즉시 반영한다.
		Update_StateBlend(0.f);
	}
	else
	{
		Stop_StateBlend();
	}

	return S_OK;
}

// clip 설정 배열 index로 현재 재생 대상을 바꾼다.
// clip 자체의 loop/playrate/root motion 설정을 Animator runtime 상태로 복사한 뒤 pose를 즉시 갱신한다.
HRESULT CAnimator::Play_Clip_ByIndex(_uint iClipIndex)
{
	return Play_Clip_ByIndex_Internal(iClipIndex, false);
}

HRESULT CAnimator::Play_Sequence(_uint64 iSequenceHash)
{
	for (_uint i = 0; i < m_EditData.Sequences.size(); i++)
	{
		if (m_EditData.Sequences[i].iNameHash == iSequenceHash)
			return Play_Sequence_ByIndex(i);
	}

	return E_FAIL;
}

HRESULT CAnimator::Play_Sequence_ByIndex(_uint iSequenceIndex)
{
	return Play_Sequence_ByIndex_Internal(iSequenceIndex, 0.f);
}

HRESULT CAnimator::Play_Sequence_ByIndex_Internal(_uint iSequenceIndex, _float fFirstStepLocalTime)
{
	if (iSequenceIndex >= m_EditData.Sequences.size())
		return E_FAIL;

	const ANIM_SEQUENCE& Sequence = m_EditData.Sequences[iSequenceIndex];

	if (Sequence.Steps.empty())
		return E_FAIL;

	m_bSequenceMode = true;
	m_iCurSequeneIndex = static_cast<_int>(iSequenceIndex);
	m_iCurSequenceStepIndex = -1;
	m_bSequenceEndRequested = false;

	if (FAILED(Enter_SequenceStep(0)))
		return E_FAIL;

	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip != nullptr && fFirstStepLocalTime > 0.f)
	{
		const _float fDuration = Calc_ClipDuration(*pClip);

		if (fDuration > 0.f)
		{
			if (fFirstStepLocalTime > fDuration)
				fFirstStepLocalTime = fDuration;

			m_fLocalTime = fFirstStepLocalTime;
			Update_GameplayFlags();
			Update_CurPose();
		}
	}

	return S_OK;
}

HRESULT CAnimator::Play_RawAnimation(_uint64 iAnimationFileHash)
{
	for (_uint i = 0; i < m_Animations.size(); i++)
	{
		if (m_Animations[i] && m_Animations[i]->Get_AnimationFileHash() == iAnimationFileHash)
			return Play_RawAnimation_ByIndex(i);
	}

	return E_FAIL;
}

HRESULT CAnimator::Play_RawAnimation(const _wstring& strAnimationFileName)
{
	return Play_RawAnimation(Hash_String(strAnimationFileName.c_str()));
}

HRESULT CAnimator::Play_RawAnimation_ByIndex(_uint iRawAnimationIndex)
{
	if (iRawAnimationIndex >= m_Animations.size())
		return E_FAIL;

	CAnimation* pAnimation = m_Animations[iRawAnimationIndex];
	if (pAnimation == nullptr)
		return E_FAIL;

	m_bRawMode = true;
	m_iCurRawAnimationIndex = static_cast<_int>(iRawAnimationIndex);
	m_iCurClipIndex = -1;

	m_fRawTime = 0.f;
	m_bRawLoop = pAnimation->IsLoop();

	m_bPlaying = true;
	m_bEnded = false;

	m_GameplayFlags = {};
	Clear_TriggeredNotifies();

	Update_CurPose();

	return S_OK;
}

HRESULT CAnimator::Play_RawAnimation_ByStateName(const _wstring& strStateName)
{
	_uint64 iHash = Hash_String(strStateName.c_str());
	for (_uint i = 0; i < m_Animations.size(); ++i)
		if (m_Animations[i] && m_Animations[i]->Get_StateHashNum() == iHash)
			return Play_RawAnimation_ByIndex(i);
	return E_FAIL;
}

// combo/cancel 요청은 현재 clip window를 통과해야만 실제 clip 전환으로 이어진다.
// 이후 Controller가 붙어도 최종 허용 판단은 이 window 검사 흐름을 재사용한다.
HRESULT CAnimator::Request_Combo(_uint64 iClipHash)
{
	if (!Can_ComboInput())
		return E_FAIL;

	return Play_Clip(iClipHash);
}

HRESULT CAnimator::Request_Cancel(_uint64 iClipHash)
{
	if (!Can_Cancel())
		return E_FAIL;

	return Play_Clip(iClipHash);
}

void CAnimator::Stop()
{
	Clear_RootMotionDelta();

	m_bRawMode = false;
	m_iCurRawAnimationIndex = -1;
	m_fRawTime = 0.f;
	m_bRawLoop = false;

	m_bSequenceMode = false;
	m_iCurSequeneIndex = -1;
	m_iCurSequenceStepIndex = -1;
	m_bSequenceEndRequested = false;

	m_iCurClipIndex = -1;
	m_fLocalTime = 0.f;
	m_fPlayRate = 1.f;

	m_bPlaying = false;
	m_bEnded = false;
	m_bLoop = false;

	m_GameplayFlags = {};
	Clear_TriggeredNotifies();

	m_CurPose = m_BasePose;
}

void CAnimator::Pause()
{
	m_bPlaying = false;
}

void CAnimator::Resume()
{
	if (m_bEnded)
		return;

	if (m_bRawMode)
	{
		if (m_iCurRawAnimationIndex < 0)
			return;

		m_bPlaying = true;
		return;
	}

	if (m_iCurClipIndex < 0)
		return;

	if (m_bEnded)
		return;

	m_bPlaying = true;
}

void CAnimator::Request_Action(const _wstring& strActionName)
{
	Request_Action(Hash_String(strActionName.c_str()));
}

void CAnimator::Request_Action(_uint64 iActionHash)
{
	if (iActionHash == 0)
		return;

	m_PendingActionHashes.push_back(iActionHash);
}

_bool CAnimator::Was_ActionAcceptedThisFrame() const
{
	return m_bActionAcceptedThisFrame;
}

_bool CAnimator::Was_StateChangedThisFrame() const
{
	return m_bStateChangedThisFrame;
}
_uint64 CAnimator::Get_AcceptedActionHash() const
{
	return m_iAcceptedActionHash;
}
_uint64 CAnimator::Get_PrevControllerStateHash() const
{
	return m_iPrevControllerStateHash;
}
_uint64 CAnimator::Get_CurControllerStateHash() const
{
	return m_iCurControllerStateHash;
}
#pragma endregion


#pragma region PRIVATE_RUNTIME_API_CONTROLLER

_int CAnimator::Find_ControllerParamIndex(_uint64 iParamHash) const
{
	const auto& Params = m_EditData.Controller.Parameters;

	for (_uint i = 0; i < Params.size(); i++)
	{
		if (Params[i].iNameHash == iParamHash)
			return static_cast<_int>(i);
	}

	return -1;
}

_int CAnimator::Find_AutoControllerState()
{
	const ANIM_CONTROLLER& Controller = m_EditData.Controller;

	_int iBestState = -1;
	_int iBestPriority = INT_MIN;

	for (_uint i = 0; i < Controller.States.size(); i++)
	{
		const ANIM_CONTROLLER_STATE& State = Controller.States[i];

		if (State.eEntryMode != ANIM_STATE_ENTRY_MODE::AUTO && State.eEntryMode != ANIM_STATE_ENTRY_MODE::BOTH)
			continue;

		if (!Is_StateConditionPassed(State))
			continue;

		if (State.iPriority > iBestPriority)
		{
			iBestPriority = State.iPriority;
			iBestState = static_cast<_int>(i);
		}
	}

	return iBestState;
}

_int CAnimator::Find_RequestedControllerState()
{
	const ANIM_CONTROLLER& Controller = m_EditData.Controller;

	if (!m_bHasRequestedState)
		return -1;

	for (_uint i = 0; i < Controller.States.size(); i++)
	{
		const ANIM_CONTROLLER_STATE& State = Controller.States[i];

		if (State.iNameHash != m_iRequestedStateHash)
			continue;

		if (State.eEntryMode != ANIM_STATE_ENTRY_MODE::REQUEST && State.eEntryMode != ANIM_STATE_ENTRY_MODE::BOTH)
			return -1;

		if (!Can_EnterRequestedState(State))
			return -1;

		if (!Is_StateConditionPassed(State))
			return -1;

		return static_cast<_int>(i);
	}

	return -1;
}

_bool CAnimator::Should_HoldCurState() const
{
	const ANIM_CONTROLLER_STATE* pState = Get_CurControllerState();

	if (pState == nullptr)
		return false;

	switch (pState->eHoldRule)
	{
	case ANIM_STATE_HOLD_RULE::NONE:
		return false;

	case ANIM_STATE_HOLD_RULE::UNTIL_END:
		// 현재 클립이 아직 안 끝났으면 Auto State가 덮지 못하게 한다.
		return !m_bEnded;

	case ANIM_STATE_HOLD_RULE::LOOP_UNTIL_REQUEST:
		// 루프 상태는 명시 요청이 오기 전까지 Auto 전환으로 빠지지 않는다.
		return true;

	default:
		return false;
	}
}

_bool CAnimator::Can_RestartCurSequenceState() const
{
	const ANIM_CONTROLLER_STATE* pState = Get_CurControllerState();
	const ANIM_SEQUENCE* pSequence = Get_CurSequence();
	const ANIM_SEQUENCE_STEP* pStep = Get_CurSequenceStep();

	if (pState == nullptr || pSequence == nullptr || pStep == nullptr)
		return false;

	if (!m_bSequenceMode)
		return false;

	if (pState->eMotionType != ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)
		return false;

	if (pState->iMotionIndex != m_iCurSequeneIndex)
		return false;

	// Sequence 재시작 가능 시점은 State가 아니라 Sequence 정책이 결정한다.
	// 예: Run은 End 중 재입력 허용, 수리 overlay는 inactive 이후 재시작만 허용.
	if (!Can_RestartSequenceFromCurStep(*pSequence, *pStep))
		return false;

	// 현재 state의 진입 조건이 다시 만족되면 재시작
	if (!Is_StateConditionPassed(*pState))
		return false;

	return true;
}

_bool CAnimator::Try_RestartCurSequenceState()
{
	if (!Can_RestartCurSequenceState())
		return false;

	const ANIM_CONTROLLER_STATE* pState = Get_CurControllerState();
	const ANIM_SEQUENCE* pSequence = Get_CurSequence();
	const ANIM_SEQUENCE_STEP* pCurStep = Get_CurSequenceStep();

	if (pState == nullptr || pSequence == nullptr || pCurStep == nullptr)
		return false;

	if (pState->iMotionIndex < 0)
		return false;

	if (pSequence->Steps.empty())
		return false;

	const _int iFromClipIndex = m_iCurClipIndex;
	const _float fFromLocalTime = m_fLocalTime;

	const ANIM_SEQUENCE_STEP& ToStep = pSequence->Steps.front();

	if (ToStep.iClipIndex < 0 || ToStep.iClipIndex >= static_cast<_int>(m_EditData.ClipSettings.size()))
		return false;

	_float fBlendDuration = 0.f;

	if (iFromClipIndex >= 0 && iFromClipIndex < static_cast<_int>(m_EditData.ClipSettings.size()))
	{
		const ANIM_CLIP_SETTING& FromClip = m_EditData.ClipSettings[iFromClipIndex];
		const ANIM_CLIP_SETTING& ToClip = m_EditData.ClipSettings[ToStep.iClipIndex];

		if (pSequence->fReentryBlendTime > 0.f)
			fBlendDuration = pSequence->fReentryBlendTime;
		else
		{
			fBlendDuration = FromClip.fBlendOutTime;

			if (fBlendDuration <= 0.f)
				fBlendDuration = ToClip.fBlendInTime;
		}
	}

	_float fReentryStartOffsetTime = pSequence->fReentryStartOffsetTime;

	if (fReentryStartOffsetTime < 0.f)
		fReentryStartOffsetTime = 0.f;

	if (FAILED(Play_Sequence_ByIndex_Internal(static_cast<_uint>(pState->iMotionIndex), fReentryStartOffsetTime)))
		return false;

	if (iFromClipIndex >= 0 && fBlendDuration > 0.f)
		Begin_StateBlend(iFromClipIndex, fFromLocalTime, fBlendDuration, true);
	else
		Stop_StateBlend();

	return true;
}

void CAnimator::Clear_ControllerRequests()
{
	m_bHasRequestedState = false;
	m_iRequestedStateHash = 0;
}

void CAnimator::Ready_ControllerRuntime()
{
	const ANIM_CONTROLLER& Controller = m_EditData.Controller;

	m_ControllerParamValues.clear();
	m_ControllerParamValues.resize(Controller.Parameters.size());

	for (_uint i = 0; i < Controller.Parameters.size(); i++)
	{
		const ANIM_CONTROLLER_PARAMETER& Param = Controller.Parameters[i];
		CONTROLLER_PARAM_VALUE& Value = m_ControllerParamValues[i];

		Value.eType = Param.eType;
		Value.bBool = Param.bDefaultBool;
		Value.iInt = Param.iDefaultInt;
		Value.fFloat = Param.fDefaultFloat;
		Value.bTriggered = false;
	}

	m_iCurControllerStateIndex = -1;

	Clear_ControllerRequests();

	m_bControllerEnabled = !Controller.States.empty() && !m_EditData.ClipSettings.empty();

	Ready_PoseLayerRunTime();
}

_bool CAnimator::Is_ConditionPassed(const ANIM_CONTROLLER_CONDITION& Condition) const
{
	if (Condition.iParameterIndex < 0 || Condition.iParameterIndex >= m_ControllerParamValues.size())
		return false;

	const CONTROLLER_PARAM_VALUE& Value = m_ControllerParamValues[Condition.iParameterIndex];

	switch (Condition.eCompareType)
	{
	case ANIM_CONTROLLER_COMPARE_TYPE::True:
		return Value.eType == ANIM_CONTROLLER_PARAM_TYPE::BOOL && Value.bBool == true;

	case ANIM_CONTROLLER_COMPARE_TYPE::False:
		return Value.eType == ANIM_CONTROLLER_PARAM_TYPE::BOOL && Value.bBool == false;

	case ANIM_CONTROLLER_COMPARE_TYPE::TRIGGERED:
		return Value.eType == ANIM_CONTROLLER_PARAM_TYPE::TRIGGER && Value.bTriggered;

	case ANIM_CONTROLLER_COMPARE_TYPE::EQUAL:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt == Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return fabsf(Value.fFloat - Condition.fFloatValue) < 0.0001f;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::BOOL)
			return Value.bBool == Condition.bBoolValue;

		return false;

	case ANIM_CONTROLLER_COMPARE_TYPE::NOT_EQUAL:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt != Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return fabsf(Value.fFloat - Condition.fFloatValue) >= 0.0001f;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::BOOL)
			return Value.bBool != Condition.bBoolValue;

		return false;

	case ANIM_CONTROLLER_COMPARE_TYPE::GREATER:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt > Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return Value.fFloat > Condition.fFloatValue;

		return false;

	case ANIM_CONTROLLER_COMPARE_TYPE::GREATER_EQUAL:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt >= Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return Value.fFloat >= Condition.fFloatValue;

		return false;

	case ANIM_CONTROLLER_COMPARE_TYPE::LESS:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt < Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return Value.fFloat < Condition.fFloatValue;

		return false;

	case ANIM_CONTROLLER_COMPARE_TYPE::LESS_EQUAL:
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::INT)
			return Value.iInt <= Condition.iIntValue;

		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::FLOAT)
			return Value.fFloat <= Condition.fFloatValue;

		return false;

	default:
		return false;
	}
}

_bool CAnimator::Is_StateConditionPassed(const ANIM_CONTROLLER_STATE& State) const
{
	for (const auto& Condition : State.EnterConditions)
	{
		if (!Is_ConditionPassed(Condition))
			return false;
	}

	return true;
}

_bool CAnimator::Can_EnterRequestedState(const ANIM_CONTROLLER_STATE& State) const
{
	switch (State.eRequestRule)
	{
	case ANIM_STATE_REQUEST_RULE::ALWAYS:
		return true;

	case ANIM_STATE_REQUEST_RULE::COMBO_WINDOW:
		return Can_ComboInput();

	case ANIM_STATE_REQUEST_RULE::CANCEL_WINDOW:
		return Can_Cancel();

	case ANIM_STATE_REQUEST_RULE::GUARD_INPUT_WINDOW:
		return Can_Cancel() || Is_InWindow(ANIM_WINDOW_TYPE::GUARD_INPUT);

	case ANIM_STATE_REQUEST_RULE::DASH_INPUT_WINDOW:
		return Can_Cancel() || Is_InWindow(ANIM_WINDOW_TYPE::DASH_INPUT);

	case ANIM_STATE_REQUEST_RULE::NEVER:
	default:
		return false;
	}
}

HRESULT CAnimator::Enter_ControllerState(_int iStateIndex)
{
	if (iStateIndex < 0 || iStateIndex >= m_EditData.Controller.States.size())
		return E_FAIL;

	const ANIM_CONTROLLER_STATE& ToState = m_EditData.Controller.States[iStateIndex];

	if (iStateIndex == m_iCurControllerStateIndex && !ToState.bCanRestart)
		return S_OK;

	const _int iFromStateIndex = m_iCurControllerStateIndex;
	const _int iFromClipIndex = m_iCurClipIndex;
	const _float fFromLocalTime = m_fLocalTime;
	const POSE FromPose = m_CurPose;

	// 이전 state와 새 state의 clip 설정을 기준으로 전환 블렌드 시간을 계산한다.
	_float fBlendDuration = 0.f;

	if (iFromStateIndex >= 0 && iFromStateIndex < m_EditData.Controller.States.size())
	{
		const ANIM_CONTROLLER_STATE& FromState = m_EditData.Controller.States[iFromStateIndex];
		fBlendDuration = Get_StateBlendDuration(FromState, ToState);
	}

	HRESULT hr = E_FAIL;

	// State는 실제 재생 단위인 clip 또는 sequence로 풀어서 진입한다.
	if (ToState.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::CLIP)
	{
		if (ToState.iMotionIndex >= 0)
			hr = Play_Clip_ByIndex(ToState.iMotionIndex);
	}
	else if (ToState.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)
	{
		if (ToState.iMotionIndex >= 0)
			hr = Play_Sequence_ByIndex(ToState.iMotionIndex);
	}

	if (SUCCEEDED(hr))
	{
		m_iCurControllerStateIndex = iStateIndex;

		if (iFromClipIndex >= 0 && fBlendDuration > 0.f)
			Begin_StateBlendFromPose(iFromClipIndex, FromPose, fBlendDuration);
		else
			Stop_StateBlend();
	}

	return hr;
}

_float CAnimator::Get_StateBlendInTime(const ANIM_CONTROLLER_STATE& State) const
{
	if (State.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::CLIP)
	{
		if (State.iMotionIndex >= 0 && State.iMotionIndex < m_EditData.ClipSettings.size())
			return m_EditData.ClipSettings[State.iMotionIndex].fBlendInTime;
	}
	else if (State.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)
	{
		if (State.iMotionIndex >= 0 && State.iMotionIndex < m_EditData.Sequences.size())
		{
			const ANIM_SEQUENCE& Sequence = m_EditData.Sequences[State.iMotionIndex];

			if (!Sequence.Steps.empty())
			{
				const ANIM_SEQUENCE_STEP& Step = Sequence.Steps.front();

				if (Step.iClipIndex >= 0 && Step.iClipIndex < m_EditData.ClipSettings.size())
					return m_EditData.ClipSettings[Step.iClipIndex].fBlendInTime;
			}
		}
	}

	return 0.f;
}

_float CAnimator::Get_StateBlendOutTime(const ANIM_CONTROLLER_STATE& State) const
{
	if (State.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::CLIP)
	{
		if (State.iMotionIndex >= 0 && State.iMotionIndex < m_EditData.ClipSettings.size())
			return m_EditData.ClipSettings[State.iMotionIndex].fBlendOutTime;
	}
	else if (State.eMotionType == ANIM_CONTROLLER_MOTION_TYPE::SEQUENCE)
	{
		// 현재 재생 중인 step clip을 우선 사용.
		if (m_iCurClipIndex >= 0 && m_iCurClipIndex < m_EditData.ClipSettings.size())
			return m_EditData.ClipSettings[m_iCurClipIndex].fBlendOutTime;

		if (State.iMotionIndex >= 0 && State.iMotionIndex < m_EditData.Sequences.size())
		{
			const ANIM_SEQUENCE& Sequence = m_EditData.Sequences[State.iMotionIndex];

			if (!Sequence.Steps.empty())
			{
				const ANIM_SEQUENCE_STEP& Step = Sequence.Steps.front();

				if (Step.iClipIndex >= 0 && Step.iClipIndex < m_EditData.ClipSettings.size())
					return m_EditData.ClipSettings[Step.iClipIndex].fBlendOutTime;
			}
		}
	}

	return 0.f;
}

_float CAnimator::Get_StateBlendDuration(const ANIM_CONTROLLER_STATE& FromState, const ANIM_CONTROLLER_STATE& ToState) const
{
	return max(Get_StateBlendOutTime(FromState), Get_StateBlendInTime(ToState));
}

void CAnimator::Update_Controller()
{
	if (!m_bControllerEnabled)
		return;

	Reset_ActionResult();

	// 1. 최초 진입
	if (m_iCurControllerStateIndex < 0)
	{
		const _int iEntry = m_EditData.Controller.iEntryStateIndex;

		if (iEntry >= 0)
			Enter_ControllerState(iEntry);

		Clear_ConsumedTriggers();
		Clear_ControllerRequests();
		Clear_ActionRequests();

		return;
	}

	_uint64 iAcceptedActionHash = 0;
	const _int iActionState = Find_ActionRequestedControllerState(iAcceptedActionHash);

	if (iActionState >= 0)
	{
		const _int iPrevStateIndex = m_iCurControllerStateIndex;

		if (SUCCEEDED(Enter_ControllerState(iActionState)))
		{
			m_bActionAcceptedThisFrame = true;
			m_iAcceptedActionHash = iAcceptedActionHash;

			if (iPrevStateIndex >= 0 && iPrevStateIndex < m_EditData.Controller.States.size())
				m_iPrevControllerStateHash = m_EditData.Controller.States[iPrevStateIndex].iNameHash;

			if (m_iCurControllerStateIndex >= 0 && m_iCurControllerStateIndex < m_EditData.Controller.States.size())
				m_iCurControllerStateHash = m_EditData.Controller.States[m_iCurControllerStateIndex].iNameHash;

			m_bStateChangedThisFrame = iPrevStateIndex != m_iCurControllerStateIndex;
		}

		Clear_ConsumedTriggers();
		Clear_ControllerRequests();
		Clear_ActionRequests();
		
		return;
	}

	// 2. 명시 요청 처리
	//    요청 상태는 RequestRule을 통해 현재 Clip Window를 검사한다.
	const _int iRequestedState = Find_RequestedControllerState();

	if (iRequestedState >= 0)
	{
		Enter_ControllerState(iRequestedState);

		Clear_ConsumedTriggers();
		Clear_ControllerRequests();
		Clear_ActionRequests();

		return;
	}

	// 3. END step 중 현재 state 조건이 다시 만족되면 sequence를 처음부터 재시작한다.
	if (Try_RestartCurSequenceState())
	{
		Clear_ConsumedTriggers();
		Clear_ControllerRequests();
		Clear_ActionRequests();

		return;
	}

	// 4. 현재 상태 유지
	//    일회성 Clip이 끝나기 전에 Idle/Move 같은 Auto 상태가 덮는 것을 막는다.

	if (Should_HoldCurState())
	{
		Clear_ConsumedTriggers();
		Clear_ControllerRequests();
		Clear_ActionRequests();

		return;

	}

	// 5. 자동 상태 처리
	const _int iAutoState = Find_AutoControllerState();

	if (iAutoState >= 0)
		Enter_ControllerState(iAutoState);

	Clear_ConsumedTriggers();
	Clear_ControllerRequests();
	Clear_ActionRequests();
}

void CAnimator::Clear_ConsumedTriggers()
{
	for (auto& Value : m_ControllerParamValues)
	{
		if (Value.eType == ANIM_CONTROLLER_PARAM_TYPE::TRIGGER)
			Value.bTriggered = false;
	}
}
_int CAnimator::Find_ActionRequestedControllerState(_uint64& OutAcceptedActionHash)
{
	const ANIM_CONTROLLER& Controller = m_EditData.Controller;

	_int iBastState = -1;
	_int iBestPriority = INT_MIN;
	OutAcceptedActionHash = 0;

	for (_uint64 iActionHash : m_PendingActionHashes)
	{
		for (const auto& Map : Controller.ActionStateMaps)
		{
			if (Map.iActionHash != iActionHash)
				continue;

			if (Map.iStateIndex < 0 || Map.iStateIndex >= Controller.States.size())
				continue;

			const ANIM_CONTROLLER_STATE& State = Controller.States[Map.iStateIndex];

			if (State.eEntryMode != ANIM_STATE_ENTRY_MODE::REQUEST && State.eEntryMode != ANIM_STATE_ENTRY_MODE::BOTH)
				continue;

			if (!Can_EnterActionRequestedState(State, iActionHash))
				continue;

			if (!Is_StateConditionPassed(State))
				continue;

			if (State.iPriority > iBestPriority)
			{
				iBestPriority = State.iPriority;
				iBastState = Map.iStateIndex;
				OutAcceptedActionHash = iActionHash;
			}
		}
	}

	return iBastState;
}
void CAnimator::Clear_ActionRequests()
{
	m_PendingActionHashes.clear();
}
void CAnimator::Reset_ActionResult()
{
	m_bActionAcceptedThisFrame = false;
	m_bStateChangedThisFrame = false;
	m_iAcceptedActionHash = 0;
	m_iPrevControllerStateHash = 0;
	m_iCurControllerStateHash = 0;
}
_bool CAnimator::Is_AutoLikeControllerState(const ANIM_CONTROLLER_STATE& State) const
{
	return State.eEntryMode == ANIM_STATE_ENTRY_MODE::AUTO ||
		State.eEntryMode == ANIM_STATE_ENTRY_MODE::BOTH;
}
_bool CAnimator::Is_ControllerStateInAction(_int iStateIndex, _uint64 iActionHash) const
{
	if (iStateIndex < 0 || iStateIndex >= static_cast<_int>(m_EditData.Controller.States.size()))
		return false;

	for (const ANIM_ACTION_STATE_MAP& Map : m_EditData.Controller.ActionStateMaps)
	{
		if (Map.iActionHash != iActionHash)
			continue;

		if (Map.iStateIndex == iStateIndex)
			return true;
	}

	return false;
}

_bool CAnimator::Can_EnterActionRequestedState(const ANIM_CONTROLLER_STATE& State, _uint64 iActionHash) const
{
	const ANIM_CONTROLLER_STATE* pCurState = Get_CurControllerState();

	if (pCurState == nullptr)
		return Can_EnterRequestedState(State);

	// Walk -> Run -> Sprint처럼 같은 Action 그룹 안에서 고르는 이동 계열 전환.
	// 이 경우 Cancel Window를 요구하면 Run/Sprint 전환이 지나치게 늦어진다.
	if (Is_ControllerStateInAction(m_iCurControllerStateIndex, iActionHash))
		return true;

	// Idle/Walk/Run/Sprint 같은 Auto 계열 상태에서 Roll/Dodge/Attack 같은 요청이 들어오면 즉시 허용.
	// 큰 동작 중일 때만 ToState의 RequestRule을 본다.
	if (Is_AutoLikeControllerState(*pCurState))
		return true;

	return Can_EnterRequestedState(State);
}
#pragma endregion


#pragma region PRIVATE_RUNTIME_API_SEQUENCE

_int CAnimator::Find_SequenceEndStepIndex() const
{
	const ANIM_SEQUENCE* pSequence = Get_CurSequence();
	if (pSequence == nullptr)
		return -1;

	return Find_SequenceStepIndexByType(*pSequence, ANIM_SEQUENCE_STEP_TYPE::END, m_iCurSequenceStepIndex);
}

_bool CAnimator::Is_SequenceStepHold(const ANIM_SEQUENCE_STEP& Step) const
{
	for (const auto& Condition : Step.HoldConditions)
	{
		if (!Is_ConditionPassed(Condition))
			return false;
	}

	return true;
}

_bool CAnimator::Should_BlendSequenceStep(const ANIM_SEQUENCE& Sequence, const ANIM_SEQUENCE_STEP& FromStep, const ANIM_SEQUENCE_STEP& ToStep) const
{
	// Sequence 설정이 step 사이의 직접 블렌드 허용 범위를 결정한다.
	// 공격 같은 단발형은 NONE, 달리기 종료처럼 발 맞춤이 필요한 경우는 LOOP_TO_END_ONLY를 사용한다.
	switch (Sequence.eStepBlendMode)
	{
	case ANIM_SEQUENCE_STEP_BLEND_MODE::NONE:
		return false;

	case ANIM_SEQUENCE_STEP_BLEND_MODE::LOOP_TO_END_ONLY:
		return FromStep.eStepType == ANIM_SEQUENCE_STEP_TYPE::LOOP && ToStep.eStepType == ANIM_SEQUENCE_STEP_TYPE::END;

	case ANIM_SEQUENCE_STEP_BLEND_MODE::ALL_STEPS:
		return true;

	default:
		return true;
	}
}

HRESULT CAnimator::Play_Clip_ByIndex_Internal(_uint iClipIndex, _bool bKeepSequenceState)
{
	if (iClipIndex >= m_EditData.ClipSettings.size())
		return E_FAIL;

	if (!bKeepSequenceState)
	{
		m_bSequenceMode = false;
		m_iCurSequeneIndex = -1;
		m_iCurSequenceStepIndex = -1;
		m_bSequenceEndRequested = false;
	}

	m_bRawMode = false;
	m_iCurRawAnimationIndex = -1;
	m_fRawTime = 0.f;
	m_bRawLoop = false;

	m_iCurClipIndex = static_cast<_int>(iClipIndex);
	m_fLocalTime = 0.f;

	const ANIM_CLIP_SETTING& Clip = m_EditData.ClipSettings[iClipIndex];

	m_bLoop = Clip.bLoop;
	m_fPlayRate = Clip.fPlayRate;
	m_bUseRootMotion = Clip.bUseRootMotion;

	m_bPlaying = true;
	m_bEnded = false;

	Clear_RootMotionDelta();
	Update_GameplayFlags();
	Clear_TriggeredNotifies();
	Update_CurPose();

	return S_OK;
}

HRESULT CAnimator::Enter_SequenceStep(_uint iStepIndex)
{
	const ANIM_SEQUENCE* pSequence = Get_CurSequence();
	if (pSequence == nullptr)
		return E_FAIL;

	if (iStepIndex >= pSequence->Steps.size())
		return E_FAIL;

	const ANIM_SEQUENCE_STEP& Step = pSequence->Steps[iStepIndex];

	if (Step.iClipIndex < 0 || Step.iClipIndex >= m_EditData.ClipSettings.size())
		return E_FAIL;

	const _int iFromClipIndex = m_iCurClipIndex;
	const _float fFromLocalTime = m_fLocalTime;
	const _bool bInternalStepChange = m_iCurSequenceStepIndex >= 0;
	ANIM_SEQUENCE_STEP FromStep = {};

	if (bInternalStepChange)
		FromStep = *Get_CurSequenceStep();

	m_iCurSequenceStepIndex = static_cast<_int>(iStepIndex);

	if (FAILED(Play_Clip_ByIndex_Internal(static_cast<_uint>(Step.iClipIndex), true)))
		return E_FAIL;

	if (Step.eStepType == ANIM_SEQUENCE_STEP_TYPE::LOOP)
		m_bLoop = true;
	else
		m_bLoop = false;

	if (bInternalStepChange &&
		iFromClipIndex >= 0 &&
		iFromClipIndex != Step.iClipIndex &&
		iFromClipIndex < m_EditData.ClipSettings.size() &&
		Should_BlendSequenceStep(*pSequence, FromStep, Step))
	{
		const ANIM_CLIP_SETTING& FromClip = m_EditData.ClipSettings[iFromClipIndex];
		const ANIM_CLIP_SETTING& ToClip = m_EditData.ClipSettings[Step.iClipIndex];

		const _float fBlendDuration = Get_SequenceStepBlendDuration(*pSequence, FromClip, ToClip, FromStep, Step);

		if (fBlendDuration > 0.f)
			Begin_StateBlend(iFromClipIndex, fFromLocalTime, fBlendDuration);
	}

	return S_OK;
}

_bool CAnimator::Update_Sequence()
{
	if (!m_bSequenceMode)
		return false;

	const ANIM_SEQUENCE* pSequence = Get_CurSequence();
	const ANIM_SEQUENCE_STEP* pStep = Get_CurSequenceStep();

	if (pSequence == nullptr || pStep == nullptr)
	{
		Stop_Sequence();
		return false;
	}

	// 외부에서 End 요청이 들어오면 현재 step 종류와 무관하게 가장 가까운 End step으로 보낸다.
	if (m_bSequenceEndRequested)
	{
		m_bSequenceEndRequested = false;

		const _int iEndStep = Find_SequenceEndStepIndex();

		if (iEndStep >= 0)
			return SUCCEEDED(Enter_SequenceStep(static_cast<_uint>(iEndStep)));

		Stop_Sequence();

		return false;
	}

	// Loop step은 hold 조건이 살아 있으면 계속 반복하고, 끊기면 End step으로 넘어가는 로직에서 
	// 다음 스텝으로 넘어가도록 변경함
	if (pStep->eStepType == ANIM_SEQUENCE_STEP_TYPE::LOOP)
	{
		/*if (!Is_SequenceStepHold(*pStep))
		{
			const _int iEndStep = Find_SequenceEndStepIndex();

			if (iEndStep >= 0)
				return SUCCEEDED(Enter_SequenceStep(static_cast<_uint>(iEndStep)));

			Stop_Sequence();

			return false;
		}*/

		if (!Is_SequenceStepHold(*pStep))
		{
			const _int iNextStep = m_iCurSequenceStepIndex + 1;

			if (iNextStep < static_cast<_int>(pSequence->Steps.size()))
				return SUCCEEDED(Enter_SequenceStep(static_cast<_uint>(iNextStep)));

			Stop_Sequence();
			return false;
		}
		return false;
	}

	if (!m_bEnded)
		return false;

	// Start/Active 같은 일반 step은 clip이 끝났을 때 다음 step으로 진행한다.
	if (pStep->eStepType == ANIM_SEQUENCE_STEP_TYPE::END)
	{
		Stop_Sequence();
		return false;
	}

	const _int iNextStep = m_iCurSequenceStepIndex + 1;

	if (iNextStep >= pSequence->Steps.size())
	{
		Stop_Sequence();
		return false;
	}

	return SUCCEEDED(Enter_SequenceStep(static_cast<_uint>(iNextStep)));
}

void CAnimator::Stop_Sequence()
{
	m_bSequenceMode = false;
	m_iCurSequeneIndex = -1;
	m_iCurSequenceStepIndex = -1;
	m_bSequenceEndRequested = false;
}

_float CAnimator::Get_SequenceStepBlendDuration(const ANIM_SEQUENCE& Sequence, const ANIM_CLIP_SETTING& FromClip, const ANIM_CLIP_SETTING& ToClip, const ANIM_SEQUENCE_STEP& FromStep, const ANIM_SEQUENCE_STEP& ToStep) const
{
	if (FromStep.eStepType == ANIM_SEQUENCE_STEP_TYPE::LOOP && ToStep.eStepType == ANIM_SEQUENCE_STEP_TYPE::END && Sequence.fLoopToEndBlendTime > 0.f)
		return Sequence.fLoopToEndBlendTime;

	return max(FromClip.fBlendOutTime, ToClip.fBlendInTime);
}

_bool CAnimator::Can_RestartSequenceFromCurStep(const ANIM_SEQUENCE& Sequence, const ANIM_SEQUENCE_STEP& Step) const
{
	if (Step.eStepType != ANIM_SEQUENCE_STEP_TYPE::END)
		return false;

	switch (Sequence.eRestartRule)
	{
	case ANIM_SEQUENCE_RESTART_RULE::ANY_TIME:
		return Sequence.bAllowEndToLoopReentry;

	case ANIM_SEQUENCE_RESTART_RULE::WHILE_START_CANCELABLE:
		return false;

	case ANIM_SEQUENCE_RESTART_RULE::AFTER_INACTIVE:
		return false;

	default:
		return false;
	}
}

HRESULT CAnimator::Jump_SequenceStep_ForPreview(_uint iSequenceIndex, _uint iStepIndex, _float fStepLocalTime)
{
	if (iSequenceIndex >= m_EditData.Sequences.size())
		return E_FAIL;

	const ANIM_SEQUENCE& Sequence = m_EditData.Sequences[iSequenceIndex];

	if (iStepIndex >= Sequence.Steps.size())
		return E_FAIL;

	const ANIM_SEQUENCE_STEP& Step = Sequence.Steps[iStepIndex];

	if (Step.iClipIndex < 0 || Step.iClipIndex >= static_cast<_int>(m_EditData.ClipSettings.size()))
		return E_FAIL;

	Stop_StateBlend();
	Clear_RootMotionDelta();
	Clear_TriggeredNotifies();

	m_bRawMode = false;
	m_iCurRawAnimationIndex = -1;
	m_fRawTime = 0.f;
	m_bRawLoop = false;

	m_bSequenceMode = true;
	m_iCurSequeneIndex = static_cast<_int>(iSequenceIndex);
	m_iCurSequenceStepIndex = static_cast<_int>(iStepIndex);
	m_bSequenceEndRequested = false;

	m_iCurClipIndex = Step.iClipIndex;

	const ANIM_CLIP_SETTING& Clip = m_EditData.ClipSettings[Step.iClipIndex];
	const _float fDuration = Calc_ClipDuration(Clip);

	if (fStepLocalTime < 0.f)
		fStepLocalTime = 0.f;

	if (fDuration > 0.f && fStepLocalTime > fDuration)
		fStepLocalTime = fDuration;

	m_fLocalTime = fStepLocalTime;
	m_fPlayRate = Clip.fPlayRate;
	m_bLoop = Step.eStepType == ANIM_SEQUENCE_STEP_TYPE::LOOP;
	m_bUseRootMotion = Clip.bUseRootMotion;
	m_bPlaying = false;
	m_bEnded = false;

	Update_GameplayFlags();
	Update_CurPose();

	Apply_SequenceStepBlend_ForPreview(Sequence, iStepIndex, fStepLocalTime);
	Update_PoseLayers(0.f);

	return S_OK;
}

void CAnimator::Apply_SequenceStepBlend_ForPreview(const ANIM_SEQUENCE& Sequence, _uint iStepIndex, _float fStepLocalTime)
{
	if (iStepIndex == 0)
		return;

	if (iStepIndex >= Sequence.Steps.size())
		return;

	const ANIM_SEQUENCE_STEP& ToStep = Sequence.Steps[iStepIndex];
	const ANIM_SEQUENCE_STEP& FromStep = Sequence.Steps[iStepIndex - 1];

	if (!Should_BlendSequenceStep(Sequence, FromStep, ToStep))
		return;

	if (FromStep.iClipIndex < 0 || FromStep.iClipIndex >= static_cast<_int>(m_EditData.ClipSettings.size()))
		return;

	if (ToStep.iClipIndex < 0 || ToStep.iClipIndex >= static_cast<_int>(m_EditData.ClipSettings.size()))
		return;

	const ANIM_CLIP_SETTING& FromClip = m_EditData.ClipSettings[FromStep.iClipIndex];
	const ANIM_CLIP_SETTING& ToClip= m_EditData.ClipSettings[ToStep.iClipIndex];

	const _float fBlendDuration = Get_SequenceStepBlendDuration(Sequence, FromClip, ToClip, FromStep, ToStep);

	if (fBlendDuration <= 0.f)
		return;

	if (fStepLocalTime > fBlendDuration)
		return;

	const _float fFromLocalTime = Calc_ClipDuration(FromClip);

	if (!Sample_ClipPose(FromClip, fFromLocalTime, m_BlendFromPose, m_BlendFromKeyFrameIndices, true))
		return;

	m_bStateBlending = true;
	m_bBlendFromPoseFrozen = true;
	m_iBlendFromClipIndex = FromStep.iClipIndex;
	m_fBlendFromLocalTime = fFromLocalTime;
	m_fBlendElapsed = fStepLocalTime;
	m_fBlendDuration = fBlendDuration;

	Update_StateBlend(0.f);
}

#pragma endregion


#pragma region PRIVATE_RUNTIME_API_SAMPLING_AND_TIME
CAnimator::ANIM_TIME_ADVANCE_RESULT CAnimator::Advance_ClipLocalTime(const ANIM_CLIP_SETTING& Clip, _float fTimeDelta, _float fPlayRate, _bool bLoop, _float& InOutLocalTime) const
{
	ANIM_TIME_ADVANCE_RESULT Result = {};

	const _float fDuration = Calc_ClipDuration(Clip);
	if (fDuration <= 0.f)
	{
		InOutLocalTime = 0.f;
		Result.bEnded = true;
		return Result;
	}

	if (fTimeDelta > 0.f && fPlayRate > 0.f)
		InOutLocalTime += fTimeDelta * 1000.f * fPlayRate;

	if (InOutLocalTime >= fDuration)
	{
		if (bLoop)
		{
			InOutLocalTime = fmodf(InOutLocalTime, fDuration);
			Result.bLooped = true;
		}
		else
		{
			InOutLocalTime = fDuration;
			Result.bEnded = true;
		}
	}

	return Result;
}

_bool CAnimator::Sample_ClipPose(const ANIM_CLIP_SETTING& Clip, _float fLocalTime, POSE& OutPose, vector<KeyFrameIndices>& KeyFrameCache, _bool bApplyRootMotionStrip) const
{
	if (!Is_ValidRawAnimationIndex(Clip.iRawAnimationIndex))
		return false;

	CAnimation* pAnimation = m_Animations[Clip.iRawAnimationIndex];
	OutPose = m_BasePose;

	if (KeyFrameCache.size() != OutPose.size())
		KeyFrameCache.resize(OutPose.size());

	const _float fRawTime = Calc_LocalToRawTime(Clip, fLocalTime);
	pAnimation->Get_Pose(OutPose, fRawTime, KeyFrameCache);

	if (bApplyRootMotionStrip && m_iRootBoneIndex >= 0)
		Strip_RootMotion(OutPose, Clip);

	return true;
}

_int CAnimator::Find_SequenceStepIndexByType(const ANIM_SEQUENCE& Sequence, ANIM_SEQUENCE_STEP_TYPE eStepType, _int iStartAfter) const
{
	for (_int i = iStartAfter + 1; i < Sequence.Steps.size(); i++)
	{
		if (Sequence.Steps[i].eStepType == eStepType)
			return i;
	}

	for (_int i = 0; i < Sequence.Steps.size(); i++)
	{
		if (Sequence.Steps[i].eStepType == eStepType)
			return i;
	}

	return -1;
}

_bool CAnimator::Sample_Pose(const ANIM_CLIP_SETTING& Clip, _float fLocalTime, POSE& OutPose) const
{
	vector<KeyFrameIndices> TempKeyFrameIndices;

	return Sample_ClipPose(Clip, fLocalTime, OutPose, TempKeyFrameIndices, false);
}

_float CAnimator::Calc_StartTime(const ANIM_CLIP_SETTING& Clip) const
{
	if (Clip.iSourceFrameCount <= 1)
		return 0.f;

	const _float fFrameTime = Clip.fSourceDuration / static_cast<_float>(Clip.iSourceFrameCount - 1);

	return static_cast<_float>(Clip.iStartFrame) * fFrameTime;
}

_float CAnimator::Calc_ClipDuration(const ANIM_CLIP_SETTING& Clip) const
{
	if (Clip.iSourceFrameCount <= 1)
		return Clip.fSourceDuration;

	if (Clip.iEndFrame < Clip.iStartFrame)
		return Clip.fSourceDuration;

	if (Clip.iEndFrame == Clip.iStartFrame)
		return 0.f;

	const _float fFrameTime = Clip.fSourceDuration / static_cast<_float>(Clip.iSourceFrameCount - 1);

	return static_cast<_float>(Clip.iEndFrame - Clip.iStartFrame) * fFrameTime;
}

_uint CAnimator::Calc_ClipFrameCount(const ANIM_CLIP_SETTING& Clip) const
{
	if (Clip.iSourceFrameCount == 0)
		return 0;

	if (Clip.iEndFrame < Clip.iStartFrame)
		return Clip.iSourceFrameCount;

	return Clip.iEndFrame - Clip.iStartFrame + 1;
}

_float CAnimator::Calc_LocalToRawTime(const ANIM_CLIP_SETTING& Clip, _float fLocalTime) const
{
	const _float fSampleLocalTime = Calc_SampleLocalTime(Clip, fLocalTime);

	return Calc_StartTime(Clip) + fSampleLocalTime;
}

_float CAnimator::Calc_LocalFramePosition() const
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return 0.f;

	const _uint iFrameCount = Calc_ClipFrameCount(*pClip);

	if (iFrameCount <= 1)
		return 0.f;

	const _float fDuration = Calc_ClipDuration(*pClip);

	if (fDuration <= 0.f)
		return 0.f;

	_float fRatio = m_fLocalTime / fDuration;

	if (fRatio < 0.f)
		fRatio = 0.f;

	if (fRatio > 1.f)
		fRatio = 1.f;

	return fRatio * static_cast<_float>(iFrameCount - 1);
}

// 기본 play rate 위에 speed section을 적용한다.
// speed section 값은 곱셈 보정이 아니라 해당 구간에서 사용할 최종 재생 속도로 해석한다.
_float CAnimator::Calc_EffectivePlayRate(const ANIM_CLIP_SETTING& Clip) const
{
	_float fRate = m_fPlayRate;

	const _uint iFrame = Get_LocalFrame();

	for (const ANIM_SPEED_SECTION& Section : Clip.SpeedSections)
	{
		if (iFrame < Section.iStartFrame || iFrame > Section.iEndFrame)
			continue;

		if (Section.fPlayRate > 0.f)
			fRate = Section.fPlayRate;

		break;
	}

	return fRate;
}

_float CAnimator::Calc_SampleLocalTime(const ANIM_CLIP_SETTING& Clip, _float fElapsedLocalTime) const
{
	const _float fDuration = Calc_ClipDuration(Clip);

	if (fDuration <= 0.f)
		return 0.f;

	if (fElapsedLocalTime < 0.f)
		fElapsedLocalTime = 0.f;

	if (fElapsedLocalTime > fDuration)
		fElapsedLocalTime = fDuration;

	if (Clip.bReverse)
		return fDuration - fElapsedLocalTime;

	return fElapsedLocalTime;
}

_float CAnimator::Calc_SampleFramePosition(const ANIM_CLIP_SETTING& Clip, _float fElapsedLocalTime) const
{
	const _uint iFrameCount = Calc_ClipFrameCount(Clip);

	if (iFrameCount <= 1)
		return 0.f;

	const _float fDuration = Calc_ClipDuration(Clip);

	if (fDuration <= 0.f)
		return 0.f;

	const _float fSampleLocalTime = Calc_SampleLocalTime(Clip, fElapsedLocalTime);

	_float fRatio = fSampleLocalTime / fDuration;

	if (fRatio < 0.f)
		fRatio = 0.f;

	if (fRatio > 1.f)
		fRatio = 1.f;

	return fRatio * static_cast<_float>(iFrameCount - 1);
}

_float CAnimator::Calc_CurSampleFramePosition() const
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return 0.f;

	return Calc_SampleFramePosition(*pClip, m_fLocalTime);
}

// local time은 clip 내부 시간이며 raw animation time이 아니다.
// fTimeDelta는 초 단위로 들어오므로 현재 animation 데이터 기준에 맞춰 ms로 변환해 누적한다.
CAnimator::ANIM_TIME_ADVANCE_RESULT CAnimator::Update_Time(_float fTimeDelta)
{
	ANIM_TIME_ADVANCE_RESULT Result = {};

	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr)
		return Result;

	Result = Advance_ClipLocalTime(*pClip, fTimeDelta, Calc_EffectivePlayRate(*pClip), m_bLoop, m_fLocalTime);

	if (Result.bEnded)
	{
		m_bPlaying = false;
		m_bEnded = true;
	}

	return Result;
}

// 현재 clip이 참조하는 원본 animation을 raw time으로 sampling해 m_CurPose를 만든다.
// root motion preview를 끄면 root bone translation은 base pose 값으로 되돌린다.
void CAnimator::Update_CurPose()
{
	if (m_bRawMode)
	{
		Update_RawPose();
		return;
	}

	const ANIM_CLIP_SETTING* pClip = Get_CurClip();
	if (pClip == nullptr)
	{
		m_CurPose = m_BasePose;
		return;
	}

	if (!Sample_ClipPose(*pClip, m_fLocalTime, m_CurPose, m_KeyFrameIndices, true))
		m_CurPose = m_BasePose;
}

// 현재 local frame이 포함된 window들을 gameplay flag로 변환한다.
// 여러 window가 겹치면 flag는 누적되고, DAMAGE_REDUCE만 fValue를 피해 배율로 사용한다.
void CAnimator::Update_GameplayFlags()
{
	m_ActiveCustomWindows.clear();

	m_GameplayFlags = {};

	const ANIM_CLIP_SETTING* pClip = Get_CurClip();
	if (pClip == nullptr)
		return;

	const _uint iFrame = Get_LocalFrame();

	for (const ANIM_WINDOW& Window : pClip->Windows)
	{
		if (iFrame < Window.iStartFrame || iFrame > Window.iEndFrame)
			continue;

		switch (Window.eType)
		{
		case ANIM_WINDOW_TYPE::MOVE_LOCK:
			m_GameplayFlags.bCanMove = false;
			break;
		case ANIM_WINDOW_TYPE::ROTATE_LOCK:
			m_GameplayFlags.bCanRotate = false;
			break;
		case ANIM_WINDOW_TYPE::TARGET_TRACKING_LOCK:
			m_GameplayFlags.bCanTargetTrack = false;
			break;
		case ANIM_WINDOW_TYPE::HIT_ACTIVE:
			m_GameplayFlags.bHitActive = true;
			break;
		case ANIM_WINDOW_TYPE::INVINCIBLE:
			m_GameplayFlags.bInvincible = true;
			break;
		case ANIM_WINDOW_TYPE::SUPER_ARMOR:
			m_GameplayFlags.bSuperArmor = true;
			break;
		case ANIM_WINDOW_TYPE::GUARD_ACTIVE:
			m_GameplayFlags.bGuardActive = true;
			break;
		case ANIM_WINDOW_TYPE::PARRY_ACTIVE:
			m_GameplayFlags.bParryActive = true;
			break;
		case ANIM_WINDOW_TYPE::DAMAGE_REDUCE:
			if (Window.fValue > 0.f)
				m_GameplayFlags.fDamageRate = Window.fValue;
			break;
		case ANIM_WINDOW_TYPE::CUSTOM:
			m_ActiveCustomWindows.push_back(&Window);
			continue;
		}
	}
}

// 이전 frame과 현재 frame 사이를 지나간 notify만 이번 update 결과로 수집한다.
// loop로 frame 위치가 뒤로 돌아간 경우 끝 구간과 시작 구간을 나눠 검사한다.
void CAnimator::Update_TriggeredNotifies(_float fPrevFrame, _float fCurFrame, _bool bLooped)
{
	const ANIM_CLIP_SETTING* pClip = Get_CurClip();

	if (pClip == nullptr || pClip->Notifies.empty())
		return;

	const _uint iFrameCount = Calc_ClipFrameCount(*pClip);

	if (iFrameCount <= 0)
		return;

	const _float fLastFrame = static_cast<_float>(iFrameCount - 1);

	if (fabsf(fPrevFrame - fCurFrame) < 0.0001f && !bLooped)
		return;

	if (!bLooped)
	{
		if (fCurFrame > fPrevFrame)
		{
			// 정방향 Sample 진행
			for (const ANIM_NOTIFY& Notify : pClip->Notifies)
			{
				const _float fNotifyFrame = static_cast<_float>(Notify.iFrame);

				if (fNotifyFrame > fPrevFrame && fNotifyFrame <= fCurFrame)
					m_TriggeredNotifies.push_back(&Notify);
			}
		}
		else
		{
			// 역방향 Sample 진행
			for (const ANIM_NOTIFY& Notify : pClip->Notifies)
			{
				const _float fNotifyFrame = static_cast<_float>(Notify.iFrame);

				if (fNotifyFrame < fPrevFrame && fNotifyFrame >= fCurFrame)
					m_TriggeredNotifies.push_back(&Notify);
			}
		}
		return;
	}

	if (!pClip->bReverse)
	{
		// 정방향 loop : Prev -> Last, 0 -> Cur
		for (const ANIM_NOTIFY& Notify : pClip->Notifies)
		{
			const _float fNotifyFrame = static_cast<_float>(Notify.iFrame);

			if ((fNotifyFrame > fPrevFrame && fNotifyFrame <= fLastFrame) || (fNotifyFrame >= 0.f && Notify.iFrame <= fCurFrame))
				m_TriggeredNotifies.push_back(&Notify);
		} 
	}
	else
	{
		// 역방향 loop : Prev -> 0, Last -> Cur
		for (const ANIM_NOTIFY& Notify : pClip->Notifies)
		{
			const _float fNotifyFrame = static_cast<_float>(Notify.iFrame);

			if ((fNotifyFrame < fPrevFrame && fNotifyFrame >= 0.f) || (fNotifyFrame <= fLastFrame && Notify.iFrame >= fCurFrame))
				m_TriggeredNotifies.push_back(&Notify);
		}
	}
}

void CAnimator::Clear_TriggeredNotifies()
{
	m_TriggeredNotifies.clear();
}

#pragma endregion

// Animator는 Model을 소유하지 않고 원본 animation 리소스와 base pose만 받는다.
// pose 계산에 필요한 CAnimation 참조는 AddRef로 수명을 보장한다.
HRESULT CAnimator::Initialize(const ANIMATOR_DESC& Desc)
{
	m_BasePose = Desc.BasePose;
	// 애니메이션이 없는 Skinned 모델도 첫 프레임부터 유효한 Bone Pose를 사용한다.
	m_CurPose = m_BasePose;

	m_Bones = Desc.Bones;
	m_PreTransformMatrix = Desc.PreTransformMatrix;

	Clear_RootMotionDelta();

	if (!Desc.RawAnimationLoadDesc.strAnimationBaseName.empty())
	{
		if (FAILED(Ready_RawAnimations(Desc.RawAnimationLoadDesc)))
		{
			MSG_BOX("Is Animation Empty");
			return E_FAIL;
		}

		if (Desc.bUseEditData)
			Set_EditData(Desc.EditData);
		else
			Try_LoadEditData_FromFile(Desc.RawAnimationLoadDesc);
	}
	else if (Desc.bUseEditData)
	{
		Set_EditData(Desc.EditData);
	}

	return S_OK;
}

// 한 tick 재생 흐름.
// 이전 frame 위치를 저장한 뒤 시간을 진행시키고, window flag/notify/pose를 현재 frame 기준으로 다시 평가한다.
void CAnimator::Update(_float fTimeDelta)
{
	Clear_TriggeredNotifies();
	m_ActiveCustomWindows.clear();

	if (m_bControllerEnabled)
		Update_Controller();

	if (m_bPlaying == false)
		return;

	if (m_bRawMode)
	{
		Update_RawTime(fTimeDelta);
		Update_RawPose();
		return;
	}

	if (m_iCurClipIndex < 0)
		return;

	const ANIM_CLIP_SETTING* pBeforeClip = Get_CurClip();

	_float fPrevFrame = 0.f;

	if (pBeforeClip != nullptr)
		fPrevFrame = Calc_SampleFramePosition(*pBeforeClip, m_fLocalTime);

	const _float fPrevLocalTime = m_fLocalTime;

	// 1. 현재 clip의 local time을 먼저 진행한다.
	const ANIM_TIME_ADVANCE_RESULT TimeResult = Update_Time(fTimeDelta);

	_bool bStepChanged = false;

	// 2. sequence라면 시간 진행 결과에 따라 step을 바꿀 수 있다.
	if (m_bSequenceMode)
		bStepChanged = Update_Sequence();

	const ANIM_CLIP_SETTING* pAfterClip = Get_CurClip();

	_float fCurFrame = 0.f;

	if (pAfterClip != nullptr)
		fCurFrame = Calc_SampleFramePosition(*pAfterClip, m_fLocalTime);

	const _float fCurLocalTime = m_fLocalTime;

	// 3. window, notify, root motion은 이전 위치와 현재 위치 사이를 기준으로 평가한다.
	Update_RootMotion(fPrevLocalTime, fCurLocalTime, bStepChanged);

	Update_GameplayFlags();

	if (!bStepChanged)
		Update_TriggeredNotifies(fPrevFrame, fCurFrame, TimeResult.bLooped);

	Update_CurPose();						// 전환 목표 포즈

	if (m_bStateBlending)
		Update_StateBlend(fTimeDelta);		// 이전 포즈에서 목표 포즈로 블렌드

	Update_PoseLayers(fTimeDelta);			// base pose 위에 overlay pose 합성

	Clear_ConsumedTriggers();

}

CAnimator* CAnimator::Create(const ANIMATOR_DESC& Desc)
{
	CAnimator* pInstance = new CAnimator();

	if (FAILED(pInstance->Initialize(Desc)))
	{
		MSG_BOX("Failed to Created : CAnimator");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CAnimator::Free()
{
	__super::Free();

	for (auto& pAnimation : m_Animations)
		Safe_Release(pAnimation);

	m_Animations.clear();

	m_EditData = {};

	m_BasePose.clear();
	m_CurPose.clear();
	m_KeyFrameIndices.clear();

	m_TriggeredNotifies.clear();
	m_ActiveCustomWindows.clear();

	m_iCurClipIndex = -1;
	m_fLocalTime = 0.f;
	m_fPlayRate = 1.f;

	m_bPlaying = false;
	m_bLoop = false;
	m_bEnded = false;
	m_bUseRootMotion = false;

	m_GameplayFlags = {};
}
