#pragma once
#include "Engine_Defines.h"
#include "Animator.h"
#include "GameInstance.h"


NS_BEGIN(Engine)
class CAnimator;
class CGameInstance;
NS_END

class CShakeWindowSync final
{
public:
	void Update(CAnimator* pAnimator, CGameInstance* pGameInstance)
	{
		if (pAnimator == nullptr || pGameInstance == nullptr) return;

		//새 윈도우가 있으면 재생 후 삽입 
		const auto& ActiveWindows = pAnimator->Get_ActiveCustomWindows();
		for (const Engine::ANIM_WINDOW* pWin : ActiveWindows)
		{
			if (pWin == nullptr || pWin->iPayloadHash == 0) continue;
			if (m_PlayingShakes.find(pWin->iPayloadHash) != m_PlayingShakes.end()) continue;

			if (SUCCEEDED(pGameInstance->Play_Shake(pWin->iPayloadHash)))
			{
				m_PlayingShakes.insert(pWin->iPayloadHash);
			}
		}

		//끝난 윈도우 체크 
		for (auto it = m_PlayingShakes.begin(); it != m_PlayingShakes.end();)
		{
			_bool bAlive = false;
			for (const Engine::ANIM_WINDOW* pWin : ActiveWindows)
			{
				if (pWin && pWin->iPayloadHash == *it) 
				{ 
					bAlive = true;
					break; 
				}
			}

			if (bAlive == false)
			{
				pGameInstance->Stop_Shake(*it);
				it = m_PlayingShakes.erase(it);
			}
			else
				++it;
		}

	}

	void Clear(CGameInstance* pGameInstance)
	{
		if (pGameInstance)
		{
			for (_uint64 iKeyHash : m_PlayingShakes)
				pGameInstance->Stop_Shake(iKeyHash);
		}
		m_PlayingShakes.clear();
	}

private:
	unordered_set<_uint64> m_PlayingShakes;

};
