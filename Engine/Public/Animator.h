#pragma once
#include "AnimationData.h"
#include "ModelDataStruct.h"
#include "Component.h"

NS_BEGIN(Engine)

class CGameInstance;
class CAnimation;

/*
 * Animator 현재 역할
 * - 원본 CAnimation 배열과 에디터에서 만든 ANIM_CLIP_SETTING을 참조해 현재 pose를 계산한다.
 * - 외부에는 clip-local time/frame 기준으로 seek/play/stop을 제공한다.
 * - clip setting의 window를 평가해 combo/cancel/gameplay flag/notify를 산출한다.
 * - 최종 본 행렬은 Model/Renderer 쪽에서 pose를 받아 계산하고, Animator는 Model을 직접 소유하지 않는다.
 */
class ENGINE_DLL CAnimator final : public CBase
{
public:
#pragma region RUNTIME_STRUCTS
	typedef struct tagPoseLayerRuntime
	{
		_bool	bWasActive = { false };      // 이전 프레임 active 여부
		_bool	bPlaying = { false };        // overlay 재생 중 여부
		_bool	bEnded = { false };          // overlay 종료 여부

		_float	fLocalTime = {};             // clip 또는 현재 sequence step local time

		_bool	bBlendOut = { false };
		_float	fBlendOutElapsed = {};
		_float	fBlendOutDuration = {};
		_int	iBlendOutClipIndex = { -1 };
		_float	fBlendOutLocalTime = {};

		_int	iSequenceStepIndex = { -1 }; // sequence overlay의 현재 step index	
		_bool	bEndRequested = { false };   // condition false 후 END step으로 빠져야 하는지

		POSE	AdditiveBasePose;							// additive 계산 기준 포즈. 기본은 additive clip의 0프레임.
		_bool	bHasAdditiveBasePose = { false };			// additive 기준 포즈를 이미 샘플링했는지.
		_uint64 iAdditiveBaseClipHash = {};					// 기준 포즈를 만든 clip hash. clip이 바뀌면 다시 샘플링한다.
	}POSE_LAYER_RUNTIME;

	// Clip local time을 공통 방식으로 진행한 결과.
	typedef struct tagAnimTimeAdvanceResult
	{
		_bool	bEnded = { false };
		_bool	bLooped = { false };
	}ANIM_TIME_ADVANCE_RESULT;

	// 애니메이터가 포즈 계산에 사용하는 본 이름/계층 정보.
	typedef struct tagAnimationBoneDesc
	{
		string		strName;
		_uint64		iNameHash = {};
		_int		iParentIndex = { -1 };
	}ANIMATOR_BONE_DESC;

	// 컨트롤러 파라미터의 현재 값. Trigger는 소비 후 자동으로 false 처리된다.
	typedef struct tagControllerParamValue
	{
		ANIM_CONTROLLER_PARAM_TYPE eType = ANIM_CONTROLLER_PARAM_TYPE::BOOL;

		_bool	bBool = false;
		_int	iInt = 0;
		_float	fFloat = 0.f;

		_bool	bTriggered = false;
	}CONTROLLER_PARAM_VALUE;

	// 원본 애니메이션 바이너리와 CSV를 읽기 위한 로드 정보.
	typedef struct tagRawAnimationLoadDesc
	{
		_wstring	strFolderName;
		_wstring	strAnimationBaseName;
		_bool		bStaticAnimation = { false };

		ANIMATION_TIME_UNIT eTimeUnit = ANIMATION_TIME_UNIT::MILLISEC;
	}RAW_ANIMATION_LOAD_DESC;

	// 애니메이터 생성 시 외부에서 넘겨주는 초기화 데이터.
	typedef struct tagAnimatorDesc
	{
		RAW_ANIMATION_LOAD_DESC		RawAnimationLoadDesc;

		ANIMATION_EDIT_DATA			EditData;
		_bool						bUseEditData = false;

		POSE						BasePose;						// animation이 없거나 root motion을 끌 때 되돌아갈 기준 pose
		vector<ANIMATOR_BONE_DESC>  Bones;
		_float4x4					PreTransformMatrix = {};
	}ANIMATOR_DESC;
#pragma endregion

private:
	CAnimator();
	virtual ~CAnimator() = default;

public:
#ifdef _DEBUG
#pragma region PUBLIC_EDITOR_API
	// 에디터가 현재 클립/시퀀스/컨트롤러 설정을 읽어 패널에 표시한다.
	const ANIMATION_EDIT_DATA&			Get_EditData() const;
	// 에디터/리소스에서 만든 clip 설정 목록을 Animator에 연결한다.
	void								Set_ClipSettings(const vector<ANIM_CLIP_SETTING>& ClipSettings);	

