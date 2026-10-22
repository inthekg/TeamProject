#pragma once
#include "SkeletalWeapon.h"

NS_BEGIN(Engine)
class CEffect;
NS_END

NS_BEGIN(GameCore)
class GAMECORE_DLL CSplitWeapon abstract : public CSkeletalWeapon
{
public:
	typedef struct tagWeaponPiece
	{
		CModel*				pModel	= { nullptr };
		const _float4x4*	pSocketBoneMatrix = { nullptr };
		_float4x4			LocalOffset;
		_float4x4			CombinedWorld;
		
		SWEEP_STATE			Sweep;
		_bool				bActive = { false };
		_bool				bHitWindow = { false };

		_int				iTrailTopIndex = { -1 };
		_int				iTrailBottomIndex = { -1 };
		_int				iEffectIndex = { -1 };

		CEffect*			pTrailEffect = { nullptr };
	} WEAPON_PIECE;
protected:
	CSplitWeapon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CSplitWeapon(const CSplitWeapon& Prototype);
	virtual ~CSplitWeapon() = default;
public:
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;
	virtual HRESULT			Render_Shadow() override;

public:
	virtual void			On_Unequipped() override;	
	virtual void			On_Collision(AttackType eType, _uint iStep, PIECE_TYPE ePiece) override;
	virtual void			Off_Collision(PIECE_TYPE ePiece) override;
	virtual void			Set_Split(_bool bSplit) override;

protected:
	HRESULT					Setup_Piece(PIECE_TYPE ePiece, CModel* pModel, const _float4x4* pSocket, const string& strTop, const string& strBottom, const string& strEffect);
	//Animator Window 반영
	void					Drive_PieceHit(PIECE_TYPE ePiece, _bool bInHitWindow);
	virtual void			Set_PieceTrail(PIECE_TYPE ePiece, _bool bActive);
	void					Deactivate_Piece(PIECE_TYPE ePiece);
	HRESULT					Get_PieceBoneWorldMatrix(const WEAPON_PIECE& Piece, _int iBoneIndex, _matrix& OutMatrix);

protected:
	const _wstring			WIN_NAME_HIT_ON_LEFT = L"Hit_Active_Left";
	const _wstring			WIN_NAME_HIT_ON_RIGHT = L"Hit_Active_Right";
	const _wstring			WIN_NAME_WEAPON_SPLIT = L"Weapon_Split";


protected:
	WEAPON_PIECE			m_Pieces[PIECE_END];
	_bool					m_bSplit = { false };

public:
	virtual void			Free() override;
};

NS_END