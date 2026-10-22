#pragma once
#include "Base.h"
#include "ModelDataStruct.h"
#include "AnimationData.h"

NS_BEGIN(Engine)

class CGameInstance;
class CAnimation;

class ENGINE_DLL CAnimationLoader final
{
public:
	static HRESULT	Ready_RawAnimations(CGameInstance* pGameInstance, const _wstring& strFolderName, const _wstring& strAnimationBaseName, ANIMATION_TIME_UNIT eTimeUint, _bool bStaticAnimation);

	static HRESULT	Parse_RawAnimationFile(const _wstring& strAnimationFilePath, const _wstring& strAnimationCSVFilePath, ANIMATION_TIME_UNIT eTimeUnit, vector<CAnimation*>& OutAnimations);

private:
	static HRESULT	Register_LoadedAnimations(CGameInstance* pGameInstance, const _wstring& strKeyFolderName, vector<CAnimation*>& LoadedAnimations, _bool bStaticAnimation);
	static _float	Calc_TimeScaleMillsec(_float fTickPerSecond, ANIMATION_TIME_UNIT eTimeUnit);
	static void		Normalize_ChannelTime(ChannelPassData& ChannelData, _float fTimeScale);
};

NS_END