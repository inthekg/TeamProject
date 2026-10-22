#pragma once
#include "Component.h"
#include "Core_Defines.h"
#include "Engine_Utility.h"
#include "ShakeWindowSync.h"

NS_BEGIN(Engine)
class CAnimator;
class CTransform;
class CModel;
NS_END

NS_BEGIN(GameCore)

class CInputComponent;
class CStatusComponent;
class CStatComponent;
class CMovement;
class CEquipment;
class CTargetComponent;
class CAIObject;
class CLegionArm_Manager;
struct CUTSCENE_ANIMATION_SECTION;

static constexpr _float CHARGE_HOLD_TIME = 0.3f;

constexpr const wchar_t* PARAM_SPEED				= L"Speed";
constexpr const wchar_t* PARAM_SPRINT				= L"Sprint";
constexpr const wchar_t* PARAM_COMBO				= L"Combo";
constexpr const wchar_t* PARAM_JUMP					= L"Jump";
constexpr const wchar_t* PARAM_INPUTDIR				= L"InputDir";

constexpr const wchar_t* PARAM_HIT					= L"Hit";
constexpr const wchar_t* PARAM_HITDIR				= L"HitDir";
constexpr const wchar_t* PARAM_HITTYPE				= L"HitType";
constexpr const wchar_t* PARAM_PARRY				= L"Parry";

constexpr const wchar_t* PARAM_MOVEDIR				= L"MoveDir";
constexpr const wchar_t* PARAM_WEAPONTYPE			= L"WeaponType";
constexpr const wchar_t* PARAM_WEAPONID				= L"WeaponID";
constexpr const wchar_t* PARAM_GRINDER				= L"Grinder";
constexpr const wchar_t* PARAM_CHARGE				= L"Charge";
constexpr const wchar_t* PARAM_GUARD				= L"Guard";
constexpr const wchar_t* PARAM_GROUNDED				= L"Grounded";
constexpr const wchar_t* PARAM_LOCKON				= L"LockOn";
constexpr const wchar_t* PARAM_DODGE				= L"Dodge";
constexpr const wchar_t* PARAM_STAMINAEMPTY			= L"StaminaEmpty";



constexpr const wchar_t* PARAM_GRAPPLE_HIT			= L"GrappleHit";
constexpr const wchar_t* PARAM_GRAPPLE_UPGRADE		= L"GrappleUpgrade";
constexpr const wchar_t* PARAM_GRAPPLE_NEAR			= L"GrappleNear";
constexpr const wchar_t* PARAM_GRAPPLE_ZIP			= L"GrappleZip";
constexpr const wchar_t* PARAM_GRAPPLE_AIMING		= L"GrappleAiming";
constexpr const wchar_t* PARAM_GRAPPLE_ZIPFALL		= L"GrappleZipFall";

constexpr const wchar_t* PARAM_GRABHOLD				= L"GrabHold";
constexpr const wchar_t* PARAM_GRABFRONT			= L"GrabFront";
constexpr const wchar_t* PARAM_THROW				= L"Throw";


constexpr const wchar_t* PARAM_CHARGEGUN_FIRE		= L"ChargeGunFire";
constexpr const wchar_t* PARAM_CHARGEGUN_EIXT		= L"ChargeGunExit";

constexpr const wchar_t* PARAM_REACT				= L"React";

constexpr const wchar_t* PARAM_CHEST_LOOPEND		= L"Open_Chest_End";
constexpr const wchar_t* PARAM_SIT_LOOPEND			= L"Sit_LoopEnd";
constexpr const wchar_t* PARAM_STARGAZER_LOOPEND	= L"Stargazer_LoopEnd";

constexpr const wchar_t* PARAM_STATROOM_WEAPON		= L"StatRoom_Weapon";
constexpr const wchar_t* PARAM_STATROOM_LEGIONARM	= L"StatRoom_LegionArm";
constexpr const wchar_t* PARAM_LEGIONARM_TYPE		= L"LegionArmType";

constexpr const wchar_t* PARAM_FALLING				= L"Falling";
constexpr const wchar_t* PARAM_REQUEST_HEAL			= L"Request_Heal";
constexpr const wchar_t* PARAM_REQUEST_LAMP_ON		= L"Request_Lamp_On";
constexpr const wchar_t* PARAM_REQUEST_LAMP_OFF		= L"Request_Lamp_Off";

