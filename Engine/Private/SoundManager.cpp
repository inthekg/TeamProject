#include "SoundManager.h"
#include "fmod.hpp"
#include "GameInstance.h"
#include "Engine_Utility.h"

namespace
{
    // BGM Sequence 는 한 번에 하나만 존재하므로 고정 키를 쓴다.
    const string BGM_SEQUENCE_KEY = "__BGM_SEQUENCE__";
}

CSound_Manager::CSound_Manager()
    : m_pGameInstance{ CGameInstance::Get_Instance() }
{
}

void CSound_Manager::Ready_Sound()
{
    if (m_pSystem) return;

    FMOD_Debug_Initialize(FMOD_DEBUG_LEVEL_NONE, FMOD_DEBUG_MODE_TTY, nullptr, nullptr);

    if (FMOD_System_Create(&m_pSystem, FMOD_VERSION) != FMOD_OK) return;
    if (FMOD_System_Init(m_pSystem, 128, FMOD_INIT_NORMAL, nullptr) != FMOD_OK) return;

    // 3D 설정값: 도플러 스케일 = 1, DistanceFactor = 1, RollOffScale = 1 설정
    FMOD_System_Set3DSettings(m_pSystem, 1.f, 1.f, 1.f);

    FMOD_System_GetMasterChannelGroup(m_pSystem, &m_pChannelGroup[SOUND_MASTER]);

    FMOD_System_CreateChannelGroup(m_pSystem, "SFX", &m_pChannelGroup[SOUND_SFX]);
    FMOD_System_CreateChannelGroup(m_pSystem, "BGM", &m_pChannelGroup[SOUND_BGM]);
    FMOD_System_CreateChannelGroup(m_pSystem, "MONSTER", &m_pChannelGroup[SOUND_MONSTER]);
    FMOD_System_CreateChannelGroup(m_pSystem, "PLAYER", &m_pChannelGroup[SOUND_PLAYER]);
    FMOD_System_CreateChannelGroup(m_pSystem, "WEAPON", &m_pChannelGroup[SOUND_WEAPON]);

    // 그룹 기본값 볼륨 세팅 . 개별 채널 볼륨이랑 곱해짐
    for (int i = SOUND_MASTER + 1; i < SOUND_END; ++i)
        FMOD_ChannelGroup_AddGroup(m_pChannelGroup[SOUND_MASTER], m_pChannelGroup[i], false, nullptr);

    Load_SoundFile();
    Load_SoundCueTables();

    FMOD_ChannelGroup_SetVolume(m_pChannelGroup[SOUND_MONSTER], MONSTER_GROUP_VOLUME);
}



void CSound_Manager::Update_Sound(_float fTimeDelta)
{
    m_fSoundTime += fTimeDelta;

    Update_SoundSequences();
    Update_BGMFade(fTimeDelta);
    Update_AmbienceFade(fTimeDelta);

    // 3D 리스너 업데이트
    Update_Listener(fTimeDelta);
    Update_SoundSources(fTimeDelta);


    if (m_pSystem)
        FMOD_System_Update(m_pSystem);
}

//내부 함수 
FMOD_CHANNEL* CSound_Manager::Play(const string& strSoundKey, CHANNEL_ID eGroup, _float fVolume, _bool bLoop, _bool bTrack, _float fPitch)
{
    FMOD_CHANNEL* pChannel = Play_Internal(strSoundKey, eGroup, fVolume, bLoop, nullptr, fPitch);
    if (pChannel == nullptr)
        return nullptr;

    if (bTrack)
    {
        auto it = m_PlayingSounds.find(strSoundKey);
        //이미 재생 중이면 이전 채널 정지 후 새 채널로 교체
        if (it != m_PlayingSounds.end())
            FMOD_Channel_Stop(it->second);

        m_PlayingSounds[strSoundKey] = pChannel;
    }
    return pChannel;
}

FMOD_CHANNEL* CSound_Manager::Play_Internal(const string& strSoundKey, CHANNEL_ID eGroup, _float fVolume, _bool bLoop, const _float3* pWorldPos, _float fPitch)
{
    auto iter = m_Sounds.find(strSoundKey);
    if (iter == m_Sounds.end())
    {
#ifdef _DEBUG
        OutputDebugStringA(("[Sound] NOT LOADED : " + strSoundKey + "\n").c_str());
#endif
        return nullptr;
    }

    FMOD_CHANNEL* pChannel = nullptr;

    const FMOD_RESULT ePlayResult = FMOD_System_PlaySound(m_pSystem, iter->second, m_pChannelGroup[eGroup], TRUE, &pChannel);

    if (ePlayResult != FMOD_OK)
    {
#ifdef _DEBUG
        OutputDebugStringA(("[Sound] PLAY FAILED(" + to_string(ePlayResult) + ") : " + strSoundKey + "\n").c_str());
#endif
        return nullptr;
    }

    const FMOD_MODE eLoopMode = bLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
    const FMOD_MODE eSpaceMode = pWorldPos ? (FMOD_3D | FMOD_3D_LINEARSQUAREROLLOFF) : FMOD_2D;

    FMOD_Channel_SetMode(pChannel, eLoopMode | eSpaceMode);

    if (pWorldPos)
    {
        FMOD_VECTOR vPos = { pWorldPos->x, pWorldPos->y, pWorldPos->z };
        FMOD_VECTOR vVel = { 0.f, 0.f, 0.f };
        FMOD_Channel_Set3DAttributes(pChannel, &vPos, &vVel);
    }

    FMOD_Channel_SetVolume(pChannel, fVolume);

    if (fPitch != 1.f)
        FMOD_Channel_SetPitch(pChannel, fPitch);

    FMOD_Channel_SetPaused(pChannel, FALSE);

    return pChannel;
}

