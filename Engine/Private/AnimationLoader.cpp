#include "AnimationLoader.h"
#include "GameInstance.h"
#include "Animation.h"
#include "Channel.h"
#include "Parser.h"

namespace 
{
	constexpr const _char* g_SkipChannelBoneNames[] =
	{
		"SK_CH_PC_Pino_FullBody",
	};

	_bool Is_SkippedChannelBone(const _char* pBoneName)
	{
		for (const _char* pSkipName : g_SkipChannelBoneNames)
		{
			if (0 == strcmp(pBoneName, pSkipName))
				return true;
		}
		return false;
	}
}

HRESULT CAnimationLoader::Ready_RawAnimations(CGameInstance* pGameInstance, const _wstring& strFolderName, const _wstring& strAnimationBaseName, ANIMATION_TIME_UNIT eTimeUint, _bool bStaticAnimation)
{
	const _wstring strResourceDir = pGameInstance->Get_ResourcePath() + L"/Models/" + strFolderName + L"/";
	const _wstring strAnimPath = strResourceDir + strAnimationBaseName + GlobalFileRoute::g_strAnimationFileTag;
	const _wstring strCSVPath = strResourceDir + strAnimationBaseName + GlobalFileRoute::g_strAnimationCSVFileTag;

	vector<CAnimation*> LoadedAnimations;

	if (FAILED(Parse_RawAnimationFile(strAnimPath, strCSVPath, eTimeUint, LoadedAnimations)))
		return E_FAIL;

	return Register_LoadedAnimations(pGameInstance, strFolderName, LoadedAnimations, bStaticAnimation);
}

HRESULT CAnimationLoader::Parse_RawAnimationFile(const _wstring& strAnimationFilePath, const _wstring& strAnimationCSVFilePath, ANIMATION_TIME_UNIT eTimeUnit, vector<CAnimation*>& OutAnimations)
{
	ifstream fin_Binary(strAnimationFilePath, ios::binary);
	ifstream fin_CSV(strAnimationCSVFilePath);

	if (!fin_CSV.is_open())
	{
		MSG_BOX("Animation CSV File Read Fail");
		return E_FAIL;
	}

	//애니메이션 CSV 메타데이터 읽기
	vector<AnimationReadData> AnimDatas;
	bool bResult = CSVParser<AnimationReadData>::CSV_Parse(AnimDatas, strAnimationCSVFilePath, [](const vector<_wstring>& tokens)
		{
			return AnimationReadData
			{
				//tokens[0]은 순번 인덱스라 패스함 
				tokens[1],                          // animName
				tokens[2],                          // FileName
				stof(tokens[3]),                    // duration
				stof(tokens[4]),                    // TickPerSecond
				(tokens[5] == L"0" ? false : true), // Loop
				stof(tokens[6]),                    // BlendTime
				(tokens[7] == L"0" ? false : true)  // IsRootMotion
			};
		}
	);

	fin_CSV.close();

	if (!bResult)
	{
		MSG_BOX("CSV File Parse Failed");
		return E_FAIL;
	}

	//Read Animation Channel 
	if (!fin_Binary.is_open())
	{
		MSG_BOX("Animation Read Fail");
		return E_FAIL;
	}

	_uint iNumAnimations = 0;
	fin_Binary.read(reinterpret_cast<char*>(&iNumAnimations), sizeof(uint32_t));
	for (uint32_t i = 0; i < iNumAnimations; ++i)
	{
		AnimationReadData AnimData = AnimDatas[i];

		const _float fTimeScale = Calc_TimeScaleMillsec(AnimData.fTickPerSecond, eTimeUnit);

		if (eTimeUnit == ANIMATION_TIME_UNIT::TICKS)
		{
			AnimData.fDuration *= fTimeScale;
			AnimData.fTickPerSecond = 1000.f;
		}

		_uint iChannelCount = 0;

		// 채널 기본 정보 읽기
		fin_Binary.read(reinterpret_cast<char*>(&iChannelCount), sizeof(_uint));

		vector<CChannel*> Channels;
		//Channels.resize(iChannelCount);
		Channels.reserve(iChannelCount);

		// 채널 키프레임 읽기
		for (uint32_t j = 0; j < iChannelCount; ++j)
		{
			ChannelData chanData{};
			fin_Binary.read(reinterpret_cast<char*>(&chanData), sizeof(ChannelData));

			ChannelPassData channelPassData;

			channelPassData.iBoneIndex = chanData.iBoneIndex;
			channelPassData.iTranslationKeyFrameCount = chanData.iTranslationKeyFrameCount;
			channelPassData.iRotationKeyFrameCount = chanData.iRotationKeyFrameCount;
			channelPassData.iScaleKeyFrameCount = chanData.iScaleKeyFrameCount;


			channelPassData.TransKeyFrames.resize(chanData.iTranslationKeyFrameCount);
			channelPassData.ScaleKeyFrames.resize(chanData.iScaleKeyFrameCount);
			channelPassData.RotKeyFrames.resize(chanData.iRotationKeyFrameCount);

			// 순서 중요!
			// 위치 키프레임 읽기
			fin_Binary.read
			(
				reinterpret_cast<char*>(channelPassData.TransKeyFrames.data()),
				sizeof(KEYFRAME_VEC) * chanData.iTranslationKeyFrameCount
			);

			// 스케일 키프레임 읽기
			fin_Binary.read
			(
				reinterpret_cast<char*>(channelPassData.ScaleKeyFrames.data()),
				sizeof(KEYFRAME_VEC) * chanData.iScaleKeyFrameCount
			);

			// 회전 키프레임 읽기
			fin_Binary.read
			(
				reinterpret_cast<char*>(channelPassData.RotKeyFrames.data()),
				sizeof(KEYFRAME_QUAT) * chanData.iRotationKeyFrameCount
			);

			//플레이어 루트 노드 채널 문제때문에 추가함 + 아래 변경사항 
			if (true == Is_SkippedChannelBone(chanData.szBoneName))
				continue;

			if (eTimeUnit == ANIMATION_TIME_UNIT::TICKS)
			{
				Normalize_ChannelTime(channelPassData, fTimeScale);
			}

			CChannel* pChannel = CChannel::Create(channelPassData);
			//실패하면 만들어진 채널들 해제하고 종료
			if (pChannel == nullptr)
			{
				/*for (_uint k = 0; k < j; k++)
				{
					Safe_Release(Channels[k]);
				}*/
				for (CChannel* pCreatedChannel : Channels)
					Safe_Release(pCreatedChannel);

				MSG_BOX("Channel Create Fail !");
				return E_FAIL;
			}
			//Channels[j] = pChannel;
			Channels.push_back(pChannel);
		}

		//Animation 생성 
		CAnimation* pAnimation = CAnimation::Create(AnimData, Channels);

		if (pAnimation == nullptr)
		{
			for (size_t i = 0; i < Channels.size(); i++)
			{
				Safe_Release(Channels[i]);
			}
			Channels.clear();
			MSG_BOX("Animation Create Fail");
			return E_FAIL;
		}

		OutAnimations.push_back(pAnimation);
		Channels.clear();
	}

	fin_Binary.close();
	return S_OK;
}