class CAnimDriver :public CComponent
{
private:
	//8이면 입력 없음 
	enum DIR8 { DIR_F, DIR_FR, DIR_R, DIR_BR, DIR_B, DIR_BL, DIR_L, DIR_FL, End};
public:
	typedef struct tagAnimDriverDesc
	{
		CTransform*				pTransform		= { nullptr };
		CAnimator*				pAnimator		= { nullptr };
		CInputComponent*		pInput			= { nullptr };
		CStatusComponent*		pStatus			= { nullptr };
		CStatComponent*			pStat			= { nullptr };
		CMovement*				pMovement		= { nullptr };
		CEquipment*				pEquipment		= { nullptr };
		CTargetComponent*		pTarget			= { nullptr };
		//CLegionArm_Manager*		pLegionArm		= { nullptr }; // 생성 시점 문제때문에 따로 Set하기 
		CModel*					m_pModel		= { nullptr };
	} ANIMDRIVER_DESC;

	typedef struct tagPendingAction
	{
		AnimAction 				eAction			= { AnimAction::End };
		_float					fStaminaCost	= { 0.f };
		_int					iComboStep		= { -1 };
	} PendingAction;

	typedef struct tagActionEntry
	{
		_uint64					iActionHash		= { 0 };
		ActionCategory			eCategory		= { ActionCategory::End };
	} ACTION_ENTRY;

private:
	CAnimDriver(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CAnimDriver() = default;

	CAnimDriver(const CAnimDriver& Prototype) = delete;
	CAnimDriver& operator=(const CAnimDriver& rhs) = delete;

public:
	HRESULT						Initialize(const ANIMDRIVER_DESC& Desc);
	void						Update(_float fTimeDelta);
public:
	/* CutScene */
	// Editor_Map에서도 호출하므로 필요한 컷신 함수만 GameCore DLL 심볼로 공개함
	GAMECORE_DLL _bool			Apply_CutSceneAnimation(const CUTSCENE_ANIMATION_SECTION* pSection, _float fCutSceneTime, _bool bSeek = false);
	GAMECORE_DLL _bool			Get_CutSceneRootMotionDelta(_float3* pTranslation, _float4* pRotation) const;
	void						Clear_CutSceneAnimation();
	_bool						Is_CutSceneControlled() const { return m_bCutSceneControlled; }
	_uint						Get_CutSceneSectionID() const { return m_iCutSceneSectionID; }

public:
	//모든 상태 전환은 이 함수를 통해서만.  상태 초기화도 여기서 진행 
	void						Set_CurrentAction(AnimAction eNext);
	void						Set_LegionArmManager(CLegionArm_Manager* pLegionArm);
	//단일 액션플레이 함수. 애니메이터 파라미터 설정외의 기능이 필요하는경우 따로 뺴기(ex Hit) 
	void						Play_Action(AnimAction eAction);
	void						Play_HitAction(HIT_TYPE eHitType = HIT_TYPE::Normal, HIT_DIR eHitDir = HIT_DIR::Front, REACT_TYPE eReact = REACT_TYPE::Hit);


	const vector<PLAYER_FX_EVENT>& Get_FxEvents() const { return m_FxEvents; }
	const vector<LegionArmEvent>& Get_LegionArmEvents() const { return m_LegionArmEvents; }
	const vector<PlayerAnimEvent>& Get_AnimEvents() const { return m_PlayerAnimEvents; }

	_bool						Is_GrappleActive() const;

	_bool						Is_ActionActive(AnimAction eAction) const;
	_bool						Is_Locomotion() const;
	_bool						Is_FatalStart();

	void						Force_Idle(_bool bForceIdle) { m_bForceIdle = bForceIdle; }
	virtual void				Clear() override;
	void						Reset_CamShakeWindow();

	void						Drive_LoopSounds();
	void						Stop_AllLoopSounds();

	void						End_InteractLoop();
public:
	void						Set_LadderMode(_bool bLadderMode);
	void						Play_LadderAction(AnimAction eAction);

public:
	//포즈레이어용 Request
	_bool						Requst_Overlap(PlayerAnimEvent eAction);
	_bool						Is_OverlayPlaying();
public:
	// 외부에서 파라미터 Set 하고 play_Action호출
	void						Set_Param(const wchar_t* szName, _bool bValue);
	void						Set_Param(const wchar_t* szName, _int iValue);
	void						Set_Param(const wchar_t* szName, _float fValue);

	void						Set_ItemSlot(ITEM_SLOT eSlot) { m_eItemSlot = eSlot; }

public:
	// 추가
	// 현재 플레이어 위치에서 실제 페이탈 실행이 가능한 대상을 찾는 함수
	CAIObject*					Find_FatalPossibleTarget(); 

private:
	void						Ready_ActionTable();
	_uint64						Find_HashName(AnimAction eAction);

	AttackType					Get_AttackType(AnimAction eAction) const;
	//인터렉션용 파라미 데이터 반환 
	const wchar_t*				Get_InteractLoopParam(AnimAction eAction) const;
private:
	//스태미나 소모 
	void 						Request_OnceAction(AnimAction eAction);
	//스테미나 소모 X
	void						Request_Reaction(AnimAction eAction);
	void						Request_Attack(AnimAction eAction);
	
	void						Drive_Intent(_float fTimeDelta);
	void						Drive_ChargeHeavy(_float fTimeDelta);
	_bool						Drive_Fatal(_float fTimeDelta);
	void						Begin_CutSceneAnimation();
	void						Play_CutSceneIdle();

	void						Drive_Locomotion(_float fTimeDelta);
	void						Drive_Actions(_float fTimeDelta);
	void						Drive_Stance(_float fTimeDelta);
	_bool						Drive_Reaction(_float fTimeDelta);
	
	void						Apply_Result(_float fTimeDelta);
	void						Apply_GameplayFlags();
	void						Apply_ActionResult();

	void						Cancel_Fatal();
	//m_pFatalTarget 초기화 X 버전 
	void						End_FatalApproach();

	void						Try_Attack(PlayerActionID eInput, AnimAction eType);

	void						On_Notify();
	void						Handle_PlaySoundRule(const string& strPayload);
	void						Play_CueNotify(const string& strNotifyName);
	void						Play_FootStepCue();

	_bool						Check_StaminaCost() const;
	_bool						Check_FableCost(AnimAction eAction) const;
	
	_bool						Can_Accept_Attack(AnimAction eType) const;
	_bool						Can_Start_Action(AnimAction eAction) const;
	

	DIR8						Compute_InputDir8(_int iRawX, _int iRawY);
	DIRECTION 					Compute_Direction(_int iRawX, _int iRawY);

	void						Play_NotifyEffect(const ANIM_NOTIFY* pNotify);

private:
	_bool						Get_OverlayInfo(PlayerAnimEvent eAction, _wstring& strOutParam, _uint64& iOutHash);
	void						Verify_OverlayStarted();
	void						Clear_OverlayRequest();

private:
	static constexpr _float		ACTION_STAMINA_COST		= { 20.f };
	static constexpr _float		ROTSPEED_MLT			= { 3.f };

	static constexpr _float		FATAL_APPROACH_SPEED	= { 8.f };
	static constexpr _float		FATAL_TURN_SPEED		= { XMConvertToRadians(720.f) };
	static constexpr _float		PARRY_WINDOW			= { 0.25f };

	const _float				FATAL_ANGLE_COS			= { cosf(XMConvertToRadians(50.f)) };

private:
	const _wstring				WIN_NAME_GRINDER_EFFECT = { L"Grinder_Effect" };
	const string				NOTIFY_CAMERA_SHAKE		= { "Play_CameraShake" };	
	const string				NOTIFY_CAMERA_SCENE		= { "Play_CameraScene" };
	const string				NOTIFY_LAST_FATAL_BEGIN	= { "Last_Fatal_Begin" };
	const string				NOTIFY_LAST_FATAL_END	= { "Last_Fatal_End" };
	const string				NOTIFY_ROOTMOTION_SCALING = { "RootMotion_Scaling" };
	const string				NOTIFY_ROOTMOTION_CLEAR = { "RootMotion_Clear" };
	const string				NOTIFY_WEAPON_SWAP_END	= { "Weapon_SwapEnd" };
	const string				NOTIFY_GRAPPLE_WIRELAUNCH = { "Grapple_WireLaunch" };
	const string				NOTIFY_GRAPPLE_PULL		= { "Grapple_Pull" };
	const string				NOTIFY_GRAPPLE_FAIL		= { "Grapple_Fail" };
	const string				NOTIFY_GRAPPLE_CHASE	= { "Grapple_Chase" };
	const string				NOTIFY_GRAPPLE_STRIKE	= { "Grapple_Strike" };
	const string				NOTIFY_GRAPPLE_SMASH	= { "Grapple_Smash" };
	const string				NOTIFY_HITSTOP			= { "HitStop" };
	const string				NOTIFY_MODEL_EFFECT		= { "Model_Effect" };

	const string				NOTIFY_CHARGEGUN_AIM_READY = { "ChargeGun_AimReady" };
	const string				NOTIFY_CHARGEGUN_FIRE	= { "ChargeGun_Fire" };
	const string				NOTIFY_CHARGEGUN_EXIT	= { "ChargeGun_Exit" };

	const string				NOTIFY_CAMERA_WORK		= { "Play_CameraWork" };
	const string				NOTIFY_CAMERA_WORK_END	= { "Stop_CameraWork" };

	const string				NOTIFY_HEAL				= { "Start_Heal" };

	const string				CAM_SHAKE_GUARD_NORMAL	= { "BP_Shake_Guard_Normal" };
	const string				CAM_SHAKE_GUARD_BREAK	= { "BP_Shake_Guard_Break" };
	const string				CAM_SHAKE_GUARD_STRONG	= { "BP_Shake_Guard_Strong" };
	const string				CAM_SHAKE_HIT_NORMAL	= { "BP_Shake_PCHit_Hit" };
	const string				CAM_SHAKE_HIT_KnockBack = { "BP_Shake_PCHit_KnockBack" };

	const _uint64				GRAPPLE_STATE_HASHES[7] = 
	{
		Hash_String(L"Grapple_Hit"),  
		Hash_String(L"Grapple_Fail"),
		Hash_String(L"Grapple_Chase"),  
		Hash_String(L"Grapple_Smash"),
		Hash_String(L"Grapple_Zip"),
		Hash_String(L"Grapple_AirZip"),
		Hash_String(L"Grapple_AirChase"),
	};

private:
	CTransform*					m_pTransformCom			= { nullptr };
	CAnimator*					m_pAnimatorCom			= { nullptr };
	CInputComponent*			m_pInputCom				= { nullptr };
	CStatusComponent*			m_pStatusCom			= { nullptr };
	CStatComponent*				m_pStatCom				= { nullptr };
	CMovement*					m_pMovementCom			= { nullptr };
	CEquipment*					m_pEquipmentCom			= { nullptr };
	CTargetComponent*			m_pTargetCom			= { nullptr };
	CLegionArm_Manager*			m_pLegionArmCom			= { nullptr };
	CModel*						m_pModelCom				= { nullptr };
private:
	unordered_map<AnimAction, ACTION_ENTRY>		m_ActionTable = {};
	//이번 프레임에 요청한 Action들 
	unordered_map<_uint64, PendingAction>		m_PendingActions;

	//오너에게 보낼 이벤트들 
	vector<PLAYER_FX_EVENT>		m_FxEvents = {};
	vector<LegionArmEvent>		m_LegionArmEvents = {};
	vector<PlayerAnimEvent>		m_PlayerAnimEvents = {};

	_bool						m_bForceIdle = { false };
	_bool						m_bCutSceneControlled = { false };
	_uint						m_iCutSceneSectionID = { UINT_MAX };
	_uint64						m_iCutSceneClipHash = {};
	_float						m_fCutSceneLastTime = {};
	_bool						m_bCutSceneRootMotionGenerated = { false };
	CAIObject*					m_pFatalTarget			= { nullptr };

	AnimAction					m_eCurrentAction		= { AnimAction::End };
	AttackType					m_eCurrentAttack		= { AttackType::End };

	_int						m_iComboStep			= { -1 };	

	const ActionData*			m_pCurActionData		= { nullptr };

	_bool						m_bGrindPrev			= { false };
	_bool 						m_bApproaching			= { false };
	_bool						m_bCharged				= { false };

	_float						m_fChargeHoldTime		= { 0.f };
	_bool						m_bStaminaEmpty			= { false };

	_bool						m_bSprint = {};

	_bool						m_bHitActive			= {};
	_bool						m_bCanCamMove = {true};

	_bool						m_bHitRequested			= { false };
	HIT_TYPE					m_ePendingHitType		= { HIT_TYPE::Normal };
	HIT_DIR						m_ePendingHitDir		= { HIT_DIR::Front };
	REACT_TYPE					m_ePendingReactType		= { REACT_TYPE::Hit };
	PlayerAnimEvent				m_ePendingVerify		= { PlayerAnimEvent::End };
	PlayerAnimEvent				m_ePendingClear			= { PlayerAnimEvent::End };

	_bool						m_bParry				= {};
	_bool						m_bGuardPrev			= { false };
	_float						m_fGuardElapsed			= { 0.f };
	_float						m_fRoootMotionScaling = { 1.f };
	
	_bool						m_bLadderMode			= { false };
	AnimAction					m_ePending_LadderAction	= { AnimAction::End };
	AnimAction					m_ePending_PlayAction	= { AnimAction::End };
	const _wstring				m_strCutSceneIdleClip	= { L"AS_Pino_B_Idle" };

	string						m_strFootStepSurface = { "Stone" };

	//Shake 윈도우 관리하는 클래스
	CShakeWindowSync			mShakeWindowSync;
	unordered_map<_uint64, _uint>	m_LoopSounds;
	ITEM_SLOT					m_eItemSlot				= { ITEM_SLOT::Heal };
public:
	static CAnimDriver* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const ANIMDRIVER_DESC& Desc);
	virtual void		Free() override;

};

NS_END