	// 에디터 미리보기에서 해시 대신 배열 인덱스로 콤보/캔슬 전환을 테스트
	HRESULT								Request_Combo_ByIndex(_uint iClipIndex);
	HRESULT								Request_Cancel_ByIndex(_uint iClipIndex);

	void								Reset_PoseLayerPreview();

public:
	// 시퀀스 미리보기
	HRESULT								Preview_Sequence_ByIndex(_uint iSequenceIndex, _uint iStepIndex, _float fStepLocalTime);

	_bool								Is_SequenceMode() const;
	_int								Get_CurSequenceIndex() const;
	_int								Get_CurSequenceStepIndex() const;
	_bool								Is_StateBlending() const;
	_float								Get_StateBlendAlpha() const;
#pragma endregion

#endif




public:
#pragma region PUBLIC_RUNTIME_API
	// 런타임에서 사용할 애니메이션 편집 데이터를 주입하고 내부 참조를 다시 연결
	void								Set_LocalFrame(_uint iLocalFrame);
	void								Set_EditData(const ANIMATION_EDIT_DATA& EditData);
	void								Set_Loop(_bool bLoop);
	// 외부 타임라인이 Animator의 재생 시간과 Root Motion 사용 여부를 직접 제어한다.
	void								Set_LocalTime(_float fLocalTime);
	void								Set_PlayRate(_float fPlayRate);
	void								Set_UseRootMotion(_bool bUseRootMotion);
	const POSE&							Get_CurPose() const;					// 현재 sampling된 pose를 반환한다.
	const ANIM_CLIP_SETTING*			Get_CurClip() const;					// 현재 Animator가 선택한 clip 설정을 반환한다.
	const ANIM_GAMEPLAY_FLAGS&			Get_GameplayFlags() const;				// 현재 frame window 기준 gameplay flag.
	const vector<const ANIM_NOTIFY*>&	Get_TriggeredNotifies() const;			// 이번 update에서 통과한 notify 목록.
	const ANIM_SEQUENCE*				Get_CurSequence() const;
	const ANIM_SEQUENCE_STEP*			Get_CurSequenceStep() const;
	const ANIM_ROOT_MOTION_DELTA&		Get_RootMotionDelta() const;
	const ANIM_CONTROLLER_STATE*		Get_CurControllerState() const;

	void								Set_Bool(const _wstring& strParamName, _bool bValue);
	void								Set_Int(const _wstring& strParamName, _int iValue);
	void								Set_Float(const _wstring& strParamName, _float fValue);
	void								Set_Trigger(const _wstring& strParamName);

	void								Request_State(const _wstring& strStateName);
	void								Request_State(_uint64 iStateHash);
	void								Request_SequenceEnd();


	_float								Get_LocalTime() const;					// 선택된 clip 내부 기준 시간. 현재 에디터에서는 ms 단위로 취급한다.
	_uint								Get_LocalFrame() const;					// local time을 선택 clip의 frame index로 환산한다.
	_float								Get_RawTime() const;					// local time에 clip 시작 offset을 더한 raw animation 시간.
	_uint								Get_RawAnimationIndex() const;			// 현재 clip이 참조하는 원본 animation 배열 index.
	_float								Get_CurClipDuration() const;			// 현재 클립 기준 계산값
	_uint								Get_CurClipFrameCount() const;
	const vector<const ANIM_WINDOW*>&	Get_ActiveCustomWindows() const;

	_bool								Is_PoseLayerActive(_uint64 iRuleHash) const;
	_bool 								Is_PoseLayerLoop(_uint64 iRuleHash) const;

