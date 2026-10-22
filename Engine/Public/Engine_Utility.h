#pragma once
#include <DirectXMath.h>
#include "Engine_Defines.h"

namespace Engine
{
	/* Math 관련*/
	constexpr float g_fEpsilon = 1e-5f;

	inline bool IsEqual(float a, float b)
	{
		return fabsf(a - b) <= g_fEpsilon;
	}

	inline bool IsEqual(float a, float b, float diff)
	{
		return fabsf(a - b) <= diff;
	}

	inline int	Random(int iMin, int iMax)
	{
		if (iMax <= iMin) return iMin;
		return iMin + rand() % (iMax - iMin + 1);
	}

	inline float Random(float fMin, float fMax)
    {
        return fMin + static_cast<float>(rand()) / RAND_MAX * (fMax - fMin);
    }

	inline float Random0To1()
    {
        return static_cast<float>(rand()) / RAND_MAX;
    }

	inline float fLerp(float fStart, float fEnd, float fPercent)
	{
		return fStart + (fEnd - fStart) * fPercent;
	}
	
	inline float LerpAngle(float fStart, float fEnd, float fPercent)
	{
		//XM_2PI 가 넘는 값들을 한번 보정 
		float fDelta = fmodf(fEnd - fStart, XM_2PI);
		//180도 넘으면 반대로 (최단경로) 
		if (fDelta > XM_PI) fDelta -= XM_2PI;
		else if (fDelta < -XM_PI) fDelta += XM_2PI;
		return fStart + fDelta * fPercent;
	}

	inline float fClamp(float fValue, float fMin, float fMax)
	{
		if (fValue < fMin) return fMin;
		if (fValue > fMax) return fMax;
		return fValue;
	}
	
	inline int iClamp(int iValue, int iMin, int iMax)
	{
		if (iValue < iMin) return iMin;
		if (iValue > iMax) return iMax;
		return iValue;
	}

	inline _float MoveTowards(_float fCurrent, _float fDest, _float fTolerance)
	{
		if (fabsf(fDest - fCurrent) <= fTolerance) return fDest;
		return fCurrent + (fDest > fCurrent ? fTolerance : -fTolerance);
	}

	//피직스의 오른손좌표계로 변환 (현재 미러링. 사용 X) 
	inline PxQuat ToPxQuat(const XMFLOAT4& vQuat)
	{
		return PxQuat(-vQuat.x, -vQuat.y, vQuat.z, -vQuat.w);
	}

	inline _float Ease(EASE_TYPE eType, _float t)
	{
		t = max(0.f, min(t, 1.f));

		switch (eType)
		{
		case EASE_TYPE::LINEAR: return t;
		case EASE_TYPE::EASE_IN: return t * t;
		case EASE_TYPE::OUT_QUAT:
			return 1.f - (1.f - t) * (1.f - t);
		case EASE_TYPE::OUT_CUBIC:
		{ 
			_float u = 1.f - t;
			return 1.f - u * u * u; 
		}
		case EASE_TYPE::OUT_EXPO:
		{
			return (t >= 1.f) ? 1.f : 1.f - powf(2.f, -10.f * t);
		}
		case EASE_TYPE::OUT_QUINT:
		{
			return 1.f - powf(1 - t, 5.f);
		}
		case EASE_TYPE::IN_OUT_CUBIC:
		{
			if (t < 0.5f)
			{
				return 4.f * t * t * t;
			}
			else
			{
				float f = -2.f * t + 2.f;
				return 1.f - (f * f * f) * 0.5f;
			}
		}
		default: break;
		}
		return t;
	}

	inline _float SmoothStep(_float fValue)
	{
		return  fValue * fValue * (3.f - 2.f * fValue);
	}


	/* 문자열 관련 */
	inline wstring StringToWString(const string str)
	{
		if (str.empty()) return wstring();

		//필요한 크기 계산 
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
		std::wstring wstrTo(size_needed, 0);

		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);

