#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CLadder final : public CMapGameplayObject
{
public:
	enum class LADDER_TYPE
	{
		STATIC,
		METALWOOD,
		METALWOOD_6M,
		END
	};

	enum class LADDER_STATE
	{
		CLOSE_IDLE,
		OPEN,
		OPEN_IDLE,
		END
	};

public:
	typedef struct tagLadderDesc : public CMapGameplayObject::MAP_GAMEPLAY_OBJECT_DESC
	{
		// 하나의 Ladder Prototype에서 실제 표현 종류를 선택한다.
		LADDER_TYPE eLadderType = { LADDER_TYPE::END };
		_bool bStartOpen = { false };
	} LADDER_DESC;

	typedef struct tagLadderEventDesc : public Engine::Event_Desc
	{
		// 모델에 배치된 마커들의 월드 위치다.
		_float3 vBottomLandingPosition = {};
		_float3 vTopLandingPosition = {};
		_float3 vTopPosition = {};

		// 플레이어 정렬에 사용할 Ladder_Top 마커의 Scale 제거 월드 행렬이다.
		_float4x4 TopWorldMatrix = {};
	} LADDER_EVENT_DESC;

private:
	CLadder(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CLadder(const CLadder& Prototype);
	virtual ~CLadder() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

	virtual void Late_Update(_float fTimeDelta) override;

public:
	HRESULT Begin_Open();

	// 실제 등반 이벤트는 최신 Bone 반영 후 Late_Update에서 처리한다.
	virtual _bool Can_Interact(CGameObject* pPlayer) const override;
	virtual _bool Interact(CGameObject* pPlayer) override;

	LADDER_TYPE Get_LadderType() const { return m_eLadderType; }
	LADDER_STATE Get_LadderState() const { return m_eState; }

	virtual void On_LinkedTriggerBegin(CGameObject* pInstigator) override;
	virtual void On_LinkedTriggerEnd(CGameObject* pInstigator) override;
	virtual void On_LinkedActivate() override;

	static HRESULT Resolve_ActorType(const _wstring& strActorType, LADDER_DESC* pDesc);

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

private:
	HRESULT Ready_DefaultDesc(LADDER_DESC* pDesc);
	HRESULT Ready_ClimbAnchors();
	OBJECT_TYPE Resolve_InteractionPromptType(CGameObject* pInstigator) const;

	HRESULT Build_AnchorWorldMatrix(_int iBoneIndex, _float4x4& OutWorldMatrix) const;
	HRESULT Build_ClimbEventDesc(LADDER_EVENT_DESC& OutEventDesc) const;

	HRESULT Enter_CloseIdle();
	HRESULT Enter_Opening();
	HRESULT Enter_OpenIdle();

private:
	LADDER_TYPE m_eLadderType = { LADDER_TYPE::END };
	LADDER_STATE m_eState = { LADDER_STATE::END };


	_int m_iBottomLandingBoneIndex = { -1 };
	_int m_iTopLandingBoneIndex = { -1 };
	_int m_iTopBoneIndex = { -1 };

	_bool m_bPlayerInTrigger = { false };
	OBJECT_TYPE m_eVisiblePromptType = { OBJECT_TYPE::END };

	// Interact에서 요청을 받고, 최신 Bone 반영 후 Late_Update에서 처리한다.
	_bool m_bClimbRequested = { false };

public:
	static CLadder* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
