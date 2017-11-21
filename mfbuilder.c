#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define ALLOC(size, type) ((type*) malloc((size) * sizeof(type)))

#define DLL_N 3
#define DLL_SIZE 21

typedef struct {
  char* source;
  char** headers;
  char** dll;
  size_t hn;
  size_t dlln;
} file_info;

/* These are the dynamic linked libraries so far. I can't find a list online of the dlls
   that there are on C.*/

typedef struct {
  uint8_t mflag : 1;
  uint8_t pflag : 1;
  uint8_t nflag : 1;
} dll_flags;

char dlls[DLL_N][2][DLL_SIZE] = {
  { "math.h", "-lm" },
  { "pthread.h", "-lpthread" },
  { "ncurses.h", "-lncurses" }
};

static inline void init_file_info(file_info*);
static inline void delete_file_info(file_info**, size_t);
static inline void generate_makefile(FILE*, file_info**, size_t);
static inline void print_object_file(FILE*, const char*);
static inline void print_all_obj_files(FILE*, file_info**, size_t);
static inline void print_headers(FILE*, char**, size_t);
static inline void check_arguments(int, char**);
file_info** alloc_and_init_file_info(size_t);
const char* get_extension(const char*);
uint8_t is_comment(const char*);
uint8_t is_space(const char*);
uint8_t is_directive(const char*);
void search_for_dll(size_t*, file_info*);
void get_file_dependencies(file_info**, size_t);

char buffer[1024];

/* This gets the proper compiler based on the extension of the source files
   assuming that all the source files have the same extension. That means that we can't
   have for example files with .c and files with .cpp at the same time. Maybe I sould add
   a check for that later.*/

char compiler[4];		

char* bin_name;

int main(int argc, char* argv[]) {
  check_arguments(argc, argv);
  int args = argc - 3;
  FILE* makefile;
  file_info** fi = alloc_and_init_file_info(args);
  size_t j = 0U;
  while (--argc) {
    ++argv;
    if (strcmp("-n", *argv) == 0) {
      bin_name = ALLOC(strlen(*argv) + 1, char);
      strncpy(bin_name, *argv, strlen(*++argv));
      --argc; continue;
    }
    fi[j]->source = ALLOC(strlen(*argv) + 1, char);
    strncpy(fi[j++]->source, *argv, strlen(*argv));
  }
  const char* ext = get_extension(fi[0]->source);
  if (strcmp("C", ext) == 0) {
    strncpy(compiler, "gcc", sizeof(compiler));
  } else {
    strncpy(compiler, "g++", sizeof(compiler));
  }
  get_file_dependencies(fi, args);
  makefile = fopen("makefile", "wb");
  generate_makefile(makefile, fi, args);
  delete_file_info(fi, args);
  fclose(makefile);
  return EXIT_SUCCESS;
}

const char* get_extension(const char* name) {
  const char** base = &name;
  char ch;
  while (*name++ != '.');
  if (strcmp("c", name) == 0) {
    name = *base; return "C";
  }
  if (strcmp("cc", name) == 0 ||
      strcmp("cpp", name) == 0) {
    name = *base; return "CPP";
  }
}

inline void generate_makefile(FILE* file, file_info** fi, size_t n) {
  fprintf(file, "CC = %s\nCFLAGS = -Wall -ggdb\n", compiler);
  /* fprintf(file, "OPTFLAGS = -O3\n"); */
  putc('\n', file);
  fprintf(file, "bin: ");
  print_all_obj_files(file, fi, n);
  putc('\n', file); putc('\t', file);
  fprintf(file, "$(CC) $(CFLAGS) ");
  print_all_obj_files(file, fi, n);
  fprintf(file, "-o %s ", bin_name);
  dll_flags flags = {0};
  size_t* info = ALLOC(DLL_N, size_t);
  for (size_t i = 0U; i != DLL_N; ++i) info[i] = 0U;
  for (size_t i = 0U; i != n; ++i) {
    search_for_dll(info, fi[i]);
    for (size_t j = 0U; j != DLL_N; ++j) {
      switch (info[j]) {
      case 0: break;
      case 1:
	switch (j) {
	case 0:
	  if (flags.mflag == 0U) fprintf(file, "%s ", dlls[j][1]);
	  flags.mflag = 1U;
	  break;
	case 1:
	  if (flags.pflag == 0U) fprintf(file, "%s ", dlls[j][1]);
	  flags.pflag = 1U;
	  break;
	case 2:
	  if (flags.nflag == 0U) fprintf(file, "%s ", dlls[j][1]);
	  flags.nflag = 1U;
	  break;
	}
      }
    }
  }
  free(info);
  putc('\n', file); putc('\n', file);
  for (size_t i = 0U; i != n; ++i) {
    print_object_file(file, fi[i]->source);
    putc(':', file); putc(' ', file);
    fprintf(file, "%s ", fi[i]->source);
    print_headers(file, fi[i]->headers, fi[i]->hn);
    putc('\n', file); putc('\t', file);
    fprintf(file, "$(CC) $(CFLAGS) -c %s ", fi[i]->source);
    for (size_t j = 0U; j != fi[i]->dlln; ++j) {
      for (size_t k = 0U; k != DLL_N; ++k) {
	if (strcmp(fi[i]->dll[j], dlls[k][0]) == 0) {
	  fprintf(file, "%s ", dlls[k][1]);
	  break;
	}
      }
    }
    putc('\n', file); putc('\n', file);
  }
  fprintf(file, ".PHONY : clear\n\n");
  fprintf(file, "clear :\n\trm -f %s ", bin_name);
  print_all_obj_files(file, fi, n);
  putc('\n', file);
  fprintf(file, "\n\n#Generated with makefile generator: https://github.com/GeorgeLS/Makefile-Generator/blob/master/mfbuilder.c\n");
}

