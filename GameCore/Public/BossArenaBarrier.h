#pragma once

#include "MapGameplayObject.h"
#include "IListener.h"

NS_BEGIN(GameCore)

class CHotelDoor;

class GAMECORE_DLL CBossArenaBarrier final : public CMapGameplayObject, public Engine::IListener
{
public:
	enum class BARRIER_ROLE
	{
		ENTRANCE,
		EXIT,
		END
	};

private:
	enum class BARRIER_STATE
	{
		OPEN,
		OPENING,
		CLOSED,
		CLOSING,
		END
	};

public:
	typedef struct tagBossArenaBarrierDesc final : public CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC
	{
		BARRIER_ROLE eRole = { BARRIER_ROLE::END };
		SPAWN_MONSTER_TYPE eClearBossType = { SPAWN_MONSTER_TYPE::END };
		SPAWN_MONSTER_TYPE eStartBossType = { SPAWN_MONSTER_TYPE::END };
		_float fDissolveDuration = { 0.8f };
		_float4 vDissolveColor = { 0.f, 0.f, 0.f, 0.f };
		_bool bEditorPreview = { false };
	} BOSS_ARENA_BARRIER_DESC;

private:
	CBossArenaBarrier(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBossArenaBarrier(const CBossArenaBarrier& Prototype);
	virtual ~CBossArenaBarrier() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

	virtual _bool Can_Interact(CGameObject* pPlayer) const override;
	virtual _bool Interact(CGameObject* pPlayer) override;

	virtual void On_LinkedTriggerBegin(CGameObject* pInstigator) override;
	virtual void On_LinkedTriggerEnd(CGameObject* pInstigator) override;

	virtual void OnEvent(EVENT_TYPE eType, Event_Desc* pData) override;
	virtual void OnEvent(EVENT_TYPE_MAP_OBJECT eType, Event_Desc* pData) override;

	// 재도전 시 제거할 기존 보스 문을 같은 섹터의 SpawnID 연결로 전달받는다.
	void Attach_LegacyDoor(CHotelDoor* pLegacyDoor);

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;
	virtual void Configure_RenderCommand(Engine::RenderCommand* pCommand) const override;

private:
	void Ready_DefaultDesc(BOSS_ARENA_BARRIER_DESC* pDesc);
	void Apply_PendingStateRequests();
	void Begin_Open();
	void Begin_Close();
	void Show_InteractionPrompt();
	void Hide_InteractionPrompt();
	void Update_Dissolve(_float fTimeDelta);
	_float Get_SignedPassageSide(CGameObject* pInstigator) const;

private:
	BARRIER_ROLE m_eRole = { BARRIER_ROLE::END };
	BARRIER_STATE m_eState = { BARRIER_STATE::END };
	SPAWN_MONSTER_TYPE m_eClearBossType = { SPAWN_MONSTER_TYPE::END };
	SPAWN_MONSTER_TYPE m_eStartBossType = { SPAWN_MONSTER_TYPE::END };

	_float m_fDissolveAmount = { 1.f };
	_float m_fDissolveSpeed = { 1.f };
	_float4 m_vDissolveColor = { 0.f, 0.f, 0.f, 0.f };

	_float m_fPassageEntrySide = { 0.f };

	_bool m_bEncounterStarted = { false };
	_bool m_bPassageEnabled = { false };
	_bool m_bPlayerInPassageTrigger = { false };
	_bool m_bInteractionPromptVisible = { false };
	_bool m_bPermanentlyOpen = { false };

	// PhysX 콜백에서 Scene Actor를 직접 변경하지 않고 Update에서 처리한다.
	_bool m_bOpenRequested = { false };
	_bool m_bCloseRequested = { false };
	_bool m_bBossStartRequested = { false };
	_bool m_bLegacyDoorDeactivateRequested = { false };

	// MapGameplayObjectManager가 수명을 보장하는 같은 Chapter의 비소유 참조다.
	CHotelDoor* m_pLegacyDoor = { nullptr };

public:
	static CBossArenaBarrier* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