// Listener의 정보를 매 프레임 갱신하는 함수
void CSound_Manager::Update_Listener(_float fTimeDelta)
{
    if (m_pSystem == nullptr)
        return;

    const _float4* pCamPos = m_pGameInstance->Get_CamPosition();
    const _float4* pCamLook = m_pGameInstance->Get_CamLook();

    const _float4x4* pCamWorld = m_pGameInstance->Get_Transform_Inverse(D3DTS::VIEW);

    if (pCamPos == nullptr || pCamLook == nullptr || pCamWorld == nullptr)
        return;

    // 초기 프레임에서 NaN이 나오는 상황을 방지해야 한다.
    _vector vLookRaw = XMLoadFloat4(pCamLook);
    _float3 vUpRaw = { pCamWorld->_21, pCamWorld->_22, pCamWorld->_23 };
    _vector vUpLoad = XMLoadFloat3(&vUpRaw);

    if (XMVectorGetX(XMVector3LengthSq(vLookRaw)) < 1e-6f ||
        XMVectorGetX(XMVector3LengthSq(vUpLoad)) < 1e-6f)
        return;

    // FMOD에서 사용하기 위해서는 정규화가 반드시 이루어져야 함.
    _vector vLook = XMVector3Normalize(vLookRaw);
    _vector vUp = XMVector3Normalize(vUpLoad);

    FMOD_VECTOR vPos = { pCamPos->x, pCamPos->y, pCamPos->z };
    FMOD_VECTOR vForward = { XMVectorGetX(vLook), XMVectorGetY(vLook), XMVectorGetZ(vLook) };
    FMOD_VECTOR vUpVec = { XMVectorGetX(vUp), XMVectorGetY(vUp), XMVectorGetZ(vUp) };


    // 도플러 효과의 크기를 판별
    FMOD_VECTOR vVel = { 0.f, 0.f, 0.f };
    if (m_bListenerReady && fTimeDelta > 0.f) // 첫 프레임은 판별하지 않음. (전 프레임 값이 없기 때문에 왜곡 발생)
    {
        vVel.x = (pCamPos->x - m_vListenerPrevPos.x) / fTimeDelta;
        vVel.y = (pCamPos->y - m_vListenerPrevPos.y) / fTimeDelta;
        vVel.z = (pCamPos->z - m_vListenerPrevPos.z) / fTimeDelta;
    }

    FMOD_System_Set3DListenerAttributes(m_pSystem, 0, &vPos, &vVel, &vForward, &vUpVec);

    m_vListenerPrevPos = { pCamPos->x, pCamPos->y, pCamPos->z };
    m_bListenerReady = true;
}

void CSound_Manager::Update_SoundSequences()
{
    for (auto iter = m_SoundSequences.begin(); iter != m_SoundSequences.end(); )
    {
        SOUND_SEQUENCE_STATE_DESC& StateDesc = iter->second;

        if (StateDesc.eState == SOUND_SEQUENCE_STATE::START)
        {
            _float fLoopStartDelay = StateDesc.Desc.fLoopStartDelaySec;

            if (fLoopStartDelay < 0.f)
            {
                const _float fStartLength = Get_SoundLengthSec(StateDesc.Desc.strStartKey);
                fLoopStartDelay = max(0.f, fStartLength - StateDesc.Desc.fLoopOverlapSec);
            }

            if (!StateDesc.bLoopStarted &&
                !StateDesc.Desc.strLoopKey.empty() &&
                m_fSoundTime - StateDesc.fStartTimeSec >= fLoopStartDelay)
            {
                StateDesc.pLoopChannel = Play(StateDesc.Desc.strLoopKey, StateDesc.Desc.eGroup, StateDesc.Desc.fVolume, true, false);
                StateDesc.bLoopStarted = (StateDesc.pLoopChannel != nullptr);
            }

            FMOD_BOOL bStartPlaying = FALSE;

            if (StateDesc.pStartChannel != nullptr)
                FMOD_Channel_IsPlaying(StateDesc.pStartChannel, &bStartPlaying);

            if (!bStartPlaying)
            {
                StateDesc.pStartChannel = nullptr;

                if (StateDesc.pLoopChannel != nullptr)
                {
                    StateDesc.eState = SOUND_SEQUENCE_STATE::LOOP;
                    ++iter;
                    continue;
                }

                Clear_SoundSequence(StateDesc);
                iter = m_SoundSequences.erase(iter);
                continue;
            }
        }
        else if (StateDesc.eState == SOUND_SEQUENCE_STATE::LOOP)
        {
            FMOD_BOOL bLoopPlaying = FALSE;

            if (StateDesc.pLoopChannel != nullptr)
                FMOD_Channel_IsPlaying(StateDesc.pLoopChannel, &bLoopPlaying);

            if (!bLoopPlaying)
            {
                Clear_SoundSequence(StateDesc);
                iter = m_SoundSequences.erase(iter);
                continue;
            }
        }
        else if (StateDesc.eState == SOUND_SEQUENCE_STATE::END)
        {
            FMOD_BOOL bEndPlaying = FALSE;

            if (StateDesc.pEndChannel != nullptr)
                FMOD_Channel_IsPlaying(StateDesc.pEndChannel, &bEndPlaying);

            if (!bEndPlaying)
            {
                Clear_SoundSequence(StateDesc);
                iter = m_SoundSequences.erase(iter);
                continue;
            }
        }

        ++iter;
    }
}

