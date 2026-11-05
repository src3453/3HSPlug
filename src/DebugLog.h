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

template <typename... Args>
inline void write(Level level, const char* fmt, Args... args)
{
    Log::instance().add(level, juce::String::formatted(fmt, args...));
}

} // namespace dbglog

#define S3_LOG(...)  ::dbglog::write(::dbglog::Level::info, __VA_ARGS__)
#define S3_WARN(...) ::dbglog::write(::dbglog::Level::warning, __VA_ARGS__)
#define S3_ERR(...)  ::dbglog::write(::dbglog::Level::error, __VA_ARGS__)
