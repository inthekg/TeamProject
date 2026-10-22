#pragma once

#include "MapGameplayObject.h"
#include "StargazerTravelData.h"

NS_BEGIN(Engine)
class CEffect;
NS_END

NS_BEGIN(GameCore)

class CStargazerManager;

class GAMECORE_DLL CStargazer final : public CMapGameplayObject
{


public:
	enum class STARGAZER_STATE
	{
		BROKEN_IDLE,
		RESTORING,
		OPENING,
		OPEN_IDLE,
		END
	};

public:
	typedef struct tagStargazerDesc : public CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC
	{
		CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC NormalDesc = {};
		_bool bActivated = { false };
		_float fInteractionRange = { 3.f };
		_bool bUseHotelModel = { false };

		// ChapterMapManager가 소유하는 Runtime Manager와 안정적인 식별자다.
		CStargazerManager* pStargazerManager = { nullptr };
		_uint64 iStargazerID = { INVALID_STARGAZER_ID };
	} STARGAZER_DESC;

private:
	CStargazer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CStargazer(const CStargazer& Prototype);
	virtual ~CStargazer() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual void Update(_float fTimeDelta) override;

public:
	HRESULT Begin_Interaction(CGameObject* pPlayer);
	
	virtual _bool Can_Interact(CGameObject* pPlayer) const override;
	virtual _bool Interact(CGameObject* pPlayer) override;
	virtual _float4x4 Get_InteractTransform(CGameObject* pPlayer) const override;

	_bool Is_Activated() const { return m_bActivated; }
	STARGAZER_STATE Get_StargazerState() const { return m_eState; }
	void Set_PlayerInTrigger(_bool bPlayerInTrigger) { m_bPlayerInTrigger = bPlayerInTrigger; }

	virtual void				On_LinkedTriggerBegin(CGameObject* pInstigator)override;
	virtual void				On_LinkedTriggerEnd(CGameObject* pInstigator)override;

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;
	virtual void On_AnimNotify(_uint64 iNotifyHash) override;

private:
	void Ready_DefaultDesc(const STARGAZER_DESC* pDesc);
	void Update_StargazerState(_float fTimeDelta);
	HRESULT Change_ModelAndAnimator(const CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC& Desc);
	HRESULT Enter_Restoring();
	void	Start_OpenLoopSound();
	void	Stop_OpenLoopSound();
	HRESULT Enter_Opening();
	HRESULT Enter_OpenIdle();
	HRESULT Begin_ActivatedInteraction();
	HRESULT Build_RespawnPoint(CGameObject* pPlayer, Respawn_Point* pOutPoint) const;

	// 현재 상호작용 가능한 상태에 맞는 UI 타입을 관리한다.
	OBJECT_TYPE Resolve_InteractionObjectType() const;
	void Show_InteractionPrompt();
	void Hide_InteractionPrompt();

private:
	 CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC m_BrokenDesc = {};
	 CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC m_NormalDesc = {};
	STARGAZER_STATE m_eState = { STARGAZER_STATE::END };
	_bool m_bActivated = { false };
	_float m_fInteractionRange = { 3.f };
	_bool m_bPlayerInTrigger = { false };
	OBJECT_TYPE m_eVisiblePromptType = { OBJECT_TYPE::END };

	// Runtime 진행 상태 Manager는 Stargazer 인스턴스가 보유 참조한다.
	CStargazerManager* m_pStargazerManager = { nullptr };
	_uint64 m_iStargazerID = { INVALID_STARGAZER_ID };
	Respawn_Point m_PendingRepairRespawnPoint = {};
	_bool m_bHasPendingRepairRespawnPoint = { false };

	CEffect* m_pEffect_Wait = { nullptr };
	_uint	 m_hOpenLoopSound = { 0 };
	const string	CUE_RESTORE = { "AMB_OJ_PR_Stargazer_Restore_Cue" };
	const string	CUE_RESTORE_BASE = { "AMB_OJ_PR_Stargazer_Restore_02_Cue" };
	const string	CUE_RESTORE_WHISPER = { "AMB_OJ_PR_Stargazer_Restore_02_Whisper_Cue" };
	const string	CUE_OPEN = { "AMB_OJ_PR_Stargazer_Open_02_Cue" };
	const string	CUE_OPEN_LOOP = { "AMB_OJ_PR_Stargazer_Open_Loop_Cue" };

public:
	static CStargazer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