	_bool								Is_Playing() const;
	_bool								Is_Ended() const;
	_bool								Is_InWindow(ANIM_WINDOW_TYPE eType) const;
	_bool								Is_InCustomWindow(const _wstring strPayload) const;
	_bool								Can_ComboInput() const;
	_bool								Can_Cancel() const;


	void								Reset_PlaybackState();
	void								Set_RootBoneIndex(_int iRootBoneIndex);
	void								Set_ControllerEnabled(_bool bEnable);


	HRESULT								Play_Default();
	HRESULT								Play_Clip(_uint64 iClipHash);					// clip hash로 즉시 재생한다.
	HRESULT								Blend_ToClip(_uint64 iClipHash, _float fTargetLocalTimeMS, _float fBlendDurationMS, _bool bUseRootMotion);	// Animator의 ms 단위로 목표 Clip 설정을 적용한 뒤 현재 pose에서 보간 전환한다.
	HRESULT								Play_Clip_ByIndex(_uint iClipIndex);			// clip 설정 배열 index로 재생 clip을 선택한다.
	
	HRESULT								Play_Sequence(_uint64 iSequenceHash);
	HRESULT								Play_Sequence_ByIndex(_uint iSequenceIndex);
	HRESULT                             Play_Sequence_ByIndex_Internal(_uint iSequenceIndex, _float fFirstStepLocalTime);

	HRESULT								Play_RawAnimation(_uint64 iAnimationFileHash);
	HRESULT								Play_RawAnimation(const _wstring& strAnimationFileName);
	HRESULT								Play_RawAnimation_ByIndex(_uint iRawAnimationIndex);
	HRESULT								Play_RawAnimation_ByStateName(const _wstring& strStateName);


	HRESULT								Request_Combo(_uint64 iClipHash);				// COMBO_INPUT window 안에서만 clip 전환을 허용한다.
	HRESULT								Request_Cancel(_uint64 iClipHash);				// CANCEL window 안에서만 clip 전환을 허용한다.


	void								Stop();											// 시간을 0으로 되돌리고 정지한다.
	void								Pause();										// 현재 시간을 유지한 채 정지한다.
	void								Resume();										// 현재 clip/time에서 재생을 재개한다.
	 
	void                                Request_Action(const _wstring& strActionName);
	void                                Request_Action(_uint64 iActionHash);

	_bool                               Was_ActionAcceptedThisFrame() const;
	_bool                               Was_StateChangedThisFrame() const;

	_uint64                             Get_AcceptedActionHash() const;
	_uint64                             Get_PrevControllerStateHash() const;
	_uint64                             Get_CurControllerStateHash() const;


	//추가 
	void Clear_Requests() 
	{ 
		m_PendingActionHashes.clear();
		m_bHasRequestedState = false;
	}
	
	//Y축 루트모션 적용 
	void								Set_RootYAnchored(_bool bRootYAnchored) { m_bRootYAnchored = bRootYAnchored; }
#pragma endregion
                               

private:
#pragma region PRIVATE_RUNTIME_API_CONTROLLER
	_int								Find_ControllerParamIndex(_uint64 iParamHash) const;
	_int								Find_AutoControllerState();
	_int								Find_RequestedControllerState();

	_bool								Should_HoldCurState() const;
	_bool								Can_RestartCurSequenceState() const;
	_bool								Try_RestartCurSequenceState();

	void								Clear_ControllerRequests();
	void								Ready_ControllerRuntime();

	_bool								Is_ConditionPassed(const ANIM_CONTROLLER_CONDITION& Condition) const;
	_bool								Is_StateConditionPassed(const ANIM_CONTROLLER_STATE& State) const;
	_bool								Can_EnterRequestedState(const ANIM_CONTROLLER_STATE& State) const;
	
	HRESULT								Enter_ControllerState(_int iStateIndex);

	_float								Get_StateBlendInTime(const ANIM_CONTROLLER_STATE& State) const;
	_float								Get_StateBlendOutTime(const ANIM_CONTROLLER_STATE& State) const;
	_float								Get_StateBlendDuration(const ANIM_CONTROLLER_STATE& FromState, const ANIM_CONTROLLER_STATE& ToState) const;

	void								Update_Controller();
	void								Clear_ConsumedTriggers();

