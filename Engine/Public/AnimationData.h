#pragma once

#include "Base.h"

NS_BEGIN(Engine)

#pragma region ANIMATION_CLIP
static constexpr _uint INVALID_ANIMATION_INDEX = static_cast<_uint>(-1);

// 특정 프레임 구간이 게임플레이에서 어떤 의미를 가지는지 구분
enum class ANIM_WINDOW_TYPE
{
	COMBO_INPUT,				// 추가 공격/연계 입력을 받을 수 있는 구간
	CANCEL,						// 다른 상태나 시퀀스로 캔슬 가능한 구간

	MOVE_LOCK,					// 이동 입력을 막는 구간
	ROTATE_LOCK,				// 회전 입력을 막는 구간
	TARGET_TRACKING_LOCK,		// 타겟 추적/자동 방향 보정 회전 금지 구간

	INVINCIBLE,					// 피격을 무시하는 무적 구간
	SUPER_ARMOR,				// 피격은 받지만 상태 전이는 막는 구간
	DAMAGE_REDUCE,				// 피해 감소 구간. fValue를 피해 배율로 사용

	HIT_ACTIVE,					// 공격 판정이 활성화되는 구간
	GUARD_ACTIVE,				// 가드 판정 활성 구간
	PARRY_ACTIVE,				// 패링 판정 활성 구간

	CUSTOM,						// 특수 처리용 사용자 정의 구간
	GUARD_INPUT,				// 가드 입력을 받을 수 있는 구간
	DASH_INPUT,					// 대쉬 입력을 받을 수 있는 구간
	END
};

enum class ANIM_NOTIFY_TYPE
{
	SOUND,						// 사운드 재생
	EFFECT,						// 이펙트 생성
	FOOTSTEP,					// 발소리/발판 반응
	ATTACH_PART,				// 파츠/메쉬를 본 위치에 부착
	DETACH_PART,				// 부착된 파츠/메쉬 분리
	CAMERA_SHAKE,				// 카메라 흔들림
	CUSTOM,						// 특수 처리
	END
};

// 편집 데이터에서 motion 참조 대상을 구분하는 범용 타입.
enum class ANIM_MOTION_TYPE
{
	CLIP,
	SEQUENCE,
	BLEND_SPACE,
	END
};

// Sequence 안에서 각 step clip이 맡는 의미.
enum class ANIM_SEQUENCE_STEP_TYPE
{
	START,
	ACTIVE,
	LOOP,
	END,
	END_ENUM
};

// Sequence가 어느 pose channel에서 재생되는지 구분한다.
// BASE는 State의 주 motion이고, OVERLAY는 base pose 위에 mask/weight로 합성된다.
enum class ANIM_SEQUENCE_USAGE
{
	BASE,
	OVERLAY,
	END
};

// Sequence 내부 흐름 정책.
// ONE_SHOT은 Start->Active->End처럼 입력 한 번으로 끝까지 진행하고,
// SUSTAINED_LOOP는 Start?->Loop->End처럼 입력 유지/해제를 감시한다.
enum class ANIM_SEQUENCE_FLOW
{
	ONE_SHOT,
	SUSTAINED_LOOP,
	END
};

// Step 사이에서 pose blend를 걸 구간.
// 공격/단발 overlay처럼 원본이 이미 자연스럽게 잘린 sequence는 NONE을 쓴다.
enum class ANIM_SEQUENCE_STEP_BLEND_MODE
{
	NONE,
	LOOP_TO_END_ONLY,
	ALL_STEPS,
	END
};

// 같은 sequence를 다시 시작할 수 있는 시점.
// 수리 overlay처럼 End 이후 inactive를 반드시 거쳐야 하는 motion은 AFTER_INACTIVE를 쓴다.
enum class ANIM_SEQUENCE_RESTART_RULE
{
	ANY_TIME,
	WHILE_START_CANCELABLE,
	AFTER_INACTIVE,
	END
};

// Controller parameter가 저장하고 비교할 값 타입.
enum class ANIM_CONTROLLER_PARAM_TYPE
{
	BOOL,
	INT,
	FLOAT,
	TRIGGER,
	END
};

// Controller state 진입 조건에서 parameter를 비교하는 방식.
enum class ANIM_CONTROLLER_COMPARE_TYPE
{
	True,
	False,

