#ifndef UTIL_ENCODE_H
#define UTIL_ENCODE_H

#include <string>
#include <vector>

namespace util {

class EncodingUtil {
public:
    /**
     * @brief ����ַ�����������
     * @param str �����ַ���
     * @return ��������: "UTF-8", "GB2312", "UNKNOWN"
     */
    static std::string DetectEncoding(const std::string& str);
    
    /**
     * @brief �ж��Ƿ�Ϊ��Ч��UTF-8����
     * @param str �����ַ���
     * @return true-��ЧUTF-8, false-��UTF-8
     */
    static bool IsValidUTF8(const std::string& str);
    
    /**
     * @brief �ж��Ƿ�ΪGB2312����
     * @param str �����ַ���
     * @return true-������GB2312, false-��GB2312
     */
    static bool IsGB2312(const std::string& str);
    
    /**
     * @brief GB2312תUTF-8
     * @param gb2312_str GB2312�����ַ���
     * @return UTF-8�����ַ���
     */
    static std::string GB2312ToUTF8(const std::string& gb2312_str);
    
    /**
     * @brief UTF-8תGB2312
     * @param utf8_str UTF-8�����ַ���
     * @return GB2312�����ַ���
     */
    static std::string UTF8ToGB2312(const std::string& utf8_str);
    
    /**
     * @brief ����ת��ΪUTF-8���Զ������벢ת��
     * @param str �����ַ���
     * @return ��֤ΪUTF-8������ַ���
     */
    static std::string ToUTF8(const std::string& str);
    
    /**
     * @brief �����ַ������Ƴ����ɴ�ӡ�ַ�
     * @param str �����ַ���
     * @return �������ַ���
     */
    static std::string SanitizeString(const std::string& str);
    
    /**
     * @brief ��ȫ��ȡ�ַ��������ת��ʧ�ܷ���Ĭ��ֵ
     * @param str �����ַ���
     * @param default_val Ĭ��ֵ
     * @return ��ȫ��UTF-8�ַ���
     */
    static std::string SafeString(const std::string& str, const std::string& default_val = "Unknown");

private:
    /**
     * @brief ��֤UTF-8�ֽ�����
     * @param bytes �ֽ�����
     * @param length ���г���
     * @return true-��Ч, false-��Ч
     */
    static bool IsValidUTF8Sequence(const unsigned char* bytes, size_t length);
    
    /**
     * @brief ����Ƿ�ΪGB2312�ַ�
     * @param byte1 ��һ���ֽ�
     * @param byte2 �ڶ����ֽ�
     * @return true-��GB2312�ַ�, false-����
     */
    static bool IsGB2312Char(unsigned char byte1, unsigned char byte2);
};

} // namespace util

#endif // UTIL_ENCODE_H