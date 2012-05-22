// validation.hpp: Masaki Hara
#ifndef VALIDATION_HPP
#define VALIDATION_HPP

#if !defined(__cplusplus)
#error "validation.hpp supports only C++."
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>

#if defined(_WIN32) && !defined(__unix__)
#include <Windows.h>

inline int err(int c,const char *pszAPI)
{
  perror(pszAPI);
  exit(c);
  return c;
}
#else
#include <err.h>
#endif

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlong-long"
#endif

namespace validation {
  const int DELIM_SPACE = ' ';
  const int DELIM_EOL = '\n';
  inline int delim_array(int i, int n) { return i+1<n ? ' ' : '\n'; }
  const int INT_MAX_DECIMAL_U = 214748364;
  const int INT_MAX_DECIMAL_L = 7;
  const int INT_MIN_DECIMAL_U = -214748364;
  const int INT_MIN_DECIMAL_L = -8;
  const long long LLONG_MAX_DECIMAL_U = 922337203685477580LL;
  const int LLONG_MAX_DECIMAL_L = 7;
  const long long LLONG_MIN_DECIMAL_U = -922337203685477580LL;
  const int LLONG_MIN_DECIMAL_L = -8;
  const char * const CapitalAlphabets = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char * const SmallAlphabets = "abcdefghijklmnopqrstuvwxyz";
  const char * const AllAlphabets = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  const char * const AlphaNumerics = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  const char * const DecimalDigits = "0123456789";
#ifdef __GNUC__
  inline int parseError(const char *format, ...) __attribute__ ((format (printf, 1, 2))) __attribute__((noreturn));
#endif
  inline int parseError(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    printf("NG\n");
    exit(1);
  }
  class Reader {
    FILE *file;
    const char *filename;
    char lastchar;
    int line,col;
    int readChar() {
      int ret = fgetc(file);
      if(ferror(file)) {
        err(1, NULL);
      }
#if defined(_WIN32) && !defined(__unix__)
      if(ret=='\r') {
        ret = fgetc(file);
        if(ferror(file)) {
          err(1, NULL);
        }
      }
#endif
      if(lastchar=='\n'){line++,col=1;}else{col++;}
      lastchar = ret;
      return ret;
    }
    int readWordImpl(char *s, int &delimChar, const char *strfmt, int lo, int hi, const char *format, va_list ap) {
      bool strfmt2[256] = {};
      for(int i = 0; strfmt[i]; i++) strfmt2[(int)strfmt[i]] = true;
      int i;
      for(i = 0; ; i++) {
        int c = readChar();
        if(c == -1 || !strfmt2[c]) {
          delimChar = c;
          if(s) s[i] = '\0';
          if(i < lo) {
            fprintf(stderr, "%s(%d,%d): error reading word ", filename, line, col);
            vfprintf(stderr, format, ap);
            parseError(": Too short word\n");
          } else {
            return i;
          }
        }
        if(i == hi) {
          fprintf(stderr, "%s(%d,%d): error reading word ", filename, line, col);
          vfprintf(stderr, format, ap);
          parseError(": Too long word\n");
        } else {
          if(s) s[i] = c;
        }
      }
    }
    void readIntImpl(int &i, int &delimChar, int lo, int hi, const char *format, va_list ap) {
      int c = readChar();
      if(c == '-') {
        c = readChar();
        if('1' <= c && c <= '9') {
          i = - (c - '0');
          for(;;) {
            c = readChar();
            if('0' <= c && c <= '9') {
              if((i == INT_MIN_DECIMAL_U && - (c - '0') < INT_MIN_DECIMAL_L) || i < INT_MIN_DECIMAL_U) {
                fprintf(stderr, "%s(%d,%d): error reading int ", filename, line, col);
                vfprintf(stderr, format, ap);
                parseError(": Too large integer constant\n");
              }
              i = i*10 - (c - '0');
            } else {
              if(lo <= i && i <= hi) {
                delimChar = c;
                return;
              } else {
                fprintf(stderr, "%s(%d,%d): error reading int ", filename, line, col);
                vfprintf(stderr, format, ap);
                parseError(": integer %d out of range [%d,%d]\n", i, lo, hi);
              }
            }
          }
        }
      } else if(c == '0') {
        i = 0;
        c = readChar();
        if(lo <= i && i <= hi) {
          delimChar = c;
          return;
        } else {
          fprintf(stderr, "%s(%d,%d): error reading int ", filename, line, col);
          vfprintf(stderr, format, ap);
          parseError(": integer %d out of range [%d,%d]\n", i, lo, hi);
        }
      } else if('1' <= c && c <= '9') {
        i = (c - '0');
        for(;;) {
          c = readChar();
          if('0' <= c && c <= '9') {
            if((i == INT_MAX_DECIMAL_U && (c - '0') > INT_MAX_DECIMAL_L) || i > INT_MAX_DECIMAL_U) {
              fprintf(stderr, "%s(%d,%d): error reading int ", filename, line, col);
              vfprintf(stderr, format, ap);
              parseError(": Too large integer constant\n");
            }
            i = i*10 + (c - '0');
          } else {
            if(lo <= i && i <= hi) {
              delimChar = c;
              return;
            } else {
              fprintf(stderr, "%s(%d,%d): error reading int ", filename, line, col);
              vfprintf(stderr, format, ap);
              parseError(": integer %d out of range [%d,%d]\n", i, lo, hi);
            }
          }
        }
      } else {
        fprintf(stderr, "%s(%d,%d): error reading int ", filename, line, col);
        vfprintf(stderr, format, ap);
        parseError(": not an integer input\n");
      }
    }
    void readIntImpl(long long &i, int &delimChar, long long lo, long long hi, const char *format, va_list ap) {
      int c = readChar();
      if(c == '-') {
        c = readChar();
        if('1' <= c && c <= '9') {
          i = - (c - '0');
          for(;;) {
            c = readChar();
            if('0' <= c && c <= '9') {
              if((i == LLONG_MIN_DECIMAL_U && - (c - '0') < LLONG_MIN_DECIMAL_L) || i < LLONG_MIN_DECIMAL_U) {
                fprintf(stderr, "%s(%d,%d): error reading long long ", filename, line, col);
                vfprintf(stderr, format, ap);
                parseError(": Too large integer constant\n");
              }
              i = i*10 - (c - '0');
            } else {
              if(lo <= i && i <= hi) {
                delimChar = c;
                return;
              } else {
                fprintf(stderr, "%s(%d,%d): error reading long long ", filename, line, col);
                vfprintf(stderr, format, ap);
                parseError(": integer %lld out of range [%lld,%lld]\n", i, lo, hi);
              }
            }
          }
        }
      } else if(c == '0') {
        i = 0;
        c = readChar();
        if(lo <= i && i <= hi) {
          delimChar = c;
          return;
        } else {
          fprintf(stderr, "%s(%d,%d): error reading long long ", filename, line, col);
          vfprintf(stderr, format, ap);
          parseError(": integer %lld out of range [%lld,%lld]\n", i, lo, hi);
        }
      } else if('1' <= c && c <= '9') {
        i = (c - '0');
        for(;;) {
          c = readChar();
          if('0' <= c && c <= '9') {
            if((i == LLONG_MAX_DECIMAL_U && (c - '0') > LLONG_MAX_DECIMAL_L) || i > LLONG_MAX_DECIMAL_U) {
              fprintf(stderr, "%s(%d,%d): error reading long long ", filename, line, col);
              vfprintf(stderr, format, ap);
              parseError(": Too large integer constant\n");
            }
            i = i*10 + (c - '0');
          } else {
            if(lo <= i && i <= hi) {
              delimChar = c;
              return;
            } else {
              fprintf(stderr, "%s(%d,%d): error reading long long ", filename, line, col);
              vfprintf(stderr, format, ap);
              parseError(": integer %lld out of range [%lld,%lld]\n", i, lo, hi);
            }
          }
        }
      } else {
        fprintf(stderr, "%s(%d,%d): error reading long long ", filename, line, col);
        vfprintf(stderr, format, ap);
        parseError(": not an integer input\n");
      }
    }
    public:
    int readWord(char *s, const char *strfmt, int lo, int hi, int delimExpected, const char *format = "<?>", ...)
#ifdef __GNUC__
      __attribute__ ((format (printf, 7, 8)))
#endif
    {
      int delimChar = -1;
      va_list ap;
      va_start(ap, format);
      int len = readWordImpl(s,delimChar,strfmt,lo,hi,format,ap);
      va_end(ap);
      if(delimChar == delimExpected) {
        return len;
      } else {
        fprintf(stderr, "%s(%d,%d): error reading word ", filename, line, col);
        vfprintf(stderr, format, ap);
        parseError(": unexpected delimiter\n");
      }
    }
    int readWord2(char *s, int &delimChar, const char *strfmt, int lo, int hi, const char *format = "<?>", ...)
#ifdef __GNUC__
      __attribute__ ((format (printf, 7, 8)))
#endif
    {
      va_list ap;
      va_start(ap, format);
      int len = readWordImpl(s,delimChar,strfmt,lo,hi,format,ap);
      va_end(ap);
      return len;
    }
    int readInt(int lo, int hi, int delimExpected, const char *format = "<?>", ...)
#ifdef __GNUC__
      __attribute__ ((format (printf, 5, 6)))
#endif
    {
      int i = -1;
      int delimChar = -1;
      va_list ap;
      va_start(ap, format);
      readIntImpl(i,delimChar,lo,hi,format,ap);
      va_end(ap);
      if(delimChar == delimExpected) {
        return i;
      } else {
        fprintf(stderr, "%s(%d,%d): error reading int ", filename, line, col);
        vfprintf(stderr, format, ap);
        parseError(": unexpected delimiter\n");
      }
    }
    void readInt2(int &i, int &delimChar, int lo, int hi, const char *format = "<?>", ...)
#ifdef __GNUC__
      __attribute__ ((format (printf, 6, 7)))
#endif
    {
      va_list ap;
      va_start(ap, format);
      readIntImpl(i,delimChar,lo,hi,format,ap);
      va_end(ap);
    }
    long long readInt(long long lo, long long hi, int delimExpected, const char *format = "<?>", ...)
#ifdef __GNUC__
      __attribute__ ((format (printf, 5, 6)))
#endif
    {
      long long i = -1;
      int delimChar = -1;
      va_list ap;
      va_start(ap, format);
      readIntImpl(i,delimChar,lo,hi,format,ap);
      va_end(ap);
      if(delimChar == delimExpected) {
        return i;
      } else {
        fprintf(stderr, "%s(%d,%d): error reading long long ", filename, line, col);
        vfprintf(stderr, format, ap);
        parseError(": unexpected delimiter\n");
      }
    }
    void readInt2(long long &i, int &delimChar, long long lo, long long hi, const char *format = "<?>", ...)
#ifdef __GNUC__
      __attribute__ ((format (printf, 6, 7)))
#endif
    {
      va_list ap;
      va_start(ap, format);
      readIntImpl(i,delimChar,lo,hi,format,ap);
      va_end(ap);
    }
    void readEof() {
      if(readChar() != -1) {
        file = NULL;
        parseError("%s(%d,%d): error reading EOF: not an EOF\n", filename, line, col);
      }
      if(ferror(file)) {
        file = NULL;
        err(1, NULL);
      }
      if(fclose(file)==EOF) {
        file = NULL;
        err(1, NULL);
      }
      file = NULL;
    }
    void setReadFile(FILE *file,const char * filename) {
      this->file = file;
      this->filename = filename;
      this->lastchar = '\n';
      this->line = 0;
      this->col = 0;
    }
    void setReadFile(FILE *file) {
      if(file != stdin) {
        fprintf(stderr, "setReadFile(FILE* file): When file != stdin, you must specify filename!\n");
        exit(1);
      }
      setReadFile(file, "<stdin>");
    }
    void setReadFile(const char * filename) {
      FILE *file = fopen(filename, "r");
      if(!file) {
        err(1, NULL);
      }
      setReadFile(file, filename);
    }
    bool isFileSet() { return file; }
    Reader(FILE *file) : file(NULL) {
      setReadFile(file);
    }
    Reader(FILE *file, const char * filename) : file(NULL) {
      setReadFile(file, filename);
    }
    Reader(const char *filename) : file(NULL) {
      setReadFile(filename);
    }
    Reader() : file(NULL) {
    }
    ~Reader() {
      if(file) {
        fprintf(stderr, "%s: call readEof() before disposing!\n", filename);
        exit(1);
      }
    }
  };

  void initOutputChecker(Reader &in1, Reader &in2, int argc, char **argv) {
    if(argc != 3) {
      fprintf(stderr, "usage: %s <input> <output>\n", argv[0]);
      exit(1);
    }
    in1.setReadFile(argv[1]);
    in2.setReadFile(argv[2]);
  }
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif
