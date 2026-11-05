// アプリ内ログ。juce::Logger を継承し、メッセージをリングバッファに溜めてDebugタブに表示する。
// コンソール(stderr)への出力は設定「Log to console」を有効にした場合のみ(既定OFF)。
#pragma once

#include <JuceHeader.h>
#include <deque>
#include <cstdio>
#include <vector>

namespace dbglog {

enum class Level { info, warning, error };

struct Entry {
    juce::int64 id = 0;
    Level level = Level::info;
    juce::String text;
};

class Log : public juce::Logger
{
public:
    static constexpr size_t maxEntries = 1000;

    static Log& instance()
    {
        static Log log;
        return log;
    }

    // コンソール出力の設定(ユーザー設定ファイルに保存。既定OFF)
    bool consoleEnabled() const { return consoleFlag.load(); }
    void setConsoleEnabled(bool on) { consoleFlag.store(on); }

    void add(Level level, const juce::String& message)
    {
        juce::String text = message.trimEnd();
        // オーディオスレッドからも呼ばれるので、競合時は待たずに捨てる
        const juce::CriticalSection::ScopedTryLockType lock(mutex);
        if (!lock.isLocked()) return;
        entries.push_back({ ++nextId, level, text });
        if (entries.size() > maxEntries) entries.pop_front();
        if (consoleFlag.load()) {
            const char* tag = level == Level::error ? "ERR" : level == Level::warning ? "WRN" : "DBG";
            std::fprintf(stderr, "[%s] %s\n", tag, text.toRawUTF8());
        }
    }

    // afterId より新しいエントリを返す(Debugタブの差分更新用)
    std::vector<Entry> getEntriesAfter(juce::int64 afterId) const
    {
        std::vector<Entry> out;
        const juce::ScopedLock lock(mutex);
        for (const auto& e : entries)
            if (e.id > afterId) out.push_back(e);
        return out;
    }

    void clear()
    {
        const juce::ScopedLock lock(mutex);
        entries.clear();
    }

private:
    // juce::Logger::writeToLog 経由のメッセージもここに入る
    void logMessage(const juce::String& message) override { add(Level::info, message); }

    mutable juce::CriticalSection mutex;
    std::deque<Entry> entries;
    juce::int64 nextId = 0;
    std::atomic<bool> consoleFlag { false };
};

// printf 形式の書式化。juce::String::formatted は Windows ではワイド版 printf を使うため、
// %s の char* が wchar_t* として読まれて文字化けする上、書式文字列の非ASCII文字も壊れる。
// ここではナロー版で書式化し、結果を UTF-8 として解釈する。
template <typename... Args>
inline juce::String format(const char* fmt, Args... args)
{
    const int length = std::snprintf(nullptr, 0, fmt, args...);
    if (length <= 0) return {};

    std::vector<char> buffer(static_cast<size_t>(length) + 1);
    std::snprintf(buffer.data(), buffer.size(), fmt, args...);

    // SysEx 由来のテキストなど、UTF-8 として不正なバイト列は '?' に置き換える
    if (!juce::CharPointer_UTF8::isValidString(buffer.data(), length))
        for (int i = 0; i < length; ++i)
            if (static_cast<unsigned char>(buffer[static_cast<size_t>(i)]) >= 0x80)
                buffer[static_cast<size_t>(i)] = '?';

    return juce::String::fromUTF8(buffer.data(), length);
}

template <typename... Args>
inline void write(Level level, const char* fmt, Args... args)
{
    Log::instance().add(level, format(fmt, args...));
}

} // namespace dbglog

#define S3_LOG(...)  ::dbglog::write(::dbglog::Level::info, __VA_ARGS__)
#define S3_WARN(...) ::dbglog::write(::dbglog::Level::warning, __VA_ARGS__)
#define S3_ERR(...)  ::dbglog::write(::dbglog::Level::error, __VA_ARGS__)
