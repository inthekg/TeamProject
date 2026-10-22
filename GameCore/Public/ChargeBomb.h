#pragma once

#include "Core_Defines.h"
#include "PoolableObject.h"

NS_BEGIN(Engine)
class CEffect;
#ifdef _DEBUG   
class CDebugCapsule;    
#endif // _DEBUG
NS_END


NS_BEGIN(GameCore)
class CAIObject;

class GAMECORE_DLL CChargeBomb final: public CPoolableObject
{
public:
	typedef struct tagChargeBombDesc 
	{
		CGameObject*		pOwner			= { nullptr };
		CAIObject*			pAttachTarget	= { nullptr };
		_float3				vPosition		= {};
		_float3				vNormal			= { 0.f, 1.f, 0.f };
		_float3				vHitDir			= { 0.f, 0.f, 1.f };
		_float				fDelay			= { 2.f };
		_float				fRadius			= { 2.f };
		_float				fDamage			= { 60.f };
		HIT_TYPE			eHitType		= { HIT_TYPE::Heavy };
		_bool				bUpgrade		= { false };
	} CHARGEBOMB_DESC;

private:
	CChargeBomb(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CChargeBomb(const CChargeBomb& Prototype) = delete;
	virtual ~CChargeBomb() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

public:
	virtual void			Activate(void* pArg = nullptr) override;
	virtual void			InActivate() override;

private:
	void					Explode();

	void					Attach_ToTarget(const CHARGEBOMB_DESC* pDesc);
	void					Update_Attach(_float fTimeDelta);
	void					Detach();

private:
	// Tail Effect
	void					Update_TailEffectMatrix(_fvector vPos, _fvector vTailDir);
	void					Stop_TailEffect();

private:
	// Beep Sound
	void					Start_BeepSound();
	void					Update_BeepSoundPos();
	void					Stop_BeepSound();


private:
	static constexpr _float			BOMB_ATTACH_OFFSET	= { 0.15f };
	static constexpr _float			MAX_LIFETIME_OFFSET	= { 1.f };

	static constexpr const char*	CHARGEBOMB_ATTACH_EFFECT = "Player_ChargeGun_BombFlare";
	static constexpr const char*	CHARGEBOMB_EXPLODE_EFFECT = "Test_Bomb_Effect";

	static constexpr const char*	SOUND_EXPLO		= "SE_PC_SK_PJ_SniperCannon_Explo_Cue";
	static constexpr const char*	SOUND_BEEP_LOOP	= "SE_PC_SK_PJ_SniperCannon_Beep";
	static constexpr const char*	SOUND_SPARK_LOOP = "SE_PC_SK_PJ_SniperCannon_Spark_Loop";

	static constexpr _float			SPARK_VOLUME			= { 0.8f };
	static constexpr _float			BEEP_VOLUME				= { 1.2f };

	//강화버전 데이터 
	static constexpr _float			PLAYER_LAUNCH_POWER		= { 18.f };
	static constexpr _float			PLAYER_LAUNCH_UP		= { 15.f };


private:
	CGameObject*			m_pOwner			= { nullptr };
	CAIObject*				m_pAttachTarget		= { nullptr };
	_int					m_iAttachBoneIndex	= { -1 };
	_float3					m_vAttachLocal		= { 0.f, 0.f, 0.f };
	_float3					m_vAttachTailDirLocal = { 0.f, 1.f, 0.f };
	_float3					m_vTailDirWorld		= { 0.f, 1.f, 0.f };

#ifdef  _DEBUG
	CDebugCapsule*			m_pDebugCapsule		= { nullptr };
#endif //  _DEBUG
	_uint					m_iSparkSound = { 0 };
	_uint					m_iBeepSound = { 0 };

	_bool					m_bExploded			= { false };

	_float					m_fDelay			= { 2.f };
	_float					m_fRadius			= { 5.f }; // 2.f
	_float					m_fDamage			= { 20.f };
	_float					m_fUpgradeDamage	= { 40.f };
	HIT_TYPE				m_eHitType			= { HIT_TYPE::Heavy };

	_float					m_fLifeTime			= {}; // Delay + MAX_LIFETIME_OFFSET
	_bool					m_bUpgrade			= { false };
private:
	CEffect*				m_pTailFX = { nullptr };
	_float4x4				m_TailMatrix = {};

public:
	static CChargeBomb*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*	Clone(void* pArg = nullptr) override { return nullptr; }
	virtual void			Free() override;

};

NS_END