void CSound_Manager::Update_BGMFade(_float fTimeDelta)
{
    if (m_fBGMFadeDuration <= 0.f)
        return;

    m_fBGMFadeTimer += fTimeDelta;

    _float fRatio = m_fBGMFadeTimer / m_fBGMFadeDuration;

    if (fRatio > 1.f)
        fRatio = 1.f;

    if (m_pCurBGM)
        FMOD_Channel_SetVolume(m_pCurBGM, m_fBGMTargetVolume * fRatio);

    if (m_pPrevBGM)
        FMOD_Channel_SetVolume(m_pPrevBGM, m_fBGMPrevVolume * (1.f - fRatio));

    if (fRatio < 1.f)
        return;

    Clear_PrevBGM();

    m_fBGMFadeDuration = 0.f;
    m_fBGMFadeTimer = 0.f;
}

void CSound_Manager::Update_AmbienceFade(_float fTimeDelta)
{
    if (m_fAmbienceFadeDuration <= 0.f)
        return;

    m_fAmbienceFadeTimer += fTimeDelta;

    _float fRatio = m_fAmbienceFadeTimer / m_fAmbienceFadeDuration;

    if (fRatio > 1.f)
        fRatio = 1.f;

    if (m_pCurAmbience)
        FMOD_Channel_SetVolume(m_pCurAmbience, m_fAmbienceTargetVolume * fRatio);

    if (m_pPrevAmbience)
        FMOD_Channel_SetVolume(m_pPrevAmbience, m_fAmbiencePrevVolume * (1.f - fRatio));

    if (fRatio < 1.f)
        return;

    Clear_PrevAmbience();

    m_fAmbienceFadeDuration = 0.f;
    m_fAmbienceFadeTimer = 0.f;
}

void CSound_Manager::Clear_PrevAmbience()
{
    if (nullptr == m_pPrevAmbience)
    {
        m_strPrevAmbienceKey.clear();
        return;
    }

    FMOD_Channel_Stop(m_pPrevAmbience);
    m_PlayingSounds.erase(m_strPrevAmbienceKey);

    m_pPrevAmbience = nullptr;
    m_strPrevAmbienceKey.clear();
}

void CSound_Manager::Clear_PrevBGM()
{
    if (nullptr == m_pPrevBGM)
    {
        m_strPrevBGMKey.clear();
        return;
    }

    FMOD_Channel_Stop(m_pPrevBGM);
    m_PlayingSounds.erase(m_strPrevBGMKey);

    m_pPrevBGM = nullptr;
    m_strPrevBGMKey.clear();
}

void CSound_Manager::Clear_SoundSequence(SOUND_SEQUENCE_STATE_DESC& StateDesc)
{
    if (StateDesc.pStartChannel != nullptr)
    {
        FMOD_Channel_Stop(StateDesc.pStartChannel);
        StateDesc.pStartChannel = nullptr;
    }

    if (StateDesc.pLoopChannel != nullptr)
    {
        FMOD_Channel_Stop(StateDesc.pLoopChannel);
        StateDesc.pLoopChannel = nullptr;
    }

    if (StateDesc.pEndChannel != nullptr)
    {
        FMOD_Channel_Stop(StateDesc.pEndChannel);
        StateDesc.pEndChannel = nullptr;
    }

    StateDesc.eState = SOUND_SEQUENCE_STATE::NONE;
    StateDesc.bLoopStarted = false;
    StateDesc.fStartTimeSec = 0.f;
}

_float CSound_Manager::Get_SoundLengthSec(const string& strSoundKey) const
{
    auto iter = m_Sounds.find(strSoundKey);

    if (iter == m_Sounds.end())
        return 0.f;

    unsigned int iLengthMS = 0;

    if (FMOD_Sound_GetLength(iter->second, &iLengthMS, FMOD_TIMEUNIT_MS) != FMOD_OK)
        return 0.f;

    return static_cast<_float>(iLengthMS) * 0.001f;
}

void CSound_Manager::Stop_BGM()
{
    // Intro->Loop 로 재생 중인 BGM은 m_pCurBGM 이 아니라 Sequence 쪽에 있다.
    Stop_SoundSequence(BGM_SEQUENCE_KEY);

    Clear_PrevBGM();

    m_fBGMFadeDuration = 0.f;
    m_fBGMFadeTimer = 0.f;

    if (m_pCurBGM)
    {
        FMOD_Channel_Stop(m_pCurBGM);
        m_PlayingSounds.erase(m_strBGMKey);
        m_pCurBGM = nullptr;
        m_strBGMKey.clear();
    }
}


void CSound_Manager::Play_Sound(const string& strSoundKey, CHANNEL_ID eGroup, _float fVolume, _bool bLoop, _bool bTrack)
{
    Play(strSoundKey, eGroup, fVolume, bLoop, bTrack);
}

void CSound_Manager::Play_SFXSound(const string& strSoundKey, _float fVolume, _bool bLoop, _bool bTrack)
{
    Play(strSoundKey, SOUND_SFX, fVolume, bLoop, bTrack);
}

void CSound_Manager::Play_BGM(const string& strSoundKey, _float fVolume)
{
    Stop_SoundSequence(BGM_SEQUENCE_KEY);

    // 페이드 아웃 중 새로운 BGM이 들어오면 남아 있는 이전 BGM을 즉시 정리한다.
    Clear_PrevBGM();
    m_fBGMFadeDuration = 0.f;
    m_fBGMFadeTimer = 0.f;

    //같은 BGM이 재생 중이면 중복 재생 방지
    if (m_pCurBGM && m_strBGMKey == strSoundKey)
    {
        FMOD_BOOL bPlaying = false;
        FMOD_Channel_IsPlaying(m_pCurBGM, &bPlaying);
        if (bPlaying) return;
    }

    //이전 곡 정지 
    if (m_pCurBGM)
    {
        FMOD_Channel_Stop(m_pCurBGM);
        m_PlayingSounds.erase(m_strBGMKey);
    }

    m_strBGMKey = strSoundKey;
    m_pCurBGM = Play(strSoundKey, SOUND_BGM, fVolume, true, true);
}

