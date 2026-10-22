#include "CrosshairHUDViewModel.h"

CCrosshairHUDViewModel* CCrosshairHUDViewModel::Create()
{
    return new CCrosshairHUDViewModel;
}

void CCrosshairHUDViewModel::Free()
{
    __super::Free();
}