	EQUAL,
	NOT_EQUAL,

	GREATER,
	GREATER_EQUAL,
	LESS,
	LESS_EQUAL,

	TRIGGERED,

	END
};

// Controller state가 실제로 재생할 motion 데이터 종류.
enum class ANIM_CONTROLLER_MOTION_TYPE
{
	NONE,
	CLIP,
	SEQUENCE,
	BLEND_SPACE_1D,
	END
};

// State가 어떤 방식으로 진입 후보가 되는지 정의한다.
enum class ANIM_STATE_ENTRY_MODE
{
	AUTO,				// 조건이 맞으면 자동 진입
	REQUEST,			// 외부 요청이 있어야 진입
	BOTH,				// 조건 또는 요청으로 진입
	ENTRY_ONLY,			// 시작 상태 전용
	END
};

// 외부 요청으로 state에 진입할 때 현재 clip window를 어떻게 검사할지 정의한다.
enum class ANIM_STATE_REQUEST_RULE
{
	ALWAYS,
	COMBO_WINDOW,
	CANCEL_WINDOW,
	NEVER,
	GUARD_INPUT_WINDOW,
	DASH_INPUT_WINDOW,
	END
};

enum class ANIM_STATE_HOLD_RULE
{
	NONE,						// Auto 상태. 조건에 따라 언제든 다른 Auto 상태로 바뀔 수 있음
	UNTIL_END,					// 현재 Clip이 끝날 때까지 Auto 상태가 덮지 못함
	LOOP_UNTIL_REQUEST,			// 루프 상태. 명시 요청이 오기 전까지 유지
	END
};

// Animator가 pose를 합성할 layer 방식.
enum class ANIM_POSE_LAYER_BLEND_TYPE
{
	OVERRIDE,					// base pose 위에 지정 bone 이하를 덮어씀
	ADDITIVE,					// base pose에 차이값을 더함
	END
};

enum class ANIMATION_TIME_UNIT
{
	MILLISEC,
	TICKS,
};

// Notify가 특정 bone/slot 위치를 필요로 할 때 사용하는 부착 정보.
// strSlotName/iSlotHash: slot 식별자, strBoneName/iBoneHash/iBoneIndex: 부착 대상 bone cache,
// vOffsetPosition/vOffsetRotation/vOffsetScale: bone 기준 local offset.
typedef struct tagAnimNotifySlot
{
	string		strSlotName;
	_uint64		iSlotHash = {};

	string		strBoneName;
	_uint64		iBoneHash = {};
	_int		iBoneIndex = -1;

	_float3		vOffsetPosition = {};
	_float3		vOffsetRotation = {};
	_float3		vOffsetScale = { 1.f, 1.f, 1.f };
}ANIM_NOTIFY_SLOT;

// 특정 프레임에 한 번 발생하는 이벤트
typedef struct tagAnimNotify
{
	string		strName;									// Notify 에디터 표시 이름
	_uint64		iNameHash = {};								// Notify 이름 해시

	ANIM_NOTIFY_TYPE	eType = ANIM_NOTIFY_TYPE::CUSTOM;	// 런타임 분기용 타입

	_uint		iFrame = {};								// 발생 프레임, local frame 기준
	_float		fTime = {};									// 발생 시간, 기준은 iFrame

	string		strPayload;									// 타입별 대상 값 : FX 이름, Sound 이름, 파츠 이름 등
	_uint64		iPayloadHash = {};

	_bool		bUseSlot = false;
	ANIM_NOTIFY_SLOT	Slot = {};							// 본/슬롯 위치가 필요한 노티파이인지

}ANIM_NOTIFY;

// 시작과 끝이 있는 애니메이션 구간, 이동 금지, 회전 금지, 콤보 입력 가능, 공격 판정 같은 상태를 표현
typedef struct tagAnimWindow
{
	string				strName;							// Window 표시 이름
	_uint64				iNameHash = {};						// Window 이름 해시

	ANIM_WINDOW_TYPE	eType = ANIM_WINDOW_TYPE::END;		// Window 의미

	_uint				iStartFrame = {};					// 구간 시작 프레임
	_uint				iEndFrame = {};						// 구간 끝 프레임

	_float              fValue = 0.f;						// DAMAGE_REDUCE 등 수치가 필요한 Window에서 사용
	_uint64             iPayloadHash = {};					// 필요 시 gameplay tag/hash
	string				strPayload;							// 특수 처리용 추가 데이터

}ANIM_WINDOW;

