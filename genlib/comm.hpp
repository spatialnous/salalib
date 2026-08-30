// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <vector>

const char *const g_default_file_set = "File set";

struct FilePath {
    std::string path;
    std::string name;
    std::string ext;
    FilePath(const std::string &pathname) : path(), name(), ext() {
        size_t dot = pathname.find_last_of('.');
#ifdef _WIN32
        size_t slash = pathname.find_last_of('\\'); // WIN32
#else
        size_t slash = pathname.find_last_of('/'); // Other
#endif
        if (slash != std::string::npos) {
            path = pathname.substr(0, slash + 1);
        }
        if (dot != std::string::npos) {
            name = pathname.substr(slash + 1, dot - slash - 1);
            ext = pathname.substr(dot + 1);
        } else {
            name = pathname.substr(slash + 1);
        }
    }
};

class Communicator {
  public:
    class CancelledException // throw from your class
    {
      public:
        CancelledException() {}
    };
    enum { NUM_STEPS, CURRENT_STEP, NUM_RECORDS, CURRENT_RECORD };

  protected:
    mutable bool m_cancelled;
    bool m_deleteFlag;

  private:
    [[maybe_unused]] unsigned _padding0 : 2 * 8;
    [[maybe_unused]] unsigned _padding1 : 4 * 8;

  protected:
    // nb. converted to Win32 UTF-16 Unicode path (AT 31.01.11) Linux, MacOS use UTF-8 (AT 29.04.11)
    std::string m_infilename;
    std::ifstream *m_infile;
    std::ifstream *m_infile2; // <- MapInfo MIF files come in two parts
    std::ofstream *m_outfile;
    // nb. converted to Win32 UTF-16 Unicode path (AT 31.01.11) Linux, MacOS use UTF-8 (AT 29.04.11)
    std::vector<std::string> m_fileset; // <- sometimes you want to load a whole set of files

  public:
    Communicator()
        : m_cancelled(false), m_deleteFlag(false), _padding0(0), _padding1(0), m_infilename(),
          m_infile(nullptr), m_infile2(nullptr), m_outfile(nullptr), m_fileset() {}
    Communicator(const Communicator &) = default;
    Communicator &operator=(Communicator &) = default;
    //
    bool GetDeleteFlag() // used by ICommunicator and IComm together
    {
        return m_deleteFlag;
    }
    //
    virtual ~Communicator() {
        if (m_infile)
            delete m_infile;
        m_infile = nullptr;
        if (m_infile2)
            delete m_infile2;
        m_infile2 = nullptr;
        if (m_outfile)
            delete m_outfile;
        m_outfile = nullptr;
    }
    //
    void SetInfile(const char *filename) {
        m_infile = new std::ifstream(filename);
        FilePath fp(filename);
        m_infilename = fp.name;
    }
    void SetInfile2(const char *filename) { m_infile2 = new std::ifstream(filename); }
    std::string GetInfileName() {
        return m_fileset.size() ? std::string(g_default_file_set) : m_infilename;
    }
    std::string GetMBInfileName() {
        std::string ret;
        if (!m_fileset.empty()) {
            ret = "File set";
        } else {
            ret = std::string(m_infilename.c_str());
        }
        return ret;
    }
    long GetInfileSize() {
        if (m_infile) {
            m_infile->seekg(0, std::ios::beg);
            auto beginPos = m_infile->tellg();
            m_infile->seekg(0, std::ios::end);
            auto endPos = m_infile->tellg();
            m_infile->seekg(0, std::ios::beg);
            return static_cast<long>(endPos - beginPos);
        }
        return 0;
    }
    void SetOutfile(const char *filename) { m_outfile = new std::ofstream(filename); }
    //
    bool IsCancelled() const { return m_cancelled; }
    void Cancel() { m_cancelled = true; }

    // const version is for cases where we need to Cancel from a const
    // context, for example from within an implemented CommPostMessage,
    // i.e. in cases where an external handler does not explicitly
    // cancel, but instead also sets a "cancelled" variable
    void Cancel() const { m_cancelled = true; }

    std::ifstream &getInFileStream() { return *m_infile; }
    std::ifstream &GetInfile2() { return *m_infile2; }
    //
    const std::vector<std::string> &GetFileSet() const { return m_fileset; }
    //
    virtual void CommPostMessage(size_t m,
                                 size_t x) const = 0; // Override for specific operating system
    virtual void logError(const std::string &message) const = 0;
    virtual void logWarning(const std::string &message) const = 0;
    virtual void logInfo(const std::string &message) const = 0;
};

// a helpful little function...
// This function is used exclusively to update the communicators at specific intervals (set in
// milliseconds by the timeout argument). Typical usage: Create a time_t t1 and pass to this
// function with timeout = 0, setting thus t1 to the current time in milliseconds. Then continuously
// pass the same t1 to this function along with an interval timeout (in most cases 500ms). The
// function only synchronises t1 to the current time if its difference to the current time is longer
// than the interval (i.e. more than 500 milliseconds have passed since the last synchronisation).
// If a synchronisation occurs then the communicator is updated along with the equivalent user
// interface element.
// TODO: All time handling in the application uses time_t and stores milliseconds in it, though
// time_t is supposed to only store seconds. Replace with std::chrono::time_point everywhere
inline bool qtimer(time_t &t1, time_t timeout) {
    auto time2 = std::chrono::system_clock::now().time_since_epoch();
    time_t t2 = std::chrono::duration_cast<std::chrono::milliseconds>(time2).count();
    if ((t2 - t1) > timeout || (t2 - t1) < 0) { // also catch a loop
        t1 = t2;
        return true;
    }
    return false;
}
