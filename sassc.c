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

int get_argv_utf8(int* argc_ptr, char*** argv_ptr) {
  int argc;
  char** argv;
  wchar_t** argv_utf16 = CommandLineToArgvW(GetCommandLineW(), &argc);
  int i;
  int offset = (argc + 1) * sizeof(char*);
  int size = offset;
  for (i = 0; i < argc; i++)
    size += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, 0, 0, 0, 0);
  argv = malloc(size);
  if (argv == NULL) return 0;
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

int output(int error_status, const char* error_message, const char* output_string, const char* outfile) {
    if (error_status) {
        if (error_message) {
          // Only needed on windows!?
          sass_print_stderr(error_message);
          // fprintf(stderr, "%s", error_message);
        } else {
            fprintf(stderr, "An error occurred, but no reason was given.\n");
        }
        return 1;
    } else if (output_string) {
        if(outfile) {
            FILE* fp = fopen(outfile, "wb");
            if(!fp) {
                perror("Error opening output file");
                return 1;
            }
            if(fprintf(fp, "%s", output_string) < 0) {
                perror("Error writing to output file");
                fclose(fp);
                return 1;
            }
            fclose(fp);
        }
        else {
            #ifdef _WIN32
              setmode(fileno(stdout), O_BINARY);
            #endif
            printf("%s", output_string);
        }
        return 0;
    } else {
        fprintf(stderr, "%s", "Unknown internal error.\n");
        return 2;
    }
}

struct
{
    char* style_string;
    int output_style;
} style_option_strings[] = {
    { "compressed", SASS_STYLE_COMPRESSED },
    { "compact", SASS_STYLE_COMPACT },
    { "expanded", SASS_STYLE_EXPANDED },
    { "nested", SASS_STYLE_NESTED }
};

#define NUM_STYLE_OPTION_STRINGS \
    sizeof(style_option_strings) / sizeof(style_option_strings[0])

void print_version() {
    printf("sassc: %s\n", SASSC_VERSION);
    printf("libsass: %s\n", libsass_version());
    printf("sass2scss: %s\n", sass2scss_version());
    printf("sass: %s\n", libsass_language_version());
}

void print_usage(char* argv0) {
    int i;
    printf("Usage: %s [options] [INPUT] [OUTPUT]\n\n", argv0);
    printf("Options:\n");
    printf("   -s, --stdin             Read input from standard input instead of an input file.\n");
    printf("   -t, --style NAME        Output style. Can be:");
    for(i = NUM_STYLE_OPTION_STRINGS - 1; i >= 0; i--) {
        printf(" %s", style_option_strings[i].style_string);
        printf(i == 0 ? ".\n" : ",");
    }
    printf("   -l, --line-numbers      Emit comments showing original line numbers.\n");
    printf("       --line-comments\n");
    printf("   -I, --load-path PATH    Set Sass import path.\n");
    printf("   -P, --plugin-path PATH  Set path to autoload plugins.\n");
    printf("   -m, --sourcemap[=TYPE]  Emit source map (auto or inline).\n");
    printf("   -M, --omit-map-comment  Omits the source map url comment.\n");
    printf("   -p, --precision         Set the precision for numbers.\n");
    printf("   -a, --sass              Treat input as indented syntax.\n");
    printf("   -v, --version           Display compiled versions.\n");
    printf("   -h, --help              Display this help message.\n");
    printf("\n");
}

void invalid_usage(char* argv0) {
    fprintf(stderr, "See '%s -h'\n", argv0);
    #ifdef _WIN32
        exit(ERROR_BAD_ARGUMENTS); // One or more arguments are not correct.
    #else
        exit(EX_USAGE); // command line usage error
    #endif

}

int main(int argc, char** argv) {
#ifdef _MSC_VER
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior( 0, _WRITE_ABORT_MSG);
#endif
#ifdef _WIN32
    get_argv_utf8(&argc, &argv);
    SetConsoleOutputCP(65001);
#endif
    if ((argc == 1) && isatty(fileno(stdin))) {
        print_usage(argv[0]);
        return 0;
    }
    /*
    struct SassValue* map = sass_make_map();

    struct SassValue* v1 = sass_make_number(23, "px/in");
    struct SassValue* v2 = sass_make_number(23, "px/in");
    struct SassValue* v3 = sass_make_number(25, "px/in");
    sass_map_set(map, v1, v2);
    sass_map_set(map, v3, v2);
    sass_delete_value(v1);
    sass_delete_value(v2);

    struct SassValue* key = sass_make_number(23, "px/in");
    struct SassValue* qwe = sass_map_get(map, key);

    sass_number_set_value(qwe, 23.3);
    sass_number_set_unit(qwe, "foobar");

    struct SassMapIterator* it = sass_map_make_iterator(map);
    while (!sass_map_iterator_exhausted(it)) {
      struct SassValue* key = sass_map_iterator_get_key(it);
      printf("result %f\n", sass_number_get_value(key));
      printf("result %s\n", sass_number_get_unit(key));
      sass_map_iterator_next(it);
    }
    sass_map_delete_iterator(it);

    sass_number_set_value(v3, 673.3);

    it = sass_map_make_iterator(map);
    while (!sass_map_iterator_exhausted(it)) {
      struct SassValue* key = sass_map_iterator_get_key(it);
      printf("result %f\n", sass_number_get_value(key));
      printf("result %s\n", sass_number_get_unit(key));
      sass_map_iterator_next(it);
    }
    sass_map_delete_iterator(it);

    printf("result %f\n", sass_number_get_value(qwe));
    printf("result %s\n", sass_number_get_unit(qwe));

    sass_delete_value(map);

    exit(1);
    */

    struct SassContextReal* context = sass_make_context();
    sass_context_set_output_style(context, SASS_STYLE_NESTED);
    sass_context_set_precision(context, 9);
    printf("result %d\n", sass_context_get_precision(context));

    struct SassImportCpp* entry = sass_make_file_import("input.scss");

    struct SassCompiler* compiler = sass_make_compiler(context, entry);

    sass_compiler_parse322(compiler);
    sass_compiler_compile322(compiler);
    sass_compiler_render322(compiler);

    puts(sass_compiler_get_output(compiler));


    exit(1);

    return 0;

}
