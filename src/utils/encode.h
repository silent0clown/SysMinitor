#ifndef UTIL_ENCODE_H
#define UTIL_ENCODE_H

#include <string>
#include <vector>

namespace util {

class EncodingUtil {
public:
    /**
     * @brief 检测字符串编码类型
     * @param str 输入字符串
     * @return 编码类型: "UTF-8", "GB2312", "UNKNOWN"
     */
    static std::string DetectEncoding(const std::string& str);
    
    /**
     * @brief 判断是否为有效的UTF-8编码
     * @param str 输入字符串
     * @return true-有效UTF-8, false-非UTF-8
     */
    static bool IsValidUTF8(const std::string& str);
    
    /**
     * @brief 判断是否为GB2312编码
     * @param str 输入字符串
     * @return true-可能是GB2312, false-非GB2312
     */
    static bool IsGB2312(const std::string& str);
    
    /**
     * @brief GB2312转UTF-8
     * @param gb2312_str GB2312编码字符串
     * @return UTF-8编码字符串
     */
    static std::string GB2312ToUTF8(const std::string& gb2312_str);
    
    /**
     * @brief UTF-8转GB2312
     * @param utf8_str UTF-8编码字符串
     * @return GB2312编码字符串
     */
    static std::string UTF8ToGB2312(const std::string& utf8_str);
    
    /**
     * @brief 智能转换为UTF-8，自动检测编码并转换
     * @param str 输入字符串
     * @return 保证为UTF-8编码的字符串
     */
    static std::string ToUTF8(const std::string& str);
    
    /**
     * @brief 清理字符串，移除不可打印字符
     * @param str 输入字符串
     * @return 清理后的字符串
     */
    static std::string SanitizeString(const std::string& str);
    
    /**
     * @brief 安全获取字符串，如果转换失败返回默认值
     * @param str 输入字符串
     * @param default_val 默认值
     * @return 安全的UTF-8字符串
     */
    static std::string SafeString(const std::string& str, const std::string& default_val = "Unknown");

private:
    /**
     * @brief 验证UTF-8字节序列
     * @param bytes 字节序列
     * @param length 序列长度
     * @return true-有效, false-无效
     */
    static bool IsValidUTF8Sequence(const unsigned char* bytes, size_t length);
    
    /**
     * @brief 检查是否为GB2312字符
     * @param byte1 第一个字节
     * @param byte2 第二个字节
     * @return true-是GB2312字符, false-不是
     */
    static bool IsGB2312Char(unsigned char byte1, unsigned char byte2);
};

} // namespace util

#endif // UTIL_ENCODE_H