	_int                                Find_ActionRequestedControllerState(_uint64& OutAcceptedActionHash);
	void                                Clear_ActionRequests();
	void                                Reset_ActionResult();

	_bool                               Is_AutoLikeControllerState(const ANIM_CONTROLLER_STATE& State) const;
	_bool                               Is_ControllerStateInAction(_int iStateIndex, _uint64 iActionHash) const;
	_bool                               Can_EnterActionRequestedState(const ANIM_CONTROLLER_STATE& State, _uint64 iActionHash) const;
#pragma endregion


private:
#pragma region PRIVATE_RUNTIME_API_SEQUENCE
	_int								Find_SequenceEndStepIndex() const;
	
	_bool								Is_SequenceStepHold(const ANIM_SEQUENCE_STEP& Step) const;
	_bool								Should_BlendSequenceStep(const ANIM_SEQUENCE& Sequence, const ANIM_SEQUENCE_STEP& FromStep, const ANIM_SEQUENCE_STEP& ToStep) const;

	HRESULT								Play_Clip_ByIndex_Internal(_uint iClipIndex, _bool bKeepSequenceState);
	HRESULT								Enter_SequenceStep(_uint iStepIndex);

	_bool								Update_Sequence();
public:
	void								Stop_Sequence();
private:
	_float								Get_SequenceStepBlendDuration(const ANIM_SEQUENCE& Sequence, const ANIM_CLIP_SETTING& FromClip, const ANIM_CLIP_SETTING& ToClip, const ANIM_SEQUENCE_STEP& FromStep, const ANIM_SEQUENCE_STEP& ToStep) const;
	_bool								Can_RestartSequenceFromCurStep(const ANIM_SEQUENCE& Sequence, const ANIM_SEQUENCE_STEP& Step) const;

private:
	//  Editor
	HRESULT								Jump_SequenceStep_ForPreview(_uint iSequenceIndex, _uint iStepIndex, _float fStepLocalTime);
	void								Apply_SequenceStepBlend_ForPreview(const ANIM_SEQUENCE& Sequence, _uint iStepIndex, _float fStepLocalTime);
#pragma endregion


private:
#pragma region PRIVATE_RUNTIME_API_ROOT_MOTION
	void								Update_RootMotion(_float fPrevLocalTime, _float fCurLocalTime, _bool bStepChanged);
	void								Strip_RootMotion(POSE& Pose, const ANIM_CLIP_SETTING& Clip) const;
	
	_matrix								Compute_BoneMatrix(const POSE& Pose, _int iBoneIndex) const;
	_matrix								Compute_ParentMatrix(const POSE& Pose, _int iBoneIndex) const;
	_vector								Calc_RootRotation(_matrix matPrev, _matrix matCur) const;
	_bool                               Try_GetRootModelY(const POSE& Pose, _float& OutY) const;
	void                                Set_RootModelY(POSE& Pose, _float fModelY) const;

	void								Clear_RootMotionDelta();
#pragma endregion


private:
#pragma region PRIVATE_RUNTIME_API_POSE_LAYER
	void								Ready_PoseLayerRunTime();
	void								Reset_PoseLayerRunTime();
	void								Update_PoseLayers(_float fTimeDelta);
	void								Update_PoseLayerClip(const ANIM_POSE_LAYER_RULE& Rule, POSE_LAYER_RUNTIME& Runtime, _float fTimeDelta);
	void								Update_PoseLayerSequence(const ANIM_POSE_LAYER_RULE& Rule, POSE_LAYER_RUNTIME& Runtime, _float fTimeDelta, _bool bConditionActive);
	_bool								Enter_PoseLayerSequenceStep(const ANIM_POSE_LAYER_RULE& Rule, POSE_LAYER_RUNTIME& Runtime, _int iStepIndex);
	void								Apply_PoseLayerClip(const ANIM_POSE_LAYER_RULE& Rule, POSE_LAYER_RUNTIME& Runtime, const ANIM_CLIP_SETTING& Clip, _float fLocalTime, _float fWeightScale = 1.f);
	_bool								Sample_PoseLayerPose(const ANIM_CLIP_SETTING& Clip, _float fLocalTime, POSE& OutPose);
	void								Blend_LocalTransform(TransformData& Dst, const TransformData& Src, _float fWeight) const;
	void								Additive_LocalTransform(TransformData& Dst, const TransformData& Base, const TransformData& AdditivePose, _float fWeight) const;
	_bool                               Begin_PoseLayerBlendOut(const ANIM_POSE_LAYER_RULE& Rule, const ANIM_SEQUENCE& Sequence, POSE_LAYER_RUNTIME& Runtime, _int iClipIndex, _float fLocalTime);


