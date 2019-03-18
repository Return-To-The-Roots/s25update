// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "s25update.h" // IWYU pragma: keep
#include "md5sum.h"
#include <boost/filesystem.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/iostream.hpp>
#include <bzlib.h>
#include <curl/curl.h>
#include <iomanip>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;

#ifndef TARGET
#ifdef _WIN32
#define TARGET "windows"
#endif

#ifdef __APPLE__
#define TARGET "apple"
#endif

#ifdef __linux__
#define TARGET "linux"
#endif
#endif

#ifndef TARGET
#error You have to set TARGET to your platform (windows/linux/apple)
#endif

#ifndef ARCH
#error You have to set ARCH to your architecture (i386/x86_64/ppc)
#endif

#define HTTPHOST "http://nightly.siedler25.org/s25client/"
#define STABLEPATH "stable/"
#define NIGHTLYPATH "nightly/"
#define FILEPATH "/updater"
#define FILELIST "/files"
#define LINKLIST "/links"
#define SAVEGAMEVERSION "/savegameversion"

#ifndef SEE_MASK_NOASYNC
#define SEE_MASK_NOASYNC 0x00000100
#endif

#ifdef _WIN32
/**
 *  \r fix-function for the stupid windows-console
 *  NOT THREADSAFE!!!
 */
static short backslashfix_y;

static short backslashrfix(short y)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsoleOutput;
    hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);

    COORD Cursor_an_Pos;
    Cursor_an_Pos.X = 0;
    Cursor_an_Pos.Y = csbi.dwCursorPosition.Y + y;
    SetConsoleCursorPosition(hConsoleOutput, Cursor_an_Pos);

    return csbi.dwCursorPosition.Y + y;
}

#endif // !_WIN32

/**
 *  curl filewriter callback
 */
static size_t WriteCallback(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    size_t realsize = size * nmemb;

    if(stream && realsize == fwrite(ptr, size, nmemb, stream))
        return realsize;

    return 0;
}

/**
 *  curl std::stringwriter callback
 */
static size_t WriteMemoryCallback(void* ptr, size_t size, size_t nmemb, std::string* data)
{
    size_t realsize = size * nmemb;

    std::string tmp(reinterpret_cast<char*>(ptr), realsize);
    *data += tmp;

    return realsize;
}

/**
 *  curl progressbar callback
 */
static int ProgressBarCallback(std::string* data, double dltotal, double dlnow, double /*ultotal*/, double /*ulnow*/)
{
#ifdef _WIN32
    // \r not working fix
    if(backslashrfix(0) != backslashfix_y)
        backslashfix_y = backslashrfix(-1);
#endif // !_WIN32

    bnw::cout << "\r" << *data;
    if(dltotal > 0) /* Avoid division by zero */
        bnw::cout << std::setw(5) << std::setprecision(2) << std::setiosflags(std::ios::fixed) << (dlnow * 100.0 / dltotal) << "%";
    bnw::cout << std::flush;

    return 0;
}

/**
 *  curl escape wrapper
 */
static std::string EscapeFile(const std::string& file)
{
    CURL* curl_handle;
    std::string result;

    curl_handle = curl_easy_init();
    char* escaped = curl_easy_escape(curl_handle, file.c_str(), static_cast<int>(file.length()));
    if(escaped)
    {
        result = escaped;
        curl_free(escaped);
    }

    curl_easy_cleanup(curl_handle);

    return result;
}

/**
 *  httpdownload function (to std::string or to file, with or without progressbar)
 */
static bool DownloadFile(const std::string& url, std::string& to, const std::string& path = "", std::string progress = "")
{
    CURL* curl_handle;
    FILE* tofp = nullptr;
    bool ok = true;

    std::string npath = path + ".new";

    curl_handle = curl_easy_init();

    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str()); //-V111
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "s25update/1.1");
    curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1);

    // Write file to Memory?
    if(path.empty())
    {
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback); //-V111
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, static_cast<void*>(&to)); //-V111
    } else
    {
        tofp = boost::nowide::fopen(npath.c_str(), "wb");
        if(!tofp)
        {
            bnw::cerr << "Can't open file \"" << npath << "\"!!!!" << std::endl;
            ok = false;
        }
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);        //-V111
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, static_cast<void*>(tofp)); //-V111
    }

    // Show Progress?
    if(!progress.empty())
    {
        curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, ProgressBarCallback);       //-V111
        curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, static_cast<void*>(&progress)); //-V111
    }

    // curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);

    if(ok && curl_easy_perform(curl_handle) != 0)
        ok = false;

    curl_easy_cleanup(curl_handle);

    if(tofp)
        fclose(tofp);

    if(ok)
        rename(npath.c_str(), path.c_str());

    return ok;
}

