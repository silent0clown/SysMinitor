// registry_encoding_utils.h
#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <sstream>
#include <iomanip>
#include "encode.h"

namespace sysmonitor {
namespace encoding {

class RegistryEncodingUtils {
public:
    // ʹ�����е� EncodingUtil
    using StringUtil = util::EncodingUtil;

    // HKEYת�ַ���
    static std::string HKEYToString(HKEY hkey);
    
    // ע�����������ת�ַ���
    static std::string RegistryTypeToString(DWORD type);
    
    // ע�������ת�ַ�����ʹ�ð�ȫ��UTF-8ת����
    static std::string RegistryDataToString(const BYTE* data, DWORD size, DWORD type);
    
    // ���ַ���תUTF-8��ʹ�����еĹ��ߣ�
    static std::string WideToUTF8(const std::wstring& wideStr);
    
    // ��ȫ���ַ�������
    static std::string SafeString(const std::string& str);
    
    // ��������֤UTF-8�ַ���
    static std::string SanitizeUTF8(const std::string& str);

private:
    // �������ַ�������
    static std::string ProcessMultiSZ(const BYTE* data, DWORD size);
    
    // ��������������
    static std::string ProcessBinaryData(const BYTE* data, DWORD size);
};

} // namespace encoding
} // namespace sysmonitor