	_bool								Is_PoseLayerAllowed() const;
	_bool								Is_PoseLayerRuleActive(const ANIM_POSE_LAYER_RULE& Rule) const;
	_bool								Is_ClipInWindow(const ANIM_CLIP_SETTING& Clip, _float fLocalTime, ANIM_WINDOW_TYPE eType) const;
	_bool								Can_CancelPoseLayerSequenceStart(const ANIM_SEQUENCE& Sequence, const POSE_LAYER_RUNTIME& Runtime) const;
	_bool								Should_KeepPoseLayerSequenceOnConditionLost(const ANIM_SEQUENCE& Sequence, const POSE_LAYER_RUNTIME& Runtime) const;
	_bool								Is_BoneInMask(_int iBoneIndex, _int iMaskRootBoneIndex) const;

	_int								Find_PoseLayerSequenceEndStepIndex(const ANIM_SEQUENCE& Sequence, _int iCurStepIndex) const;
#pragma endregion


private:
#pragma region PRIVATE_RUNTIME_API_BLEND
	void								Begin_StateBlend(_int iFormClipIndex, _float fFromLocalTime, _float fBlendDuration, _bool bFreezeFromPose = false);
	void								Update_StateBlend(_float fTimeDelta);
	void								Stop_StateBlend();

	_bool								Sample_StateBlendFromPose(POSE& OutPose);
	void								Blend_Pose(const POSE& FromPose, const POSE& ToPose, _float fWeight, POSE& OutPose) const;

	void								Begin_StateBlendFromPose(_int iFromClipIndex, const POSE& FromPose, _float fBlendDuration);
#pragma endregion


private:
#pragma region PRIVATE_RUNTIME_API_SAMPLING_AND_TIME
	ANIM_TIME_ADVANCE_RESULT			Advance_ClipLocalTime(const ANIM_CLIP_SETTING& Clip, _float fTimeDelta, _float fPlayRate, _bool bLoop, _float& InOutLocalTime) const;

	_bool								Sample_ClipPose(const ANIM_CLIP_SETTING& Clip, _float fLocalTime, POSE& OutPose, vector<KeyFrameIndices>& KeyFrameCache, _bool bApplyRootMotionStrip) const;
	_int								Find_SequenceStepIndexByType(const ANIM_SEQUENCE& Sequence, ANIM_SEQUENCE_STEP_TYPE eStepType, _int iStartAfter) const;
	_bool								Sample_Pose(const ANIM_CLIP_SETTING& Clip, _float fLocalTime, POSE& OutPose) const;

	_float								Calc_StartTime(const ANIM_CLIP_SETTING& Clip) const;							// clip 시작 frame을 raw animation 시간으로 환산한다.
	_float								Calc_ClipDuration(const ANIM_CLIP_SETTING& Clip) const;							// start/end frame 기준 clip 재생 길이를 계산한다.
	_uint								Calc_ClipFrameCount(const ANIM_CLIP_SETTING& Clip) const;						// start/end frame 기준 clip frame 수를 계산한다.
	_float								Calc_LocalToRawTime(const ANIM_CLIP_SETTING& Clip, _float fLocalTime) const;	// clip-local time을 raw animation time으로 변환한다.
	_float								Calc_LocalFramePosition() const;
	_float								Calc_EffectivePlayRate(const ANIM_CLIP_SETTING& Clip) const;
	_float								Calc_SampleLocalTime(const ANIM_CLIP_SETTING& Clip, _float fElapsedLocalTime) const;
	_float								Calc_SampleFramePosition(const ANIM_CLIP_SETTING& Clip, _float fElapsedLocalTime) const;
	_float								Calc_CurSampleFramePosition() const;
	
