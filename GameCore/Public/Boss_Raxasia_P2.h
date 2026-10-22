#pragma once
#include "AIObject.h"
#include "BT_Node.h"
#include "HitBox_Helper.h"
#include "IListener.h"

NS_BEGIN(GameCore)

class CWeapon_Monster;
class CShield_Raxasia;

class GAMECORE_DLL CBoss_Raxasia_P2 final : public CAIObject, public IListener
{
#ifdef _DEBUG
	DECLARE_REFLECTION(CBoss_Raxasia_P2)
#endif

public:
	enum class EFFECT_KEY { FURY, SYMBOL, END };

	typedef struct tagRaxasiaP2Desc : public CAIObject::AIOBJECT_DESC
	{
		_float fAttackGap = { 0.35f };
		_float fFatalFront = { 2.f };
		_float fFatalBack = { 0.f };
	} MONSTER_DESC;

public:
	typedef struct tagRaxasiaP2Link
	{
		_int   iFrom = { -1 };
		_int   iTo = { -1 };
		_bool  bUseRange = { false };
		_float fDistMin = { 0.f };
		_float fDistMax = { 0.f };
		_bool  bUsePercent = { false };
		_float fPercent = { 100.f };
		_bool  bUseAngle = { false };
		_float fAngleStart = { 0.f };
		_float fAngleEnd = { 360.f };
		_bool  bUseCool = { false };
		_float fCool = { 0.f };
		_int   iPriority = { 0 };
		_int   iWinStart = { -1 };
		_int   iWinEnd = { -1 };
	} P2_LINK;

	typedef struct tagRaxasiaP2Entry
	{
		_int   iPattern = { -1 };
		_float fDistMin = { 0.f };
		_float fDistMax = { 9999.f };
	} P2_ENTRY;

	typedef struct tagRaxasiaP2Warp
	{
		_int   iPattern = { -1 };
		_float fOffset = { 2.f };
		_float fSpeed = { 30.f };
	} P2_WARP;

	typedef struct tagRaxasiaP2RotWin
	{
		_int   iPattern = { -1 };
		_int   iF0 = { 0 };
		_int   iF1 = { 0 };
		_float fRad = { 0.f };
	} P2_ROTWIN;

	typedef struct tagRaxasiaP2FWarp
	{
		_int   iPattern = { -1 };
		_int   iF0 = { 0 };
		_int   iF1 = { 0 };
		_float fOffset = { 0.f };
		_float fSpeed = { 0.f };
		_int   iChance = { 100 };
	} P2_FWARP;

	typedef struct tagRaxasiaP2FGlide
	{
		_int   iPattern = { -1 };
		_int   iF0 = { 0 };
		_int   iF1 = { 0 };
		_float fSpeed = { 0.f };
		_float fOffset = { 0.f };
	} P2_FGLIDE;

	typedef struct tagRaxasiaP2OrbitGlide
	{
		_int   iPattern = { -1 };
		_int   iDir = { 1 };
		_int   iF0 = { 0 };
		_int   iF1 = { 0 };
		_float fSpeed = { 0.f };
	} P2_ORBITGLIDE;

	typedef struct tagRaxasiaP2Thunder
	{
		_float fRight = { 0.f };
		_float fForward = { 0.f };
		_float fUp = { 0.f };
	} THUNDER_OFFSET;

