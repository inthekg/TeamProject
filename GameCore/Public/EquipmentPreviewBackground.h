#pragma once
#include "Core_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)

class CShader;
class CMaterial;
class CVIBuffer_Rect;

NS_END

NS_BEGIN(GameCore)

class GAMECORE_DLL  CEquipmentPreviewBackground final : public CGameObject
{
private:
	CEquipmentPreviewBackground(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CEquipmentPreviewBackground(const CEquipmentPreviewBackground& Prototype);
	virtual ~CEquipmentPreviewBackground() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

	virtual void Priority_Update(_float fTimeDelta) override;
	virtual void Update(_float fTimeDelta) override;
	virtual void Late_Update(_float fTimeDelta) override;

	virtual HRESULT Render() override;

public:
	// 장비 UI와 함께 배경의 갱신 및 렌더링을 활성화한다.
	void Set_PreviewActive(_bool bActive) { m_bPreviewActive = bActive; }

	// 현재 프리뷰 배경이 활성화되어 있는지 반환한다.
	_bool Is_PreviewActive() const { return m_bPreviewActive; }

private:
	HRESULT Ready_Components();
	HRESULT Ready_Material(const _wstring& strRelativeTexturePath, CMaterial** ppOutMaterial);
	void Submit_BackgroundCommand(CMaterial* pMaterial, const _float4& vColor, _uint iSortOrder);

private:
	CShader* m_pShaderCom = { nullptr };
	CVIBuffer_Rect* m_pVIBufferCom = { nullptr };

	CMaterial* m_pSolidMaterial = { nullptr };
	CMaterial* m_pPatternMaterial = { nullptr };

	_bool m_bPreviewActive = { false };

public:
	static CEquipmentPreviewBackground* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg) override;
	virtual void Free() override;
};

NS_END

