#pragma once
#include "UIViewModelBase.h"
#include "Core_Enums.h"

NS_BEGIN(GameCore)

class CInteractionPromptViewModel final : public CUIViewModelBase
{
private:
    CInteractionPromptViewModel() = default;
    virtual ~CInteractionPromptViewModel() = default;

public:
    void                Set_Visible(_bool bVisible);
    void                Set_Prompt(OBJECT_TYPE eObjectType, const _wstring& strPromptText);

public:
    _bool               Is_Visible() const { return m_bVisible; }

    OBJECT_TYPE         Get_ObjectType() const { return m_eObjectType; }
    const _wstring&     Get_PromptText() const { return m_strPromptText; }

private:
    _bool               m_bVisible = { false };
    OBJECT_TYPE         m_eObjectType = { OBJECT_TYPE::END };
    _wstring            m_strPromptText;

public:
    static CInteractionPromptViewModel* Create();
    virtual void Free() override;
};

NS_END