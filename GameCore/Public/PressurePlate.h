#pragma once

#include "MapGameplayObject.h"

NS_BEGIN(GameCore)

class GAMECORE_DLL CPressurePlate final : public CMapGameplayObject
{
public:
	enum class PLATE_STATE
	{
		READY,
		ACTIVE,
		RETURNING,
		END
	};

public:
	// 발판의 게임플레이 설정이다.
	typedef struct tagPressurePlateDesc : public MAP_GAMEPLAY_OBJECT_DESC
	{
		// 눌린 상태로 유지되는 시간이다.
		_float fResetDelay = { 3.0f };
	} PRESSURE_PLATE_DESC;

private:
	CPressurePlate(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CPressurePlate(const CPressurePlate& Prototype);
	virtual ~CPressurePlate() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

	virtual void On_LinkedTriggerBegin(CGameObject* pInstigator) override;

protected:
	virtual void Update_MapGameplayObject(_float fTimeDelta) override;

private:
	void Ready_DefaultDesc(PRESSURE_PLATE_DESC* pDesc);
	void Enter_Active();
	void Begin_Return();
	void Finish_Return();

private:
	PLATE_STATE m_eState = { PLATE_STATE::END };

	_uint m_iTargetSpawnID = { UINT_MAX };

	_float m_fResetDelay = { 3.0f };
	_float m_fResetTimer = { 0.f };

public:
	static CPressurePlate* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END
