#pragma once
#include "UIElement.h"

NS_BEGIN(Engine)

class CMaterial;

class ENGINE_DLL CUIProgressBar : public CUIElement
{
public:
    enum class UI_PROGRESS_SLOT_TYPE
    {
        BACKGROUND,
        FILL,
        FX,
    };

    enum class UI_PROGRESS_BLEND_MODE
    {
        ALPHA,
        ADDITIVE,
    };

    typedef struct tagUIProgressSlot
    {
        _int                    iSlotID = -1;
        _int                    iTargetSlotID = -1;

        _wstring                strName = L"";

        UI_PROGRESS_SLOT_TYPE   eType = UI_PROGRESS_SLOT_TYPE::FILL;
        UI_PROGRESS_BLEND_MODE  eBlendMode = UI_PROGRESS_BLEND_MODE::ALPHA;

        CMaterial*              pMaterial = nullptr;

        _float                  fAmount = 1.f;
        _float                  fStartAmount = 0.f;
        _float                  fEndAmount = 1.f;

        _float4                 vUVRect = _float4(0.f, 0.f, 1.f, 1.f);
        _bool                   bStretchUVToRange = false;

        _float4                 vColor = _float4(1.f, 1.f, 1.f, 1.f);
        _float4                 vColorLT = _float4(1.f, 1.f, 1.f, 1.f);
        _float4                 vColorRT = _float4(1.f, 1.f, 1.f, 1.f);
        _float4                 vColorRB = _float4(1.f, 1.f, 1.f, 1.f);
        _float4                 vColorLB = _float4(1.f, 1.f, 1.f, 1.f);

        _int                    iSortOffset = 0;
        _bool                   bVisible = true;
    }UI_PROGRESS_SLOT;

protected:
    CUIProgressBar(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CUIProgressBar() = default;

public:
    void                    Set_FillAmount(_float fFillAmount);
    void                    Set_LengthScale(_float fLengthScale);
    void                    Set_FillColor(const _float4& vFillColor);

    void                    Set_BackgroundMaterial(CMaterial* pMaterial);
    void                    Set_FillMaterial(CMaterial* pMaterial);

    void                    Set_FillType(UI_PROGRESS_FILL_TYPE eType);
    void                    Set_FillDirection(UI_PROGRESS_FILL_DIRECTION eDirection);
    void                    Set_ResizeDirection(UI_PROGRESS_RESIZE_DIRECTION eDirection);

    void                    Set_SlotRange(_int iSlotID, _float fStart, _float fEnd);
    void                    Set_SlotAmount(_int iSlotID, _float fAmount);
    void                    Set_SlotVisible(_int iSlotID, _bool bVisible);

public:
    HRESULT                 Add_Slot(const UI_PROGRESS_SLOT& Slot);
    _int                    Find_SlotID(const _wstring& strSlotName) const;
    UI_PROGRESS_SLOT*       Find_Slot(_int iSlotID);
    UI_PROGRESS_SLOT*       Find_Slot(UI_PROGRESS_SLOT_TYPE eType);
    UI_PROGRESS_SLOT*       Find_Slot(const _wstring& strSlotName);


public:
    virtual HRESULT			Initialize() override;
    virtual void			Priority_Update(_float fTimeDelta) override;
    virtual void			Update(_float fTimeDelta) override;
    virtual void			Late_Update(_float fTimeDelta) override;
    virtual void			Submit_RenderCommand() override;
    virtual void			Submit_RenderCommand(_uint iObjectSortOrder) override;

private:
    void                    Make_ProgressWorldMatrix(_float4x4* pOutWorldMatrix) const;
    void                    Make_ProgressWorldMatrix(_float4x4* pOutWorldMatrix, const UI_PROGRESS_SLOT& Slot, _float fStart, _float fEnd) const;
    _float4                 Make_SlotUVRect(const UI_PROGRESS_SLOT& Slot, _float fStart, _float fEnd) const;
    void                    Submit_ProgressCommand(_uint iSortOrder, CMaterial* pMaterial, const _float4& vColor, _float fFillAmount);
    void                    Submit_ProgressSlotCommand(_uint iSortOrder, const UI_PROGRESS_SLOT& Slot, _float fStart, _float fEnd);

private:
    _float                  Clamp01(_float fValue) const;

    void                    Clear_Slots();

private:
    _float                          m_fLengthScale = { 1.f };

    UI_PROGRESS_FILL_TYPE           m_eFillType = { UI_PROGRESS_FILL_TYPE::HORIZONTAL };
    UI_PROGRESS_FILL_DIRECTION      m_eFillDirection = { UI_PROGRESS_FILL_DIRECTION::LEFT_TO_RIGHT };
    UI_PROGRESS_RESIZE_DIRECTION    m_eResizeDirection = { UI_PROGRESS_RESIZE_DIRECTION::RIGHT };

    vector<UI_PROGRESS_SLOT>        m_Slots;

public:
    static CUIProgressBar* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void Free() override;
};

NS_END