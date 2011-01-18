// $Id: main.cpp 6993 2011-01-18 17:55:58Z FloSoft $
//
// Copyright (c) 2005 - 2010 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "md5sum.h"

using namespace std;

#ifndef TARGET
#ifdef _WIN32
#	define TARGET "windows"
#endif

#ifdef __APPLE__
#	define TARGET "apple"
#endif

#ifdef __linux__
#	define TARGET "linux"
#endif
#endif

#ifndef TARGET
#	error You have to set TARGET to your platform (windows/linux/apple)
#endif

#ifdef _WIN32
#	define ARCH "i386"
#endif

#ifndef ARCH
#	error You have to set ARCH to your architecture (i386/x86_64/ppc)
#endif

#define HTTPHOST "http://nightly.ra-doersch.de/s25client/"
#define STABLEPATH "stable/"
#define HTTPPATH TARGET "." ARCH
#define FILELIST HTTPPATH "/files"
#define OLDFILELIST HTTPPATH ".old/files"

#define LINKLIST HTTPPATH "/links"
#define OLDLINKLIST HTTPPATH ".old/links"

#ifdef _WIN32
///////////////////////////////////////////////////////////////////////////////
/**
 *  \r fix-function for the stupid windows-console
 *  NOT THREADSAFE!!!
 *
 *  @author FloSoft
 */
static short backslashfix_y;

static short backslashrfix(short y)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hConsoleOutput;
	hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hConsoleOutput, &csbi);

	COORD Cursor_an_Pos = { 0, csbi.dwCursorPosition.Y + y};
	SetConsoleCursorPosition(hConsoleOutput , Cursor_an_Pos);

	return csbi.dwCursorPosition.Y + y;
}
#endif // !_WIN32

///////////////////////////////////////////////////////////////////////////////
/**
 *  replace all 'a' in a string 's' with 'b's
 *
 *  @author FloSoft
 */
