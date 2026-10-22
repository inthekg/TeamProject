#pragma once

#include "MapObject.h"

NS_BEGIN(GameCore)

class CCutScene_Actor;

class GAMECORE_DLL CCutScene_Prop final : public CMapObject
{
public:
	struct CUTSCENE_PROP_DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_wstring strMeshName = {};
		_wstring strMaterialName = {};
		_float4x4 WorldMatrix = {};
		_float fEmissiveAmount = {};
	};

private:
	CCutScene_Prop(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CCutScene_Prop() = default;

public:
	virtual HRESULT Initialize(void* pArg) override;
	virtual void Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;
	void Set_CutSceneVisible(_bool bVisible) { m_bCutSceneVisible = bVisible; }
	void Set_CutSceneDissolve(_float fAmount, const _float4& vColor) { m_fDissolveAmount = max(0.f, min(fAmount, 1.f)); m_vDissolveColor = vColor; }
	void Set_CutSceneTransform(_fmatrix Transform);
	void Set_CutSceneAttachment(CCutScene_Actor* pParentActor, const string& strBoneName, _float fBlendWeight = 1.f);
	void Set_CutSceneAttachmentActive(_bool bActive);
	void Clear_CutSceneAttachment();

private:
	_bool m_bCutSceneVisible = { true };
	_float m_fDissolveAmount = {};
	_float4 m_vDissolveColor = { 0.05f, 0.45f, 1.f, 6.f };
	_float4x4 m_LocalWorldMatrix = {};
	CCutScene_Actor* m_pParentActor = { nullptr };
	string m_strAttachBoneName = {};
	_bool m_bCutSceneAttachmentActive = { false };
	_float4x4 m_AttachmentBlendStartWorld = {};
	_float m_fAttachmentBlendWeight = { 1.f };
	_bool m_bAttachmentBlendActive = { false };

public:
	static CCutScene_Prop* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CUTSCENE_PROP_DESC& Desc);
	virtual void Free() override;
};

NS_END
