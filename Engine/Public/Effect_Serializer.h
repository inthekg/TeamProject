#pragma once
#include "Effect_Define.h"

NS_BEGIN(Engine)

ENGINE_DLL HRESULT Save_Effect(const EffectDesc& tEffect, const string& strPath);
ENGINE_DLL HRESULT Load_Effect(EffectDesc& tOut, const string& strPath);

NS_END