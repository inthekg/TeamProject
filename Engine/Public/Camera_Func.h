#pragma once
#include "Engine_Defines.h"

using ejson = nlohmann::ordered_json;

namespace Engine
{
	constexpr _float UE_TO_METER = 0.01f; // Unreal cm -> m
	constexpr _float DEG_2RAD = XM_PI / 180.f;

	string Extract_UEName(const string& strName)
	{
		size_t FirstPos = strName.find('\''), LastPos = strName.rfind('\'');
		if (FirstPos == string::npos || LastPos == string::npos || LastPos <= FirstPos) return "";
		return strName.substr(FirstPos + 1, LastPos - FirstPos -1);
	}

	static void Read_OscObj(const ejson& n, FOscillatorDesc& OutDesc, _float ampScale)
	{
		if (n.contains("Amplitude")) OutDesc.fAmplitude = n["Amplitude"].get<_float>() * ampScale;
		if (n.contains("Frequency")) OutDesc.fFrequency = n["Frequency"].get<_float>();
		if (n.contains("Waveform") && n["Waveform"].is_number()) OutDesc.iWaveform = n["Waveform"].get<_int>();
		if (n.contains("InitialOffset") && n["InitialOffset"].is_number()) OutDesc.iInitialOffset = n["InitialOffset"].get<_int>();
	}
	
	//UE의 단위를 DX로 변환. fValueScale 변환에 필요한 값 
	static void Parse_Channel(const ejson& sec, const char* szKey, _int iTickRes, _float fValueScale, FCurveChannel& OutChannel)
	{
		auto iter = sec.find(szKey);
		if (iter == sec.end()) return;
		const ejson& ch = *iter;

		OutChannel.fDefaultValue = ch.value("DefaultValue", 0.f) * fValueScale;
		if (!ch.contains("Times") || !ch.contains("Values")) return;

		const ejson& times = ch["Times"];
		const ejson& values = ch["Values"];
		
		size_t count = times.size() < values.size() ? times.size() : values.size();
		
		OutChannel.Keys.reserve(count);
		for (size_t i = 0; i < count; i++)
		{
			FCurveKey key; 
			_int tick = times[i].value("Value", 0);
			key.fTime = iTickRes ? (_float)tick / (_float)iTickRes : 0.f;

			key.fValue = values[i].value("Value", 0.f) * fValueScale;
			if (values[i].contains("Tangent"))
			{
				const ejson& tg = values[i]["Tangent"];
				key.fArriveTangent = tg.value("ArriveTangent", 0.f);
				key.fLeaveTangent = tg.value("LeaveTangent", 0.f);
			}
			key.Interp = (CURVE_INTERP)values[i].value("InterpMode", 2);
			key.Tangent = (CURVE_TANGENT)values[i].value("TangentMode", 0);
			OutChannel.Keys.push_back(key);
		}

	}

	static _bool Parse_Shake(const ejson& json, SHAKE_DESC& OutDesc)
	{
		const ejson* pJson = nullptr;
		for (const ejson& e : json)
		{
			if (e.contains("Properties"))
			{
				const ejson& P = e["Properties"];
				if (P.contains("OscillationDuration") || P.contains("RotOscillation"))
				{
					pJson = &P;
					break;
				}
			}
		}

		if (pJson == nullptr) return false;
		const ejson& Json = *pJson;

		OutDesc.fOscillationDuration = Json.value("OscillationDuration", 0.f);
		OutDesc.fOscillationBlendInTime = Json.value("OscillationBlendInTime", 0.f);
		OutDesc.fOscillationBlendOutTime = Json.value("OscillationBlendOutTime", 0.f);
		OutDesc.bSingleInstance = Json.value("bSingleInstance", false);

		if (Json.contains("RotOscillation"))
		{
			const ejson& r = Json["RotOscillation"];
			if (r.contains("Pitch")) Read_OscObj(r["Pitch"], OutDesc.RotPitch, DEG_2RAD);
			if (r.contains("Yaw"))   Read_OscObj(r["Yaw"], OutDesc.RotYaw, DEG_2RAD);
			if (r.contains("Roll"))  Read_OscObj(r["Roll"], OutDesc.RotRoll, DEG_2RAD);
		}
		if (Json.contains("LocOscillation"))
		{
			const ejson& l = Json["LocOscillation"];
			if (l.contains("X")) Read_OscObj(l["X"], OutDesc.LocX, UE_TO_METER);
			if (l.contains("Y")) Read_OscObj(l["Y"], OutDesc.LocY, UE_TO_METER);
			if (l.contains("Z")) Read_OscObj(l["Z"], OutDesc.LocZ, UE_TO_METER);
		}
		if (Json.contains("FOVOscillation")) Read_OscObj(Json["FOVOscillation"], OutDesc.FOV, 1.f);

		OutDesc.fAnimPlayRate = Json.value("AnimPlayRate", 1.f);
		OutDesc.fAnimBlendInTime = Json.value("AnimBlendInTime", 0.f);
		OutDesc.fAnimBlendOutTime = Json.value("AnimBlendOutTime", 0.f);

		if (Json.contains("AnimSequence"))
			OutDesc.AnimSequenceKey = Extract_UEName(Json["AnimSequence"].value("ObjectName", string()));
		return true;
	}

