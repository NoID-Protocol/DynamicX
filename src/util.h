// Copyright (c) 2009-2017 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Developers
// Copyright (c) 2014-2017 The Dash Core Developers
// Copyright (c) 2016-2017 Duality Blockchain Solutions Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Server/client environment: argument handling, config file parsing,
 * logging, thread wrappers
 */
#ifndef DYNAMIC_UTIL_H
#define DYNAMIC_UTIL_H

#if defined(HAVE_CONFIG_H)
#include "config/dynamic-config.h"
#endif

#include "amount.h"
#include "compat.h"
#include "tinyformat.h"
#include "utiltime.h"
#include "sync.h"
#include "fs.h"

#include <atomic>
#include <exception>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

#include <boost/signals2/signal.hpp>
#include <boost/thread/exceptions.hpp>

// Debugging macros

// Uncomment the following line to enable debugging messages
// or enable on a per file basis prior to inclusion of util.h
//#define ENABLE_DYNAMIC_DEBUG
#ifdef ENABLE_DYNAMIC_DEBUG
#define DBG( x ) x
#else
#define DBG( x )
#endif

// Dynamic only features
extern bool fDyNode;
extern bool fLiteMode;
extern int nWalletBackups;

static const bool DEFAULT_LOGTIMEMICROS  = false;
static const bool DEFAULT_LOGIPS         = false;
static const bool DEFAULT_LOGTIMESTAMPS  = true;
static const bool DEFAULT_LOGTHREADNAMES = false;

/** Signals for translation. */
class CTranslationInterface
{
public:
    /** Translate a message to the native language of the user. */
    boost::signals2::signal<std::string (const char* psz)> Translate;
};

extern CTranslationInterface translationInterface;

extern bool fDebug;
extern bool fPrintToConsole;
extern bool fPrintToDebugLog;
extern bool fServer;
extern std::string strMiscWarning;
extern bool fLogTimestamps;
extern bool fLogTimeMicros;
extern bool fLogThreadNames;
extern bool fLogIPs;
extern volatile bool fReopenDebugLog;

extern const char * const DYNAMIC_CONF_FILENAME;
extern const char * const DYNAMIC_PID_FILENAME;

extern std::atomic<uint32_t> logCategories;

/**
 * Translation function: Call Translate signal on UI interface, which returns a boost::optional result.
 * If no translation slot is registered, nothing is returned, and simply return the input.
 */
inline std::string _(const char* psz)
{
    boost::optional<std::string> rv = translationInterface.Translate(psz);
    return rv ? (*rv) : psz;
}

void SetupEnvironment();
bool SetupNetworking();

struct CLogCategoryActive
{
    std::string category;
    bool active;
};

namespace DYNLog {
enum LogFlags : uint32_t {
    NONE        = 0,
    NET         = (1 <<  0),
    TOR         = (1 <<  1),
    MEMPOOL     = (1 <<  2),
    HTTP        = (1 <<  3),
    BENCH       = (1 <<  4),
    ZMQ         = (1 <<  5),
    DB          = (1 <<  6),
    RPC         = (1 <<  7),
    ESTIMATEFEE = (1 <<  8),
    ADDRMAN     = (1 <<  9),
    SELECTCOINS = (1 << 10),
    REINDEX     = (1 << 11),
    CMPCTBLOCK  = (1 << 12),
    RAND        = (1 << 13),
    PRUNE       = (1 << 14),
    PROXY       = (1 << 15),
    MEMPOOLREJ  = (1 << 16),
    LIBEVENT    = (1 << 17),
    COINDB      = (1 << 18),
    QT          = (1 << 19),
    LEVELDB     = (1 << 20),
    ALERT       = (1 << 21),
    // Dynamic-specific log flags
    PRIVATESEND = (1 << 22),
    INSTANTSEND = (1 << 23),
    DYNODE     	= (1 << 24),
    SPORK	    = (1 << 25),
    KEEPASS     = (1 << 26),
    DNPAYMENTS  = (1 << 27),
    GOBJECT     = (1 << 28),
    BLOCKGEN    = (1 << 29),
    VERIFY		= (1 << 30),

