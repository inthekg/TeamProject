#pragma once
#include "UIElement.h"

NS_BEGIN(Engine)

class ENGINE_DLL CUIImage : public CUIElement
{
public:
    enum class UI_IMAGE_DRAW_TYPE
    {
        NORMAL,
        NINE_SLICE,
    };

protected:
    CUIImage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CUIImage() = default;

public:
    void                    Set_DrawType(UI_IMAGE_DRAW_TYPE eDrawType);
    void                    Set_SliceMargin(const _float4& vSliceMargin);
    void                    Set_TextureSize(const _float2& vTextureSize);
    void                    Set_SliceBaseSize(const _float2& vSliceBaseSize);

public:
    virtual HRESULT			Initialize() override;
    virtual void			Priority_Update(_float fTimeDelta) override;
    virtual void			Update(_float fTimeDelta) override;
    virtual void			Late_Update(_float fTimeDelta) override;
    virtual void			Submit_RenderCommand() override;
    virtual void			Submit_RenderCommand(_uint iObjectSortOrder) override;

    void                    Submit_NineSlicePiece(_uint iFinalSortOrder, const _float2& vCenter, const _float2& vSize, const _float4& vUVRect);
    void                    Submit_NineSliceRenderCommand(_uint iObjectSortOrder);


private:
    UI_IMAGE_DRAW_TYPE      m_eDrawType = UI_IMAGE_DRAW_TYPE::NORMAL;

    // x = Left, y = Top, z = Right, w = Bottom
    // 원본 텍스쳐 픽셀 기준 마진.
    _float4                 m_vSliceMargin = { 0.f, 0.f, 0.f, 0.f };

    _float2                 m_vTextureSize = { 1.f, 1.f };
    _float2                 m_vSliceBaseSize = { 1.f, 1.f };

public:
    static CUIImage* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END