string &replace_all(string &s, char a, char b)
{
	int idx;
	char bb[2] = {b, '\0'};
	while( (idx = s.find_first_of(a)) >= 0 ) {
		s.replace( idx, 1, bb );
	}
	return s;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  curl filewriter callback
 *
 *  @author FloSoft
 */
static size_t WriteCallback(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t realsize = size * nmemb;

	if(stream && realsize == fwrite(ptr, size, nmemb, stream))
		return realsize;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  curl stringwriter callback
 *
 *  @author FloSoft
 */
static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, string *data)
{
	size_t realsize = size * nmemb;

	char *cstr = new char[realsize + 1];
	memset(cstr, 0, realsize + 1);
	memcpy(cstr, ptr, realsize);
	*data += cstr;
	delete[] cstr;
	
	return realsize;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  create a directory-tree ( "mkdir -p" )
 *
 *  @author FloSoft
 */
static bool CreateDirRecursive(string dir)
{
	string::size_type npos = 0;
	while(npos != string::npos)
	{
		npos = dir.find('/', npos + 1);
		string ndir = (npos == string::npos ? dir : dir.substr(0, npos));

#ifdef _WIN32
		replace_all(ndir, '/', '\\');
#endif
		mkdir(ndir.c_str(), 0755);

		//cout << setw(npos) << "ndir: " << ndir << endl;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  curl progressbar callback
 *
 *  @author FloSoft
 */
static int ProgressBarCallback(string *data, double dltotal, double dlnow, double ultotal, double ulnow)
{
#ifdef _WIN32
	// \r not working fix
	if(backslashrfix(0) != backslashfix_y)
		backslashfix_y = backslashrfix(-1);
#endif // !_WIN32

	cout << "\r" << *data << setw(5) << setprecision(2) << setiosflags(ios:: fixed) << (dlnow * 100.0 / dltotal) << "%" << flush;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  httpdownload function (to string or to file, with or without progressbar)
 *
 *  @author FloSoft
 */
static bool DownloadFile(string url, string &to, string path = "", string progress = "")
{
	CURL *curl_handle;
	FILE *tofp = NULL;
	bool ok = true;

	curl_handle = curl_easy_init();

	curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "s25update/1.0");
	curl_easy_setopt(curl_handle, CURLOPT_FAILONERROR, 1);

	// Write file to Memory?
	if(path == "")
	{
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, static_cast<void *>(&to));
	}
	else
	{
		tofp = fopen(path.c_str(), "wb");
		if(!tofp)
			cout << "Can't open file!!!!" << endl;
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, static_cast<void *>(tofp));
	}

	// Show Progress?
	if(progress != "")
	{
		curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, ProgressBarCallback);
		curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, static_cast<void *>(&progress));
	}

	//curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 1L);
	
	if(curl_easy_perform(curl_handle) != 0)
		ok = false;
	
	curl_easy_cleanup(curl_handle);

	if(tofp)
		fclose(tofp);

	return ok;
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  calculate md5sum for a file
 *
 *  @author FloSoft
 */
string md5sum(string file)
{
	string digest = "";

	FILE *fp = fopen(file.c_str(), "rb");
	if(fp)
	{
		md5file(fp, digest);
		fclose(fp);
	}
	return digest;
}

#ifdef _WIN32
///////////////////////////////////////////////////////////////////////////////
/**
 *  prints the last error (win only)
 *
 *  @author FloSoft
 */
void print_last_error()
{
	LPVOID lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		0, // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
	// Process any inserts in lpMsgBuf.
	// ...
	// Display the string.
	std::cerr << (LPCTSTR)lpMsgBuf << std::endl;
	// Free the buffer.
	LocalFree( lpMsgBuf );
}
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  main function
 *
 *  @author FloSoft
 */
int main(int argc, char *argv[])
{
	bool verbose = false;
	bool nightly = true;
	string path = argv[0];
	path = path.substr(0, path.find_last_of("/\\"));

	if(argc > 1)
	{
		for(int i = 1; i < argc; ++i)
		{
			if(strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0 )
				verbose = true;
			if(strcmp(argv[i], "--dir") == 0 || strcmp(argv[i], "-d") == 0 )
				path = argv[++i];
			if(strcmp(argv[i], "--stable") == 0 || strcmp(argv[i], "-s") == 0 )
				nightly = false;
		}
	}

	string httpbase = HTTPHOST;
	if(!nightly)
		httpbase += STABLEPATH;

	if(chdir(path.c_str()) < 0)
		cerr << "Warning: Failed to set working directory: " << strerror(errno) << endl;

#ifdef _WIN32
	HANDLE hFile = CreateFile("write.test", GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		if(GetLastError() != ERROR_ACCESS_DENIED)
		{
			print_last_error();
			return 1;
		}

		std::cout << "Cannot write to update directory, trying to elevate to administrator" << std::endl;

		std::stringstream arguments;
		for(int i = 1; i < argc; ++i)
			arguments << argv[i];

		// Launch itself as administrator.
		SHELLEXECUTEINFO sei = { sizeof(sei) };
		sei.lpVerb = "runas";
		sei.lpFile = argv[0];
		sei.hwnd = GetConsoleWindow();
		sei.lpParameters = arguments.str().c_str();
		sei.nShow = SW_NORMAL;
		sei.fMask = SEE_MASK_NOASYNC;

		if (!ShellExecuteEx(&sei))
		{
			DWORD dwError = GetLastError();
			if (dwError == ERROR_CANCELLED)
			{
				std::cerr << "You refused to elevate - cannot update" << std::endl;
				return 1;
			}
		}

		std::cout << "Update should have been run successfully" << std::endl;
		return 0;
	}
	else
	{
		CloseHandle(hFile);
		DeleteFile("write.test");
	}
#endif

	// initialize curl
	curl_global_init(CURL_GLOBAL_ALL);
	atexit(curl_global_cleanup);

	string filelist, linklist;
	string hash, file;
	size_t longestname = 0,longestpath = 0;
	map<string,string> files;
	map<string,string> links;

	// download filelist
	if(!DownloadFile(httpbase + string(FILELIST), filelist))
	{
		cout << "Warning: Was not able to get current masterfile, trying old one" << endl;
		if(!DownloadFile(httpbase + string(OLDFILELIST), filelist))
		{
			cout << "Update failed: Downloading the masterfile was unsuccessful!" << endl;
	#if defined _DEBUG && defined _MSC_VER
			cout << "Press return to continue . . ." << flush;
			cin.get();
	#endif
			return 1;
		}
	}

	// download linklist
	if(!DownloadFile(httpbase + string(LINKLIST), linklist))
	{
		cout << "Warning: Was not able to get current master-link-file, trying old one" << endl;
		if(!DownloadFile(httpbase + string(OLDLINKLIST), linklist))
		{
			cout << "Update failed: Downloading the master-link-file was unsuccessful!" << endl;
	#if defined _DEBUG && defined _MSC_VER
			cout << "Press return to continue . . ." << flush;
			cin.get();
	#endif
			return 1;
		}
	}

	stringstream flstream(filelist);

	//cout << filelist << endl;
	
	// parse filelist
	string line;
	while( getline(flstream, line) )
	{
		if(line.length() == 0)
			break;

		hash = line.substr(0, 32);
		file = line.substr(34);

		files.insert(pair<string,string>(hash,file));
		
		if(flstream.fail())
			break;

		string name = file.substr(file.rfind('/') + 1);
		string path = file.substr(0, file.rfind('/'));

		// find longest name and path
		if(name.length() > longestname)
			longestname = name.length();
		if(path.length() > longestpath)
			longestpath = path.length();
	}

	stringstream llstream(linklist);

	//cout << linklist << endl;
	
	// parse linklist
	while( getline(llstream, line) )
	{
		if(line.length() == 0)
			break;

		string target = line.substr(line.find(' ') + 1);
		string source = line.substr(0, line.rfind(' '));

		links.insert(pair<string,string>(source,target));
		
		if(llstream.fail())
			break;
	}

	// check md5 of files and download them
	map<string,string>::iterator it = files.begin();
	while(it != files.end())
	{
		hash = it->first;
		file = it->second;
		++it;
		
		string tfile = file;
#ifdef _WIN32
		replace_all(tfile, '/', '\\');
#endif

		// check hash of file
		string nhash = md5sum(tfile);
		//cerr << hash << " - " << nhash << endl;
		if(hash != nhash)
		{
			string name = file.substr(file.rfind('/') + 1);
			string path = file.substr(0, file.rfind('/'));
			string bzfile = file + ".bz2";

			// create path of file
			CreateDirRecursive(path);

			stringstream progress;
			progress << "Updating \"" << /*setw(longestname) << */setiosflags(ios::left) << name << "\"";
			
			if(verbose)
				progress << " to \"" << /*setw(longestpath) << */path << "\"";

			progress << ": ";

			while(65 - progress.str().size() > 0)
				progress << " ";

			string url = httpbase + string(HTTPPATH) + "/" +  bzfile;
			string fdata = "";

#ifdef _WIN32
			replace_all(bzfile, '/', '\\');
#endif
			// download the file
			DownloadFile(url, fdata, bzfile, progress.str().c_str());

			cout << " - ";

			// extract the file
			int bzerror = BZ_OK;
			FILE *bzfp = fopen(bzfile.c_str(), "rb");
			if(!bzfp)
			{
				cerr << "decompression failed: download failure?" << endl;
				return 1;
			}
			else
			{
				bzerror = BZ_OK;
				BZFILE *bz2fp = BZ2_bzReadOpen( &bzerror, bzfp, 0, 0, NULL, 0);
				if(!bz2fp)
				{
					cout << "decompression failed: compressed file corrupt?" << endl;
					return 1;
				}
				else
				{
					FILE *fp = fopen(tfile.c_str(), "wb");
#ifdef _WIN32
					if(!fp)
					{
						// move file out of the way ...
						if(!MoveFileEx(tfile.c_str(), (tfile + ".bak").c_str(), MOVEFILE_REPLACE_EXISTING))
						{
							cout << "failed to move blocked file \"" << tfile << "\" out of the way ..." << endl;
							return 1;
						}
						fp = fopen(tfile.c_str(), "wb");
					}
#endif
					if(!fp)
					{
						cout << "decompression failed: compressed data corrupt?" << endl;
						return 1;
					}
					else
					{
						while(bzerror == BZ_OK)
						{
							char buffer[1024];
							unsigned int read = BZ2_bzRead ( &bzerror, bz2fp, buffer, 1024 );
							if(fwrite(buffer, 1, read, fp) != read)
								cout << "failed to write to disk";
						
						}
						fclose(fp);

						cout << "ok";
					}

					BZ2_bzReadClose(&bzerror, bz2fp);
				}
				fclose(bzfp);

				// remove compressed file
				unlink(bzfile.c_str());
			}

			cout << endl;
#ifdef _WIN32
			// \r not working fix
			backslashfix_y = backslashrfix(0);
#endif // !_WIN32
		}
	}

	it = links.begin();
	while(it != links.end())
	{
#ifdef _WIN32
		//cout << "creating file " << it->second << endl;
		string path = it->first.substr(0, it->first.rfind('/') + 1);
		string target = path + it->second;

		CopyFileA(it->first.c_str(), target.c_str(), FALSE);
#else
		cout << "creating symlink " << it->second << endl;
		int avoid_unused_retval_warn = symlink(it->second.c_str(), it->first.c_str());
		avoid_unused_retval_warn = 0;
#endif
		++it;
	}
		
#if defined _DEBUG && defined _MSC_VER
	cout << "Press return to continue . . ." << flush;
	cin.get();
#endif

	return 0;
}
