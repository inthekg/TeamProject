#pragma once
#include "PoolableObject.h"
#include "Effect_Define.h"

NS_BEGIN(Engine)

class CEffect_Emitter;
class CEffect_Trail;
class CDecal;
class CModel;
struct EMITTER_SHADER_BUNDLE;

struct EFFECT_PLAY_DESC
{
	const EffectDesc*				pDesc = { nullptr };
	_float3							vWorldPos = { 0.f, 0.f, 0.f };

	const _float4x4*				pFollowBoneMat = { nullptr };
	const _float4x4*				pFollowBoneMat2 = { nullptr };
	const _float4x4*				pOwnerWorld = { nullptr };
	_float3							vFollowOffset = { 0.f, 0.f, 0.f };
	_float3							vFollowAngle = { 0.f, 0.f, 0.f };
	const EMITTER_SHADER_BUNDLE*	pShaderBundle = { nullptr };

	CModel*							pFollowModel = { nullptr };
	const vector<CModel*>*			pFollowParts = { nullptr };
};

class ENGINE_DLL CEffect final : public CPoolableObject
{
private:
	CEffect(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CEffect() = default;

public:
	virtual HRESULT Initialize(void* pArg) override;
	virtual void	Activate(void* pArg = nullptr) override;
	virtual void	InActivate() override;
	virtual void	Update(_float fTimeDelta) override;

	_bool			Is_Finished() const { return m_bFinished; }
	void			Stop_Follow();
	void			Update_Root();

	void			Stop();
	void            Stop_Immediate();

private:
	void			Release_Emitters();

private:
	const EffectDesc*					m_pDesc = { nullptr };
	vector<CEffect_Emitter*>			m_Emitters;
	vector<CEffect_Trail*>				m_Trails;
	vector<CDecal*>						m_Decals;
	vector<_float>						m_PrevLocalTime;
	vector<_bool>                       m_EmitterBurstDone;
	_float3								m_vWorldPos = {};

	_float4x4							m_matRoot = {};
	_float4x4							m_matRoot2 = {};
	_float4x4							m_matFollowRot = {};
	_float4x4							m_matRootSpawn = {};

	_float4x4							m_matRootPrev = {};
	_float4x4							m_matRoot2Prev = {};
	_bool								m_bHasPrevRoot = { false };
	_float4x4                           m_matRootPrev2 = {};
	_float4x4                           m_matRoot2Prev2 = {};
	_bool                               m_bHasPrevRoot2 = { false };
	const _float4x4*					m_pFollowBoneMat = { nullptr };
	const _float4x4*					m_pFollowBoneMat2 = { nullptr };
	const _float4x4*					m_pOwnerWorld = { nullptr };
	_float3								m_vFollowOffset = {};
	_float3								m_vFollowAngle = {};
	_bool								m_bFollow = { false };
	_bool								m_bPositionOnly = { false };

	CModel*								m_pFollowModel = { nullptr };
	vector<vector<const _float4x4*>>	m_EmitterBonePtrs;
	vector<_float4x4>					m_BoneMatScratch;

	string								m_strFollowMeshPath;

	vector<CModel*>						m_FollowParts;
	vector<string>						m_FollowPartMeshPaths;
	vector<_uint>						m_PinGroupBaseScratch;
	vector<string>						m_PinGroupPathScratch;

	_float								m_fPlayTime = { 0.f };
	_float								m_fTotalDuration = { 0.f };
	_float								m_fLoopPeriod = { 0.f };
	_bool								m_bLoop = { false };
	_bool								m_bFinished = { false };

	_bool								m_bStopping = { false };
	_float								m_fMaxLife = { 0.f };
	_float								m_fMaxTrailLife = { 0.f };
	const EMITTER_SHADER_BUNDLE*		m_pShaderBundle = { nullptr };

public:
	static CEffect* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void	Free() override;
};

NS_END