#pragma once
#include "Base.h"
#include "RenderStruct.h"
#include "UI_Defines.h"


NS_BEGIN(Engine)

class CGameInstance;

class ENGINE_DLL CUIElement : public CBase
{
protected:
	CUIElement(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CUIElement() = default;

public:
	void						Set_Position(const _float2& vPosition);
	void						Set_Size(const _float2& vSize);
	void						Set_Pivot(const _float2& vPivot);
	void						Set_Anchor(UI_ANCHOR eAnchor);
	void						Set_Rotation(_float fRotation);
	void						Set_Scale(const _float2& vScale);

	void						Set_Color(const _float4& vColor);
	void						Set_UVRect(const _float4& vUVRect);
	void						Set_RevealType(UI_REVEAL_TYPE eRevealType);
	void						Set_RevealProgress(_float fProgress);
	void						Set_RevealSoftness(_float fSoftness);
	void						Set_RevealCenter(const _float2& vCenter);
	void						Set_SortOrder(_uint iSortOrder);
	void						Set_Visible(_bool bVisible);
	void						Set_UseTexture(_bool bUseTexture);

	void						Set_Shader(class CShader* pShader);
	void						Set_VIBuffer(class CVIBuffer* pVIBuffer);
	void						Set_Material(class CMaterial* pMaterial);

	void						Set_ElementID(_uint iElementID);
	void						Set_RuntimeElementTag(const _wstring& strRuntimeElementTag);

public:
	const UITransform&			Get_UITransform() const;
	const UIRenderData&			Get_RenderData() const;
	_bool						Is_Visible() const;

	CUIElement*					Get_Parent() const;
	const vector<CUIElement*>&	Get_Children() const;

	_uint						Get_ElementID() const;
	const _wstring&				Get_RuntimeElementTag() const;
	// 자신과 모든 부모의 Alpha를 곱한 최종 Alpha 반환
	_float						Get_HierarchyAlpha() const;						

	_bool						Is_PointInside(const _float2& vUIPoint) const;

public:
	HRESULT						Add_Child(CUIElement* pChild);
	void						Remove_Child(CUIElement* pChild);

public:
	virtual HRESULT				Initialize();
	virtual void				Priority_Update(_float fTimeDelta);
	virtual void				Update(_float fTimeDelta);
	virtual void				Late_Update(_float fTimeDelta);
	virtual void				Submit_RenderCommand();
	virtual void				Submit_RenderCommand(_uint iObjectSortOrder);


protected:
	_float2						Get_AnchoredPosition() const;

	_matrix						Get_ParentWorldLayoutMatrix() const;					// 부모의 좌표계 가져옴
	_matrix						Get_WorldLayoutMatrix() const;							// 자신의 자식에게 전달할 좌표계를 계산
	_bool						Is_HierarchyVisible() const;							// 자신을 포함한 모든 부모가 표시 상태인지 확인

	void						Make_WorldMatrix(_float4x4* pOutWorldMatrix) const;

protected:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	CGameInstance*			m_pGameInstance = { nullptr };

	UITransform				m_tTransform = {};
	UIRenderData			m_tRenderData = {};

	CUIElement*				m_pParent = { nullptr };
	vector<CUIElement*>		m_Children;

	// 원본 UI Asset 안에서 사용하는 Element ID
	_uint					m_iElementID = { UINT_MAX };

	/*
	 * Component Namespace까지 적용된 런타임 태그
	 * 예: MaterialCategory.Button
	 */
	_wstring				m_strRuntimeElementTag;

public:
	static CUIElement* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END