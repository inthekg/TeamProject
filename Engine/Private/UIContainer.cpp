#include "UIContainer.h"

CUIContainer::CUIContainer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUIElement{ pDevice, pContext }
{
}

HRESULT CUIContainer::Initialize()
{
	if (FAILED(__super::Initialize()))
		return E_FAIL;

	return S_OK;
}

void CUIContainer::Submit_RenderCommand()
{
}

void CUIContainer::Submit_RenderCommand(_uint iObjectSortOrder)
{
}

CUIContainer* CUIContainer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUIContainer* pInstance = new CUIContainer(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CUIContainer");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CUIContainer::Free()
{
    __super::Free();
}
