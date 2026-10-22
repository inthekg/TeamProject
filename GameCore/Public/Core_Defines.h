#pragma once
#include "Engine_Defines.h"

#ifdef	GAMECORE_EXPORTS
#define GAMECORE_DLL		_declspec(dllexport)
#else
#define GAMECORE_DLL		_declspec(dllimport)
#endif

#include "Core_Enums.h"
#include "Core_Struct.h"

namespace GameCore
{
	//static const unsigned int		g_iWinSizeX = { 2560 };
	//static const unsigned int		g_iWinSizeY = { 1440 };

	static const unsigned int		g_iWinSizeX = { 1280 };
	static const unsigned int		g_iWinSizeY = { 720 };
	

	enum class LEVEL { STATIC, LOADING, LOGO, GAMEPLAY, EDIT, SHADER, 
		CHAP_01, CHAP_02, CHAP_03, CHAP_TEST, CHAP_TESTOBJ, CHAP_BOSS,
		END };
	
	static const unsigned int g_iMaxChapter = { 6 };
	static const wchar_t* g_ChapterFiles[g_iMaxChapter] =
	{
		TEXT("Chapter01.chapter.json"),
		TEXT("Hotel.chapter.json"),
		TEXT("Chapter02.chapter.json"),
		TEXT("TestFloor.json"),
		TEXT("Test_object.json"),
		TEXT("BossMap.chapter.json"),
	};

	static const _float4 g_ChapterPlayer_StartPos[g_iMaxChapter] =
	{
		{ 92.7f,  40.42f,  -660.3f, 1.f },		// 시작맵 //{ 97.f, 40.5f, -557.f, 1.f },
		{ 203.5f, 67.5f,  -120.4f,  1.f },		// 호텔    
		{ 49.5f,  100.f,  -397.5f,  1.f },		// 락사시아맵 	{ 76.4f,  128.2f,  -256.8f,  1.f },	
		{ 0.f,    0.5f,    0.f,     1.f },		// 테스트맵 
		{ 0.f,    0.5f,    0.f,     1.f },		// 테스트맵 
		{ 154.9f, 59.1f, -291.4f,	1.f }		//{ 203.853f, 66.277f, -168.1f, 1.f }	// 호텔 앞 
												//{ -1.7f,  401.2f,  -61.9f,  1.f }		// 락사시아 보스방 
	};

	inline LEVEL Chapter_To_Level(unsigned int iChapterIndex)
	{
		return static_cast<LEVEL>(ETOI(LEVEL::CHAP_01)+ iChapterIndex);
	}
}

using namespace GameCore;
