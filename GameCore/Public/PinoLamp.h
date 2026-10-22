#pragma once
#include "PartObject.h"
#include "Core_Defines.h"

NS_BEGIN(Engine)
class CModel;
class CShader;
class CDynamicRigidBody;
NS_END

NS_BEGIN(GameCore)

class CPlayer;

class GAMECORE_DLL CPinoLamp final : public CPartObject
{
public:
	typedef struct tagPinoLampDesc : public CPartObject::PARTOBJECT_DESC
	{
		CPlayer* pOwner;
		const _float4x4* pSocketBoneMatrix = { nullptr };
	} PINOLAMP_DESC;

private:
	CPinoLamp(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CPinoLamp(const CPinoLamp& Prototype) = delete;
	virtual ~CPinoLamp() = default;

public:
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Physical_Update(_float fFixedTime) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;
	void						Set_Dissolve(_float fAmount, const _float4& vColor);

public:
	void						Set_Visibility(_bool bVisible) { m_bVisibility = bVisible; }
	//void						Toggle_PlayerLight();
	void						Set_LightEnable(_bool bEnable);
	_bool						Is_LightOn() const { return m_bPlayerLightEnable; }
private:
	HRESULT						Ready_Components();
	HRESULT						Ready_Physics();
	HRESULT						Ready_PlayerLight();
	void						Update_PlayerLight(_fvector vLightPosition);

private:
	static constexpr _float		PIVOT_LOCAL_Y				= { 0.18f };
	static constexpr _float		CONE_LIMIT_RAD				= { XM_PIDIV4 * 0.5f };
	static constexpr _float		LAMP_MASS					= { 5.f };
	static constexpr _float		LAMP_RADIUS					= { 0.08f };

	static constexpr _float		DIST_DIFF_SQRT				= { 1.5f };
	static constexpr _float		PLAYER_LIGHT_UP_OFFSET		= { 0.6f };
	static constexpr _float		PLAYER_LIGHT_BACK_OFFSET	= { 0.35f };


private:
	static constexpr const char* EFFECT_LAMP				= { "Effect_Turn_On_Lamp" };
	static constexpr const char* SOUND_LAMP					= { "SE_PC_MT_Item_Monard_Lamp_Cue" };
private:
	CPlayer*					m_pOwner					= { nullptr };
	CModel*						m_pModelCom					= { nullptr };
	const _float4x4*			m_pSocketBoneMatrix			= { nullptr };


	_bool						m_bVisibility				= { true };
	_bool						m_bPlayerLightEnable		= { true };
	_uint						m_iPlayerLightIndex			= { UINT_MAX };
	LIGHT_DESC					m_PlayerLightDesc			= {};

	_float						m_fDissolveAmount			= { 0.f };
	_float4						m_vDissolveColor			= { 0.f, 0.f, 0.f, 0.f };

	_float						fLocalX, fLocalY, fLocalZ = { 0.f };
	_int						m_iSoundHandle = { -1 };
private:
	//조인트 Data
	CDynamicRigidBody*			m_pAnchorBody				= { nullptr };
	CDynamicRigidBody*			m_pLampBody					= { nullptr };
	PxSphericalJoint*			m_pJoint					= { nullptr };

public:
	static CPinoLamp*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, void* pArg);
	virtual CGameObject*		Clone(void* pArg) override { return nullptr; }
	virtual void				Free() override;
};

NS_END
