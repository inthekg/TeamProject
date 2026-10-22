#pragma once
#include "Component.h"

NS_BEGIN(Engine)
class ENGINE_DLL CResource : public CComponent
{
protected:
	CResource(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CResource() = default;

	//복사생성 금지. 원본을 공유함
	CResource(const CResource& Prototype);
	CResource& operator = (const CResource& Prototype) = delete;

public:
	_uint						Get_ID() { return m_iID; }
	const _wstring				Get_Name() { return m_strResourceName; }

	void						Set_Name(_wstring strResourceName);
	void						Set_ID(_uint& iID) { m_iID = iID; }

protected:
	_uint						m_iID = {};
	_wstring					m_strResourceName = {};

public:
	virtual CComponent*			Clone(void* pArg) { return nullptr; };
	virtual void				Free() override;

};

NS_END