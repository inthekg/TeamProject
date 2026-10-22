#pragma once
#include "Component.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CEffect;
NS_END

NS_BEGIN(GameCore)
class CBuffComponent : public CComponent
{
public:
	typedef struct tagBuffComponentDesc
	{
		const _float4x4* pOwnerWorld	= { nullptr };
	} BUFFCOM_DESC;

	typedef struct tagBuffDesc
	{
		const _float4x4*	pFollowBone		= { nullptr };
		_float3				vEffectOffset	= { 0.f, 0.f, 0.f };
		_float				fDuration		= { 0.f };
		_float				fValue			= { 0.f };
		CEffect*			pEffect			= { nullptr };
		const _char*		szEffectTag		= { nullptr };
		_float				fMaxDuration	= { 0.f };
		//시간 누적 
		_bool				bStackTime		= { false };
	} BUFF_DESC;


private:
	CBuffComponent(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CBuffComponent() = default;

public:
	virtual HRESULT			Initialize(void* pArg) override;

	void					Apply(BUFF_TYPE eType, const BUFF_DESC& tDesc);
	void					Remove(BUFF_TYPE eType);
	void					Update(_float fTimeDelta);
	virtual void			Clear() override;

	_float					Get_AttackMul() const { return m_fAttackMul; }
	_float					Get_DamageTakenMul() const { return m_fDamageTakenMul; }

	_bool					Is_Active(BUFF_TYPE eType) const;
	_float					Get_Remain(BUFF_TYPE eType) const;
	_float					Get_Value(BUFF_TYPE eType) const;
	_float					Get_Ratio(BUFF_TYPE eType) const;

private:
	void					Start_Effect(BUFF_TYPE eType, const _char* szTag);
	void					Stop_Effect(BUFF_TYPE eType);
	void					Refresh_Multiplier();


private:
	struct BUFF_SLOT
	{
		_bool				bActive			= { false };
		_float				fRemain			= { 0.f };
		_float				fDuration		= { 0.f };
		_float				fValue			= { 0.f };
		CEffect*			pEffect			= { nullptr };
		const _float4x4*	pFollowBone		= { nullptr };
		_float3				vEffectOffset	= { 0.f, 0.f, 0.f };
	};

	BUFF_SLOT				m_Slots[ETOUI(BUFF_TYPE::End)] = {};
	_float					m_fAttackMul = { 1.f };
	_float					m_fDamageTakenMul = { 1.f };

private:
	const _float4x4*		m_pOwnerWorld = { nullptr };

public:
	static CBuffComponent* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg);
	virtual void Free() override;
};

NS_END