	ANIM_TIME_ADVANCE_RESULT			Update_Time(_float fTimeDelta);									// playrate/loop/end를 반영해 local time을 진행한다.
	void								Update_CurPose();												// 현재 raw animation time으로 pose를 sampling한다.
	void								Update_GameplayFlags();											// 현재 frame이 포함된 window들을 gameplay flag로 변환한다.
	void								Update_TriggeredNotifies(_float iPrevFrame, _float iCurFrame, _bool bLooped);	// 이전 frame과 현재 frame 사이의 notify를 수집한다.
	void								Clear_TriggeredNotifies();
#pragma endregion
	

private:
#pragma region PRIVATE_RUNTIME_API_RAW_AND_EDIT_DATA
	void								Clear_RawAnimations();
	void								Resolve_RawAnimationIndices(ANIMATION_EDIT_DATA& EditData) const;
	_bool								Is_ValidRawAnimationIndex(_uint iRawAnimationIndex) const;

	HRESULT								Ready_RawAnimations(const RAW_ANIMATION_LOAD_DESC& Desc);
	HRESULT								Ready_RawAnimations_FromResourceManager(const RAW_ANIMATION_LOAD_DESC& Desc);

	void								Update_RawTime(_float fTimeDelta);
	void								Update_RawPose();

	HRESULT								Try_LoadEditData_FromFile(const RAW_ANIMATION_LOAD_DESC& Desc);
	HRESULT								Parse_AnimationEditDataJson(const _wstring& strFilePath, ANIMATION_EDIT_DATA& OutEditData);
	HRESULT								Parse_ClipSettingJson(const json& Root, vector<ANIM_CLIP_SETTING>& OutClipSettings);
	HRESULT								Parse_SequencesJson(const json& Root, vector<ANIM_SEQUENCE>& OutSequences);
	HRESULT								Parse_ControllerJson(const json& Root, ANIM_CONTROLLER& OutController);

	void								Resolve_EditData(ANIMATION_EDIT_DATA& EditData);
	void								Validate_EditData(ANIMATION_EDIT_DATA& EditData);
#pragma endregion

public:
	HRESULT								Initialize(const ANIMATOR_DESC& Desc);
	void								Update(_float fTimeDelta);						// playing 상태일 때 local time을 진행시킨다.


private:
#pragma region RUNTIME_MEMBERS
	// 공용 시스템 참조
	CGameInstance* m_pGameInstance = { nullptr };

	// 원본 애니메이션 리소스와 에디터에서 만든 클립/시퀀스/컨트롤러 데이터
	vector<CAnimation*>					m_Animations;							// 원본 animation 리소스 참조 목록
	ANIMATION_EDIT_DATA					m_EditData;								// 에디터/파일에서 로드한 clip, sequence, controller 전체 데이터.

	// 현재 프레임에 외부로 전달되는 최종 포즈와 샘플링 캐시
	POSE								m_BasePose;								// 기준 pose. 재생 clip이 없거나 root motion off일 때 사용한다.
	POSE								m_CurPose;								// 현재 animation sampling 결과 pose.
	vector<KeyFrameIndices>				m_KeyFrameIndices;						// channel별 keyframe search cache.

	// window와 notify 평가 결과
	ANIM_GAMEPLAY_FLAGS					m_GameplayFlags = {};					// 현재 clip frame window에서 계산된 gameplay flag.
	vector<const ANIM_NOTIFY*>			m_TriggeredNotifies;					// 이번 update 구간에서 발생한 notify 목록.
	_int								m_iCurClipIndex = { -1 };				// 현재 선택된 clip 설정 index. -1이면 선택 없음.
	vector<const ANIM_WINDOW*>			m_ActiveCustomWindows;

	// 상태 전환 시 이전 포즈에서 새 포즈로 섞기 위한 블렌드 상태
	_bool								m_bStateBlending = { false };			// State 전환 블렌딩 중인지
	_int								m_iBlendFromClipIndex = { -1 };			// 전환 직전 재생 중이던 clip index
	_float								m_fBlendFromLocalTime = {};				// From clip의 독립 local time
	_float								m_fBlendElapsed = {};					// blend 진행 시간(ms)
	_float								m_fBlendDuration = {};					// blend 총 시간(ms)
	POSE								m_BlendFromPose;						// From clip sample pose
	POSE								m_BlendToPose;							// To pose 임시 저장
	vector<KeyFrameIndices>				m_BlendFromKeyFrameIndices;
	_bool                               m_bBlendFromPoseFrozen = { false };

