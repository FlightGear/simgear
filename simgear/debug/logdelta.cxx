#include "debug_types.h"

#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <assert.h>


/* A single item in SG_LOG_DELTAS. */
struct logDeltaItem
{
    logDeltaItem(const std::string& file_prefix, int line,
            const std::string& function_prefix, int delta)
    :
    file_prefix(file_prefix),
    line(line),
    function_prefix(function_prefix),
    delta(delta)
    {}
    std::string file_prefix;
    int         line;
    std::string function_prefix;
    int         delta;
};

/* A single item in SG_LOG_DELTAS for use in logDelta's cache. */
struct logDeltaCacheItem
{
    logDeltaCacheItem(const char* file, int line, const char* function)
    :
    file(file),
    line(line),
    function(function)
    {}
    const char* file;
    int         line;
    const char* function;
};

/* We compare logDeltaCacheItem's based on the raw values of the <file> and
<function> pointer values rather than doing string comparisons; this works
because these are generally from __FILE__ and __FUNCTION__ so don't change. */
bool operator < (const logDeltaCacheItem& a, const logDeltaCacheItem& b)
{
    if (a.file != b.file) {
        return a.file < b.file;
    }
    if (a.function != b.function) {
        return a.function < b.function;
    }
    return a.line < b.line;
}

struct logDelta
{
    logDelta()
    {
        const char* items = getenv("SG_LOG_DELTAS");
        if (!items) {
            return;
        }
        std::cerr << __FILE__ << ":" << __LINE__ << ": "
                << "Parsing $SG_LOG_DELTAS: '" << items << "'"
                << "\n";
        update(items);
    }
    
    void update(const char* items)
    {
        // We output diagnostics to std::cerr.
        //
        std::lock_guard<std::mutex> lock( m_mutex);
        m_cache.clear();
        m_items.clear();
        
        std::string text = items;
        for(;;) {
            if (text.empty()) break;
            
            std::string item = next_subitem(text, " ,");
            std::string item_orig = item;
            
            std::string ffl = next_subitem(item, "=");
            std::string delta = item;
            
            if (delta.empty()
                    || delta.find_first_not_of("0123456789-+") != std::string::npos
                    ) {
                std::cerr << __FILE__ << ":" << __LINE__ << ": "
                        << "Ignoring item '" << item_orig << "'"
                        << " because invalid delta '" << delta
                        << "'\n";
                continue;
            }
            int delta2 = std::stoi(delta);
            
            std::string file = next_subitem(ffl, ":");
            std::string function = next_subitem(ffl, ":");
            std::string line = next_subitem(ffl, ":");
            
            if (line.empty()) {
                /* If <function> is all digits, treat as line number. */
                if (function.find_first_not_of("0123456789-+") == std::string::npos) {
                    std::swap(function, line);
                }
            }
            
            int line2 = -1;
            if (!line.empty()) {
                if (line.find_first_not_of("0123456789-+") != std::string::npos) {
                    std::cerr << __FILE__ << ":" << __LINE__ << ": "
                            << "Ignoring item '" << item_orig << "'"
                            << " because invalid line number '" << line
                            << "'\n";
                    continue;
                }
                line2 = std::stoi(line);
            }
            if (1) std::cerr << __FILE__ << ":" << __LINE__ << ": "
                    << "adding"
                    << " file_prefix=" << file
                    << " function_prefix=" << function
                    << " line=" << line2
                    << " delta=" << delta2
                    << "\n";
            m_items.push_back(logDeltaItem(file, line2, function, delta2));
        }
    }
    
    /* Returns delta logging level for (file,line,function), using m_items and
    caching results in m_cache. */
    int operator()(const char* file, int line, const char* function)
    {
        if (m_items.empty()) {
            // Fast path in default case where SG_LOG_DELTAS not set.
            return 0;
        }
        if (!file)      file = "";
        if (!function)  function = "";
        logDeltaCacheItem flf(file, line, function);
        
        std::lock_guard<std::mutex> lock( m_mutex);
        
        auto it = m_cache.find(flf);
        
        if (it == m_cache.end()) {
            auto best = m_items.end();
            for (auto item=m_items.begin(); item!=m_items.end(); ++item) {
                if (startswith(file, item->file_prefix)
                        && startswith(function, item->function_prefix)
                        && (item->line == -1 || line == item->line)
                        ) {
                    // (file,line,function) matches <item>.
                    if (best == m_items.end() || cmp(*item, *best) > 0) {
                        // Prefer longer matches.
                        best = item;
                    }
                }
            }
            int delta = (best == m_items.end()) ? 0 : best->delta;
            m_cache[flf] = delta;
            it = m_cache.find(flf);
            
            assert(m_cache.find(flf)->second == delta);
            if (0 && delta != 0) {
                std::cerr << __FILE__ << ":" << __LINE__ << ": "
                        << "caching delta=" << delta
                        << " for: " << file << ":" << line << ":" << function
                        << "\n";
            }
        }
        
        int delta = it->second;
        if (0) {
            std::cerr << __FILE__ << ":" << __LINE__ << ": "
                        << "returning delta=" << delta
                        << " for: " << file << ":" << line << ":" << function
                        << "\n";
        }
        return delta;
    }
    
    static bool startswith(const std::string s, const std::string& prefix)
    {
        return s.rfind(prefix, 0) == 0;
    }
    
    /* Implements comparison of matching logDeltaItem's. We prefer longer
    filename prefixes and longer function name prefixes. */
    static int cmp(const logDeltaItem& a, const logDeltaItem& b)
    {
        if (a.file_prefix.size() != b.file_prefix.size()) {
            return a.file_prefix.size() - b.file_prefix.size();
        }
        if (a.function_prefix.size() != b.function_prefix.size()) {
            return a.function_prefix.size() - b.function_prefix.size();
        }
        return 0;
    }
    
    /* Splits <io_item> at first char in <separators>. Changes io_item to be
    the remainder and returns the first portion. */
    static std::string next_subitem(std::string& io_item, const char* separators)
    {
        std::string ret;
        size_t p = io_item.find_first_of(separators);
        if (p == std::string::npos) {
            ret = io_item;
            io_item = "";
        }
        else {
            ret = io_item.substr(0, p);
            io_item = io_item.substr(p+1);
        }
        return ret;
    }
    
    private:
        std::mutex                          m_mutex;
        std::map<logDeltaCacheItem, int>    m_cache;
        std::vector<logDeltaItem>           m_items;
};

/* s_log_delta(filename, line, function) returns the logging level delta for
the specified line of code. */
static logDelta s_log_delta;

sgDebugPriority logDeltaAdd(sgDebugPriority priority,
        const char* file, int line, const char* function,
        bool freeFilename)
{
    if (freeFilename) {
        // <file> and <function> are from strdup() so using them in std::map
        // key is unreliable and will leak memory.
        return priority;
    }
    int priority_new = (int) priority + s_log_delta(file, line, function);
    
    // Don't cause new popups.
    if (priority < SG_POPUP && priority_new >= SG_POPUP) {
        priority_new = SG_POPUP - 1;
    }
    // Keep new priority within range of sgDebugPriority enum.
    if (priority_new < SG_BULK) {
        priority_new = SG_BULK;
    }
    if (priority_new > SG_MANDATORY_INFO) {
        priority_new = SG_MANDATORY_INFO;
    }
    return (sgDebugPriority) priority_new;
}

void logDeltaSet(const char* items)
{
    s_log_delta.update(items);
}