	typedef struct tagRaxasiaP2ThunderReserve
	{
		_float  fRemain = { 0.f };
		_float3 vOrigin = {};
		_float3 vDir = {};
	} THUNDER_RESERVE;
private:
	CBoss_Raxasia_P2(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBoss_Raxasia_P2(const CBoss_Raxasia_P2& Prototype);
	virtual ~CBoss_Raxasia_P2() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

public:
	virtual void				Action_Notify(_uint64 iHashKey) override;
	virtual void				On_Groggy(_bool bGroggy) override;
	virtual void				On_SuperArmor(_bool bOn) override;
	virtual void				On_AnimStateChanged(_uint64 iPrevStateHash, _uint64 iCurStateHash);
	virtual void				Begin_Battle() override;
	virtual void				OnEvent(EVENT_TYPE eType, Event_Desc* pAdata) override;
	virtual void				OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;
	virtual void				Spawn() override;
	virtual void				Despawn() override;
	virtual _bool				Can_FatalBack() const override;

	virtual void				Apply_ShieldBlock(_float fDamage) override;
	virtual void				On_ShieldBroken() override;

	_bool						Get_BladeGroundPos(_vector& OutPos);

private:
	virtual void				Update_BlackBoard(_float fTimeDelta) override;
	void						Fire_BoltSector(_bool bFull);
	void						Fire_BoltRange();
	void						Fire_GroundWave();
	void						Fire_ElectricBall();
	void						Reserve_Thunderstroke();
	void						Update_Thunderstroke(_float fTimeDelta);
	void						Fire_ReservedThunder(const THUNDER_RESERVE& tReserve);

private:
	_vector						Calc_ThunderPos(const THUNDER_OFFSET& Off) const;

	HRESULT						Ready_LinkTable();
	HRESULT						Ready_Weapon();
	HRESULT						Ready_Shield();
	HRESULT						Ready_HitBoxes();

	void						Collect_AuthoredStates();

	void						Mirror_ControllerParams();
	void						Update_CombatParams();
	void						Drive_Steering(_float fTimeDelta);
	void						Drive_FrameWarp(_float fTimeDelta);
	void						Drive_FrameGlide(_float fTimeDelta);
	void						Drive_OrbitGlide(_float fTimeDelta);
	virtual _float				Calc_RootMotionScale() const override;
	void						Update_LinkRunner(_float fTimeDelta);
	void						Begin_Charge();
	void						Update_Charge(_float fTimeDelta);
	void						End_Charge();

	_bool						Is_AnimIdle() const;
	_bool						Is_CurClipLooping() const;
	_bool						Has_PatternState(_int iPatternID) const;
	_int						Pick_NextLink(_int iFromID, _bool bWindowOnly);
	_bool						In_LinkWindow(_int iFromID, _int iToID) const;
	void						Get_ShowcaseBand(_uint iIdx, _float& fNear, _float& fFar) const;
	void						Commit_Pattern(_int iPatternID);
	void						Commit_Entry();
	const P2_WARP* Find_Warp(_int iPatternID) const;
	void						Begin_SkillMoveWarp(const P2_WARP& Warp);
	_bool						Pattern_HasRotWindows(_int iPatternID) const;
	_bool						Find_RotSpeed(_int iPatternID, _int iFrame, _float& fOutRad) const;
	_bool						Is_StepHoming(_int iPatternID) const;


	void						On_Fallen();
	void						On_GroggyStart();
	void						Break_Shield();
	void						Begin_StaggerMotion();


	/* Effect 관련 함수 */
	void						Turn_On_Sword();
	void						Turn_Off_Sword();
	void						Turn_On_Normal_Trail();
	void						Turn_On_Elec_Trail();
	void						Turn_Off_Trail();
	void						Turn_On_Body_Thunder();
	void						Turn_Off_Body_Thunder();
	void						Turn_On_Run_Trail();
	void						Turn_Off_Run_Trail();

private:
	CWeapon_Monster* m_pWeapon = { nullptr };
	CShield_Raxasia* m_pShield = { nullptr };
	CHitBox_Helper				m_HitBoxes;

	unordered_map<EFFECT_KEY, _uint64> m_EffectKeys;


	vector<P2_LINK>				m_Links;
	vector<P2_ENTRY>			m_Entries;
	vector<P2_WARP>				m_Warps;
	vector<P2_ROTWIN>			m_RotWindows;
	vector<P2_FWARP>			m_FWarps;
	vector<P2_FGLIDE>			m_FGlides;
	vector<P2_ORBITGLIDE>		m_OrbitGlides;
	_float						m_fGlideMinGap = { 3.f };
	unordered_map<_int, _int>	m_OrbitBlink;
	_float						m_fOrbitDist = { 3.5f };
	_float						m_fOrbitAngle = { 60.f };
	unordered_set<_int>			m_ChargeIn;
	unordered_set<_int>			m_FaceMove;
	_float						m_fFaceSpeed = { 720.f };
	_float						m_fGroggyTrackSpeed = { 360.f };
	_int						m_iActiveFWarp = { -1 };
	_bool						m_bCharging = { false };
	_float						m_fChargeTimer = { 0.f };
	_float						m_fChargeHold = { 2.f };
	unordered_set<_int>			m_StepHoming;
	_int						m_iStepPattern = { -1 };
	_float						m_fStepTimer = { 0.f };
	_float						m_fStepInterval = { 0.5f };
	_float4						m_vStepTarget = {};
	_float						m_fHomeStep = { 0.03f };
	_float						m_fHomeStopDist = { 2.f };
	_int						m_iHomeMode = { 1 };
	_float						m_fHomeRatio = { 0.1f };
	unordered_map<_int, _float>	m_MoveScale;
	unordered_set<_int>			m_AuthoredStates;
	unordered_map<_int, _float>	m_PatternLastUsed;
	unordered_map<_int, _int>	m_PatternUseCount;
	unordered_map<_int, _int>	m_PatternGroup;
	unordered_map<_int, _float>	m_GroupLastUsed;
	_float						m_fGroupCooldown = { 6.f };

	vector<THUNDER_OFFSET>		m_ThunderOffsets;
	_uint						m_iThunderIndex = { 0 };
	_float						m_fThunderDelay = { 1.0f };
	_float						m_fThunderLift = { 0.3f };
	_float						m_fThunderAccel = { 10.f };

	_float						m_fThunderSpeed = { 60.f };
	_float						m_fThunderRange = { 500.f };
	_float						m_fThunderRadius = { 0.4f };
	_float						m_fThunderDamage = { 700.f };
	string						m_strThunderWarnFX;
	vector<THUNDER_RESERVE>		m_ThunderReserves;

	_bool						m_bDead = { false };
	_bool						m_bBattleStarted = { false };
	_bool						m_bWeaponLive = { true };
	_bool						m_bPerceived = { false };
	_bool						m_bAttackCommit = { false };
	_bool						m_bAttackStarted = { false };
	_bool						m_bShieldBreaking = { false };
	_float						m_fShieldBreakGuard = { 0.f };

	vector<_int>				m_ShowcaseSeq;
	vector<_float>				m_ShowcaseDist;
	_uint						m_iShowcaseIdx = { 0 };
	_float						m_fIntroRunSpeed = { 3.f };

	_float						m_fDistToTarget = { 9999.f };
	_float						m_fAngleToTarget = { 0.f };
	_float						m_fAttackGap = { 0.35f };
	_float						m_fIdleTimer = { 0.f };
	_float						m_fCommitTimer = { 0.f };
	_float						m_fCommitTimeOut = { 1.2f };
	_float						m_fRunnerTime = { 0.f };

	_int						m_iCurPatternID = { -1 };
	unordered_map<_int, _bool>	m_LinkRoll;
	unordered_map<_int, _float>	m_TurnSpeed;
	_float						m_fRepeatCooldown = { 5.f };
	_int						m_iComboEntry = { -1 };
	_float						m_fStuckTimer = { 0.f };
	_float						m_fStartedTimeout = { 2.0f };



	static constexpr _float		BLADE_GROUND_PROBE_UP = { 2.f };
	static constexpr _float		BLADE_GROUND_PROBE_DOWN = { 8.f };
	static constexpr _float		EMITTER_GROUND_LIFT = { 0.5f };

	CEffect* m_pSwordFx = { nullptr };
	CEffect* m_pFuryFx = { nullptr };
	CEffect* m_pBodyFx = { nullptr };
	CEffect* m_pLeftFootFx = { nullptr };
	CEffect* m_pRightFootFx = { nullptr };

public:
	static CBoss_Raxasia_P2* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void				Free() override;

#ifdef _DEBUG
private:
	void						Debug_MoveLog(_float fTimeDelta);

	_float						m_fDbgLogTimer = { 0.f };
	_float3						m_vDbgPrevPos = {};
	_bool						m_bDbgPrevValid = { false };
	_int						m_iDbgLastState = { -2 };
	_int						m_iDbgForceID = { 106 };
	_int						m_iDbgGlideWin = { -1 };
	unordered_map<_int, _wstring>	m_DbgClipNames;
	void						Debug_ForceInput();
	void						Debug_ForcePattern(_int iPatternID);
#endif
};

NS_END
