#pragma once
#include <string>
#include <Windows.h>

namespace sf {
namespace util {

    // ============================================================
    // 文字コード変換ユーティリティ
    // UTF-8 / Shift-JIS / wstring (UTF-16) 間の変換を提供
    // ============================================================

    /// <summary>
    /// UTF-8 → wstring (UTF-16) に変換
    /// </summary>
    inline std::wstring Utf8ToWstring(const std::string& str) {
        if (str.empty()) return L"";
        int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], sizeNeeded);
        if (!wstr.empty() && wstr.back() == L'\0') wstr.pop_back();
        return wstr;
    }

    /// <summary>
    /// wstring (UTF-16) → UTF-8に変換
    /// </summary>
    inline std::string WstringToUtf8(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);
        if (!str.empty() && str.back() == '\0') str.pop_back();
        return str;
    }

    /// <summary>
    /// UTF-8 → Shift-JIS に変換
    /// Windowsapiに日本語パスを渡す際につかう
    /// </summary>
    inline std::string Utf8ToShiftJis(const std::string& utf8Str) {
        std::wstring wstr = Utf8ToWstring(utf8Str);
        if (wstr.empty()) return "";
        int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);
        if (!str.empty() && str.back() == '\0') str.pop_back();
        return str;
    }

    /// <summary>
    /// Shift-JIS → wstring (UTF-16) に変換
    /// </summary>
    inline std::wstring ShiftJisToWstring(const std::string& sjisStr) {
        if (sjisStr.empty()) return L"";
        int sizeNeeded = MultiByteToWideChar(CP_ACP, 0, sjisStr.c_str(), -1, nullptr, 0);
        std::wstring wstr(sizeNeeded, 0);
        MultiByteToWideChar(CP_ACP, 0, sjisStr.c_str(), -1, &wstr[0], sizeNeeded);
        if (!wstr.empty() && wstr.back() == L'\0') wstr.pop_back();
        return wstr;
    }

    /// <summary>
    /// wstring (UTF-16) → Shift-JIS に変換
    /// </summary>
    inline std::string WstringToShiftJis(const std::wstring& wstr) {
        if (wstr.empty()) return "";
        int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), -1, &str[0], sizeNeeded, nullptr, nullptr);
        if (!str.empty() && str.back() == '\0') str.pop_back();
        return str;
    }

    /// <summary>
    /// UTF-8かどうかを判定して自動変換
    /// </summary>
    inline std::wstring AutoDetectToWstring(const std::string& str) {
        if (str.empty()) return L"";
        // まずUTF-8として試す
        int sizeUtf8 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.c_str(), -1, nullptr, 0);
        if (sizeUtf8 > 0) {
            std::wstring wstr(sizeUtf8, 0);
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], sizeUtf8);
            if (!wstr.empty() && wstr.back() == L'\0') wstr.pop_back();
            return wstr;
        }
        // UTF-8として無効ならShift-JISとして変換
        return ShiftJisToWstring(str);
    }

} // namespace util
} // namespace sf