    ALL         = ~(uint32_t)0,
};
}
/** Return true if log accepts specified category */
static inline bool LogAcceptCategory(uint32_t category)
{
    return (logCategories.load(std::memory_order_relaxed) & category) != 0;
}

/** Returns a string with the log categories. */
std::string ListLogCategories();

/** Returns a vector of the active log categories. */
std::vector<CLogCategoryActive> ListActiveLogCategories();

/** Return true if str parses as a log category and set the flags in f */
bool GetLogCategory(uint32_t *f, const std::string *str);

/** Send a string to the log output */
int LogPrintStr(const std::string &str);

/** Get format string from VA_ARGS for error reporting */
template<typename... Args> std::string FormatStringFromLogArgs(const char *fmt, const Args&... args) {
    return fmt;
}

#define LogPrintf(...) do { \
    std::string _log_msg_; /* Unlikely name to avoid shadowing variables */ \
    try { \
        _log_msg_ = tfm::format(__VA_ARGS__); \
    } catch (tinyformat::format_error &fmterr) { \
        /* Original format string will have newline so don't add one here */ \
        _log_msg_ = "Error \"" + std::string(fmterr.what()) + "\" while formatting log message: " + FormatStringFromLogArgs(__VA_ARGS__); \
    } \
    LogPrintStr(_log_msg_); \
} while(0)


#define LogPrint(category, ...) do { \
    if (LogAcceptCategory((category))) { \
        LogPrintf(__VA_ARGS__); \
    } \
} while(0)

template<typename... Args>
bool error(const char* fmt, const Args&... args)
{
    LogPrintStr("ERROR: " + tfm::format(fmt, args...) + "\n");
    return false;
}

void PrintExceptionContinue(const std::exception *pex, const char* pszThread);
void FileCommit(FILE *file);
bool TruncateFile(FILE *file, unsigned int length);
int RaiseFileDescriptorLimit(int nMinFD);
void AllocateFileRange(FILE *file, unsigned int offset, unsigned int length);
bool RenameOver(fs::path src, fs::path dest);
bool TryCreateDirectory(const fs::path& p);
fs::path GetDefaultDataDir();
const fs::path &GetDataDir(bool fNetSpecific = true);
const fs::path &GetBackupsDir();
void ClearDatadirCache();
fs::path GetConfigFile();
fs::path GetDynodeConfigFile();
#ifndef WIN32
fs::path GetPidFile();
void CreatePidFile(const fs::path &path, pid_t pid);
#endif
#ifdef WIN32
fs::path GetSpecialFolderPath(int nFolder, bool fCreate = true);
#endif
fs::path GetTempPath();
void OpenDebugLog();
void ShrinkDebugFile();
void runCommand(const std::string& strCommand);


inline bool IsSwitchChar(char c)
{
#ifdef WIN32
    return c == '-' || c == '/';
#else
    return c == '-';
#endif
}

class ArgsManager
{
protected:
    CCriticalSection cs_args;
    std::map<std::string, std::string> mapArgs;
    std::map<std::string, std::vector<std::string> > mapMultiArgs;
public:
    void ParseParameters(int argc, const char*const argv[]);
    void ReadConfigFile(const std::string& confPath);
    std::vector<std::string> GetArgs(const std::string& strArg);
    /**
     * Return true if the given argument has been manually set
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @return true if the argument has been set
     */
    bool IsArgSet(const std::string& strArg);

    /**
     * Return string argument or default value
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param default (e.g. "1")
     * @return command-line argument or default value
     */
    std::string GetArg(const std::string& strArg, const std::string& strDefault);

    /**
     * Return integer argument or default value
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param default (e.g. 1)
     * @return command-line argument (0 if invalid number) or default value
     */
    int64_t GetArg(const std::string& strArg, int64_t nDefault);

