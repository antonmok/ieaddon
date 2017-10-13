#pragma once

#include <string>

long GetTabID();
long GetVolumeHash();

void s2ws(const std::string& str, std::wstring& outStr);
void ws2s(const std::wstring& wstr, std::string& outStr);

