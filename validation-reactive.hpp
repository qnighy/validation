// validation-reactive.hpp: Masaki Hara
#ifndef VALIDATION_REACTIVE_HPP
#define VALIDATION_REACTIVE_HPP

#include "validation.hpp"

#if defined(_WIN32) && !defined(__unix__)
#include <io.h>
#include <fcntl.h>

inline void DisplayError(const char *pszAPI)
{
  LPVOID lpvMessageBuffer;
  CHAR szPrintBuffer[512];
  DWORD nCharsWritten;

  FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&lpvMessageBuffer, 0, NULL);

  wsprintf(szPrintBuffer,
      "ERROR: API    = %s.\n   error code = %d.\n   message    = %s.\n",
      pszAPI, GetLastError(), (char *)lpvMessageBuffer);

  WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),szPrintBuffer,
      lstrlen(szPrintBuffer),&nCharsWritten,NULL);

  LocalFree(lpvMessageBuffer);
  ExitProcess(GetLastError());
}
#else
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#endif

namespace validation {
  class Process : public Reader {
    FILE *write_file;
#if defined(_WIN32) && !defined(__unix__)
    HANDLE hChildProcess;
    public:
    void executeProcess(const char *file, char *const argv[]) {
      SECURITY_ATTRIBUTES sa;
      sa.nLength = sizeof(SECURITY_ATTRIBUTES);
      sa.lpSecurityDescriptor = NULL;
      sa.bInheritHandle = TRUE;

      HANDLE hOutputReadTmp,hOutputRead,hOutputWrite;
      if(!CreatePipe(&hOutputReadTmp,&hOutputWrite,&sa,0)) DisplayError("CreatePipe");
      if (!DuplicateHandle(GetCurrentProcess(),hOutputReadTmp,
            GetCurrentProcess(),
            &hOutputRead,
            0,FALSE,
            DUPLICATE_SAME_ACCESS))
        DisplayError("DupliateHandle");
      if (!CloseHandle(hOutputReadTmp)) DisplayError("CloseHandle");


      HANDLE hInputWriteTmp,hInputWrite,hInputRead;
      if(!CreatePipe(&hInputRead,&hInputWriteTmp,&sa,0)) DisplayError("CreatePipe");
      if (!DuplicateHandle(GetCurrentProcess(),hInputWriteTmp,
            GetCurrentProcess(),
            &hInputWrite,
            0,FALSE,
            DUPLICATE_SAME_ACCESS))
        DisplayError("DupliateHandle");
      if (!CloseHandle(hInputWriteTmp)) DisplayError("CloseHandle");

      PROCESS_INFORMATION pi;
      STARTUPINFO si;

      // Set up the start up info struct.
      ZeroMemory(&si,sizeof(STARTUPINFO));
      si.cb = sizeof(STARTUPINFO);
      si.dwFlags = STARTF_USESTDHANDLES;
      si.hStdOutput = hOutputWrite;
      si.hStdInput  = hInputRead;
      si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

      if (!CreateProcess(NULL,(LPSTR)file,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi))
        DisplayError("CreateProcess");

      hChildProcess = pi.hProcess;
      if (!CloseHandle(pi.hThread)) DisplayError("CloseHandle");
      if (!CloseHandle(hOutputWrite)) DisplayError("CloseHandle");
      if (!CloseHandle(hInputRead )) DisplayError("CloseHandle");

      int write_fd = _open_osfhandle((intptr_t)hInputWrite, _O_WRONLY);
      if(write_fd == 0) DisplayError("_open_osfhandle");
      write_file = _fdopen(write_fd, "w");
      if(write_file == 0) DisplayError("_fdopen");

      int read_fd = _open_osfhandle((intptr_t)hOutputRead, _O_RDONLY);
      if(read_fd == 0) DisplayError("_open_osfhandle");
      FILE *read_file = _fdopen(read_fd, "r");
      if(read_file == 0) DisplayError("_fdopen");
      setReadFile(read_file, file);

    }
    ~Process() {
      WaitForSingleObject(hChildProcess,INFINITE);
      CloseHandle(hChildProcess);
    }
#else
    pid_t pid;
    public:
    void executeProcess(const char *file, char *const argv[]) {
      int pipe_c2p[2], pipe_p2c[2];

      signal(SIGPIPE, SIG_IGN);
      if (pipe(pipe_c2p) < 0 || pipe(pipe_p2c) < 0) {
        err(1, NULL);
      }
      if ((pid = fork()) < 0) {
        err(1, NULL);
      }
      if (pid == 0) {
        close(pipe_p2c[1]); close(pipe_c2p[0]);
        dup2(pipe_p2c[0], 0); dup2(pipe_c2p[1], 1);
        close(pipe_p2c[0]); close(pipe_c2p[1]);
        execvp(file, argv);
        err(1, NULL);
      }
      close(pipe_p2c[0]); close(pipe_c2p[1]);
      write_file = fdopen(pipe_p2c[1], "w");
      setReadFile(fdopen(pipe_c2p[0], "r"), file);
    }
    ~Process() {
      int status;
      fclose(write_file);
      waitpid(pid, &status, WUNTRACED);
      if(status != 0) {
        fprintf(stderr, "waitpid: returned status nonzero\n");
        exit(1);
      }
    }
#endif
    Process() {
    }
    Process(const char *file, char *const argv[]) {
      executeProcess(file, argv);
    }
#ifdef __GNUC__
    int printf(const char *format, ...) __attribute__ ((format (printf, 2, 3))) {
#else
    int printf(const char *format, ...) {
#endif
      va_list ap;
      va_start(ap, format);
      int ret = vfprintf(write_file, format, ap);
      va_end(ap);
      return ret;
    }
    void flush() {
      fflush(write_file);
    }
  };

  void initReactive(Process &p, int argc, char **argv) {
    if(argc < 2) {
      fprintf(stderr, "usage: %s <command..>\n", argv[0]);
      exit(1);
    }
    p.executeProcess(argv[1], argv+1);
  }
}
#endif
