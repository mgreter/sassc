#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS 1
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sass.h>
#include "sassc_version.h"

#define BUFSIZE 512
#ifdef _WIN32
#define PATH_SEP ';'
#else
#define PATH_SEP ':'
#endif

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <wincon.h>

#define isatty(h) _isatty(h)
#define fileno(m) _fileno(m)

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
    argv[i] = (char*) argv + offset;
    offset += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1,
      argv[i], size-offset, 0, 0);
  }
  *argc_ptr = argc;
  *argv_ptr = argv;
  return 0;
}
#else
#include <unistd.h>
#include <sysexits.h>
#endif

int output(struct SassCompiler* compiler, const char* outfile, bool quiet)
{

  struct SassError* error = sass_compiler_get_error(compiler);

  const char* warnings = sass_compiler_get_stderr(compiler);

  /*
  printf("Got traces: %d\n", sass_error_count_traces(error));

  struct SassTrace* trace0 = sass_error_get_trace(error, 0);



  struct SassSrcSpan* pstate0 = sass_trace_get_srcspan(trace0);

  printf("Span[0]: %d\n", sass_srcspan_get_src_ln(pstate0));
  printf("Span[0]: %d\n", sass_srcspan_get_src_col(pstate0));
  printf("Span[0]: %d\n", sass_srcspan_get_span_ln(pstate0));
  printf("Span[0]: %d\n", sass_srcspan_get_span_col(pstate0));
  printf("Span[0]: %d\n", sass_srcspan_get_src_line(pstate0));
  printf("Span[0]: %d\n", sass_srcspan_get_src_column(pstate0));

  struct SassSource* source0 = sass_srcspan_get_source(pstate0);
  
  printf("Source[0]: %s\n", sass_source_get_abs_path(source0));
  printf("Source[0]: %s\n", sass_source_get_imp_path(source0));
  printf("Source[0]: %s\n", sass_source_get_content(source0));
  printf("Source[0]: %s\n", sass_source_get_srcmap(source0));

  struct SassTrace* trace1 = sass_error_get_trace(error, 1);
  struct SassSrcSpan* pstate1 = sass_trace_get_srcspan(trace1);
  struct SassSource* source1 = sass_srcspan_get_source(pstate1);

  printf("Source[1]: %s\n", sass_source_get_abs_path(source1));
  printf("Source[1]: %s\n", sass_source_get_imp_path(source1));
  printf("Source[1]: %s\n", sass_source_get_content(source1));
  printf("Source[1]: %s\n", sass_source_get_srcmap(source1));

  exit(1);
  */
  // First print out all warnings
  // if (!quiet && sass_error_get_warnings(error)) {
  //   sass_print_stderr(sass_error_get_warnings(error));
  // }
  // Then print out all error messages
  if (!quiet && sass_compiler_get_stderr(compiler)) {
    sass_print_stderr(sass_compiler_get_stderr(compiler));
  }
  // Finally the formatted error message
  if (sass_error_get_formatted(error)) {
    sass_print_stderr(sass_error_get_formatted(error));
  }

  // Write to output if no errors occurred
  if (sass_error_get_status(error) == 0) {

    // Get the parts to be added to the output file (or stdout)
    const char* content = sass_compiler_get_output_string(compiler);
    const char* footer = sass_compiler_get_footer_string(compiler);
    
    if (content || footer) {
      if (outfile) {
        FILE* fd = fopen(outfile, "wb");
        if (!fd) {
          perror("Error opening output file");
          return 1;
        }
        // Seems already set (makes sense)
        // setvbuf(fp, 0, _IOFBF, BUFSIZ);
        if (content && fprintf(fd, "%s", content) < 0) {
          perror("Error writing to output file");
          fclose(fd);
          return 1;
        }
        if (footer && fprintf(fd, "%s", footer) < 0) {
          perror("Error writing to output file");
          fclose(fd);
          return 1;
        }
        fclose(fd);
      }
      else {
        #ifdef _WIN32
        // deliberately ignore the return value
        (void)setmode(fileno(stdout), O_BINARY);
        #endif
        if (content) printf("%s", content);
        if (footer) printf("%s", footer);
      }
    }

  }

  // Return the original error status
  return sass_error_get_status(error);

}
// EO output

int compile_import(struct SassCompiler* compiler, const char* outfile, bool quiet)
{
  sass_compiler_set_output_path(compiler, outfile);
  if (sass_compiler_parse(compiler)) {
    if (sass_compiler_compile(compiler)) {
      sass_compiler_render(compiler);
    }
  }
  int rv = output(compiler, outfile, quiet);
  sass_delete_compiler(compiler);
  return rv;
}

struct
{
  char* string;
  int type;
}
style_option_strings[] = {
  { "compressed", SASS_STYLE_COMPRESSED },
  { "compact", SASS_STYLE_COMPACT },
  { "expanded", SASS_STYLE_EXPANDED },
  { "nested", SASS_STYLE_NESTED }
};

#define NUM_STYLE_OPTION_STRINGS \
    sizeof(style_option_strings) / sizeof(style_option_strings[0])

struct
{
  char* string;
  int type;
}
import_format_strings[] = {
  { "auto", SASS_IMPORT_AUTO },
  { "css", SASS_IMPORT_CSS },
  { "sass", SASS_IMPORT_SASS },
  { "scss", SASS_IMPORT_SCSS }
};

#define NUM_IMPORT_FORMAT_STRINGS \
    sizeof(import_format_strings) / sizeof(import_format_strings[0])

struct
{
  char* string;
  int type;
}
src_map_mode_strings[] = {
  { "none", SASS_SRCMAP_NONE },
  { "create", SASS_SRCMAP_CREATE },
  { "link", SASS_SRCMAP_EMBED_LINK },
  { "embed", SASS_SRCMAP_EMBED_JSON }
};

#define NUM_SRC_MAP_MODE_STRINGS \
    sizeof(src_map_mode_strings) / sizeof(src_map_mode_strings[0])

void print_version() {
    printf("sassc: %s\n", SASSC_VERSION);
    printf("libsass: %s\n", libsass_version());
    printf("sass: %s\n", libsass_language_version());
}

void print_usage(char* argv0) {
    printf("Usage: %s [options] [INPUT] [OUTPUT]\n\n", argv0);
    printf("Options:\n");
    printf("   -s, --stdin                Read input from standard input instead of an input file.\n");
    printf("   -t, --style=NAME           Set output style (nested, expanded, compact or compressed).\n");
    printf("   -f, --format[=TYPE]        Set explicit input syntax (scss, sass, css or auto).\n");
    printf("   -l, --line-comments        Emit comments showing original line numbers.\n");
    printf("   -I, --include-path PATH    Add include path to look for imports.\n");
    printf("   -P, --plugin-path PATH     Add plugin path to auto load plugins.\n");
    printf("   -m, --sourcemap[=TYPE]     Create and emit source mappings.\n");
    printf("         [TYPE] can be create, link (default) or embed.\n");
    printf("       --sourcemap-file-urls  Emit absolute file:// urls in includes array.\n");
    printf("   -C  --sourcemap-contents   Embed contents of imported files in source map.\n");
    printf("   -M  --sourcemap-path       Set path where source map file is saved.\n");
    printf("   -p, --precision         Set the precision for numbers.\n");
    printf("   -v, --version           Display compiled versions.\n");
    printf("   -h, --help              Display this help message.\n");
    printf("\n");
}

