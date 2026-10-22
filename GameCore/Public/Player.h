#pragma once
#include "Core_Defines.h"
#include "Character.h"
#include "InputComponent.h"
#include "IListener.h"

NS_BEGIN(Engine)
class CMasterModel;
class CAnimator;
class CCT;
class CSpringArm;
class CStatusComponent;
class CEffect;
class CDynamicRigidBody;
class CDissolve;
NS_END

NS_BEGIN(GameCore)
class CAIObject;
class CBodyPart;
class CMovement;
class CStatusComponent;
class CStatComponent;
class CPlayerStateMachine;
class CEquipment;
class CAnimDriver;
class CTargetComponent;
class CLegionArm_Manager;
class CMapGameplayObject;
class CPinoLamp;
class CAimComponent;
class CPartSet;
class CInventory;
typedef struct tagItemData ITEM_DATA;
struct CUTSCENE_TRANSFORM_VALUE;
struct CUTSCENE_ANIMATION_SECTION;

class GAMECORE_DLL CPlayer : public CCharacter, public IListener
{
private:
	CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CPlayer(const CPlayer& Prototype);
	virtual ~CPlayer() = default;

public:
	virtual HRESULT					Initialize_Prototype() override;
	virtual HRESULT					Initialize(void* pArg) override;

	virtual void					Priority_Update(_float fTimeDelta) override;
	virtual void					Update(_float fTimeDelta) override;
	virtual void					Physical_Update(_float fTimeDelta) override;
	virtual void					Late_Update(_float fTimeDelta) override;
	virtual HRESULT					Render() override;
	virtual HRESULT					Render_Shadow() override;
	
public:
	CInputComponent*				Get_InputComponent() { return m_pInputCom; }
	CStatusComponent*				Get_StatusComponent() { return m_pStatusCom; }
	CStatComponent*					Get_StatComponent() { return m_pStatCom; }
	CMovement*						Get_MovementComponent() { return m_pMovementCom; }
	CEquipment*						Get_EquipmentComponent() { return m_pEquipmentCom; }
	CCT*							Get_CCTComponent() { return m_pCCT; }
	CSpringArm*						Get_SpringArmComponent() { return m_pSpringArm; }
	CAnimDriver*					Get_AnimDriverComponent() { return m_pAnimDriver; }
	CPartSet*						Get_PartSet() { return m_pPartSet; }
	CLegionArm_Manager*				Get_LegionArmManager() { return m_pLegionArm_Manager; }
	CInventory*						Get_Inventory() { return m_pInventory; }
	CTargetComponent*				Get_TargetComponent() { return m_pTargetCom; }
	CAimComponent*					Get_AimComponent() { return m_pAimCom; }
public:
	// 리스폰 - 전체 리셋 + 풀 회복
	void							Respawn(const Respawn_Point& Point);
	// 맵 전환용 일부 컴포넌트 초기화 
	void							Reset_ForChangeMap(const Respawn_Point& Point);
	void							Teleport(const Respawn_Point& Point);
	void							Apply_CutSceneControl(const CUTSCENE_TRANSFORM_VALUE& Transform, const CUTSCENE_ANIMATION_SECTION* pAnimationSection, _float fCutSceneTime);
	void							Clear_CutSceneControl();

	// 추가 
	void							Apply_ExternalDisplacement(_fvector vDisplacement);

	virtual void					Begin_Overlap(CGameObject* pTrigger) override;
	virtual void					End_Overlap(CGameObject* pTrigger) override;

	//몬스터에게 데미지를 입혔을때
	void							On_Attack(_float fDamage);

	/* Item Slot*/
	ITEM_SLOT						Get_ItemSlot() const {return m_eItemSlot;}

	_bool							Is_DonkeyMask() const;


public:
	virtual _bool					Take_Damage(const HitInfo& hitInfo) override;
	virtual void					Begin_Contact(CGameObject* pOther, _fvector vNormal) override;
	virtual _bool 					Is_Dead() const override;
	_bool							Is_LadderActive() const { return m_eLadderState != LADDER_CLIMB_STATE::END; }
	_bool							Can_Interact();
public:
	/* Stargazer */
	void							Play_Stargazer_Active();
	void							Play_Stargazer_InActive();
	CEffect*						Play_BodySurfaceEffect(const string& strEffectName); 

	/* Grab */
	void							Start_Grab(CAIObject* pGrabber);
	void							Cancel_Grab();
	void							End_Grab();

