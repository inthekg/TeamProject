#pragma once
#include "Base.h"
#include "Effect_Define.h"
#include "Effect_Emitter.h"

NS_BEGIN(Engine)
class CGameInstance;
class CModel;
class CEffect;

class CEffect_Manager final : public CBase
{
private: 
	CEffect_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CEffect_Manager() = default;

public:
	HRESULT		Initialize();
	void		Register_Effect(const string& strName, const EffectDesc& tDesc);
	HRESULT		Load_Effect_File(const string& strName, const string& strPath);
	const EMITTER_SHADER_BUNDLE* Get_ShaderBundle() const { return &m_tEmitterShaders; }

	CEffect*	Play(const string& strName, const _float3& vWorldPos, const _float3& vAngle = {});
	CEffect*	Play(const string& strName, const _float4x4* pFollowBoneMat,
					const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	CEffect*	Play(const string& strName, CModel* pModel, const _char* pBoneName,
					const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	CEffect*	Play(const string& strName, CModel* pModel, const _char* pBoneName, const _char* pBoneName2,
					const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	CEffect*	Play(const string& strName, CModel* pModel,
					const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	CEffect*	Play(const string& strName, const vector<CModel*>& Parts,
					const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});
	CEffect*	Play(const string& strName, const _float4x4* pFollowBoneMat, const _float4x4* pFollowBoneMat2,
					const _float4x4* pOwnerWorld, const _float3& vOffset = {}, const _float3& vAngle = {});

	void		Tick(_float fTimeDelta);
	void		Clear();

private:
	void		Preload_Effect_Textures(const EffectDesc& tDesc);

private:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	CGameInstance*			m_pGameInstance = { nullptr };

	map<string, EffectDesc> m_DescCache;
	list<CEffect*>			m_ActivateEffects;

	EMITTER_SHADER_BUNDLE	m_tEmitterShaders = {};

	static constexpr size_t EFFECT_POOL_SIZE = 128;
	static constexpr size_t EMITTER_POOL_SIZE = 256;
	static constexpr size_t TRAIL_POOL_SIZE = 64;
	static constexpr size_t DECAL_POOL_SIZE = 64;

public:
	static CEffect_Manager* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void			Free() override;
};

NS_END