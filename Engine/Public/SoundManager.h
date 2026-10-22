#pragma once
#include "Base.h"
#include <io.h>

struct FMOD_SOUND;
struct FMOD_CHANNEL;
struct FMOD_SYSTEM;
struct FMOD_CHANNELGROUP;

NS_BEGIN(Engine)

class CSound_Manager : public CBase
{
public:
    enum class SOUND_SEQUENCE_STATE
    {
        NONE,
        START,
        LOOP,
        END
    };

    struct SOUND_SEQUENCE_DESC
    {
        string strSequenceKey;

        string strStartKey;
        string strLoopKey;
        string strEndKey;

        CHANNEL_ID eGroup = SOUND_SFX;
        _float fVolume = 1.f;

        _float      fLoopStartDelaySec = -1.f;
        _float      fLoopOverlapSec = 0.08f;
    };

    struct SOUND_SOURCE
    {
        FMOD_CHANNEL* pChannel = { nullptr };
        const _float4x4* pFollowMat = { nullptr };
        _float3 vLastPos = {};
        _bool bDoppler = { false };
    };

    struct SOUND_SEQUENCE_STATE_DESC
    {
        SOUND_SEQUENCE_DESC Desc;

        SOUND_SEQUENCE_STATE eState = SOUND_SEQUENCE_STATE::NONE;

        FMOD_CHANNEL* pStartChannel = { nullptr };
        FMOD_CHANNEL* pLoopChannel = { nullptr };
        FMOD_CHANNEL* pEndChannel = { nullptr };

        _float  fStartTimeSec = 0.f;
        _bool   bLoopStarted = false;
    };

    struct SOUND_CUE_DESC
    {
        string          strName;

        vector<string>  Waves;
        vector<_float>  Weights;

        _float          fPitchMin = { 1.f };
        _float          fPitchMax = { 1.f };
        _float          fVolumeMin = { 1.f };
        _float          fVolumeMax = { 1.f };

        _float          fVolumeMultiplier = { 1.f };
        _float          fMaxDistance = { 3500.f };
    };

private:
    CSound_Manager();
    virtual ~CSound_Manager() = default;

public:
    void                            Ready_Sound();
    void                            Update_Sound(_float fTimeDelta);

public:
    void                            Play_Sound(const string& strSoundKey, CHANNEL_ID eGroup, _float fVolume = 1.0f, _bool bLoop = false, _bool bTrack = false);
    void                            Play_SFXSound(const string& strSoundKey, _float fVolume = 1.0f, _bool bLoop = false, _bool bTrack = false);
    void                            Play_BGM(const string& strSoundKey, _float fVolume = 1.0f);
    void                            Play_BGM(const string& strSoundKey, _float fVolume, _float fFadeSec);
    void                            Play_BGM_Sequence(const string& strIntroKey, const string& strLoopKey, _float fVolume);
    void                            Play_MonsterSound(const string& strSoundKey, _float fVolume = 1.0f, _bool bLoop = false, _bool bTrack = false);
    void                            Play_PlayerSound(const string& strSoundKey, _float fVolume = 1.0f, _bool bLoop = false, _bool bTrack = false, _float fPitch = 1.f);
    void                            Play_WeaponSound(const string& strSoundKey, _float fVolume = 1.0f, _bool bLoop = false, _bool bTrack = false);
    void                            Play_Ambience(const string& strSoundKey, _float fVolume, _float fFadeSec);
    void                            Stop_Ambience();

    _uint                           Play_SoundAt(const string& strSoundKey, CHANNEL_ID eGroup, const _float3& vWorldPos, _float fVolume = 1.f, _bool bLoop = false);
    _uint                           Play_SoundFollow(const string& strSoundKey, CHANNEL_ID eGroup, const _float4x4* pFollowMat, _float fVolume = 1.f, _bool bLoop = false, _bool bDoppler = true);
    void                            Stop_SoundSource(_uint hHandle);
    _bool                           Is_SoundSourcePlaying(_uint hHandle);
    void                            Update_SoundSourcePos(_uint hHandle, const _float3& vWorldPos);

    _uint                           Play_Cue(const string& strCueKey, CHANNEL_ID eGroup, const _float3& vWorldPos, _float fVolumeScale = 1.f, _bool bLoop = false);
    _bool                           Has_Cue(const string& strCueKey) const;

    void                            StopSound_ByKey(const string& strSoundKey);
    void                            SetVolume_ByKey(const string& strSoundKey, _float fVolume);
    _bool                           IsPlaying_ByKey(const string& strSoundKey);