		return wstrTo;
	}

	inline string WStringToString(const wstring& wstr)
	{
		if (wstr.empty()) return string();

		//필요한 크기 계산 
		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(size_needed, 0);

		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);

		return strTo;
	}


	inline wstring CharArrayToWstring(const char* charArray)
	{
		if (charArray == nullptr) return L"";

		int len = static_cast<int>(strlen(charArray));
		if (len == 0) return L"";

		int size_needed = MultiByteToWideChar(CP_UTF8, 0, charArray, len, NULL, 0);

		std::wstring wstrTo(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, charArray, len, &wstrTo[0], size_needed);

		return wstrTo;
	}

	inline unsigned int Combine_ID(unsigned int a, unsigned int b)
	{
		return ((a<<16) & 0xffff0000) | b;
	}

	inline wstring Get_FileNameFromPath(wstring& strFilePath)
	{
		wchar_t szFileName[MAX_PATH] = {};
		_wsplitpath_s(strFilePath.c_str(), nullptr, 0, nullptr, 0, szFileName, MAX_PATH, nullptr, 0);
		return _wstring(szFileName);
	}

	constexpr uint64_t FNV_OFFSET_BASIS_64 = 0xcbf29ce484222325ull;
	constexpr uint64_t FNV_PRIME_64 = 0x00000100000001b3ull;

	inline constexpr uint64_t Hash_String(const wchar_t* str)
    {
		uint64_t hash = FNV_OFFSET_BASIS_64;
        while (*str)
        {
            hash ^= static_cast<uint64_t>(*str++);
            hash *= FNV_PRIME_64;
        }
        return hash;
    }

	//각 문자값을 XOR하는거라 wstring 해시변환과 동일한 결과임
	inline constexpr uint64_t Hash_StringA(const char* str)
	{
		uint64_t hash = FNV_OFFSET_BASIS_64;
		while (*str)
		{
			hash ^= static_cast<uint64_t>(static_cast<unsigned char>(*str++));
			hash *= FNV_PRIME_64;
		}
		return hash;
	}

	inline XMFLOAT4 Convertfloat3To4(XMFLOAT3 fValue)
	{
		_vector v = XMVectorSetW(XMLoadFloat3(&fValue), 1.0f);
		XMFLOAT4 ret = {};
		XMStoreFloat4(&ret, v);
		return ret; 
	}

	inline TEXTURE_TYPE MapKeyToTextureType(const string& Key, const string& FileNameOrPath = string())
	{
		auto ToLower = [](string s) { transform(s.begin(), s.end(), s.begin(), ::tolower); return s; };

		string k = ToLower(Key);
		if (k.find("iris") != string::npos) return TEXTURE_TYPE::MASK1;
		if (k.find("sclera") != string::npos) return TEXTURE_TYPE::BASE_COLOR;

		string file = FileNameOrPath;
		{
			size_t slash = file.find_last_of("/\\");
			if (slash != string::npos) file = file.substr(slash + 1);
			size_t dot = file.find_last_of('.');
			if (dot != string::npos) file = file.substr(dot + 1).empty() ? file.substr(0, dot) : file.substr(dot + 1);
		}
		string f = ToLower(file);

		auto EndsWith = [](const string& s, const char* suf) {
			size_t n = strlen(suf);
			return s.size() >= n && s.compare(s.size() - n, n, suf) == 0;
			};
		auto Has = [&](const char* t) { return f.find(t) != string::npos; };

		if (!f.empty())
		{
			if (k.find("moss") != string::npos &&
				(EndsWith(f, "_n") || EndsWith(f, "_n1") || EndsWith(f, "_n2") || EndsWith(f, "_n3")))
				return TEXTURE_TYPE::DETAIL_NORMAL;

			if (EndsWith(f, "_dn"))   return TEXTURE_TYPE::DETAIL_NORMAL;
			if (EndsWith(f, "_bc5"))  return TEXTURE_TYPE::DETAIL_NORMAL;
			if (EndsWith(f, "_bn"))   return TEXTURE_TYPE::BENT_NORMAL;
			if (EndsWith(f, "_n") || EndsWith(f, "_n1") || EndsWith(f, "_n2") || EndsWith(f, "_n3"))
				return TEXTURE_TYPE::NORMAL;

			if (EndsWith(f, "_smh") || EndsWith(f, "_stc") || EndsWith(f, "_dou"))
				return TEXTURE_TYPE::MASK2;
			if (EndsWith(f, "_arm") || EndsWith(f, "_arm2") || EndsWith(f, "_armk") ||
				EndsWith(f, "_armt") || EndsWith(f, "_armh") || EndsWith(f, "_arh") ||
				EndsWith(f, "_arh1") || EndsWith(f, "_ard") || EndsWith(f, "_asr") ||
				EndsWith(f, "_dr") || EndsWith(f, "_orh"))
				return TEXTURE_TYPE::MASK1;

			if (EndsWith(f, "_bc") || EndsWith(f, "_bc1") || EndsWith(f, "_bg") ||
				EndsWith(f, "_bck") || EndsWith(f, "_d") || EndsWith(f, "_d2"))
				return TEXTURE_TYPE::BASE_COLOR;

			if (EndsWith(f, "_e"))
				return TEXTURE_TYPE::EMISSIVE;
			if (Has("hdri") || Has("cubemap"))
				return TEXTURE_TYPE::CUBE_MAP;

			if (Has("sclera")) return TEXTURE_TYPE::BASE_COLOR;
			if (Has("iris"))   return TEXTURE_TYPE::MASK1;
			if (Has("eyelash"))
				return TEXTURE_TYPE::MASK1;

			if (EndsWith(f, "_snm") || EndsWith(f, "_pdo") || Has("snownoise"))
				return TEXTURE_TYPE::END;

			if (EndsWith(f, "_m"))    return TEXTURE_TYPE::MASK1;
		}

		if (k.find("basecolor") != string::npos || k.find("diffuse") != string::npos) return TEXTURE_TYPE::BASE_COLOR;
		if (k.find("detailnormal") != string::npos || k.find("detail_normal") != string::npos) return TEXTURE_TYPE::DETAIL_NORMAL;
		if (k.find("bent_normal") != string::npos) return TEXTURE_TYPE::BENT_NORMAL;
		if (k.find("normal") != string::npos) return TEXTURE_TYPE::NORMAL;
		if (k.find("mask2") != string::npos || k.find("smh") != string::npos || k.find("stc") != string::npos) return TEXTURE_TYPE::MASK2;
		if (k.find("mask") != string::npos || k.find("arm") != string::npos || k.find("specularmasks") != string::npos) return TEXTURE_TYPE::MASK1;
		if (k.find("emissive") != string::npos) return TEXTURE_TYPE::EMISSIVE;
		if (k.find("cubemap") != string::npos) return TEXTURE_TYPE::CUBE_MAP;
		return TEXTURE_TYPE::END;
	}