void invalid_usage(char* argv0) {
    fprintf(stderr, "See '%s --help'\n", argv0);
    #ifdef _WIN32
        exit(ERROR_BAD_ARGUMENTS); // One or more arguments are not correct.
    #else
        exit(EX_USAGE); // command line usage error
    #endif

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

  if ((argc == 1) && isatty(fileno(stdin))) {
    print_usage(argv[0]);
    return 0;
  }

  struct SassCompiler* compiler = sass_make_compiler();

  // Massively increase output speed!
  setvbuf(stderr, 0, _IOFBF, BUFSIZ);
  setvbuf(stdout, 0, _IOLBF, BUFSIZ);


  int c;
  size_t i;
  int long_index = 0;
  bool quiet = false;
  bool from_stdin = false;
  bool auto_source_map = false;
  bool generate_source_map = false;
  bool source_map_embed = false;
  bool omit_source_map_url = false;

  // LoggerStyle
  static struct option long_options[] =
  {
      { "help",                 no_argument,       0, 'h' },
      { "quiet",                no_argument,       0, 'q' },
      { "version",              no_argument,       0, 'v' },
      { "stdin",                no_argument,       0, 's' },
      { "line-numbers",         no_argument,       0, 'l' },
      { "line-comments",        no_argument,       0, 'l' },
      { "sourcemap",            optional_argument, 0, 'm' },
      { "sourcemap-file-urls",  no_argument,       0, 'F' },
      { "sourcemap-contents",   no_argument,       0, 'C' },
      { "sourcemap-path",       required_argument, 0, 'M' },
      { "format",               required_argument, 0, 'f' },
      { "include-path",         required_argument, 0, 'I' },
      { "plugin-path",          required_argument, 0, 'P' },
      { "style",                required_argument, 0, 't' },
//      { "omit-map-comment",     no_argument,       0, 'M' },
      { "precision",            required_argument, 0, 'p' },
      { NULL,                   0,                 NULL, 0}
  };

  enum SassImportFormat format = SASS_IMPORT_AUTO;

  // optstring is a string containing the legitimate option characters.
  // If such a character is followed by a colon, the option requires an
  // argument, so getopt() places a pointer to the following text in the
  // same argv - element, or the text of the following argv - element, in
  // optarg. Two colons mean an option takes an optional arg; if there is
  // text in the current argv - element(i.e., in the same word as the
  // option name itself, for example, "-oarg"), then it is returned in
  // optarg, otherwise optarg is set to zero. This is a GNU extension.
  // If optstring contains W followed by a semicolon, then - W foo is
  // treated as the long option --foo. (The - W option is reserved by
  // POSIX.2 for implementation extensions.) This behavior is a GNU
  // extension, not available with libraries before glibc 2.
  while ((c = getopt_long(argc, argv, "hvslFCm::M:f:I:P:t:p:", long_options, &long_index)) != -1)
  {
    switch (c) {
    case 's':
      from_stdin = true;
      break;
    case 'q':
      quiet = true;
      break;
    case 'I':
      sass_compiler_add_include_paths(compiler, optarg);
      break;
    case 'P':
      sass_compiler_load_plugins(compiler, optarg);
      break;

    case 'F':compiler, 
      sass_compiler_set_srcmap_file_urls(compiler, true);
      break;
    case 'C':
      sass_compiler_set_srcmap_embed_contents(compiler, true);
      break;
    case 'M':
      sass_compiler_set_srcmap_path(compiler, optarg);
      break;

    case 'f':
      // Search for the string in the available options
      for (i = 0; i < NUM_IMPORT_FORMAT_STRINGS; ++i) {
        if (strcmp(optarg, import_format_strings[i].string) == 0) {
          format = import_format_strings[i].type;
          break;
        }
      }
      // Check if we didn't find a valid one
      if (i == NUM_IMPORT_FORMAT_STRINGS) {
        fprintf(stderr, "Invalid argument for -f flag: '%s'. Allowed arguments are:", optarg);
        for (i = 0; i < NUM_IMPORT_FORMAT_STRINGS; ++i) {
          fprintf(stderr, i > 0 ? ", %s" : " %s",
            import_format_strings[i].string);
        }
        fprintf(stderr, "\n");
        invalid_usage(argv[0]);
      }
      break;
    case 't':
      // Search for the string in the available options
      for (i = 0; i < NUM_STYLE_OPTION_STRINGS; ++i) {
        if (strcmp(optarg, style_option_strings[i].string) == 0) {
          sass_compiler_set_output_style(compiler, style_option_strings[i].type);
          break;
        }
      }
      // Check if we didn't find a valid one
      if (i == NUM_STYLE_OPTION_STRINGS) {
        fprintf(stderr, "Invalid argument for -t flag: '%s'. Allowed arguments are:", optarg);
        for (i = 0; i < NUM_STYLE_OPTION_STRINGS; ++i) {
          fprintf(stderr, i > 0 ? ", %s" : " %s",
            style_option_strings[i].string);
        }
        fprintf(stderr, "\n");
        invalid_usage(argv[0]);
      }
      break;
    case 'l':
      // sass_compiler_set_source_comments(compiler, true);
      break;
    case 'm':
      if (optarg) { // optional argument
        // Search for the string in the available options
        for (i = 0; i < NUM_SRC_MAP_MODE_STRINGS; ++i) {
          if (strcmp(optarg, src_map_mode_strings[i].string) == 0) {
            sass_compiler_set_srcmap_mode(compiler, src_map_mode_strings[i].type);
            break;
          }
        }
        // Check if we didn't find a valid one
        if (i == NUM_SRC_MAP_MODE_STRINGS) {
          fprintf(stderr, "Invalid argument for -m flag: '%s'. Allowed arguments are:", optarg);
          for (i = 0; i < NUM_SRC_MAP_MODE_STRINGS; ++i) {
            fprintf(stderr, i > 0 ? ", %s" : " %s",
              src_map_mode_strings[i].string);
          }
          fprintf(stderr, "\n");
          invalid_usage(argv[0]);
        }
      }
      else {
        sass_compiler_set_srcmap_mode(compiler,
          SASS_SRCMAP_CREATE);
      }

      generate_source_map = true;

      break;
    case 'p':
      sass_compiler_set_precision(compiler, atoi(optarg)); // TODO: make this more robust
      if (sass_compiler_get_precision(compiler) < 0) sass_compiler_set_precision(compiler, 10);
      break;
    case 'v':
      print_version();
      sass_delete_compiler(compiler);
      return 0;
    case 'h':
      print_usage(argv[0]);
      sass_delete_compiler(compiler);
      return 0;
    case '?':
      /* Unrecognized flag or missing an expected value */
      /* getopt should produce it's own error message for this case */
      invalid_usage(argv[0]);
    default:
      fprintf(stderr, "Unknown error while processing arguments\n");
      sass_delete_compiler(compiler);
      return 2;
    }
  }

    if (optind < argc - 2) {
      fprintf(stderr, "Error: Too many arguments.\n");
      sass_delete_compiler(compiler);
      invalid_usage(argv[0]);
    }










    int result = 0;
    char* outfile = 0;
    const char* dash = "-";
    char* source_map_file = 0;
    struct SassImport* import = 0;
    if (optind < argc && strcmp(argv[optind], dash) != 0 && !from_stdin) {
      if (optind + 1 < argc) {
        outfile = argv[optind + 1];
      }
      if (generate_source_map && outfile) {
        const char* extension = ".map";
        source_map_file = calloc(strlen(outfile) + strlen(extension) + 1, sizeof(char));
        // ToDo this on the C++ side!
        strcpy(source_map_file, outfile);
        strcat(source_map_file, extension);
        // sass_context_set_source_map_file(context, source_map_file);
      }
      else if (auto_source_map) {
        // sass_context_set_source_map_embed(context, true);
        source_map_embed = true;
      }

      import = sass_make_file_import(compiler, argv[optind]);

    }
    else {
      if (optind < argc) {
        outfile = argv[optind];
      }

      import = sass_make_stdin_import();
    }

    sass_import_set_format(import, format);
    sass_compiler_set_entry_point(compiler, import);
    result = compile_import(compiler, outfile, quiet);
    sass_delete_compiler(compiler);

    #ifdef _WIN32
        return result ? ERROR_INVALID_DATA : 0; // The data is invalid.
    #else
        return result ? EX_DATAERR : 0; // data format error
    #endif
}
