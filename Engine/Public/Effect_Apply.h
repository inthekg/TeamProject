#pragma once
#include "Effect_Define.h"

NS_BEGIN(Engine)

class CEffect_Emitter;

ENGINE_DLL void  Apply_Desc(Engine::CEffect_Emitter* pEmitter, const EmitterDesc& e, _bool bEmitting, float fPlayTime);
ENGINE_DLL _bool Is_Emitting(const MainModuleDesc& tMain, float fEffectTime);

NS_END