    void                            FadeOut_BGM(_float fFadeSec);
    void                            Stop_GroupSound(CHANNEL_ID eID);
    void                            Stop_AllSound();
    void                            Stop_BGM();

    void                            Set_GroupVolume(CHANNEL_ID eID, _float fVolume);
    void                            Pause_Group(CHANNEL_ID eGroup, _bool bPause);
    void                            Pause_All(_bool bPause);
    void                            Set_GroupMute(CHANNEL_ID eGroup, _bool bMute);
    void                            Set_MasterMute(_bool bMute);

    _bool                           Can_PlaySoundByInterval(const string& strRuleKey, _float fIntervalSec);
    void                            Play_SoundSequence(const SOUND_SEQUENCE_DESC& Desc);
    void                            Stop_SoundSequence(const string& strSequenceKey);
    void                            Stop_AllSoundSequences();

private:
    HRESULT                         Load_SoundFile();
    void                            Load_SoundFolder(const string& strSoundDir, _bool bStream2D = false);
    FMOD_CHANNEL* Play(const string& pSoundKey, CHANNEL_ID eGroup, _float fVolume, _bool bLoop, _bool bTrack, _float fPitch = 1.f);
    FMOD_CHANNEL* Play_Internal(const string& strSoundKey, CHANNEL_ID eGroup, _float fVolume, _bool bLoop, const _float3* pWorldPos, _float fPitch = 1.f);
    void                            Update_Listener(_float fTimeDelta);
    void                            Update_SoundSources(_float fTimeDelta);

    HRESULT                         Load_SoundCueTables();
    void                            Load_SoundCueFolder(const string& strCueDir);
    void                            Load_SoundCueFile(const string& strFilePath);

    void                            Update_SoundSequences();
    void                            Update_BGMFade(_float fTimeDelta);
    void                            Update_AmbienceFade(_float fTimeDelta);
    void                            Clear_PrevAmbience();
    void                            Clear_PrevBGM();
    void                            Clear_SoundSequence(SOUND_SEQUENCE_STATE_DESC& StateDesc);
    _float                          Get_SoundLengthSec(const string& strSoundKey) const;

private:
    class CGameInstance* m_pGameInstance = { nullptr };
    FMOD_SYSTEM* m_pSystem = { nullptr };

    _float3                         m_vListenerPrevPos = {};
    _bool                           m_bListenerReady = { false };

    _float                          m_fSoundTime = { 0.f };

    map<_uint, SOUND_SOURCE>        m_SoundSources;
    _uint                           m_iNextSourceID = { 1 };

    map<string, FMOD_SOUND*>        m_Sounds;
    map<string, FMOD_CHANNEL*>      m_PlayingSounds;
    FMOD_CHANNELGROUP* m_pChannelGroup[SOUND_END] = { nullptr };

    FMOD_CHANNEL* m_pCurBGM = { nullptr };
    string                          m_strBGMKey = {};

    FMOD_CHANNEL* m_pPrevBGM = { nullptr };
    string                          m_strPrevBGMKey = {};

    _float                          m_fBGMFadeDuration = { 0.f };
    _float                          m_fBGMFadeTimer = { 0.f };
    _float                          m_fBGMTargetVolume = { 1.f };
    _float                          m_fBGMPrevVolume = { 1.f };

    FMOD_CHANNEL* m_pCurAmbience = { nullptr };
    string                          m_strAmbienceKey = {};

    FMOD_CHANNEL* m_pPrevAmbience = { nullptr };
    string                          m_strPrevAmbienceKey = {};

    _float                          m_fAmbienceFadeDuration = { 0.f };
    _float                          m_fAmbienceFadeTimer = { 0.f };
    _float                          m_fAmbienceTargetVolume = { 1.f };
    _float                          m_fAmbiencePrevVolume = { 1.f };

    map<string, _double>            m_SoundNextPlayTimes;
    map<string, SOUND_SEQUENCE_STATE_DESC> m_SoundSequences;

    map<string, SOUND_CUE_DESC>     m_SoundCues;

private:
    static constexpr _float         CUE_VOLUME_CLAMP = { 2.5f };
    static constexpr _float         CUE_MIN_DISTANCE = { 3.f };
    static constexpr _float         CUE_MAX_DISTANCE_LIMIT = { 300.f };
    static constexpr _float         CUE_RETRIGGER_INTERVAL = { 0.08f };
    static constexpr _float         CUE_RETRIGGER_INTERVAL_VOICE = { 0.3f };
    static constexpr _float         MONSTER_GROUP_VOLUME = { 0.5f };

public:
    static CSound_Manager* Create();
    virtual void           Free();
};

NS_END
