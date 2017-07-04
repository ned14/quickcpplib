/* Implements backtrace() et al from glibc on win64
(C) 2016-2017 Niall Douglas <http://www.nedproductions.biz/> (14 commits)
File Created: Mar 2016


Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License in the accompanying file
Licence.txt or at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


Distributed under the Boost Software License, Version 1.0.
    (See accompanying file Licence.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

#include "../include/execinfo_win64.h"

#include <stdlib.h>  // for abort
#include <string.h>

// To avoid including windows.h, this source has been macro expanded and win32 function shimmed for C++ only
#if defined(__cplusplus) && !defined(__clang__)
namespace win32
{
  extern "C" __declspec(dllimport) _Ret_maybenull_ void *__stdcall LoadLibraryA(_In_ const char *lpLibFileName);
  typedef int(__stdcall *GetProcAddress_returntype)();
  extern "C" GetProcAddress_returntype __stdcall GetProcAddress(_In_ void *hModule, _In_ const char *lpProcName);
  extern "C" __declspec(dllimport) _Success_(return != 0) unsigned short __stdcall RtlCaptureStackBackTrace(_In_ unsigned long FramesToSkip, _In_ unsigned long FramesToCapture, _Out_writes_to_(FramesToCapture, return ) void **BackTrace, _Out_opt_ unsigned long *BackTraceHash);
  extern "C" __declspec(dllimport) _Success_(return != 0)
  _When_((cchWideChar == -1) && (cbMultiByte != 0), _Post_equal_to_(_String_length_(lpMultiByteStr) + 1)) int __stdcall WideCharToMultiByte(_In_ unsigned int CodePage, _In_ unsigned long dwFlags, const wchar_t *lpWideCharStr, _In_ int cchWideChar, _Out_writes_bytes_to_opt_(cbMultiByte, return ) char *lpMultiByteStr,
                                                                                                                                            _In_ int cbMultiByte, _In_opt_ const char *lpDefaultChar, _Out_opt_ int *lpUsedDefaultChar);
}
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#ifdef __cplusplus
namespace
{
#endif

  typedef struct _IMAGEHLP_LINE64
  {
    unsigned long SizeOfStruct;
    void *Key;
    unsigned long LineNumber;
    wchar_t *FileName;
    unsigned long long int Address;
  } IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;

  typedef int(__stdcall *SymInitialize_t)(_In_ void *hProcess, _In_opt_ const wchar_t *UserSearchPath, _In_ int fInvadeProcess);

  typedef int(__stdcall *SymGetLineFromAddr64_t)(_In_ void *hProcess, _In_ unsigned long long int dwAddr, _Out_ unsigned long *pdwDisplacement, _Out_ PIMAGEHLP_LINE64 Line);

#if defined(__cplusplus) && !defined(__clang__)
  static void *dbghelp;
#else
static HMODULE dbghelp;
#endif
  static SymInitialize_t SymInitialize;
  static SymGetLineFromAddr64_t SymGetLineFromAddr64;

  static void load_dbghelp()
  {
#if defined(__cplusplus) && !defined(__clang__)
    using win32::LoadLibraryA;
    using win32::GetProcAddress;
#endif
    if(dbghelp)
      return;
    dbghelp = LoadLibraryA("DBGHELP.DLL");
    if(dbghelp)
    {
      SymInitialize = (SymInitialize_t) GetProcAddress(dbghelp, "SymInitializeW");
      if(!SymInitialize)
        abort();
      if(!SymInitialize((void *) (size_t) -1 /*GetCurrentProcess()*/, NULL, 1))
        abort();
      SymGetLineFromAddr64 = (SymGetLineFromAddr64_t) GetProcAddress(dbghelp, "SymGetLineFromAddrW64");
      if(!SymGetLineFromAddr64)
        abort();
    }
  }

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

_Check_return_ size_t backtrace(_Out_writes_(len) void **bt, _In_ size_t len)
{
#if defined(__cplusplus) && !defined(__clang__)
  using win32::RtlCaptureStackBackTrace;
#endif
  return RtlCaptureStackBackTrace(1, (unsigned long) len, bt, NULL);
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 6385 6386)  // MSVC static analyser can't grok this function. clang's analyser gives it thumbs up.
#endif
_Check_return_ _Ret_writes_maybenull_(len) char **backtrace_symbols(_In_reads_(len) void *const *bt, _In_ size_t len)
{
#if defined(__cplusplus) && !defined(__clang__)
  using win32::WideCharToMultiByte;
#endif
  size_t bytes = (len + 1) * sizeof(void *) + 256, n;
  if(!len)
    return NULL;
  else
  {
    char **ret = (char **) malloc(bytes);
    char *p = (char *) (ret + len + 1), *end = (char *) ret + bytes;
    if(!ret)
      return NULL;
    for(n = 0; n < len + 1; n++)
      ret[n] = NULL;
    load_dbghelp();
    for(n = 0; n < len; n++)
    {
      unsigned long displ;
      IMAGEHLP_LINE64 ihl;
      memset(&ihl, 0, sizeof(ihl));
      ihl.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
      int please_realloc = 0;
      if(!bt[n])
      {
        ret[n] = NULL;
      }
      else
      {
        // Keep offset till later
        ret[n] = (char *) ((char *) p - (char *) ret);
        if(!SymGetLineFromAddr64 || !SymGetLineFromAddr64((void *) (size_t) -1 /*GetCurrentProcess()*/, (size_t) bt[n], &displ, &ihl))
        {
          if(n == 0)
          {
            free(ret);
            return NULL;
          }
          ihl.FileName = (wchar_t *) L"unknown";
          ihl.LineNumber = 0;
        }
      retry:
        if(please_realloc)
        {
          char **temp = (char **) realloc(ret, bytes + 256);
          if(!temp)
          {
            free(ret);
            return NULL;
          }
          p = (char *) temp + (p - (char *) ret);
          ret = temp;
          bytes += 256;
          end = (char *) ret + bytes;
        }
        if(ihl.FileName && ihl.FileName[0])
        {
          int plen = WideCharToMultiByte(65001 /*CP_UTF8*/, 0, ihl.FileName, -1, p, (int) (end - p), NULL, NULL);
          if(!plen)
          {
            please_realloc = 1;
            goto retry;
          }
          p[plen - 1] = 0;
          p += plen - 1;
        }
        else
        {
          if(end - p < 16)
          {
            please_realloc = 1;
            goto retry;
          }
          _ui64toa_s((size_t) bt[n], p, end - p, 16);
          p = strchr(p, 0);
        }
        if(end - p < 16)
        {
          please_realloc = 1;
          goto retry;
        }
        *p++ = ':';
        _itoa_s(ihl.LineNumber, p, end - p, 10);
        p = strchr(p, 0) + 1;
      }
    }
    for(n = 0; n < len; n++)
    {
      if(ret[n])
        ret[n] = (char *) ret + (size_t) ret[n];
    }
    return ret;
  }
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

// extern void backtrace_symbols_fd(void *const *bt, size_t len, int fd);

#ifdef __cplusplus
}
#endif