void CSound_Manager::Play_BGM(const string& strSoundKey, _float fVolume, _float fFadeSec)
{
    Stop_SoundSequence(BGM_SEQUENCE_KEY);

    if (fFadeSec <= 0.f)
    {
        Play_BGM(strSoundKey, fVolume);
        return;
    }

    if (m_pCurBGM && m_strBGMKey == strSoundKey)
    {
        FMOD_BOOL bPlaying = false;
        FMOD_Channel_IsPlaying(m_pCurBGM, &bPlaying);
        if (bPlaying) return;
    }

    // 이전 페이드가 끝나기 전에 또 바뀌면 더 오래된 곡은 즉시 버린다.
    Clear_PrevBGM();

    m_pPrevBGM = m_pCurBGM;
    m_strPrevBGMKey = m_strBGMKey;

    _float fPrevVolume = 0.f;
    if (m_pPrevBGM)
        FMOD_Channel_GetVolume(m_pPrevBGM, &fPrevVolume);

    m_strBGMKey = strSoundKey;
    m_pCurBGM = Play(strSoundKey, SOUND_BGM, 0.f, true, true);

    if (nullptr == m_pCurBGM)
    {
        // 새 곡을 못 틀면 이전 곡을 그대로 유지한다.
        m_pCurBGM = m_pPrevBGM;
        m_strBGMKey = m_strPrevBGMKey;
        m_pPrevBGM = nullptr;
        m_strPrevBGMKey.clear();
        return;
    }

    m_fBGMTargetVolume = fVolume;
    m_fBGMPrevVolume = fPrevVolume;
    m_fBGMFadeDuration = fFadeSec;
    m_fBGMFadeTimer = 0.f;
}

void CSound_Manager::Play_BGM_Sequence(const string& strIntroKey, const string& strLoopKey, _float fVolume)
{
    // Stop_BGM 이 Sequence 도 같이 정리하므로 여기서 이전 BGM을 한 번에 끊는다.
    Stop_BGM();

    SOUND_SEQUENCE_DESC Desc{};
    Desc.strSequenceKey = BGM_SEQUENCE_KEY;
    Desc.strStartKey = strIntroKey;
    Desc.strLoopKey = strLoopKey;
    Desc.eGroup = SOUND_BGM;
    Desc.fVolume = fVolume;

    Play_SoundSequence(Desc);
}

void CSound_Manager::Play_MonsterSound(const string& strSoundKey, _float fVolume, _bool bLoop, _bool bTrack)
{
    Play(strSoundKey, SOUND_MONSTER, fVolume, bLoop, bTrack);
}

void CSound_Manager::Play_PlayerSound(const string& strSoundKey, _float fVolume, _bool bLoop, _bool bTrack, _float fPitch)
{
    Play(strSoundKey, SOUND_PLAYER, fVolume, bLoop, bTrack, fPitch);
}

void CSound_Manager::Play_WeaponSound(const string& strSoundKey, _float fVolume, _bool bLoop, _bool bTrack)
{
    Play(strSoundKey, SOUND_WEAPON, fVolume, bLoop, bTrack);
}

void CSound_Manager::Play_Ambience(const string& strSoundKey, _float fVolume, _float fFadeSec)
{
    if (m_pCurAmbience && m_strAmbienceKey == strSoundKey)
    {
        FMOD_BOOL bPlaying = false;
        FMOD_Channel_IsPlaying(m_pCurAmbience, &bPlaying);
        if (bPlaying) return;
    }

    Clear_PrevAmbience();

    m_pPrevAmbience = m_pCurAmbience;
    m_strPrevAmbienceKey = m_strAmbienceKey;

    _float fPrevVolume = 0.f;
    if (m_pPrevAmbience)
        FMOD_Channel_GetVolume(m_pPrevAmbience, &fPrevVolume);

    m_strAmbienceKey = strSoundKey;
    m_pCurAmbience = Play(strSoundKey, SOUND_SFX, 0.f, true, true);

    if (nullptr == m_pCurAmbience)
    {
        m_pCurAmbience = m_pPrevAmbience;
        m_strAmbienceKey = m_strPrevAmbienceKey;
        m_pPrevAmbience = nullptr;
        m_strPrevAmbienceKey.clear();
        return;
    }

    m_fAmbienceTargetVolume = fVolume;
    m_fAmbiencePrevVolume = fPrevVolume;
    m_fAmbienceFadeDuration = (fFadeSec > 0.f) ? fFadeSec : 0.0001f;
    m_fAmbienceFadeTimer = 0.f;
}

void CSound_Manager::Stop_Ambience()
{
    Clear_PrevAmbience();

    m_fAmbienceFadeDuration = 0.f;
    m_fAmbienceFadeTimer = 0.f;

    if (m_pCurAmbience)
    {
        FMOD_Channel_Stop(m_pCurAmbience);
        m_PlayingSounds.erase(m_strAmbienceKey);
        m_pCurAmbience = nullptr;
        m_strAmbienceKey.clear();
    }
}