	_float							Get_CCTRadius() const;

	void							Launch_Explosion(_float fDamage, _fvector vExplosionPos, _float fPower, _float fUpPower);

	/* Global Time Scale*/
	void							Set_GlobalTimeScale_Offset(_float fTimeScaleNeg)
	{
		m_fGlobalTimeScale_Offset_Pending = fTimeScaleNeg;
	}

private:
	void							Update_Dead(_float fTimeDelta);
	void							Update_Materialize(_float fTimeDelta);
	void							Play_DissolveStartFx(const string& strFxName);

	void							Update_Ladder(_float fTimeDelta);
	void							Update_LockOn();
	void							Update_Teleport(_float fTimeDelta);

	// Test0731
	void							Face_DialogueSpeaker(Event_Desc* pEventDesc);
	void							Release_DialogueCamera();

	/* Grab */
	void							Update_Grab(_float fTimeDelta);
	void							LateUpdate_Grab();

private:
	HRESULT							Ready_Components();
	HRESULT							Ready_PartObjects();
	HRESULT							Ready_Animator();
	HRESULT							Ready_Inventory();
	void							Ready_Event_Subscription();

	void							Apply_Dissolve(_float fAmount);
	
private:
	/* Throw */
	HRESULT							Throw(_fvector vCenter, _fvector vThrowDir, _float fThrowSpeed, _float fUpSpeed);
	void							Finish_Throw();
	void							RemoveActor_FromScene();

	void							Launch_Back(const HitInfo& hitInfo);
	void							Interact_Triggers();


private:
	virtual void					OnEvent(EVENT_TYPE etype, Event_Desc* pData) override;
	virtual void					OnEvent(EVENT_TYPE_MAP_OBJECT etype, Event_Desc* pData) override;
	virtual void					OnEvent(EVENT_TYPE_UI etype, Event_Desc* pData) override;
	void							Drive_Effect(const PLAYER_FX_EVENT& fxEvent);
	void						    Drive_ItemSlot();

	_bool							Handle_GetItem(ITEM_TYPE eItemType, _uint64 iItemID);
private:
	/* Hit */
	HIT_DIR							Compute_HitDir(const HitInfo& hitInfo, _float& outForwardDot) const;
	_bool							Is_Defenseless(_bool bFrontHit);
	_bool							Can_Guard(const HitInfo& hitInfo) const;

	void							React_NoStaminaGuard(const HitInfo& hitInfo, HIT_DIR hitDir);
	void							React_NoGuard(const HitInfo& hitInfo, HIT_DIR hitDir);
	void							React_Unguarded(const HitInfo& hitInfo, HIT_DIR hitDir);
	void							React_Parry(const HitInfo& hitInfo, HIT_DIR hitDir);
	void							React_Guard(const HitInfo& hitInfo, HIT_DIR hitDir);


private:
	/* Ladder */
	// 사다리 Bottom 발판 Pos / 사다리 Top 발판 Pos / 사다리 자체 Top 위치 / 사다리 Top 월드 매트릭스(정렬용)
	void							Begin_LadderClimb(const _float3& vBottomLanding, const _float3& vTopLanding, const _float3& vTopPosition, const _float4x4& TopWorldMatrix);
	void							End_LadderClimb();
	void							Enter_LadderIdle();
	void							Release_LadderMode();

	void							Try_LadderStep(_int iDir);
	void							Play_LadderAction(AnimAction eAction);

	AnimAction						Get_LadderStepAction(_int iDir) const;
	AnimAction						Get_LadderEndAction(_int iDir) const;
	AnimAction						Get_LadderIdleAction() const;

private:
	/* UI Event */
	void							Update_UILock();
	void							Set_Player_UILock(_bool bLock);
	
	//바로 텔레포트 시작 
	void							Translation_Chapater();

private:
	static vector<_wstring>			m_EffectNames[ETOUI(AttackType::End)];
	static _float					m_KnockBackDists[ETOUI(HIT_TYPE::End)];

	private:
	CPinoLamp*						m_pLamp								= { nullptr };
	string							m_strLampBoneName					= { "BN_Lamp_01" }; //BN_Lamp_02 // BN_PCLamp
	_bool							m_bWantLockOn						= { false };
	_bool							m_bGrinding							= { false };

