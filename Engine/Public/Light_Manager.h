#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CLight_Manager final : public CBase
{
private:
	CLight_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLight_Manager() = default;

public:
	const LIGHT_DESC*           Get_LightDesc(LIGHT eLightType, _uint iIndex);
	_uint						Get_LightCount(LIGHT eLightType) const;
	_bool						Is_LightEnabled(LIGHT eLightType, _uint iIndex) const;

public:
	HRESULT                     Add_Light(LIGHT eLightType,const LIGHT_DESC& LightDesc);
	HRESULT                     Fill_LightData(LIGHT eLightType, _uint iIndex, PerLightData& perLightData);
	HRESULT						Set_LightEnable(LIGHT eLightType, _uint iIndex, _bool bEnable);

	HRESULT						Remove_Light(LIGHT eLightType, _uint iIndex);

	HRESULT						Set_LightDesc(LIGHT eLightType, _uint iIndex, const LIGHT_DESC& LightDesc);

private:
	ID3D11Device*               m_pDevice = { nullptr };
	ID3D11DeviceContext*        m_pContext = { nullptr };

	vector<class CLight*>		m_Lights[ETOUI(LIGHT::END)];
public:
	static CLight_Manager*      Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void                Free() override;
};

NS_END