_uint CSound_Manager::Play_SoundAt(const string& strSoundKey, CHANNEL_ID eGroup, const _float3& vWorldPos, _float fVolume, _bool bLoop)
{
    FMOD_CHANNEL* pChannel = Play_Internal(strSoundKey, eGroup, fVolume, bLoop, &vWorldPos);
    if (pChannel == nullptr)
        return 0;

    const _uint hHandle = m_iNextSourceID++;

    SOUND_SOURCE tSource;
    tSource.pChannel = pChannel;
    tSource.pFollowMat = nullptr;
    tSource.vLastPos = vWorldPos;
    tSource.bDoppler = false;

    m_SoundSources.emplace(hHandle, tSource);

    return hHandle;
}

_uint CSound_Manager::Play_SoundFollow(const string& strSoundKey, CHANNEL_ID eGroup, const _float4x4* pFollowMat, _float fVolume, _bool bLoop, _bool bDoppler)
{
    if (pFollowMat == nullptr)
        return 0;

    const _float3 vSpawnPos = { pFollowMat->_41, pFollowMat->_42, pFollowMat->_43 };

    FMOD_CHANNEL* pChannel = Play_Internal(strSoundKey, eGroup, fVolume, bLoop, &vSpawnPos);
    if (pChannel == nullptr)
        return 0;

    const _uint hHandle = m_iNextSourceID++;

    SOUND_SOURCE tSource;

    tSource.pChannel = pChannel;
    tSource.pFollowMat = pFollowMat;
    tSource.vLastPos = vSpawnPos;
    tSource.bDoppler = bDoppler;

    m_SoundSources.emplace(hHandle, tSource);

    return hHandle;
}


void CSound_Manager::Stop_SoundSource(_uint hHandle)
{
    auto iter = m_SoundSources.find(hHandle);
    if (iter == m_SoundSources.end())
        return;

    if (iter->second.pChannel != nullptr)
        FMOD_Channel_Stop(iter->second.pChannel);

    m_SoundSources.erase(iter);
}

_bool CSound_Manager::Is_SoundSourcePlaying(_uint hHandle)
{
    auto iter = m_SoundSources.find(hHandle);
    if (iter == m_SoundSources.end())
        return false;

    FMOD_BOOL bPlaying = FALSE;
    if (iter->second.pChannel == nullptr ||
        FMOD_Channel_IsPlaying(iter->second.pChannel, &bPlaying) != FMOD_OK || !bPlaying)
    {
        m_SoundSources.erase(iter);
        return false;
    }
    return true;
}

void CSound_Manager::Update_SoundSources(_float fTimeDelta)
{
    for (auto iter = m_SoundSources.begin(); iter != m_SoundSources.end(); )
    {
        FMOD_BOOL bPlaying = FALSE;

        if (iter->second.pChannel != nullptr)
            FMOD_Channel_IsPlaying(iter->second.pChannel, &bPlaying);

        if (!bPlaying)
        {
            iter = m_SoundSources.erase(iter);
            continue;
        }

        SOUND_SOURCE& tSource = iter->second;

        if (tSource.pFollowMat != nullptr)
        {
            const _float3 vCur = { tSource.pFollowMat->_41, tSource.pFollowMat->_42, tSource.pFollowMat->_43 };

            FMOD_VECTOR vPos = { vCur.x, vCur.y, vCur.z };
            FMOD_VECTOR vVel = { 0.f, 0.f, 0.f };
            if (tSource.bDoppler && fTimeDelta > 0.f)
            {
                vVel.x = (vCur.x - tSource.vLastPos.x) / fTimeDelta;
                vVel.y = (vCur.y - tSource.vLastPos.y) / fTimeDelta;
                vVel.z = (vCur.z - tSource.vLastPos.z) / fTimeDelta;
            }

            FMOD_Channel_Set3DAttributes(tSource.pChannel, &vPos, &vVel);
            tSource.vLastPos = vCur;

        }
        ++iter;
    }
}

HRESULT CSound_Manager::Load_SoundCueTables()
{
    const string strCueDir = m_pGameInstance->Get_ResourcePathA() + "\\Data\\Sound\\";
    Load_SoundCueFolder(strCueDir);
    return S_OK;
}

void CSound_Manager::Load_SoundCueFolder(const string& strCueDir)
{
    static const string strTableTag = "_SoundCue.json";

    _finddata_t fd;
    intptr_t handle = _findfirst((strCueDir + "*.*").c_str(), &fd);
    if (handle == -1)
        return;

    int iResult = 0;

    while (iResult != -1)
    {
        if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0)
        {
            iResult = _findnext(handle, &fd);
            continue;
        }

        if (fd.attrib & _A_SUBDIR)
        {
            Load_SoundCueFolder(strCueDir + fd.name + "\\");
            iResult = _findnext(handle, &fd);
            continue;
        }

        const string strFileName = fd.name;

        if (strFileName.size() > strTableTag.size() &&
            0 == strFileName.compare(strFileName.size() - strTableTag.size(), strTableTag.size(), strTableTag))
            Load_SoundCueFile(strCueDir + strFileName);

        iResult = _findnext(handle, &fd);
    }

    _findclose(handle);
}

