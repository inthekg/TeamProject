#pragma once
#include "UIElement.h"

NS_BEGIN(Engine)

class ENGINE_DLL CUIContainer final : public CUIElement
{
protected:
	CUIContainer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CUIContainer() = default;

public:
    virtual HRESULT Initialize() override;

    // 컨테이너는 배치 기준만 제공, 렌더링 X
    virtual void Submit_RenderCommand() override;
    virtual void Submit_RenderCommand(_uint iObjectSortOrder) override;

public:
    static CUIContainer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END