HRESULT CAnimationLoader::Register_LoadedAnimations(CGameInstance* pGameInstance, const _wstring& strKeyFolderName, vector<CAnimation*>& LoadedAnimations, _bool bStaticAnimation)
{
	for (CAnimation*& pLoadedAnimation : LoadedAnimations)
	{
		if (pLoadedAnimation == nullptr)
			continue;

		const _uint64 iAnimationKey = Hash_String((strKeyFolderName + L"/" + pLoadedAnimation->Get_AnimationFileName()).c_str());

		CAnimation* pAnimation = nullptr;

		if (SUCCEEDED(pGameInstance->Get_Animation(iAnimationKey, &pAnimation)))
		{
			// 이미 등록된 애니메이션
			Safe_Release(pAnimation);
			Safe_Release(pLoadedAnimation);
			continue;
		}

		const HRESULT hr = bStaticAnimation ? pGameInstance->Register_StaticAnimation(pLoadedAnimation, iAnimationKey)
											: pGameInstance->Register_DynamicAnimation(pLoadedAnimation, iAnimationKey);

		if (FAILED(hr))
		{
			Safe_Release(pLoadedAnimation);
			continue;
		}

		// 등록 성공 시 리소스 매니저가 소유
		pLoadedAnimation = nullptr;
	}

	return S_OK;
}

_float CAnimationLoader::Calc_TimeScaleMillsec(_float fTickPerSecond, ANIMATION_TIME_UNIT eTimeUnit)
{
	if (eTimeUnit == ANIMATION_TIME_UNIT::TICKS)
	{
		if (fTickPerSecond > 0.f)
			return 1000.f / fTickPerSecond;
	}

	return 1.f;
}

void CAnimationLoader::Normalize_ChannelTime(ChannelPassData& ChannelData, _float fTimeScale)
{
	if (fabsf(fTimeScale - 1.f) < 0.0001f)
		return;

	for (KEYFRAME_VEC& KeyFrame : ChannelData.TransKeyFrames)
		KeyFrame.fTrackPosition *= fTimeScale;

	for (KEYFRAME_VEC& KeyFrame : ChannelData.ScaleKeyFrames)
		KeyFrame.fTrackPosition *= fTimeScale;

	for (KEYFRAME_QUAT& KeyFrame : ChannelData.RotKeyFrames)
		KeyFrame.fTrackPosition *= fTimeScale;
}