void CSound_Manager::Load_SoundCueFile(const string& strFilePath)
{
    ifstream File(strFilePath);
    if (false == File.is_open())
        return;

    json Root;

    try
    {
        File >> Root;
    }
    catch (...)
    {
        return;
    }

    if (false == Root.contains("Cues") || false == Root["Cues"].is_array())
        return;

    for (const auto& CueJson : Root["Cues"])
    {
        SOUND_CUE_DESC Cue = {};

        Cue.strName = CueJson.value("Name", "");
        if (true == Cue.strName.empty())
            continue;

        if (CueJson.contains("Waves") && CueJson["Waves"].is_array())
        {
            for (const auto& WaveJson : CueJson["Waves"])
                Cue.Waves.push_back(WaveJson.get<string>());
        }

        if (true == Cue.Waves.empty())
            continue;

        if (CueJson.contains("Weights") && CueJson["Weights"].is_array())
        {
            for (const auto& WeightJson : CueJson["Weights"])
                Cue.Weights.push_back(WeightJson.get<_float>());
        }

        Cue.Weights.resize(Cue.Waves.size(), 1.f);

        Cue.fPitchMin = CueJson.value("PitchMin", 1.f);
        Cue.fPitchMax = CueJson.value("PitchMax", 1.f);
        Cue.fVolumeMin = CueJson.value("VolumeMin", 1.f);
        Cue.fVolumeMax = CueJson.value("VolumeMax", 1.f);
        Cue.fVolumeMultiplier = CueJson.value("VolumeMultiplier", 1.f);
        Cue.fMaxDistance = CueJson.value("MaxDistance", 3500.f);

        const _float fPitchMultiplier = CueJson.value("PitchMultiplier", 1.f);
        Cue.fPitchMin *= fPitchMultiplier;
        Cue.fPitchMax *= fPitchMultiplier;

        if (Cue.fPitchMax < Cue.fPitchMin)
            swap(Cue.fPitchMin, Cue.fPitchMax);

        if (Cue.fVolumeMax < Cue.fVolumeMin)
            swap(Cue.fVolumeMin, Cue.fVolumeMax);

        m_SoundCues[Cue.strName] = Cue;
    }
}

void CSound_Manager::Update_SoundSourcePos(_uint hHandle, const _float3& vWorldPos)
{
    auto iter = m_SoundSources.find(hHandle);
    if (iter == m_SoundSources.end())
        return;

    SOUND_SOURCE& tSource = iter->second;

    FMOD_VECTOR vPos = { vWorldPos.x, vWorldPos.y, vWorldPos.z };
    FMOD_VECTOR vVel = { 0.f, 0.f, 0.f };

    if (tSource.pChannel != nullptr)
        FMOD_Channel_Set3DAttributes(tSource.pChannel, &vPos, &vVel);

    tSource.vLastPos = vWorldPos;
}

_uint CSound_Manager::Play_Cue(const string& strCueKey, CHANNEL_ID eGroup, const _float3& vWorldPos, _float fVolumeScale, _bool bLoop)
{
    if (true == strCueKey.empty())
        return 0;

    auto iter = m_SoundCues.find(strCueKey);

    if (iter == m_SoundCues.end())
        return Play_SoundAt(strCueKey, eGroup, vWorldPos, fVolumeScale, bLoop);

    const SOUND_CUE_DESC& Cue = iter->second;

    _float fTotalWeight = 0.f;
    const string* pLastValid = nullptr;

    for (size_t i = 0; i < Cue.Waves.size(); ++i)
    {
        if (m_Sounds.find(Cue.Waves[i]) == m_Sounds.end())
            continue;

        fTotalWeight += max(0.f, Cue.Weights[i]);
        pLastValid = &Cue.Waves[i];
    }

    if (nullptr == pLastValid || fTotalWeight <= 0.f)
        return 0;

    if (false == bLoop)
    {
        static const string strVoicePrefix = "VO_";

        const _float fRetriggerInterval = (0 == strCueKey.rfind(strVoicePrefix, 0))
            ? CUE_RETRIGGER_INTERVAL_VOICE
            : CUE_RETRIGGER_INTERVAL;

        if (false == Can_PlaySoundByInterval(strCueKey, fRetriggerInterval))
            return 0;
    }

    const string* pChosen = nullptr;
    _float fPick = Random(0.f, fTotalWeight);

    for (size_t i = 0; i < Cue.Waves.size(); ++i)
    {
        if (m_Sounds.find(Cue.Waves[i]) == m_Sounds.end())
            continue;

        fPick -= max(0.f, Cue.Weights[i]);

        if (fPick <= 0.f)
        {
            pChosen = &Cue.Waves[i];
            break;
        }
    }

    if (nullptr == pChosen)
        pChosen = pLastValid;

    const _float fVolume = Random(Cue.fVolumeMin, Cue.fVolumeMax)
        * min(Cue.fVolumeMultiplier, CUE_VOLUME_CLAMP)
        * fVolumeScale;

    const _float fPitch = Random(Cue.fPitchMin, Cue.fPitchMax);

    FMOD_CHANNEL* pChannel = Play_Internal(*pChosen, eGroup, fVolume, bLoop, &vWorldPos, fPitch);
    if (nullptr == pChannel)
        return 0;

    const _float fMaxDistance = min(Cue.fMaxDistance * 0.01f, CUE_MAX_DISTANCE_LIMIT);
    FMOD_Channel_Set3DMinMaxDistance(pChannel, CUE_MIN_DISTANCE, max(CUE_MIN_DISTANCE, fMaxDistance));

    const _uint hHandle = m_iNextSourceID++;

    SOUND_SOURCE tSource;
    tSource.pChannel = pChannel;
    tSource.pFollowMat = nullptr;
    tSource.vLastPos = vWorldPos;
    tSource.bDoppler = false;

    m_SoundSources.emplace(hHandle, tSource);

    return hHandle;
}



_bool CSound_Manager::Has_Cue(const string& strCueKey) const
{
    return m_SoundCues.find(strCueKey) != m_SoundCues.end();
}