inline void init_file_info(file_info* fi) {
  fi->source = NULL;
  fi->headers = NULL;
  fi->dll = NULL;
  fi->hn = 0U;
  fi->dlln = 0U;
}

<<<<<<< HEAD
inline void delete_file_info(file_info** fi, size_t n) {
  for (size_t i = 0U; i != n; ++i) {
    free(fi[i]->source);
    for (size_t j = 0U; j != fi[i]->hn; ++j) {
      free(fi[i]->headers[j]);
    } free(fi[i]->headers);
    for (size_t j = 0U; j != fi[i]->dlln; ++j) {
      free(fi[i]->dll[j]);
    } free(fi[i]->dll);
    free(fi[i]);
  } free(fi);
=======
inline void delete_file_info(file_info* fi) {
  free(fi->source);
  for (size_t i = 0U; i != fi->hn; ++i) {
    free(fi->headers[i]);
  }
  for (size_t i = 0U; i != fi->dlln; ++i) {
    free(fi->dll[i]);
  }
  free(fi->headers);
  free(fi->dll);
  free(fi);
>>>>>>> f952ea3e36a9bb5f3689bc6edc0fbb7f00469ccf
}

void get_file_dependencies(file_info** fi, size_t n) {
  for (size_t i = 0U; i != n; ++i) {
    FILE* handler = fopen(fi[i]->source, "rb");
    size_t j = 0U;
    size_t k = 0U;
    while (1) {
      fgets(buffer, sizeof(buffer), handler);
      buffer[strlen(buffer) - 1] = '\0';
      if (strstr(buffer, "#include") == NULL) {
	if (is_comment(buffer) ||
	    is_space(buffer) ||
	    is_directive(buffer)) continue;
	break;
      }
      if (strstr(buffer, ".h\"") != NULL) {
	const char* aux = buffer;
	while (*aux++ != '"');
	const char* start = aux;
	size_t len = 0U;
	while (*aux++ != '"') ++len;
	fi[i]->headers[j] = ALLOC(len + 1, char);
	strncpy(fi[i]->headers[j++], start, len);
      } else if (strstr(buffer, ".h>") != NULL) {
	const char* aux = buffer;
	while (*aux++ != '<');
	const char* start = aux;
	size_t len = 0U;
	while (*aux++ != '>') ++len;
	for (size_t j = 0U; j != DLL_N; ++j) {
	  if (strncmp(start, dlls[j][0], len) == 0) {
	    fi[i]->dll[k] = ALLOC(len + 1, char);
	    strncpy(fi[i]->dll[k++], start, len);
	    break;
	  }
	}
      }
    }
    fi[i]->hn = j; fi[i]->dlln = k; fclose(handler);
  }
}

inline void print_object_file(FILE* file, const char* name) {
  const char** base = &name;
  while (*name != '.') { putc(*name, file); ++name; }
  fprintf(file, ".o ");
}

inline void print_headers(FILE* file, char** headers, size_t n) {
  for (size_t i = 0U; i != n; ++i) {
    fprintf(file, "%s ", headers[i]);
  }
}

uint8_t is_comment(const char* buffer) {
  const char** base = &buffer;
  while (*buffer != '\0') {
    if (*buffer++ == '/') {
      if (*buffer == '*' || *buffer == '/') {
	buffer = *base; return 1U;
      }
    }
  }
  buffer = *base; return 0U;
}

uint8_t is_space(const char* buffer) {
  const char** base = &buffer;
  while (*buffer != '\0') {
    if (!isspace(*buffer++)) {
      buffer = *base; return 0U;
    }
  }
  buffer = *base; return 1U;
}

uint8_t is_directive(const char* buffer) {
  if (strstr(buffer, "#if") != NULL ||
      strstr(buffer, "else") != NULL ||
      strstr(buffer, "#end") != NULL ||
      strstr(buffer, "#define") != NULL ||
      strstr(buffer, "undef") != NULL) return 1U;
  return 0U;
}

void search_for_dll(size_t* info, file_info* fi) {
  for (size_t i = 0U; i != fi->dlln; ++i) {
    for (size_t j = 0U; j != DLL_N; ++j) {
      if (strcmp(fi->dll[i], dlls[j][0]) == 0) info[j] = 1U;
    }
  }
}

file_info** alloc_and_init_file_info(size_t n) {
  file_info** new_fi = ALLOC(n, file_info*);
  for (size_t i = 0U; i != n; ++i) {
    new_fi[i] = ALLOC(1, file_info);
    init_file_info(new_fi[i]);
    new_fi[i]->headers = ALLOC(n, char*);
    new_fi[i]->dll = ALLOC(DLL_N, char*);
  } return new_fi;
}

inline void check_arguments(int argc, char** argv) {
  if ((argc - 3U) <= 0U) {
    fprintf(stderr, "You should provide more arguments!\n");
    fprintf(stderr, "Program usage: ./mfbuilder -n exec_name source_files\n");
    exit(EXIT_FAILURE);
  }
  uint8_t seen = 0U;
  while (--argc) {
    if (strcmp("-n", argv[argc]) == 0) seen = 1U;
  }
  if (!seen) {
    fprintf(stderr, "You should provide an executable name with the parameter -n\n");
    exit(EXIT_FAILURE);
  }
}

inline void print_all_obj_files(FILE* file, file_info** fi, size_t n) {
  for (size_t i = 0U; i != n; ++i) {
    print_object_file(file, fi[i]->source);
  }
}
