#pragma once
#include "PartObject.h"
#include "Core_Defines.h"
#include "IItemable.h"

/*
	하위 무기 클래스에서
	Initalize_prototype에서 트레일 본 이름 입력
	Ready_Components에서 본 인덱스 얻기
*/
NS_BEGIN(Engine)
class CDynamicRigidBody;
class CShader;
class CModel;
class CDebugCapsule;
class CEffect;
class CAnimator;
NS_END

NS_BEGIN(GameCore)
class CPlayer;
class GAMECORE_DLL CWeapon abstract : public CPartObject, public IItemable
{
public:
	typedef struct tagWeaponDesc : public CPartObject::PARTOBJECT_DESC
	{
		CPlayer* pOwner;
		const _float4x4* pSocketBoneMatrix = { nullptr };
		_uint				iLevelIndex = {};
	} WEAPON_DESC;

	typedef struct tagSweepState
	{
		_bool				bCollisionActive = { false };
		_bool				bPrevPosValid = { false };
		_float3				vPrevCenter = {};
		const ActionData* pAction = { nullptr };
		CDebugCapsule* pDebugCapsule = { nullptr };
		unordered_set<CGameObject*>	Targets;
	} SWEEP_STATE;

protected:
	CWeapon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CWeapon(const CWeapon& Prototype);
	virtual ~CWeapon() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	_bool					Is_Durablity_Zero() { return m_fCurDurability <= 0.f; }
	_float					Get_Durability() const { return m_fCurDurability; }

	_uint					Get_WeaponTypeID() const { return m_iWeaponTypeID; }
	const WeaponData& Get_WeaponData() const { return m_tWeaponData; }
	WeaponType				Get_WeaponType() const { return m_tWeaponData.eType; }
	const ActionData* Get_ActionData(AttackType eAttackType, _uint iStep);

	virtual HRESULT			Get_TrailBoneMatPtr(const _float4x4*& pOutTopBoneMat, const _float4x4*& pOutBottomBoneMat);
	virtual HRESULT			Get_TrailMatrix(_matrix& OutTopMatrix, _matrix& OutBottomMatrix);
	virtual HRESULT			Get_EffectBoneMatrix(_matrix& OutEffectMatrix);
	virtual HRESULT			Get_EffectBoneMatPtr(const _float4x4*& pOutBoneMat);

	const _float4x4* Get_Bone_CombinedMatPtr(_uint iBoneIndex);
	const _float4x4* Get_Bone_CombinedMatPtr(const _char* pBoneName);
	HRESULT					Get_BoneWorldMatrix(_uint iBoneIndex, _matrix& OutMatrix);

public:
	virtual void			Execute_FableArts(_uint iStep) PURE;
	void					Begin_Sweep(SWEEP_STATE& State, AttackType eType, _uint iStep);
	void					End_Sweep(SWEEP_STATE& State);
	void					Sweeping(SWEEP_STATE& State, _fmatrix BladeTop, _fmatrix BladeBot);

	virtual void			Drive_ActionWindows(class CAnimator* pAnimator);
	void					Set_AttackContext(AttackType eType, _uint iStep)
	{
		m_eCurAttackType = eType; m_iCurComboStep = iStep;
	}
	virtual void			Set_Split(_bool bSplit) {};

public:
	_bool					Consume_Durability();
	void					Set_FullDurability();
	void					Regen_Durability(_float fAmountPerSec, _float fDeltaTime);


	void					On_Swap_Begin() { m_bSwapping = true; }
	void					On_Swap_End() { m_bSwapping = false; }

	virtual void			On_Unequipped();

	virtual void			On_Collision(AttackType eType, _uint iStep, PIECE_TYPE ePieceType = PIECE_TYPE::PIECE_MERGE);
	virtual void			Off_Collision(PIECE_TYPE ePieceType = PIECE_TYPE::PIECE_MERGE);
	virtual void			On_AnimNotify(const _wstring& strName) {}
	virtual void			Update_Collision(_float fTimeDelta);


	virtual void			Set_TrailActive(_bool bActive, _uint eType = ETOUI(AttackType::End), _uint iComboStep = 0);
	virtual void			Spawn_SwingFX(_fvector vFXPos, _fvector vSwingDir, ATTACK_STYLE eStyle);


	void					Set_Visibility(_bool bVisible)
	{
		m_bVisibility = bVisible;
	}

	void					Set_Dissolve(_float fAmount, const _float4& vColor);
protected:
	//static _float			Get_HitTypeDamageAlpha(HIT_TYPE eHitType);

protected:
	virtual HRESULT			Ready_Components() PURE;
	virtual HRESULT 		Ready_WeaponData() PURE;

	_fmatrix				Make_EffectStampMatrix(_fvector vPos, _fvector vSlashDir);
	_fmatrix				Make_ScreenAlignedMatrix(_fvector vPos, _fvector vThrustDir);

protected:
	const _wstring			WIN_NAME_WEAPON_EFFECT = L"Weapon_Effect";

	static constexpr _uint	MAX_COMBO = { 3 };
	static constexpr _uint	EFFECT_MATRIX_POOL_SIZE = { 5 };
	static constexpr _float	BASE_DAMAGE = { 100.f };

	static constexpr _float	MAX_DURABILITY = { 100.f };
	static constexpr _float	DURABILITY_PER_HIT = { 5.f };
	static constexpr _float BASE_GROGGY = { 10.f };

protected:
	const _float4x4* m_pSocketBoneMatrix = { nullptr };
	CModel* m_pModelCom = { nullptr };
	CPlayer* m_pOwner = { nullptr };

	//Clone Data
	_uint					m_iLevelIndex = {};
	WeaponData				m_tWeaponData = {};
	_float					m_fDamageVariance = { 0.f };

	string					m_strTrailTopBoneName = {};
	string					m_strTrailBottomBoneName = {};
	string					m_strEffectBoneName = {};

	_int					m_iTrailTopBoneIndex = { -1 };
	_int					m_iTrailBottomBoneIndex = { -1 };
	_int					m_iEffectBoneIndex = { -1 };

	//무기 종류별 ID(애니메이션 식별용). 최하위에서 설정하기 -> 현재 사용X
	_uint					m_iWeaponTypeID = {};
protected:
	_bool					m_bSwapping = { false };

	SWEEP_STATE				m_SweepState;
	_float					m_fSweepRadius = { 0.1f };
	_float					m_fSweepHalfHeight = { 0.5f };

	AttackType				m_eCurAttackType = { AttackType::End };
	_int					m_iCurComboStep = { };
	_bool					m_bHitWindow = { false };

	string					m_FXNames[ETOUI(AttackType::End)][MAX_COMBO];

	FILTER_GROUP			m_eTargetGroup = { static_cast<FILTER_GROUP>(ETOUI(FILTER_GROUP::MONSTER) | ETOUI(FILTER_GROUP::MONSTEROBJECT)) };

	_float3					m_vAttackStartCenter = { 0.f, 0.f, 0.f };

	_float4x4				m_EffectMatrixPool[EFFECT_MATRIX_POOL_SIZE];
	_uint					m_iEffectMatrixPoolIndex = { 0 };

	CEffect* m_pChannel_TrailFX = { nullptr };

	_bool					m_bVisibility = { true };
protected:
	_float					m_fCurDurability = {};
	_float					m_fRepairSpeed = { 10.f };
	_bool					m_bRepairing = { false };

	_float					m_fDissolveAmount = { 0.f };
	_float4					m_vDissolveColor = { 0.f, 0.f, 0.f, 0.f };

public:
	virtual CGameObject* Clone(void* pArg) PURE;
	virtual void			Free() override;
};

NS_END