#pragma region PARSER
	//문자열 앞뒤 공백 제거
	inline bool Trim(_wstring& InOutStr)
	{
		auto firstStr = InOutStr.find_first_not_of(L" \t\r\n");
		if (firstStr == _wstring::npos) return false;
		auto lastStr = InOutStr.find_last_not_of(L" \t\r\n");
		InOutStr = InOutStr.substr(firstStr, lastStr - firstStr + 1);
		return true;
	}

	//','으로 잘라서 토큰 배열 얻기
	inline void Split_CSV_Line(const _wstring& Line, vector<_wstring>& Tokens)
	{
		const wchar_t cDelimiter = L',';
		size_t iStartPos = 0;
		size_t iEndPos = Line.find(cDelimiter);
		_wstring token = { };

		while (iEndPos != _wstring::npos)
		{
			token = Line.substr(iStartPos, iEndPos - iStartPos);

			if (Trim(token)) Tokens.push_back(token); // 공백만 있는경우
			else Tokens.push_back(L" ");

			iStartPos = iEndPos + 1;
			iEndPos = Line.find(cDelimiter, iStartPos);
		}

		//마지막 토큰 읽기 
		token = Line.substr(iStartPos);

		if (Trim(token)) Tokens.push_back(token);
		else Tokens.push_back(L" ");
	}

#pragma endregion



#pragma region EASING

	inline float EaseOutExpo(float t)
	{
		return float(t == 1.f ? 1.f : 1.f - pow(2.f, -10.f * t));
	}	

	inline float EaseOutCubic(float t)
	{
		return float(1.0f - float(pow(1.0f - t, 3.f)));
	}	

#pragma endregion
#ifdef _DEBUG
	inline void CreateDebugConsole()
	{
		AllocConsole();

		FILE* fp;

		freopen_s(&fp, "CONOUT$", "w", stdout);
		freopen_s(&fp, "CONOUT$", "w", stderr);
		freopen_s(&fp, "CONIN$", "r", stdin);

		cout.clear();
		cerr.clear();
		cin.clear();

		cout << "Debug Console Created\n";
	}

	inline void ConsoleLogFormat(const char* fmt, ...)
	{
		char buffer[1024] = {};

		va_list args;
		va_start(args, fmt);
		vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, fmt, args);
		va_end(args);

		cout << buffer << endl;
	}

	inline void DebugOutput(const char* text)
	{
		OutputDebugStringA(text);
	}

#endif

#pragma region Make_Matrix
	inline _matrix Build_LightViewProj(_fvector vEye, _fvector vDir, _fvector vUp,_matrix matProj)
	{
		_matrix matView = XMMatrixLookToLH(vEye, vDir, vUp);
		return XMMatrixTranspose(matView * matProj);
	}

	inline _matrix Make_BoneSocketWorld(const _float4x4* pCombinedMatrix, const _float4x4* pWorldMatrix)
	{
		_matrix SocketMatrix = XMLoadFloat4x4(pCombinedMatrix);
		_vector vScale, vRot, vTrans;

		//스케일 제외 
		if(XMMatrixDecompose(&vScale, &vRot, &vTrans, SocketMatrix))
		{
			SocketMatrix = XMMatrixAffineTransformation(XMVectorSet(1.f, 1.f, 1.f,0.f), XMVectorZero(), vRot, vTrans);
		}

		return SocketMatrix * XMLoadFloat4x4(pWorldMatrix);
	}

#pragma endregion


}