	// 루트 모션 계산과 본 마스크 판단에 쓰는 본 정보
	_int								m_iRootBoneIndex = { -1 };				// root motion 계산/제거 기준 root bone index.
	ANIM_ROOT_MOTION_DELTA				m_RootMotionDelta = {};					// 이번 update에서 추출된 root motion delta.
	vector<ANIMATOR_BONE_DESC>			m_Bones;								// pose mask/root motion 계산에 쓰는 bone parent cache.
	_float4x4							m_PreTransformMatrix = {};				// skeleton/model 기준 pre-transform matrix.

	// 컨트롤러 파라미터와 현재/요청 상태.
	vector<CONTROLLER_PARAM_VALUE>		m_ControllerParamValues;				// controller parameter의 runtime 현재값.
	_int								m_iCurControllerStateIndex = { -1 };	// 현재 controller state index.
	_bool								m_bHasRequestedState = { false };		// 이름/hash 기반 state 요청이 있는지.
	_uint64								m_iRequestedStateHash = {};				// 요청된 state name hash.
	_bool								m_bControllerEnabled = { false };		// true면 Update에서 controller가 state/clip을 자동 평가한다.

	vector<_uint64>                     m_PendingActionHashes;                  // 이번 프레임 들어온 Action 요청들

	_bool                               m_bActionAcceptedThisFrame = { false };
	_bool                               m_bStateChangedThisFrame = { false };

	_uint64                             m_iAcceptedActionHash = {};
	_uint64                             m_iPrevControllerStateHash = {};
	_uint64                             m_iCurControllerStateHash = {};

	// 현재 클립 재생 상태.
	_float								m_fLocalTime = {};						// 현재 clip 내부 재생 시간. 현재 에디터 데이터 기준 ms 단위.
	_float								m_fPlayRate = { 1.f };					// 현재 재생 속도 배율.
	_bool								m_bPlaying = { false };					// 재생 중 여부.
	_bool								m_bLoop = { false };					// 현재 clip loop 여부.
	_bool								m_bEnded = { false };					// non-loop clip이 끝까지 재생됐는지 여부.
	_bool								m_bUseRootMotion = { false };			// false면 root translation을 base pose로 유지한다.

	// 시퀀스 재생 상태. 클립 단독 재생과 구분해서 step 전환을 관리
	_bool								m_bSequenceMode = { false };			// 현재 sequence 재생 모드인지.
	_int								m_iCurSequeneIndex = { -1 };			// 현재 sequence index.
	_int								m_iCurSequenceStepIndex = { -1 };		// 현재 sequence step index.
	_bool								m_bSequenceEndRequested = { false };	// loop step에서 end step으로 빠지라는 요청.

	// 오버레이 포즈 레이어별 독립 재생 상태와 샘플 포즈.
	vector<POSE_LAYER_RUNTIME>			m_PoseLayerRuntimes;					// PoseLayer rule별 독립 재생 시간/상태.
	POSE								m_PoseLayerSamplePose;					// overlay clip sampling 임시 pose.
	vector<KeyFrameIndices>				m_PoseLayerKeyFrameIndices;				// overlay sampling용 keyframe cache.

	// 에디터나 테스트용 원본 애니메이션 직접 재생 상태.
	_bool								m_bRawMode = { false };					// clip setting 없이 원본 animation을 직접 재생 중인지.
	_int								m_iCurRawAnimationIndex = { -1 };		// 현재 raw animation index.
	_float								m_fRawTime = {};						// raw animation 직접 재생 시간.
	_bool								m_bRawLoop = { false };					// raw animation loop 여부.

	//추가 
	_bool								m_bRootYAnchored = { false };
public:
	static CAnimator* Create(const ANIMATOR_DESC& Desc);
	virtual void Free() override;
};

NS_END