	//언리얼 CameraAnim은 초기 트랜스폼 대비 Delta만 가산함
	// 따라서 모든 키를 첫번째 키값을 기준으로 재배치함
	static void Rebase_Channel(FCurveChannel& ch)
	{
		if (ch.Keys.empty()) { ch.fDefaultValue = 0.f; return; }

		const _float fBase = ch.Keys.front().fValue;
		for (FCurveKey& k : ch.Keys)
			k.fValue -= fBase;
		ch.fDefaultValue -= fBase;
	}

	static _bool Parse_Scene(const ejson& json, SCENE_DESC& OutDesc)
	{
		const ejson* pCam = nullptr, * pScene = nullptr, * pSec = nullptr;

		for (const ejson& e : json)
		{
			string strType = e.value("Type", string());

			if (strType == "CineCameraComponent" && !pCam)   pCam = &e;
			else if (strType == "MovieScene" && !pScene) pScene = &e;
			else if (strType == "MovieScene3DTransformSection" && !pSec)   pSec = &e;
		}
		if (!pSec) return false;

		if (pCam && pCam->contains("Properties"))
		{
			const ejson& P = (*pCam)["Properties"];
			OutDesc.fFieldOfView = P.value("FieldOfView", 0.f) * DEG_2RAD;
			OutDesc.fAspectRatio = P.value("AspectRatio", 0.f);
		}


		_int tickRes = 24000;
		if (pScene && pScene->contains("Properties"))
		{
			const ejson& P = (*pScene)["Properties"];
			if (P.contains("TickResolution")) tickRes = P["TickResolution"].value("Numerator", 24000);
			if (P.contains("DisplayRate"))    OutDesc.iDisplayRate = P["DisplayRate"].value("Numerator", 30);
			OutDesc.iTickResolution = tickRes;

			_int upper = 0;
			if (P.contains("PlaybackRange"))
			{
				const ejson& pr = P["PlaybackRange"];
				if (pr.contains("Value") && pr["Value"].contains("UpperBound"))
				{
					const ejson& ub = pr["Value"]["UpperBound"];
					if (ub.contains("Value")) upper = ub["Value"].value("Value", 0);
				}
			}
			OutDesc.fDuration = tickRes ? (_float)upper / (_float)tickRes : 0.f;
		}

		const ejson& sec = (*pSec)["Properties"];
		//축 재매핑 - UE는 Z,X,Y -> DX는 X,Y,Z 
		Parse_Channel(sec, "Translation[1]", tickRes, UE_TO_METER, OutDesc.LocX);
		Parse_Channel(sec, "Translation[2]", tickRes, UE_TO_METER, OutDesc.LocY);
		Parse_Channel(sec, "Translation", tickRes, UE_TO_METER, OutDesc.LocZ);

		Parse_Channel(sec, "Rotation[1]", tickRes, DEG_2RAD, OutDesc.RotX);
		Parse_Channel(sec, "Rotation[2]", tickRes, DEG_2RAD, OutDesc.RotY);
		Parse_Channel(sec, "Rotation", tickRes, DEG_2RAD, OutDesc.RotZ);

		Parse_Channel(sec, "Scale[1]", tickRes, 1.f, OutDesc.ScaleX);
		Parse_Channel(sec, "Scale[2]", tickRes, 1.f, OutDesc.ScaleY);
		Parse_Channel(sec, "Scale", tickRes, 1.f, OutDesc.ScaleZ);

		//Rebase_Channel(OutDesc.LocX); Rebase_Channel(OutDesc.LocY); Rebase_Channel(OutDesc.LocZ);
		//Rebase_Channel(OutDesc.RotX); Rebase_Channel(OutDesc.RotY); Rebase_Channel(OutDesc.RotZ);

		return true;
	}