	_bool							m_bRespawnRequested					= { false };
	_bool							m_bDeadFlag							= { false };
	_float							m_fDeadTimer						= { 0.f };	
	_bool							m_bMaterializing					= { false };
	_bool							m_bDissolveFx						= { false };

	const _float4x4*				m_pGrabSocketBone					= { nullptr };
	GRAB_STATE						m_eGrabState						= { GRAB_STATE::END };
	CAIObject*						m_pGrabber							= { nullptr };
	_float							m_fGrabPhaseTime					= { 0.f };
	_bool							m_bThrowImpact						= { false };

private:
	/* Teleport*/
	_bool							m_bTeleportPending					= { false };
	_bool							m_bTeleportAnimStarted				= { false };
	_float							m_fTeleportTimer					= { 0.f };
	ChangeChapter_EventDesc			m_TeleportEventDesc					= {};

private:
	/* Ladder */
	_bool							m_bLadder_QuatFixPending			= { false };
	LADDER_CLIMB_STATE				m_eLadderState						= { LADDER_CLIMB_STATE::END };
	_bool							m_bLadder_FootLeft					= { false };
	_bool							m_bLadder_EnterFromBottom			= { true };	
	_bool							m_bLadder_ActionPending				= { false };

	_float3							m_vLadder_TopLandingPos				= {};
	_float3							m_vLadder_BottomLandingPos			= {};
	_float4							m_vLadder_FaceQuat					= {};
	_float3							m_vLadder_AttachPos					= {};

	_float							m_fLadder_LastY						= { 0.f };
	_float							m_fLadder_StepStartY				= { 0.f };

	_float							m_fLadder_GroundY					= { 0.f }; // 랜딩 Down 착지 높이  
	_float							m_fLadder_DeckY						= { 0.f }; // 랜딩 최종 높이

	_float3							m_vLadder_ExitStart					= {};
	_float3							m_vLadder_ExitTarget				= {};

	_float							m_fLadder_ExitAccumXZ				= {};
	_float							m_fLadder_ExitAccumY				= {};

	_float3							m_vLadder_EnterStart				= {};
	_float4							m_vLadder_EnterStartQuat			= {};

	_float							m_fGlobalTimeScale_Offset			= { 1.f };
	//1프레임 미루기용 . 슬로우를 건 1프레임동안 Offset을 적용하지않도록
	_float							m_fGlobalTimeScale_Offset_Pending	= { 1.f };

	ITEM_SLOT						m_eItemSlot							= { ITEM_SLOT::Heal };
private:
	//조준 자세를 얹은 포즈
	POSE							m_AimPose;
	list<CMapGameplayObject*>		m_TriggerObjects					= {};
	_bool							m_bUILock							= { false };

	//Test0731
	_bool							m_bDialogueCameraLocked = { false };
	_float3							m_vDialogueLookPos = {};
	_bool							bElectricityActive = { false };
	_int							m_iElectric_SoundID = { -1 };

	CEffect*						m_pElectricityFx = { nullptr };
private:
	/* Component */
	//ModelCom 업캐스트용 
	CMasterModel*					m_pMasterModelCom					= { nullptr };
	CCT*							m_pCCT								= { nullptr };

	CAnimator*						m_pAnimator							= { nullptr };
	CInputComponent*				m_pInputCom							= { nullptr };
	CSpringArm*						m_pSpringArm						= { nullptr };
	CStatusComponent*				m_pStatusCom						= { nullptr };
	CMovement*						m_pMovementCom						= { nullptr };
	CStatComponent*					m_pStatCom							= { nullptr };
	CEquipment*						m_pEquipmentCom						= { nullptr };
	CAnimDriver*					m_pAnimDriver						= { nullptr };
	CTargetComponent*				m_pTargetCom						= { nullptr };
	CEffect*						m_pChannel_GrinderFx				= { nullptr };
	//던져질때 CCT에서 교체될 Dynamic Rigid Body
	CDynamicRigidBody*				m_pThrowBody						= { nullptr };
	CLegionArm_Manager*				m_pLegionArm_Manager				= { nullptr };
	CPartSet*						m_pPartSet							= { nullptr };
	CInventory*						m_pInventory						= { nullptr };
	CDissolve*						m_pDissolveCom						= { nullptr };
	CAimComponent*					m_pAimCom							= { nullptr };
	//Ridig Actor 캐싱 
	PxRigidDynamic*					m_pRigidActor						= { nullptr };
public:
	static CPlayer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg);
	virtual void			Free();
};

NS_END