/**
 *  calculate md5sum for a file
 */
std::string md5sum(const std::string& file)
{
    std::string digest = "";

    FILE* fp = boost::nowide::fopen(file.c_str(), "rb");
    if(fp)
    {
        md5file(fp, digest);
        fclose(fp);
    }
    return digest;
}

#ifdef _WIN32
/**
 *  get the last error (win only)
 */
std::string get_last_error_string()
{
    LPVOID lpMsgBuf;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(),
                   0, // Default language
                   (LPSTR)&lpMsgBuf, 0, NULL);
    std::string result = (LPCSTR)lpMsgBuf;
    // Free the buffer.
    LocalFree(lpMsgBuf);
    return result;
}
#endif

// Checks the savegame version and return true if update can continue
bool ValidateSavegameVersion(const std::string& httpbase, const bfs::path& savegameversionFilePath)
{
    // check new savegame version before downloading
    std::string remote_savegameversion_content;
    if(!DownloadFile(httpbase + SAVEGAMEVERSION, remote_savegameversion_content))
    {
        bnw::cerr << "Error: Was not able to get remote savegame version, ignoring for now" << std::endl;
        return true;
        // return false; // uncomment this if it actually works (to not break updater now)
    }
    bnw::ifstream local_savegame_version(savegameversionFilePath);
    std::stringstream remote_savegame_version(remote_savegameversion_content);
    int localVersion, remoteVersion;
    if(!(local_savegame_version >> localVersion && remote_savegame_version >> remoteVersion))
    {
        local_savegame_version.seekg(0);
        std::string curVersion;
        local_savegame_version >> curVersion;
        bnw::cerr << "Error: Could not parse savegame versions" << std::endl
                  << "Current: " << curVersion << std::endl
                  << "Update:  " << remote_savegameversion_content << std::endl;
    } else
    {
        bnw::cout << "Savegame version of currently installed version: " << localVersion << std::endl;
        bnw::cout << "Savegame version of updated version: " << remoteVersion << std::endl;
        if(localVersion == remoteVersion)
        {
            bnw::cout << "You will be able to load your existing savegames." << std::endl;
            return true;
        }
        bnw::cout << "Warning: You will not be able to load your existing savegames. " << std::endl;
    }
    bnw::cout << "Cancel update? (y/n) ";
    auto input = static_cast<char>(std::cin.get());
    if(input != 'n' && input != 'N')
    {
        bnw::cout << std::endl
                  << "Canceling update." << std::endl
                  << "Warning: You will not be able to play with players using a newer version." << std::endl;
        return false;
    } else
    {
        bnw::cout << std::endl << "Continuing update." << std::endl;
        return true;
    }
}

bool isCurrentDirWritable()
{
#ifdef _WIN32
    HANDLE hFile = CreateFileW(L"write.test", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        if(GetLastError() != ERROR_ACCESS_DENIED)
            throw std::runtime_error(get_last_error_string());
        return false;
    } else
    {
        CloseHandle(hFile);
        DeleteFileW(L"write.test");
        return true;
    }
#else
    bnw::ofstream testFile("write.test", bnw::ofstream::trunc);
    if(testFile)
    {
        testFile.close();
        bfs::remove("write.test");
        return true;
    } else
        return false;
#endif
}

bool runAsAdmin(int argc, char* argv[])
{
#ifdef _WIN32
    std::stringstream arguments;
    for(int i = 1; i < argc; ++i)
        arguments << argv[i];

    // Launch itself as administrator.
    SHELLEXECUTEINFOA sei = {sizeof(sei)};
    sei.lpVerb = "runas";
    sei.lpFile = argv[0];
    sei.hwnd = GetConsoleWindow();
    sei.lpParameters = arguments.str().c_str();
    sei.nShow = SW_NORMAL;
    sei.fMask = SEE_MASK_NOASYNC;

    if(ShellExecuteExA(&sei))
        return true;
    DWORD dwError = GetLastError();
    if(dwError == ERROR_CANCELLED)
        bnw::cerr << "You refused to elevate - cannot update" << std::endl;
#endif
    return false;
}