	template<class T, class ParseFunc>
	void Load_JsonFolder(const string& strFolderPath, unordered_map<string, T>& outMap, ParseFunc parse)
	{
		outMap.clear();

		string strSearch = strFolderPath + "\\*.json";
		WIN32_FIND_DATAA fd{};
		HANDLE hFind = FindFirstFileA(strSearch.c_str(), &fd);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			MSG_BOX("Cam JSON Load Failed");
			return;
		}

		do
		{
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			char szName[MAX_PATH] = {};
			_splitpath_s(fd.cFileName, nullptr, 0, nullptr, 0, szName, MAX_PATH, nullptr, 0);
			
			string strFullPath = strFolderPath + "\\" + fd.cFileName;

			try
			{
				std::ifstream fin(strFullPath.c_str());
				if (!fin.is_open()) continue;
				ejson j; fin >> j;

				T desc;
				if (parse(j, desc))
					outMap.emplace(string(szName), std::move(desc));
			}
			catch (const exception&)
			{
				MSG_BOX("Cam Data Parse Failed");
			}
		} while (FindNextFileA(hFind, &fd));
		
		FindClose(hFind);
	}


	static _float Auto_Slope(const vector<FCurveKey>& Keys, size_t i)
	{
		if (Keys.size() < 2) return 0.f;
		if (i == 0)
		{
			_float dt = Keys[1].fTime - Keys[0].fTime;
			return dt > 0.f ? (Keys[1].fValue - Keys[0].fValue) / dt : 0.f;
		}
		if (i == Keys.size() - 1)
		{
			_float dt = Keys[i].fTime - Keys[i - 1].fTime;
			return dt > 0.f ? (Keys[i].fValue - Keys[i - 1].fValue) / dt : 0.f;
		}
		_float dt = Keys[i + 1].fTime - Keys[i - 1].fTime;
		return dt > 0.f ? (Keys[i + 1].fValue - Keys[i - 1].fValue) / dt : 0.f;
	}

	static _float Sample_Channel(const FCurveChannel& ch, _float fTime)
	{
		const vector<FCurveKey>& K = ch.Keys;
		if (K.empty())            return ch.fDefaultValue;
		if (fTime <= K.front().fTime) return K.front().fValue;
		if (fTime >= K.back().fTime)  return K.back().fValue;

		size_t i = 0;
		while (i + 1 < K.size() && fTime >= K[i + 1].fTime) ++i;

		const FCurveKey& k0 = K[i];
		const FCurveKey& k1 = K[i + 1];
		_float fDeltaTime = k1.fTime - k0.fTime;
		if (fDeltaTime <= 0.f) return k0.fValue;
		_float u = (fTime - k0.fTime) / fDeltaTime;

		switch (k0.Interp)
		{
			case CURVE_INTERP::CONSTANT: return k0.fValue;
			case CURVE_INTERP::LINEAR:   return k0.fValue + (k1.fValue - k0.fValue) * u;
			case CURVE_INTERP::CUBIC:
			default:
			{
				_float m0 = (k0.Tangent == CURVE_TANGENT::AUTO) ? Auto_Slope(K, i) : k0.fLeaveTangent;
				_float m1 = (k1.Tangent == CURVE_TANGENT::AUTO) ? Auto_Slope(K, i + 1) : k1.fArriveTangent;

				_float u2 = u * u, u3 = u2 * u;
				_float h00 = 2.f * u3 - 3.f * u2 + 1.f;
				_float h10 = u3 - 2.f * u2 + u;
				_float h01 = -2.f * u3 + 3.f * u2;
				_float h11 = u3 - u2;
				return h00 * k0.fValue + h10 * (m0 * fDeltaTime) + h01 * k1.fValue + h11 * (m1 * fDeltaTime);
			}
		}
	}

	static void Sample_Scene(const SCENE_DESC& desc, _float fTime, _float3& OutLocalPos, _float3& OutEulerRad, _float& OutFovy)
	{
		OutLocalPos = { Sample_Channel(desc.LocX, fTime), Sample_Channel(desc.LocY, fTime), Sample_Channel(desc.LocZ, fTime) };
		OutEulerRad = { Sample_Channel(desc.RotX, fTime), Sample_Channel(desc.RotY, fTime), Sample_Channel(desc.RotZ, fTime) };
		OutFovy = desc.fFieldOfView;
	}

	static _float Shake_BlendWeight(const SHAKE_DESC& desc, _float fTime)
	{
		_float fWeight = 1.f;

		if(desc.fOscillationBlendInTime > 0.f && fTime < desc.fOscillationBlendInTime)
			fWeight *= fTime / desc.fOscillationBlendInTime;

		if (desc.fOscillationDuration > 0.f && desc.fOscillationBlendOutTime > 0.f)
		{
			_float fTimeOut = desc.fOscillationDuration - desc.fOscillationBlendOutTime;
			if (fTime > fTimeOut)
			{
				_float fWeightOut = (desc.fOscillationDuration - fTime) / desc.fOscillationBlendOutTime;
				if (fWeightOut < fWeight) fWeight = fWeightOut;
			}
		}
		return fWeight < 0.f ? 0.f : fWeight;
	}

	static void Parse_WorkKeys(const ejson& arr, FCurveChannel& Out)
	{
		if (!arr.is_array()) return;

		Out.Keys.reserve(arr.size());
		for (const ejson& k : arr)
		{
			FCurveKey key;
			key.fTime = k.value("Time", 0.f);
			key.fValue = k.value("Value", 0.f);
			key.fArriveTangent = k.value("Arrive", 0.f);
			key.fLeaveTangent = k.value("Leave", 0.f);

			// 보간 방식: 문자열 → enum. 없으면 Cubic 기본.
			string interp = k.value("Interp", string("Cubic"));

			if (interp == "Linear") key.Interp = CURVE_INTERP::LINEAR;
			else if(interp == "Constant") key.Interp = CURVE_INTERP::CONSTANT;
			else key.Interp = CURVE_INTERP::CUBIC;

			key.Tangent = k.value("Auto", true) ? CURVE_TANGENT::AUTO : CURVE_TANGENT::USER;

			Out.Keys.push_back(key);
		}
	}

	static _bool Parse_CamWork(const ejson& json, CAMWORK_DESC& Out)
	{
		const ejson& J = json.is_array() ? json.front() : json;

		//스칼라데이터
		Out.fDuration = J.value("Duration", 1.f);
		Out.fDistanceRate = J.value("DistanceRate", 1.f);
		Out.fSideAmount = J.value("SideAmount", 0.f);  
		Out.fInterpSpeed = J.value("InterpSpeed", 0.f);
		Out.fBlendInTime = J.value("BlendIn", 0.1f);
		Out.fBlendOutTime = J.value("BlendOut", 0.3f);
		Out.bUseSide = J.value("UseSide", false);
		Out.bUseStartDistance = J.value("UseStartDistance", false);
		Out.bWorldFixed = J.value("WorldFixed", false);

		//커브데이터
		if (J.contains("OffsetX")) Parse_WorkKeys(J["OffsetX"], Out.OffsetX);
		if (J.contains("OffsetY")) Parse_WorkKeys(J["OffsetY"], Out.OffsetY);
		if (J.contains("OffsetZ")) Parse_WorkKeys(J["OffsetZ"], Out.OffsetZ);
		if (J.contains("FovOffset")) Parse_WorkKeys(J["FovOffset"], Out.FovOffset);
		if (J.contains("BoneAlpha")) Parse_WorkKeys(J["BoneAlpha"], Out.BoneAlpha);

		return true;
	}

}