void CSound_Manager::StopSound_ByKey(const string& pSoundKey)
{
    auto iter = m_PlayingSounds.find(pSoundKey);
    if (iter == m_PlayingSounds.end()) return;

    FMOD_Channel_Stop(iter->second);
    m_PlayingSounds.erase(iter);
}

void CSound_Manager::SetVolume_ByKey(const string& strSoundKey, _float fVolume)
{
    auto iter = m_PlayingSounds.find(strSoundKey);
    if (iter == m_PlayingSounds.end()) return;

    FMOD_Channel_SetVolume(iter->second, fVolume);
}

_bool CSound_Manager::IsPlaying_ByKey(const string& strSoundKey)
{
    auto iter = m_PlayingSounds.find(strSoundKey);
    if (iter == m_PlayingSounds.end()) return false;

    FMOD_BOOL bPlaying = FALSE;
    if (FMOD_Channel_IsPlaying(iter->second, &bPlaying) != FMOD_OK || !bPlaying)
    {
        m_PlayingSounds.erase(iter);
        return false;
    }
    return true;
}

void CSound_Manager::FadeOut_BGM(_float fFadeSec)
{
    // Intro/Loop 시퀀스 BGM은 현재 일반 BGM 페이드 구조에 포함되지 않으므로 즉시 정리한다.
    Stop_SoundSequence(BGM_SEQUENCE_KEY);

    if (fFadeSec <= 0.f)
    {
        Stop_BGM();
        return;
    }

    // 이미 진행 중인 이전 페이드 채널을 먼저 정리한다.
    Clear_PrevBGM();

    if (m_pCurBGM == nullptr)
        return;

    _float fCurrentVolume = 0.f;
    FMOD_Channel_GetVolume(m_pCurBGM, &fCurrentVolume);

    // 현재 BGM을 이전 BGM 슬롯으로 옮기면 기존 Update_BGMFade()가 1에서 0으로 낮춘다.
    m_pPrevBGM = m_pCurBGM;
    m_strPrevBGMKey = m_strBGMKey;
    m_fBGMPrevVolume = fCurrentVolume;

    m_pCurBGM = nullptr;
    m_strBGMKey.clear();

    m_fBGMTargetVolume = 0.f;
    m_fBGMFadeDuration = fFadeSec;
    m_fBGMFadeTimer = 0.f;
}

void CSound_Manager::Stop_GroupSound(CHANNEL_ID eID)
{
    if (m_pChannelGroup[eID])
        FMOD_ChannelGroup_Stop(m_pChannelGroup[eID]);
}

void CSound_Manager::Stop_AllSound()
{
    // 테스트용 추가
    Stop_AllSoundSequences();

    if (m_pChannelGroup[SOUND_MASTER])
        FMOD_ChannelGroup_Stop(m_pChannelGroup[SOUND_MASTER]);

    m_PlayingSounds.clear();
    m_SoundSources.clear();
    m_pCurBGM = nullptr;
    m_strBGMKey.clear();
}

void CSound_Manager::Set_GroupVolume(CHANNEL_ID eID, _float fVolume)
{
    if (m_pChannelGroup[eID])
        FMOD_ChannelGroup_SetVolume(m_pChannelGroup[eID], fVolume);
}

_bool CSound_Manager::Can_PlaySoundByInterval(const string& strRuleKey, _float fIntervalSec)
{
    auto iter = m_SoundNextPlayTimes.find(strRuleKey);

    if (iter != m_SoundNextPlayTimes.end() && m_fSoundTime < iter->second)
        return false;

    m_SoundNextPlayTimes[strRuleKey] = m_fSoundTime + fIntervalSec;
    return true;
}

void CSound_Manager::Play_SoundSequence(const SOUND_SEQUENCE_DESC& Desc)
{
    if (Desc.strSequenceKey.empty())
        return;

    if (Desc.strStartKey.empty() && Desc.strLoopKey.empty())
        return;

    SOUND_SEQUENCE_STATE_DESC& StateDesc = m_SoundSequences[Desc.strSequenceKey];

    if (StateDesc.eState == SOUND_SEQUENCE_STATE::START ||
        StateDesc.eState == SOUND_SEQUENCE_STATE::LOOP)
        return;

    if (StateDesc.pEndChannel != nullptr)
    {
        FMOD_Channel_Stop(StateDesc.pEndChannel);
        StateDesc.pEndChannel = nullptr;
    }

    StateDesc.Desc = Desc;
    StateDesc.eState = SOUND_SEQUENCE_STATE::NONE;
    StateDesc.bLoopStarted = false;
    StateDesc.fStartTimeSec = m_fSoundTime;

    if (!Desc.strStartKey.empty())
    {
        StateDesc.pStartChannel = Play(Desc.strStartKey, Desc.eGroup, Desc.fVolume, false, false);

        if (StateDesc.pStartChannel != nullptr)
        {
            StateDesc.eState = SOUND_SEQUENCE_STATE::START;
            return;
        }
    }

    if (!Desc.strLoopKey.empty())
    {
        StateDesc.pLoopChannel = Play(Desc.strLoopKey, Desc.eGroup, Desc.fVolume, true, false);

        if (StateDesc.pLoopChannel != nullptr)
        {
            StateDesc.eState = SOUND_SEQUENCE_STATE::LOOP;
            StateDesc.bLoopStarted = true;
        }
    }
}