void executeUpdate(int argc, char* argv[])
{
    bool updated = false;
    bool verbose = false;
    bool nightly = true;
    bfs::path workPath = bfs::path(argv[0]).parent_path().lexically_normal();

    // If the installation is the default one, update current installation
    // TODO: get these paths from cmake and implement some kind of automatic search?
#ifdef _WIN32
    bfs::path tmpPath = workPath.parent_path();
    if(bfs::exists(workPath.parent_path() / std::string("RTTR/s25update.exe")))
        workPath = tmpPath;
#elif defined(__APPLE__)
    bfs::path tmpPath = workPath.parent_path().parent_path().parent_path();
    if(bfs::exists(tmpPath / std::string("s25client.app/Contents/MacOS/s25update")))
        workPath = tmpPath;
#else
    bfs::path tmpPath = workPath.parent_path();
    if(bfs::exists(tmpPath / std::string("libexec/s25rttr/s25update")))
        workPath = tmpPath;
#endif

    if(argc > 1)
    {
        for(int i = 1; i < argc; ++i)
        {
            if(strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0)
                verbose = true;
            if(strcmp(argv[i], "--dir") == 0 || strcmp(argv[i], "-d") == 0)
                workPath = argv[++i];
            if(strcmp(argv[i], "--stable") == 0 || strcmp(argv[i], "-s") == 0)
                nightly = false;
        }
    }

    if(verbose)
        bnw::cout << "Using directory " << workPath << std::endl;
    boost::system::error_code error;
    bfs::current_path(workPath, error);
    if(error)
        bnw::cerr << "Warning: Failed to set working directory: " << error << std::endl;

    if(!isCurrentDirWritable())
    {
        if(runAsAdmin(argc, argv))
        {
            bnw::cout << "Update should have been run successfully" << std::endl;
            return;
        } else
            throw std::runtime_error("Update failed. Current dir is not writeable");
    }

    // initialize curl
    curl_global_init(CURL_GLOBAL_ALL);
    atexit(curl_global_cleanup);

    std::string httpbase = HTTPHOST;
    if(nightly)
        httpbase += NIGHTLYPATH;
    else
        httpbase += STABLEPATH;

    std::stringstream url;

    // download filelist
    url << httpbase << TARGET << "." << ARCH << FILEPATH << FILELIST;
    if(verbose)
        bnw::cout << "Requesting current version information from server..." << std::endl;
    std::string filelist;
    if(DownloadFile(url.str(), filelist))
    {
        url.str("");
        url << httpbase << TARGET << "." << ARCH << FILEPATH;
        httpbase = url.str();
    } else
    {
        bnw::cout << "Warning: Was not able to get current masterfile, trying older ones" << std::endl;

        bool ok = false;
        for(int i = 0; i < 5; ++i)
        {
            url.str("");
            url << httpbase << TARGET << "." << ARCH << "." << i + 1 << FILEPATH << FILELIST;
            if(DownloadFile(url.str(), filelist))
            {
                ok = true;

                // set base for later use
                url.str("");
                url << httpbase << TARGET << "." << ARCH << "." << i + 1 << FILEPATH;
                httpbase = url.str();

                break;
            }
            bnw::cout << "Warning: Was not able to get masterfile " << i + 1 << ", trying older one" << std::endl;
        }
        if(!ok)
            throw std::runtime_error("Could not get any master file");
    }

    // httpbase now includes targetpath and filepath

    // download linklist
    url.str("");
    url << httpbase << LINKLIST;
    std::string linklist;
    if(!DownloadFile(url.str(), linklist))
        bnw::cout << "Warning: Was not able to get linkfile, ignoring" << std::endl;

    std::stringstream flstream(filelist);

    if(verbose)
        bnw::cout << "Parsing update list..." << std::endl;
    // parse filelist
    std::vector<std::pair<std::string, std::string>> files;
    std::string line;
    bfs::path savegameversionFilepath;

    while(getline(flstream, line))
    {
        if(line.length() == 0)
            break;

        std::string hash = line.substr(0, 32);
        std::string file = line.substr(34);

        files.push_back(std::pair<std::string, std::string>(hash, file));

        if(file.find(SAVEGAMEVERSION) != std::string::npos)
            savegameversionFilepath = file;

        if(flstream.fail())
            break;
    }

    if(!savegameversionFilepath.empty() && bfs::exists(savegameversionFilepath))
    {
        if(!ValidateSavegameVersion(httpbase, savegameversionFilepath))
            return;
    }

    std::stringstream llstream(linklist);

    std::vector<std::pair<std::string, std::string>> links;
    // parse linklist
    while(getline(llstream, line))
    {
        if(line.length() == 0)
            break;

        std::string target = line.substr(line.find(' ') + 1);
        std::string source = line.substr(0, line.rfind(' '));

        links.push_back(std::pair<std::string, std::string>(source, target));

        if(llstream.fail())
            break;
    }

    // check md5 of files and download them
    for(auto& file : files)
    {
        std::string hash = file.first;
        bfs::path filePath = file.second;
        filePath.make_preferred();

        // check hash of file
        std::string nhash = md5sum(filePath.string());
        // bnw::cout << hash << " - " << nhash << std::endl;
        if(hash == nhash)
            continue;

        bfs::path name = filePath.filename();
        bfs::path path = filePath.parent_path();
        bfs::path bzfile = filePath;
        bzfile += ".bz2";

        bnw::cout << "Updating " << name;
        if(verbose)
            bnw::cout << " to " << path;
        bnw::cout << std::endl;

        // create path of file
        if(!bfs::is_directory(path))
        {
            try
            {
                bfs::create_directories(path);
            } catch(const std::exception& e)
            {
                std::stringstream msg;
                msg << "Failed to create directories to path " << path << " for " << name << ": " << e.what() << std::endl;
                throw std::runtime_error(msg.str());
            }
        }

        std::stringstream progress;
        progress << "Downloading " << name;
        while(progress.str().size() < 50)
            progress << " ";

        url.str("");
        bfs::path urlPath = bfs::path(file.second).parent_path();
        url << httpbase << "/" << urlPath.string() << "/" << EscapeFile(name.string()) << ".bz2";
        std::string fdata = "";

        // download the file
        bool dlOk = DownloadFile(url.str(), fdata, bzfile.string(), progress.str());

        bnw::cout << " - ";
        if(!dlOk)
        {
            bnw::cout << "failed!" << std::endl;
            throw std::runtime_error("Download of " + bzfile.string() + "failed!");
        }

        // extract the file
        int bzerror = BZ_OK;
        FILE* bzfp = boost::nowide::fopen(bzfile.string().c_str(), "rb");
        if(!bzfp)
            throw std::runtime_error("decompression failed: download failure?");

        bzerror = BZ_OK;
        BZFILE* bz2fp = BZ2_bzReadOpen(&bzerror, bzfp, 0, 0, nullptr, 0);
        if(!bz2fp)
            throw std::runtime_error("decompression failed: compressed file corrupt?");

        bnw::ofstream outputFile(filePath, bnw::ofstream::binary | bnw::ofstream::trunc);
        if(!outputFile)
        {
            boost::system::error_code error;
            bfs::path bakFilePath(filePath);
            bakFilePath += ".bak";
            bfs::rename(filePath, bakFilePath, error);
            // move file out of the way ...
            if(error)
                throw std::runtime_error("failed to move blocked file " + filePath.string() + " out of the way ...");
            outputFile.open(filePath, bnw::ofstream::binary | bnw::ofstream::trunc);
        }
        if(!outputFile)
            throw std::runtime_error("Failed to open output file " + filePath.string());

        while(bzerror == BZ_OK)
        {
            char buffer[1024];
            unsigned read = BZ2_bzRead(&bzerror, bz2fp, buffer, 1024);
            if(!outputFile.write(buffer, read))
                bnw::cerr << "failed to write to disk" << std::endl;
        }

        bnw::cout << "ok";

        BZ2_bzReadClose(&bzerror, bz2fp);
        fclose(bzfp);

        // remove compressed file
        bfs::remove(bzfile);

        bnw::cout << std::endl;

        updated = true;
#ifdef _WIN32
        // \r not working fix
        backslashfix_y = backslashrfix(0);
#endif // !_WIN32
    }

    if(verbose)
        bnw::cout << "Updating folder structure..." << std::endl;

    for(auto& link : links)
    {
    // Note: Symlink = it->first pointing to it->second (it->second) exists
#ifdef _WIN32
        bnw::cout << "Copying file " << it->second << std::endl;
        bfs::path path = bfs::path(it->first).parent_path();
        bfs::path srcFilepath = path / it->second;
        boost::system::error_code ec;
        bfs::copy_file(srcFilepath, it->first, bfs::copy_option::overwrite_if_exists, ec);
        if(ec)
            bnw::cerr << "Failed to copy file '" << srcFilepath << "' to '" << it->first << "': " << ec.message() << std::endl;
#else
        bnw::cout << "creating symlink " << link.first << std::endl;
        if(!bfs::exists(link.first))
        {
            boost::system::error_code ec;
            bfs::create_symlink(link.second, link.first, ec);
            if(ec)
                bnw::cerr << "Failed to create symlink: '" << link.first << "' to '" << link.second << "': " << ec.message() << std::endl;
        }
#endif
    }

    if(updated)
        bnw::cout << "Update finished!" << std::endl;
}

/**
 *  main function
 */
int main(int argc, char* argv[])
{
    try
    {
        executeUpdate(argc, argv);
    } catch(const std::exception& e)
    {
        bnw::cerr << "Update failed: " << e.what() << std::endl;
        return 1;
    }

#if defined _DEBUG && defined _MSC_VER
    bnw::cout << "Press return to continue . . ." << std::flush;
    std::cin.get();
#endif

    return 0;
}
