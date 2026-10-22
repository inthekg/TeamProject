#pragma once

#include "Core_Defines.h"
#include "GameObject.h"
#include "AnimationData.h"

NS_BEGIN(Engine)

class CAnimator;
class CModel;
class CStaticRigidBody;
struct RenderCommand;
NS_END

NS_BEGIN(GameCore)

typedef struct tagObjUIEventDesc : public Engine::Event_Desc
{
	_bool bVisible = { false };
	 OBJECT_TYPE eObjType= OBJECT_TYPE::END;
} OBJ_UI_EVENT_DESC;


class GAMECORE_DLL CMapGameplayObject : public CGameObject
{
public:
	enum class VISUAL_TYPE
	{
		NONE,
		STATIC,
		SKINNED,
		END
	};

public:
	typedef struct tagMapGameplayObjectDesc : public CGameObject::GAMEOBJECT_DESC
	{
		// 다른 MapGameplayObject를 연결할 때 사용하는 SpawnID다.
		_uint iTargetSpawnID = { UINT_MAX };

		// 오브젝트의 실제 렌더링 방식을 선택한다.
		VISUAL_TYPE eVisualType = { VISUAL_TYPE::SKINNED };

		// Static/Skinned 공통 모델 정보다.
		_wstring strModelPrototypeTag = {};
		_uint iModelLevel = {};

		// Skinned 모드에서만 사용하는 Animator 정보다.
		_wstring strAnimFolderTag = {};
		_wstring strAnimTag = {};
		_wstring strDefaultClip = {};
		_wstring strDefaultSequence = {};

		const char* pRootBoneName = { "Root" };

		ANIMATION_TIME_UNIT eAnimTimeUnit = { ANIMATION_TIME_UNIT::MILLISEC };

		_bool bStaticAnimation = { true };
		_bool bControllerEnabled = { false };
		_bool bPlayDefault = { true };
		_bool bApplyRootMotion = { false };
		// 범용 MapGameplayObject가 지정된 기본 Clip을 계속 반복할지 결정한다.
		_bool bLoopDefaultClip = { false };
		_bool bInteractionEnabled = { true };

		// Static Rigidbody 정보다.
		_bool bUsePhysics = { false };
		_bool bPhysicsStartRegistered = { true };

		_float3 vPhysicsHalfExtents = { 0.5f, 0.5f, 0.5f };
		_float3 vPhysicsLocalOffset = { 0.f, 0.f, 0.f };
		_float4 vPhysicsLocalQuat = { 0.f, 0.f, 0.f, 1.f };

		_uint iPhysicsFilterWord0 = { ETOUI(FILTER_GROUP::OBJECT) };
		_uint iPhysicsFilterWord1 = { ETOUI(FILTER_GROUP::PLAYER) | ETOUI(FILTER_GROUP::MONSTER) };

		// 상호작용 위치 보정 사용 여부, 액터 로컬 XZ 위치와 고정 Yaw 회전값이다.
		_bool bUseInteractPosition = { false };
		_float2 vInteractLocalOffsetXZ = { 0.f, 0.f };
		_float fInteractLocalYaw = { 0.f };
	} MAP_GAMEPLAY_OBJECT_DESC;

protected:
	CMapGameplayObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMapGameplayObject(const CMapGameplayObject& Prototype);
	virtual ~CMapGameplayObject() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

	virtual void Priority_Update(_float fTimeDelta) override;
	virtual void Update(_float fTimeDelta) override;
	virtual void Physical_Update(_float fFixedTime) override;
	virtual void Late_Update(_float fTimeDelta) override;

	virtual HRESULT Render() override;
	virtual HRESULT Render_Shadow() override;

public:
	VISUAL_TYPE Get_VisualType() const { return m_eVisualType; }

	CAnimator* Get_Animator() const { return m_pAnimator; }
	CModel* Get_Model() const { return m_pModelCom; }

public:
	//추가 
	virtual _bool Can_Interact(CGameObject* pPlayer) const { return m_bInteractionEnabled; }
	virtual _bool Interact(CGameObject* pPlayer) { return false; }

	// 플레이어 정렬에 사용할 Scale 제거 월드 행렬을 반환한다.
	virtual _float4x4 Get_InteractTransform(CGameObject* pPlayer) const;

	// 액터 기준으로 설정된 원본 로컬 XZ 보정값을 반환한다.
	_float2 Get_InteractOffset() const { return m_vInteractLocalOffsetXZ; }

	// 최종 상호작용 행렬이 바라보는 월드 Look 방향을 반환한다.
	_float3 Get_InteractLook(CGameObject* pPlayer) const;
public:
	virtual void Set_MapTransform(const _float3& vScale, const _float3& vRotationDegree, const _float3& vPosition);

	// MapTriggerManager가 SpawnID로 대상을 찾아 호출하는 공통 Hook이다.
	virtual void On_LinkedTriggerBegin(CGameObject* pInstigator) {}
	virtual void On_LinkedTriggerEnd(CGameObject* pInstigator) {}
	virtual void On_LinkedActivate() {}

