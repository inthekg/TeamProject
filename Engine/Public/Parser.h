#pragma once
#include "Engine_Defines.h"
#include "Engine_Utility.h"

template<typename T>
class CSVParser
{
public:
	using RowMapper = function <T(const vector<_wstring>&)>;

	//RowMapper : 한 행의 각 토큰을 어떻게 매핑할지 정의하는 객체 
	static bool CSV_Parse(vector<T>& OutResult, const _wstring& strFilePath, RowMapper Mapper )
	{
		wifstream fin(strFilePath);
		if (!fin.is_open())
		{
			MSG_BOX("Failed to open CSV file");
			return false;
		}
		_wstring strline;
		getline(fin, strline); // CSV 헤더 읽기 
		while (getline(fin, strline))
		{
			vector<_wstring> tokens;
			Split_CSV_Line(strline, tokens);
			OutResult.push_back(Mapper(tokens));
		}
		return true;
	}
};