    /**
     * Return boolean argument or default value
     *
     * @param strArg Argument to get (e.g. "-foo")
     * @param default (true or false)
     * @return command-line argument or default value
     */
    bool GetBoolArg(const std::string& strArg, bool fDefault);

    /**
     * Set an argument if it doesn't already have a value
     *
     * @param strArg Argument to set (e.g. "-foo")
     * @param strValue Value (e.g. "1")
     * @return true if argument gets set, false if it already had a value
     */
    bool SoftSetArg(const std::string& strArg, const std::string& strValue);

    /**
     * Set a boolean argument if it doesn't already have a value
     *
     * @param strArg Argument to set (e.g. "-foo")
     * @param fValue Value (e.g. false)
     * @return true if argument gets set, false if it already had a value
     */
    bool SoftSetBoolArg(const std::string& strArg, bool fValue);

    /// Forces a arg setting, used only in testing
    void ForceSetArg(const std::string& strArg, const std::string& strValue);
	/// Delete arg data, used in critical functions
    void DeleteArg(const std::string& strArg);
};

extern ArgsManager gArgs;

// wrappers using the global ArgsManager:
static inline void ParseParameters(int argc, const char*const argv[])
{
    gArgs.ParseParameters(argc, argv);
}

static inline void ReadConfigFile(const std::string& confPath)
{
    gArgs.ReadConfigFile(confPath);
}

static inline bool SoftSetArg(const std::string& strArg, const std::string& strValue)
{
    return gArgs.SoftSetArg(strArg, strValue);
}

static inline void ForceSetArg(const std::string& strArg, const std::string& strValue)
{
    gArgs.ForceSetArg(strArg, strValue);
}

static inline void DeleteArgument(const std::string& strArg)
{
    gArgs.DeleteArg(strArg);
}

static inline bool IsArgSet(const std::string& strArg)
{
    return gArgs.IsArgSet(strArg);
}

static inline std::string GetArg(const std::string& strArg, const std::string& strDefault)
{
    return gArgs.GetArg(strArg, strDefault);
}

static inline int64_t GetArg(const std::string& strArg, int64_t nDefault)
{
    return gArgs.GetArg(strArg, nDefault);
}

static inline bool GetBoolArg(const std::string& strArg, bool fDefault)
{
    return gArgs.GetBoolArg(strArg, fDefault);
}

static inline bool SoftSetBoolArg(const std::string& strArg, bool fValue)
{
    return gArgs.SoftSetBoolArg(strArg, fValue);
}

/**
 * Format a string to be used as group of options in help messages
 *
 * @param message Group name (e.g. "RPC server options:")
 * @return the formatted string
 */
std::string HelpMessageGroup(const std::string& message);

/**
 * Format a string to be used as option description in help messages
 *
 * @param option Option message (e.g. "-rpcuser=<user>")
 * @param message Option description (e.g. "Username for JSON-RPC connections")
 * @return the formatted string
 */
std::string HelpMessageOpt(const std::string& option, const std::string& message);

/**
 * Return the number of cores available on the current system.
 * @note This does count virtual cores, such as those provided by HyperThreading.
 */
int GetNumCores();

void SetThreadPriority(int nPriority);
void RenameThread(const char* name);
std::string GetThreadName();

/**
 * .. and a wrapper that just calls func once
 */
template <typename Callable> void TraceThread(const char* name,  Callable func)
{
    std::string s = strprintf("bitcoin-%s", name);
    RenameThread(s.c_str());
    try
    {
        LogPrintf("%s thread start\n", name);
        func();
        LogPrintf("%s thread exit\n", name);
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("%s thread interrupt\n", name);
        throw;
    }
    catch (const std::exception& e) {
        PrintExceptionContinue(&e, name);
        throw;
    }
    catch (...) {
        PrintExceptionContinue(NULL, name);
        throw;
    }
}

#endif // DYNAMIC_UTIL_H
