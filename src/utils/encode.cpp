#include "encode.h"
#include <cctype>
#include <algorithm>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

namespace util {

std::string EncodingUtil::DetectEncoding(const std::string& str) {
    if (str.empty()) {
        return "UTF-8"; // ���ַ���Ĭ��ΪUTF-8
    }
    
    if (IsValidUTF8(str)) {
        return "UTF-8";
    }
    
    if (IsGB2312(str)) {
        return "GB2312";
    }
    
    return "UNKNOWN";
}

bool EncodingUtil::IsValidUTF8(const std::string& str) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.c_str());
    size_t len = str.length();
    size_t i = 0;
    
    while (i < len) {
        unsigned char current = bytes[i];
        
        // ASCII�ַ� (0x00-0x7F)
        if (current <= 0x7F) {
            i++;
            continue;
        }
        
        // ���ֽ�UTF-8����
        if ((current & 0xE0) == 0xC0) { // 2�ֽ�����
            if (i + 1 >= len || !IsValidUTF8Sequence(bytes + i, 2)) {
                return false;
            }
            i += 2;
        } else if ((current & 0xF0) == 0xE0) { // 3�ֽ�����
            if (i + 2 >= len || !IsValidUTF8Sequence(bytes + i, 3)) {
                return false;
            }
            i += 3;
        } else if ((current & 0xF8) == 0xF0) { // 4�ֽ�����
            if (i + 3 >= len || !IsValidUTF8Sequence(bytes + i, 4)) {
                return false;
            }
            i += 4;
        } else {
            return false; // ��Ч��UTF-8��ʼ�ֽ�
        }
    }
    
    return true;
}

bool EncodingUtil::IsValidUTF8Sequence(const unsigned char* bytes, size_t length) {
    for (size_t i = 1; i < length; i++) {
        if ((bytes[i] & 0xC0) != 0x80) { // �����ֽڱ�����10xxxxxx
            return false;
        }
    }
    return true;
}

bool EncodingUtil::IsGB2312(const std::string& str) {
    if (str.empty()) {
        return false;
    }
    
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(str.c_str());
    size_t len = str.length();
    size_t i = 0;
    int gb2312Count = 0;
    int totalChars = 0;
    
    while (i < len) {
        // ASCII�ַ�
        if (bytes[i] <= 0x7F) {
            i++;
            totalChars++;
            continue;
        }
        
        // ������GB2312˫�ֽ��ַ�
        if (i + 1 < len) {
            if (IsGB2312Char(bytes[i], bytes[i + 1])) {
                gb2312Count++;
            }
            totalChars++;
            i += 2;
        } else {
            break; // ��������˫�ֽ��ַ�
        }
    }
    
    // ������㹻���GB2312�ַ�����Ϊ��GB2312����
    return totalChars > 0 && (gb2312Count * 2 >= totalChars);
}

bool EncodingUtil::IsGB2312Char(unsigned char byte1, unsigned char byte2) {
    // GB2312���뷶Χ: ��һ���ֽ�0xA1-0xF7, �ڶ����ֽ�0xA1-0xFE
    return (byte1 >= 0xA1 && byte1 <= 0xF7) && (byte2 >= 0xA1 && byte2 <= 0xFE);
}

std::string EncodingUtil::GB2312ToUTF8(const std::string& gb2312_str) {
#ifdef _WIN32
    // Windowsƽ̨ʹ��ϵͳAPI����ת��
    int len = MultiByteToWideChar(936, 0, gb2312_str.c_str(), -1, nullptr, 0);
    if (len == 0) {
        return gb2312_str; // ת��ʧ�ܣ�����ԭ�ַ���
    }
    
    std::vector<wchar_t> wideBuf(len);
    MultiByteToWideChar(936, 0, gb2312_str.c_str(), -1, wideBuf.data(), len);
    
    len = WideCharToMultiByte(CP_UTF8, 0, wideBuf.data(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) {
        return gb2312_str;
    }
    
    std::vector<char> utf8Buf(len);
    WideCharToMultiByte(CP_UTF8, 0, wideBuf.data(), -1, utf8Buf.data(), len, nullptr, nullptr);
    
    return std::string(utf8Buf.data());
#else
    // Linux/macOSƽ̨����ʹ��iconv�������ʵ�ַ���ԭ�ַ���
    // ��ʵ����Ŀ�п��Լ���iconv��
    return gb2312_str;
#endif
}

std::string EncodingUtil::UTF8ToGB2312(const std::string& utf8_str) {
#ifdef _WIN32
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (len == 0) {
        return utf8_str;
    }
    
    std::vector<wchar_t> wideBuf(len);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, wideBuf.data(), len);
    
    len = WideCharToMultiByte(936, 0, wideBuf.data(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) {
        return utf8_str;
    }
    
    std::vector<char> gb2312Buf(len);
    WideCharToMultiByte(936, 0, wideBuf.data(), -1, gb2312Buf.data(), len, nullptr, nullptr);
    
    return std::string(gb2312Buf.data());
#else
    return utf8_str;
#endif
}

std::string EncodingUtil::ToUTF8(const std::string& str) {
    if (str.empty()) {
        return str;
    }
    
    std::string encoding = DetectEncoding(str);
    
    if (encoding == "UTF-8") {
        return str;
    } else if (encoding == "GB2312") {
        return GB2312ToUTF8(str);
    } else {
        // δ֪���룬������������
        return SanitizeString(str);
    }
}

std::string EncodingUtil::SanitizeString(const std::string& str) {
    if (str.empty()) {
        return str;
    }
    
    std::string result;
    bool lastWasSpace = false;
    
    for (char c : str) {
        unsigned char uc = static_cast<unsigned char>(c);
        
        // ����ɴ�ӡASCII�ַ�
        if (uc >= 0x20 && uc <= 0x7E) {
            result += c;
            lastWasSpace = false;
        }
        // ����ո��ַ��������������ո�
        else if (std::isspace(uc)) {
            if (!lastWasSpace) {
                result += ' ';
                lastWasSpace = true;
            }
        }
        // ���������ַ��������滻Ϊ�ʺŻ�ֱ������
        else {
            result += '?';
            lastWasSpace = false;
        }
    }
    
    // ȥ����β�ո�
    size_t start = result.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    
    size_t end = result.find_last_not_of(" \t\n\r");
    return result.substr(start, end - start + 1);
}

std::string EncodingUtil::SafeString(const std::string& str, const std::string& default_val) {
    try {
        std::string result = ToUTF8(str);
        if (result.empty()) {
            return default_val;
        }
        return result;
    } catch (const std::exception&) {
        return default_val;
    }
}

} // namespace util