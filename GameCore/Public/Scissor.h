#pragma once
#include "SplitWeapon.h"

NS_BEGIN(Engine)
class CEffect;
class CDebugCapsule;
NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL CScissor : public CSplitWeapon
{
private:
	CScissor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CScissor(const CScissor& Prototype);
	virtual ~CScissor() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;

public:
	virtual void			Execute_FableArts(_uint iStep) override;
private:
	virtual HRESULT			Ready_Components() override;
	virtual HRESULT			Ready_WeaponData() override;

private:
	virtual void			Drive_ActionWindows(CAnimator* pAnimator) override;
	virtual void			Set_PieceTrail(PIECE_TYPE ePiece, _bool bActive) override;
private:
	_bool					m_bFXEffect_Active = {};
	CEffect*				m_pChannel_GrinderLeftFx = {};
	CEffect*				m_pChannel_GrinderRightFx = {};

public:	
	static CScissor*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END