// 특정 프레임 구간에서 재생 속도를 다르게 적용하기 위한 데이터
typedef struct tagAnimSpeedSection
{
	_uint	iStartFrame = {};			// 속도 보정 시작 프레임
	_uint	iEndFrame = {};				// 속도 보정 끝 프레임
	_float	fPlayRate = { 1.f };		// 해당 구간 재생 속도

}ANIM_SPEED_SECTION;

// 원본 애니메이션을 어떤 방식으로 재생할지 정의하는 클립 설정, 원본 Animation을 직접 복사하지 않고 인덱스/해시로 참조
typedef struct tagAnimClipSetting
{
	string	strSourceName;								// 원본 애니메이션 이름
	string	strExportName;								// 게임에서 사용할 클립 이름

	_uint64	iClipHash = {};								// 런타임에서 이 클립을 찾기 위한 해시
	_uint64	iRawAnimationHash = {};						// 원본 애니메이션을 찾기 위한 해시
	_uint	iRawAnimationIndex = static_cast<_uint>(-1);	// 원본 애니메이션 배열 인덱스

	_float	fSourceDuration = {};						// 원본 애니메이션 전체 길이
	_uint	iSourceFrameCount = {};						// 원본 애니메이션 전체 프레임 수

	_uint	iStartFrame = {};							// 실제 사용할 시작 프레임
	_uint	iEndFrame = {};								// 실제 사용할 끝 프레임

	_bool   bLoop = {};									// 클립 루프 여부
	_bool	bUseRootMotion = {};						// 루트 모션 사용 여부
	_bool	bReverse = { false };						// 역재생 여부

	_float	fPlayRate = { 1.f };						// 기본 재생 속도
	_float	fBlendInTime = {};							// 이 클립으로 들어올 때 기본 블렌드 시간
	_float	fBlendOutTime = {};							// 이 클립에서 나갈 때 기본 블렌드 시간

	vector<ANIM_SPEED_SECTION>	SpeedSections;			// 구간별 재생 속도 보정
	vector<ANIM_NOTIFY>			Notifies;				// 프레임 이벤트 목록
	vector<ANIM_WINDOW>			Windows;				// 게임플레이 구간 목록

}ANIM_CLIP_SETTING;

// Animator가 현재 애니메이션 구간을 바탕으로 Player에게 전달하는 상태 플래그
typedef struct tagAnimGameplayFlags
{
	_bool	bCanMove = { true };			// 이동 가능 여부
	_bool	bCanRotate = { true };			// 회전 가능 여부
	_bool	bCanTargetTrack = { true };

	_bool	bHitActive = { false };			// 공격 판정 활성 여부
	_bool	bInvincible = { false };		// 무적 여부
	_bool	bSuperArmor = { false };		// 슈퍼아머 여부
	_bool   bGuardActive = { false };		
	_bool   bParryActive = { false };		

	_float  fDamageRate = { 1.f };			// 데미지 배율 (막기 시 데미지 감소 등에 사용)

}ANIM_GAMEPLAY_FLAGS;

typedef struct tagAnimRootMotionData
{
	_float3 vTranslation = {};
	_float4 vRotation = { 0.f, 0.f, 0.f, 1.f };
	_float3 vScale = { 1.f, 1.f, 1.f };
}ANIM_ROOT_MOTION_DELTA;

// Animator에 외부 코드가 넣을 수 있는 입력값 정의.
// 실제 runtime 현재값은 Animator가 별도로 들고, Controller에는 default와 타입만 저장한다.
typedef struct tagAnimControllerParameter
{
	string			strName;														// 파라미터 식별자
	_uint64			iNameHash = {};													// 파라미터 식별자

	ANIM_CONTROLLER_PARAM_TYPE eType = ANIM_CONTROLLER_PARAM_TYPE::BOOL;	// 파라미터 저장, 비교 값 타입

	_bool			bDefaultBool = { false };										// 런 타임 초기값
	_int			iDefaultInt = {};												// 런 타임 초기값
	_float			fDefaultFloat = {};												// 런 타임 초기값
}ANIM_CONTROLLER_PARAMETER;