void CSound_Manager::Stop_SoundSequence(const string& strSequenceKey)
{
    auto iter = m_SoundSequences.find(strSequenceKey);

    if (iter == m_SoundSequences.end())
        return;

    SOUND_SEQUENCE_STATE_DESC& StateDesc = iter->second;

    if (StateDesc.pStartChannel != nullptr)
    {
        FMOD_Channel_Stop(StateDesc.pStartChannel);
        StateDesc.pStartChannel = nullptr;
    }

    if (StateDesc.pLoopChannel != nullptr)
    {
        FMOD_Channel_Stop(StateDesc.pLoopChannel);
        StateDesc.pLoopChannel = nullptr;
    }

    if (StateDesc.Desc.strEndKey.empty())
    {
        Clear_SoundSequence(StateDesc);
        m_SoundSequences.erase(iter);
        return;
    }

    StateDesc.pEndChannel = Play(StateDesc.Desc.strEndKey, StateDesc.Desc.eGroup, StateDesc.Desc.fVolume, false, false);

    if (StateDesc.pEndChannel != nullptr)
        StateDesc.eState = SOUND_SEQUENCE_STATE::END;
    else
    {
        Clear_SoundSequence(StateDesc);
        m_SoundSequences.erase(iter);
    }
}

void CSound_Manager::Stop_AllSoundSequences()
{
    for (auto& Pair : m_SoundSequences)
        Clear_SoundSequence(Pair.second);

    m_SoundSequences.clear();

}

HRESULT CSound_Manager::Load_SoundFile()
{
    string strSoundDir = m_pGameInstance->Get_ResourcePathA() + "\\Sound\\";
    Load_SoundFolder(strSoundDir);
    return S_OK;
}

void CSound_Manager::Load_SoundFolder(const string& strSoundDir, _bool bStream2D)
{
    _finddata_t fd;
    intptr_t handle = _findfirst((strSoundDir + "*.*").c_str(), &fd);
    if (handle == -1)
        return;

    int iResult = 0;

    while (iResult != -1)
    {
        if (strcmp(fd.name, ".") == 0 || strcmp(fd.name, "..") == 0)
        {
            iResult = _findnext(handle, &fd);
            continue; // 다음 파일로
        }

        //하위 폴더면 재귀 호출
        if (fd.attrib & _A_SUBDIR)
        {
            // Sound\BGM 과 Sound\CutScene 은 위치 없는 2D 재생만 하므로 하위 폴더까지 스트리밍으로 내린다.
            const _bool bChildStream2D = bStream2D || 0 == strcmp(fd.name, "BGM") || 0 == strcmp(fd.name, "CutScene");

            Load_SoundFolder(strSoundDir + fd.name + "\\", bChildStream2D);
            iResult = _findnext(handle, &fd);
            continue;
        }

        string strFullPath = strSoundDir + fd.name;

        FMOD_SOUND* pSound = nullptr;
        FMOD_RESULT eRes;

        //2D 스트리밍 폴더거나 이름에 BGM이 들어가면 BGM으로 인식 후 루프로 셋
        if (true == bStream2D || strstr(fd.name, "BGM"))
        {
            eRes = FMOD_System_CreateSound(m_pSystem, strFullPath.c_str(), FMOD_CREATESTREAM | FMOD_LOOP_NORMAL | FMOD_2D, 0, &pSound);
        }
        else
        {
            eRes = FMOD_System_CreateSound(m_pSystem, strFullPath.c_str(), FMOD_3D | FMOD_3D_LINEARSQUAREROLLOFF, 0, &pSound);
            if (eRes == FMOD_OK)
                FMOD_Sound_Set3DMinMaxDistance(pSound, 1.f, 35.f);
        }

        if (eRes == FMOD_OK)
        {
            string soundKey(fd.name);
            //확장자 제거 
            size_t dotPos = soundKey.rfind(L'.');

            if (dotPos != string::npos)
            {
                soundKey = soundKey.substr(0, dotPos);
            }

            //중복키면 emplace실패 -> 새로 만든 사운드 해제 
            if (!m_Sounds.emplace(soundKey, pSound).second)
                FMOD_Sound_Release(pSound);
        }
        iResult = _findnext(handle, &fd);
    }

    _findclose(handle);
}


void CSound_Manager::Pause_Group(CHANNEL_ID eGroup, _bool bPause)
{
    if (m_pChannelGroup[eGroup])
        FMOD_ChannelGroup_SetPaused(m_pChannelGroup[eGroup], bPause);
}

void CSound_Manager::Pause_All(_bool bPause)
{
    Pause_Group(SOUND_MASTER, bPause);
}

void CSound_Manager::Set_GroupMute(CHANNEL_ID eGroup, _bool bMute)
{
    if (m_pChannelGroup[eGroup])
        FMOD_ChannelGroup_SetMute(m_pChannelGroup[eGroup], bMute);
}

void CSound_Manager::Set_MasterMute(_bool bMute)
{
    Set_GroupMute(SOUND_MASTER, bMute);
}

CSound_Manager* CSound_Manager::Create()
{
    CSound_Manager* pInstance = new CSound_Manager();
    return pInstance;
}

void CSound_Manager::Free()
{
    Stop_AllSound();

    // 테스트용
    m_SoundNextPlayTimes.clear();
    m_SoundCues.clear();

    for (auto& pair : m_Sounds)
    {
        if (pair.second)
            FMOD_Sound_Release(pair.second);
    }
    m_Sounds.clear();

    //마스터는 시스템이 소유. 나머지 서브 그룹만 Release
    for (int i = SOUND_MASTER + 1; i < SOUND_END; ++i)
    {
        if (m_pChannelGroup[i])
            FMOD_ChannelGroup_Release(m_pChannelGroup[i]);
    }

    if (m_pSystem)
    {
        FMOD_System_Close(m_pSystem);
        FMOD_System_Release(m_pSystem);
    }
}
