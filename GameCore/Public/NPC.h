#pragma once

#include "MapGameplayObject.h"
#include "IListener.h"
#include "Core_Defines.h"

NS_BEGIN(GameCore)

class CPlayer;

class GAMECORE_DLL CNPC final : public CMapGameplayObject, public Engine::IListener
{
public:
	// UI가 NPC별 대화 데이터를 구분할 때 사용한다.

	// NPC 대화 시작과 종료 이벤트가 공유하는 대상 정보다.
	typedef struct tagNPCDialogueEventDesc : public Engine::Event_Desc
	{
		_uint iTargetSpawnID = { UINT_MAX };
		NPC_TYPE eNPCType = { NPC_TYPE::END };

		// 추가
		string	strDialogueID; // 재생할 대사 스크립트 ID
		_int	iEventIndex = { -1 };

		_float3	vSpeakerLookPos = {};
		_bool	bLockCamera = { false };
	} NPC_DIALOGUE_EVENT_DESC;

	enum class NPC_STATE
	{
		IDLE,
		TALK_START,
		TALKING,
		TALK_END,
		END
	};

public:
	typedef struct tagNPCDesc : public CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC
	{
		_uint iSpawnID = { UINT_MAX };
		NPC_TYPE eNPCType = { NPC_TYPE::END };
		_float fTalkMaxYawDegree = { 45.f };
	} NPC_DESC;

private:
	CNPC(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CNPC(const CNPC& Prototype);
	virtual ~CNPC() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

	// UI가 보내는 NPC_TALK_END 이벤트만 처리한다.
	virtual void OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;
	virtual void OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;

	// 외부 Map Trigger Box가 호출한다.
	virtual void On_LinkedTriggerBegin(CGameObject* pInstigator) override;
	virtual void On_LinkedTriggerEnd(CGameObject* pInstigator) override;

	// 현재 Trigger에 들어온 플레이어만 대화를 시작할 수 있다.
	virtual _bool Can_Interact(CGameObject* pPlayer) const override;
	virtual _bool Interact(CGameObject* pPlayer) override;

	NPC_STATE Get_NPCState() const { return m_eState; }
	NPC_TYPE Get_NPCType() const { return m_eNPCType; }

	// Runtime과 Map Editor가 동일한 NPC 리소스 매핑을 사용한다.
	static HRESULT Resolve_ActorType(const _wstring& strActorType, NPC_DESC* pDesc);

protected:
	// 대화 애니메이션 상태와 바라보기 회전을 갱신한다.
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

private:
	void Ready_DefaultDesc(NPC_DESC* pDesc);

	_bool Begin_Dialogue();
	void Show_InteractionPrompt();
	void Hide_InteractionPrompt();
	void Enter_Idle();
	void Enter_TalkStart();
	void Enter_Talking();
	void Enter_TalkEnd();

	void Face_InteractionPlayer();
	void Restore_DefaultFacing();
	void Begin_FacingInterpolation(_fvector vTargetQuaternion);
	void Update_FacingInterpolation(_float fTimeDelta);

	// 추가
	string Resolve_DialogueID() const;				// 스토리 조건을 검사할 진입점
	string Resolve_DefaultDialogueID() const;		// 특별한 조건이 없을 때 NPC별 기본 대사 반환

private:
	static constexpr _float FACING_INTERPOLATION_DURATION = { 1.f };

	_uint m_iSpawnID = { UINT_MAX };
	NPC_TYPE m_eNPCType = { NPC_TYPE::END };
	NPC_STATE m_eState = { NPC_STATE::END };
	_float m_fTalkMaxYawDegree = { 45.f };

	// 거리 대신 외부 Trigger Box 체류 여부만 저장한다.
	_bool m_bPlayerInTrigger = { false };
	_bool m_bInteractionPromptVisible = { false };
	CPlayer* m_pInteractionPlayer = { nullptr };

	// 대화 종료 후 되돌아갈 맵 배치 기준 회전이다.
	_float4 m_vDefaultQuaternion = { 0.f, 0.f, 0.f, 1.f };

	// 현재 회전에서 목표 회전까지 1초 동안 보간할 때 사용한다.
	_float4 m_vFacingStartQuaternion = { 0.f, 0.f, 0.f, 1.f };
	_float4 m_vFacingTargetQuaternion = { 0.f, 0.f, 0.f, 1.f };
	_float m_fFacingInterpolationTime = { 0.f };
	_bool m_bFacingInterpolating = { false };

public:
	static CNPC* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
