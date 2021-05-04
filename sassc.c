/*****************************************************************************/
/* Part of SassC, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS 1
#endif
#endif

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sass.h>
#include "sassc_version.h"

#ifdef _MSC_VER
#include <crtdbg.h>
/// AvoidMessageBoxHook - Emulates hitting "retry" from an "abort, retry,
/// ignore" CRT debug report dialog. "retry" raises a regular exception.
static int AvoidMessageBoxHook(int ReportType, char* Message, int* Return) {
  // Set *Return to the retry code for the return value of _CrtDbgReport:
  // http://msdn.microsoft.com/en-us/library/8hyw4sy7(v=vs.71).aspx
  // This may also trigger just-in-time debugging via DebugBreak().
  if (Return)
    *Return = 1;
  // Don't call _CrtDbgReport.
  return true;
}
#endif

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <wincon.h>

// Normalize input arguments to utf8 encoding
int get_argv_utf8(int* argc_ptr, char*** argv_ptr)
{
  int argc;
  char** argv;
  wchar_t** argv_utf16 = CommandLineToArgvW(GetCommandLineW(), &argc);
  int i;
  int offset = (argc + 1) * sizeof(char*);
  int size = offset;
  for (i = 0; i < argc; i++)
    size += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, 0, 0, 0, 0);
  // ToDo: is the ever freed?
  argv = malloc(size);
  // Will probably go undetected?
  if (argv == NULL) return 0;
  // Disable warnings since we correctly calculated
  // the necessary size before the allocation!
  #ifdef _MSC_VER
  #pragma warning(disable : 6385)
  #pragma warning(disable : 6386)
  #endif
  for (i = 0; i < argc; i++) {
    argv[i] = (char*)argv + offset;
    offset += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1,
      argv[i], size - offset, 0, 0);
  }
  *argc_ptr = argc;
  *argv_ptr = argv;
  return 0;
}
#else
#include <unistd.h>
#include <sysexits.h>
#endif

// Additional command to print all linked versions
void getopt_version_cmd(struct SassGetOpt* getopt, union SassOptionValue value)
{
  printf("sassc: %s\n", SASSC_VERSION);
  printf("libsass: %s\n", libsass_version());
  printf("sass: %s\n", libsass_language_version());
  exit(0);
}

// Additional command to print help page to screen
void getopt_help_cmd(struct SassGetOpt* getopt, union SassOptionValue value)
{
  char* usage = sass_getopt_get_help(getopt);
  fprintf(stderr, "Usage: sassc [options] INPUT_FILE [OUTPUT_FILE]\n");
  fprintf(stderr, "         set INPUT_FILE to '--' to read from stdin.\n");
  fprintf(stderr, "\nOptions:\n");
  sass_print_stderr(usage);
  sass_free_c_string(usage);
  exit(0);
}

int main(int argc, char** argv)
{
  #ifdef _MSC_VER
  _set_error_mode(_OUT_TO_STDERR);
  _set_abort_behavior(0, _WRITE_ABORT_MSG);
  #endif
  #ifdef _WIN32
  get_argv_utf8(&argc, &argv);
  SetConsoleOutputCP(65001);
  #endif

  // Massively increases output speed!
  setvbuf(stderr, 0, _IOFBF, BUFSIZ);
  setvbuf(stdout, 0, _IOLBF, BUFSIZ);

  // Create main compiler (also creates the logger)
  struct SassCompiler* compiler = sass_make_compiler();

  // Call auto-detection routines of LibSass
  sass_compiler_autodetect_logger_capabilities(compiler);

  // Create the option parser and attach compiler to it
  // Whenever we parse options it will setup the compiler.
  struct SassGetOpt* getopt = sass_make_getopt(compiler);

  // Populate GetOpt parser with defaults
  sass_getopt_populate_options(getopt);
  sass_getopt_populate_arguments(getopt);

  // Additional option parser target (just call me)
  sass_getopt_register_option(getopt, 'v', "version",
    "Display linked LibSass versions.",
    false, NULL, false, NULL,
    getopt_version_cmd);

  // Additional option parser target (just call me)
  sass_getopt_register_option(getopt, 'h', "help",
    "Show basic usage information.",
    false, NULL, false, NULL,
    getopt_help_cmd);

  // Now parse all passed arguments
  for (int i = 1; i < argc; i += 1) {
    sass_getopt_parse(getopt, argv[i]);
  }

  // Delete the parser to finalize parsing
  // Errors if we wait for required arguments
  sass_delete_getopt(getopt);

  // Execute all compiler steps and write/print results
  int result = sass_compiler_execute(compiler);

  // Everything done, now clean-up
  sass_delete_compiler(compiler);

  #ifdef _WIN32
    return result ? ERROR_INVALID_DATA : 0; // data invalid
  #else
    return result ? EX_DATAERR : 0; // data format error
  #endif
}