// Controller state가 자동 진입 가능한지 판단할 때 사용하는 조건 하나.
typedef struct tagAnimControllerCondition
{
	_uint64	iParameterHash = {};
	_int	iParameterIndex = { -1 };

	ANIM_CONTROLLER_COMPARE_TYPE eCompareType = ANIM_CONTROLLER_COMPARE_TYPE::True;		// 파라미터 비교 방식

	_bool	bBoolValue = { false };														// 비교 기준값	
	_int	iIntValue = {};																// 비교 기준값
	_float	fFloatValue = {};															// 비교 기준값
}ANIM_CONTROLLER_CONDITION;


// Sequence를 구성하는 단일 clip step.
typedef struct tagAnimSequenceStep
{
	ANIM_SEQUENCE_STEP_TYPE eStepType = ANIM_SEQUENCE_STEP_TYPE::ACTIVE;	// Step 종류 

	_uint64		iClipHash = {};													// 저장/런타임 조회용
	_int		iClipIndex = { -1 };												// 저장/런타임 조회용

	vector<ANIM_CONTROLLER_CONDITION> HoldConditions;						// Loop Step의 유지 조건
}ANIM_SEQUENCE_STEP;

// 여러 clip을 하나의 동작 단위처럼 재생하기 위한 데이터.
// 예: Groggy_Start -> Groggy_Loop -> Groggy_End.
typedef struct tagAnimSequence
{
	string							strName;				// 표시/저장 이름
	_uint64							iNameHash = {};		    // state/rule에서 sequence 찾을 때 사용

	ANIM_SEQUENCE_USAGE				eUsage = ANIM_SEQUENCE_USAGE::BASE;								// 해당 시퀀스의 포즈 레이어 채널
	ANIM_SEQUENCE_FLOW				eFlow = ANIM_SEQUENCE_FLOW::SUSTAINED_LOOP;						// 해당 시퀀스 내부 흐름 정책
	ANIM_SEQUENCE_STEP_BLEND_MODE	eStepBlendMode = ANIM_SEQUENCE_STEP_BLEND_MODE::ALL_STEPS;		// 해당 시퀀스 내 Step 사이의 블렌드 정책
	ANIM_SEQUENCE_RESTART_RULE		eRestartRule = ANIM_SEQUENCE_RESTART_RULE::ANY_TIME;			// 해당 시퀀스를 다시 시작할 수 있는 시점

	_bool							bStartCancelByWindow = { true };								// Start Step 도중 입력이 끊겼을 때 CANCEL window 안에서 즉시 취소할지 여부
	_bool							bAllowEndToLoopReentry = { true }; 								// End step 도중 재입력이 들어왔을 때 Loop 또는 처음 step으로 재진입을 허용할지 여부
	_bool							bUseCommitTime = { false };										// 특정 시간이 지나야 동작을 확정한 것으로 볼지 여부. 예: 수리/차지/사용 동작에서 너무 짧은 입력은 취소 처리하기 위한 확장용
	_float							fCommitTime = {};
	_float							fLoopToEndBlendTime = {};										// Loop Step에서 End Step으로 넘어갈 때 사용할 전용 블렌드 시간
	_float							fEndBlendOutTime = {};											// overlay sequence의 End가 끝난 뒤 base pose로 빠질 때 사용할 blend out 시간

	_float                          fReentryBlendTime = {};            // End step 중 재입력으로 sequence를 다시 시작할 때 사용할 blend 시간. 0이면 clip blend 값을 사용한다.
	_float                          fReentryStartOffsetTime = {};      // 재진입 시 첫 step을 이 local time부터 시작한다.

	vector<ANIM_SEQUENCE_STEP>		Steps;					// 순서대로 재생할 clip 목록
}ANIM_SEQUENCE;

// Controller가 선택할 수 있는 하나의 애니메이션 상태.
// 자신이 어떤 조건/요청으로 들어갈 수 있는지를 가진다.
typedef struct tagAnimControllerState
{
	string	strName;
	_uint64	iNameHash = {};

	ANIM_CONTROLLER_MOTION_TYPE eMotionType = ANIM_CONTROLLER_MOTION_TYPE::NONE;	// 해당 State의 모션 타입

	_uint64	iMotionHash = {};					 // clip/sequence/blendspace 참조
	_int	iMotionIndex = { -1 };				 // clip/sequence/blendspace 참조

	ANIM_STATE_ENTRY_MODE eEntryMode = ANIM_STATE_ENTRY_MODE::AUTO;			  // 클립의 진입 방식 
	ANIM_STATE_REQUEST_RULE eRequestRule = ANIM_STATE_REQUEST_RULE::NEVER;	  // 현재 재생 클립 Window 검사 방식
	ANIM_STATE_HOLD_RULE eHoldRule = ANIM_STATE_HOLD_RULE::NONE;			  // 현재 클립의 유지 방식

	vector<ANIM_CONTROLLER_CONDITION> EnterConditions;						  // 자동 진입 조건 목록

	_int	iPriority = {};								// 클립이 중첩 되었을 때의 우선도
	_bool	bCanRestart = { false };					// 클립 요청 시 즉시 재생 플래그 (ex : HIt같은 현재 클립에 구애받지 않고 즉시 실행 가능한 클립)
	_bool	bAllowPoseLayers = { true };				// false면 이 state 재생 중 PoseLayer overlay를 전부 막는다.

}ANIM_CONTROLLER_STATE;

typedef struct tagAnimPoseLayerRule
{
	string	strName;								// rule 표시/저장 이름
	_uint64	iNameHash = {};							// rule 이름 해시

	_bool	bEnable = { true };						// false면 조건이 맞아도 overlay를 적용하지 않는다.

	ANIM_CONTROLLER_MOTION_TYPE eMotionType = ANIM_CONTROLLER_MOTION_TYPE::CLIP;		// overlay motion 종류

	_uint64 iMotionHash = {};							// clip 또는 sequence hash
	_int	iMotionIndex = { -1 };					// clip 또는 sequence index cache

	string	strMaskRootBoneName;					// overlay를 덮기 시작할 root bone 이름
	_uint64	iMaskRootBoneHash = {};					// mask root bone 이름 hash
	_int	iMaskRootBoneIndex = { -1 };			// mask root bone index cache. 이 bone과 하위 bone만 overlay 대상.


	ANIM_POSE_LAYER_BLEND_TYPE eBlendType = ANIM_POSE_LAYER_BLEND_TYPE::OVERRIDE;		// 합성 방식

	_float	fWeight = { 1.f };						// base pose와 overlay pose의 blend 비율
	_bool	bLoop = { true };						// overlay clip loop 여부
	_float	fPlayRate = { 1.f };					// overlay clip 독립 재생 속도

	vector<ANIM_CONTROLLER_CONDITION> Conditions;	// overlay 시작/유지 조건 목록. 모두 true여야 active.
}ANIM_POSE_LAYER_RULE;

typedef struct tagAnimActionStateMap
{
	string  strActionName;                  // 외부에서 요청하는 행동 이름. 예: Move, Dodge, Attack_L
	_uint64 iActionHash = {};               // 행동 이름 해시

	_uint64 iStateHash = {};                // 이 행동으로 진입 가능한 Controller State
	_int    iStateIndex = { -1 };           // Resolve 후 캐시된 State index
} ANIM_ACTION_STATE_MAP;

// 애니메이션 컨트롤러 전체 데이터.
// 에디터는 이 데이터를 편집/저장하고, Animator는 이 데이터를 평가해서 clip/sequence를 선택한다.
typedef struct tagAnimController
{
	string	strName;
	_uint64	iNameHash = {};

	_uint64	iEntryStateHash = {};								// 기본 시작 state
	_int	iEntryStateIndex = { -1 };							// 기본 시작 state

	vector<ANIM_CONTROLLER_PARAMETER>	Parameters;
	vector<ANIM_CONTROLLER_STATE>		States;					// 재생 가능한 상태 목록
	vector<ANIM_POSE_LAYER_RULE>		PoseLayerRules;
	vector<ANIM_ACTION_STATE_MAP> ActionStateMaps;                // Action 요청 -> State 후보 목록
}ANIM_CONTROLLER;

typedef struct tagAnimationEditData
{
	vector<ANIM_CLIP_SETTING>	ClipSettings;
	vector<ANIM_SEQUENCE>		Sequences;
	ANIM_CONTROLLER				Controller;
}ANIMATION_EDIT_DATA;


#pragma endregion

NS_END