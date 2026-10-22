#pragma once

namespace GameCore
{
	namespace PlayerData
	{
		//TODO : 후에 CSV 데이터 파일로 관리하기 
		constexpr float		g_fDodgeStaminaCost			 = { 25.f };
		constexpr float		g_fRollStaminaCost			 = { 25.f };

		constexpr float		g_fFableArtsBaseStaminaCost	 = { 20.f };
		constexpr float		g_fFableArtsFinalStaminaCost = { 25.f };
		constexpr float		g_fHeavyAttackStaminaCost	 = { 15.f };
		constexpr float		g_fLightAttackStaminaCost	 = { 10.f };
	}
}