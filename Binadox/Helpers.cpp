#include "Helpers.h"
#include <windows.h>
#include <sstream>
#include <locale>
#include <codecvt>
#include <iterator>
#include <random>


long GetTabID()
{
	std::random_device rd;     // only used once to initialise (seed) engine
	std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
	std::uniform_int_distribution<int> uni(0, 1000); // guaranteed unbiased

	return uni(rng);
}

long GetVolumeHash()
{
	DWORD serialNum = 0;

	// Determine if this volume uses an NTFS file system.      
	GetVolumeInformation(L"c:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0);
	long hash = (long)((serialNum + (serialNum >> 16)) & 0xFFFF);

	return hash;
}

void s2ws(const std::string& str, std::wstring& outStr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	outStr.assign(converterX.from_bytes(str));
}

void ws2s(const std::wstring& wstr, std::string& outStr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	outStr.assign(converterX.to_bytes(wstr));
}

static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static inline bool is_base64(BYTE c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(BYTE const* buf, unsigned int bufLen)
{
	std::string ret;
	int i = 0;
	int j = 0;
	BYTE char_array_3[3];
	BYTE char_array_4[4];

	while (bufLen--) {
		char_array_3[i++] = *(buf++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';
	}

	return ret;
}









/*std::wstring GetName(IAccessible *pAcc)
{
CComBSTR bstrName;
if (!pAcc || FAILED(pAcc->get_accName(CComVariant((int)CHILDID_SELF), &bstrName)) || !bstrName.m_str)
return L"";

return bstrName.m_str;
}

HRESULT WalkTreeWithAccessibleChildren(CComPtr<IAccessible> pAcc)
{
long childCount = 0;
long returnCount = 0;

HRESULT hr = pAcc->get_accChildCount(&childCount);

if (childCount == 0)
return S_OK;

CComVariant* pArray = new CComVariant[childCount];
hr = ::AccessibleChildren(pAcc, 0L, childCount, pArray, &returnCount);
if (FAILED(hr))
return hr;

for (int x = 0; x < returnCount; x++) {
CComVariant vtChild = pArray[x];
if (vtChild.vt != VT_DISPATCH)
continue;

CComPtr<IDispatch> pDisp = vtChild.pdispVal;
CComQIPtr<IAccessible> pAccChild = pDisp;
if (!pAccChild)
continue;

std::wstring name = GetName(pAccChild).data();
if (name.find(L"�������� ������") != -1) {
CComBSTR bstrValue;
if (SUCCEEDED(pAccChild->get_accValue(CComVariant((int)CHILDID_SELF), &bstrValue)) && bstrValue.m_str)
OutputDebugString(std::wstring(bstrValue.m_str).c_str());

return S_FALSE;
}

if (WalkTreeWithAccessibleChildren(pAccChild) == S_FALSE)
return S_FALSE;
}

delete[] pArray;
return S_OK;
}*/