	// MapTriggerManager가 연결 대상의 외부 Trigger 제거 요청을 GameCore 안에서 바인딩한다.
	void Bind_LinkedTriggerDeactivate(function<void()> FuncDeactivate) { m_Func_DeactivateLinkedTriggers = FuncDeactivate; }

	// Manager가 실제 생성 객체에 부여한 Chapter 내 고유 SpawnID다.
	void Set_SpawnID(_uint iSpawnID) { m_iSpawnID = iSpawnID; }
	_uint Get_SpawnID() const { return m_iSpawnID; }

	// TargetSpawnID로 연결된 이벤트와 컷신 Asset을 Player 상호작용 완료 시점까지 보관한다.
	void Configure_InteractionEvent(EVENT_TYPE_MAP_OBJECT eEventType, const string& strAssetName, _bool bPlayOnce);
	void Dispatch_InteractionEvent();

public:
	HRESULT Play_AnimClip(const _wstring& strClipName);
	HRESULT Play_AnimSequence(const _wstring& strSequenceName);

	void Request_AnimState(const _wstring& strStateName);
	void Request_AnimAction(const _wstring& strActionName);
	void Request_AnimSequenceEnd();

	void Set_AnimBool(const _wstring& strKey, _bool bValue);
	void Set_AnimInt(const _wstring& strKey, _int iValue);
	void Set_AnimFloat(const _wstring& strKey, _float fValue);
	void Set_AnimTrigger(const _wstring& strKey);

	_bool Is_AnimationFinished() const;
	_bool Is_AnimInState(const _wstring& strStateName) const;
	_bool Was_AnimStateChanged() const;
	_bool Set_MeshVisible(_uint iMeshIndex, _bool bVisible);
	_bool Is_UseInteractPosition() const { return m_bUseInteractPosition; }

protected:
	// Stargazer처럼 런타임에 모델과 Animator를 교체하는 객체가 사용한다.
	HRESULT Change_Visual(const MAP_GAMEPLAY_OBJECT_DESC& Desc);

	void Drive_Animator(_float fTimeDelta);

protected:
	HRESULT Register_Physics();
	void UnRegister_Physics();
	_bool Is_PhysicsRegistered() const;

	void Broadcast_ObjUI(_bool bVisible, OBJECT_TYPE eObjType);
	void Deactivate_LinkedTriggers();

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) {}
	virtual void On_AnimNotify(_uint64 iNotifyHash) {}
	virtual void Apply_RootMotion(const ANIM_ROOT_MOTION_DELTA& Root, _float fTimeDelta);

	// 파생 MapGameplayObject가 공통 렌더 코드를 복사하지 않고 객체별 렌더 값을 설정하는 seam이다.
	virtual void Configure_RenderCommand(Engine::RenderCommand* pCommand) const;

private:
	HRESULT Ready_Visual(const MAP_GAMEPLAY_OBJECT_DESC& Desc);
	HRESULT Ready_Model(const MAP_GAMEPLAY_OBJECT_DESC& Desc);
	HRESULT Ready_Animator(const MAP_GAMEPLAY_OBJECT_DESC& Desc);
	HRESULT Play_InitialAnimation(const MAP_GAMEPLAY_OBJECT_DESC& Desc);

	HRESULT Ready_Physics(const MAP_GAMEPLAY_OBJECT_DESC& Desc);

	HRESULT Render_Static(_bool bCheckMaterialPass);
	HRESULT Render_Skinned(_bool bCheckMaterialPass);

protected:
	CModel* m_pModelCom = { nullptr };
	CAnimator* m_pAnimator = { nullptr };
	CStaticRigidBody* m_pPhysicsRigidBody = { nullptr };

	vector<_bool> m_MeshVisibles = {};
	function<void()> m_Func_DeactivateLinkedTriggers = { nullptr };
	_uint m_iSpawnID = { UINT_MAX };
	EVENT_TYPE_MAP_OBJECT m_eInteractionEvent = { EVENT_TYPE_MAP_OBJECT::END };
	string m_strInteractionCutSceneAssetName = {};
	_bool m_bInteractionEventPlayOnce = { false };
	_bool m_bInteractionEventConsumed = { false };

	VISUAL_TYPE m_eVisualType = { VISUAL_TYPE::END };
	_bool m_bApplyRootMotion = { false };
	_bool m_bLoopDefaultClip = { false };
	_bool m_bInteractionEnabled = { true };
	_wstring m_strLoopDefaultClip = {};

	// 액터가 제공하는 플레이어 정렬용 로컬 XZ 위치와 고정 Yaw 회전값이다.
	_bool m_bUseInteractPosition = { false };
	_float2 m_vInteractLocalOffsetXZ = { 0.f, 0.f };
	_float m_fInteractLocalYaw = { 0.f };

public:
	static CMapGameplayObject* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
