#pragma once
#include "Engine_Defines.h"

/* TODO : 특별한 디폴트 값이 필요한 경우 추가하기 */

_float Get_DefaultScalarProperty(const string& strValueName)
{
	if (strValueName == "Roughness") return 1.f;
	else if (strValueName == "Specular") return 0.5f;
	return 0.f;
}


_float4 Get_DefaultColorProperty(const string& strValueName)
{
	return { 0.f, 0.f, 0.f, 0.f };
}