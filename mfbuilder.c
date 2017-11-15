#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define ALLOC(size, type) ((type*) malloc((size) * sizeof(type)))

typedef struct {
  char* source;
  char** headers;
  size_t hn;
} file_info;

void get_file_dependencies(file_info**, size_t);
static inline void init_file_info(file_info*);
static inline void delete_file_info(file_info*);
static inline void generate_makefile(FILE*, file_info**, size_t);
static inline void print_object_file(FILE*, char*);
static inline void print_headers(FILE*, char**, size_t);
const char* get_extension(const char*);

char buffer[1024];

/* This gets the proper compiler based on the extension of the source files
   assuming that all the source files have the same extension. That means that we can't
   have for example files with .c and files with .cpp at the same time. Maybe I sould add
   a check for that later.*/

char compiler[4];		

char* bin_name;

int main(int argc, char* argv[]) {
  int args = argc - 3;
  FILE* makefile;
  file_info** fi = ALLOC(args, file_info*);
  for (size_t i = 0U; i != args; ++i) {
    fi[i] = ALLOC(1, file_info);
    init_file_info(fi[i]);
    fi[i]->headers = ALLOC(args, char*);
  }
  size_t j = 0U;
  while (--argc) {
    ++argv;
    if (strcmp("-n", *argv) == 0) {
      bin_name = ALLOC(strlen(*argv) + 1, char);
      ++argv;
      strncpy(bin_name, *argv, strlen(*argv));
      --argc;
      continue;
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
  for (size_t i = 0U; i != args; ++i) {
    delete_file_info(fi[i]);
  } free(fi);
  fclose(makefile);
  return EXIT_SUCCESS;
}

const char* get_extension(const char* name) {
  char ch;
  const char** original = &name;
  while (*name++ != '.');
  if (strcmp("c", name) == 0) {
    name = *original;
    return "C";
  }
  if (strcmp("cc", name) == 0 ||
      strcmp("cpp", name) == 0) {
    name = *original;
    return "CPP";
  }
}

inline void generate_makefile(FILE* file, file_info** fi, size_t n) {
  fprintf(file, "CC = %s\nFLAGS = -Wall -ggdb\n", compiler);
  fprintf(file, "OPTFLAGS = -O3\n");
  putc('\n', file);
  fprintf(file, "bin: ");
  for (size_t i = 0U; i != n; ++i) {
    print_object_file(file, fi[i]->source);
  } putc('\n', file);
  putc('\t', file);
  fprintf(file, "$(CC) $(FLAGS) $(OPTFLAGS) ");
  for (size_t i = 0U; i != n; ++i) {
    print_object_file(file, fi[i]->source);
  }
  fprintf(file, "-o %s\n\n", bin_name);
  for (size_t i = 0U; i != n; ++i) {
    print_object_file(file, fi[i]->source);
    putc(':', file); putc(' ', file);
    fprintf(file, "%s ", fi[i]->source);
    print_headers(file, fi[i]->headers, fi[i]->hn);
    putc('\n', file); putc('\t', file);
    fprintf(file, "$(CC) $(FLAGS) $(OPTFLAGS) -c %s\n\n", fi[i]->source);
  }
  fprintf(file, ".PHONY : clear\n\n");
  fprintf(file, "clear :\n\trm -f %s ", bin_name);
  for (size_t i = 0U; i != n; ++i) {
    print_object_file(file, fi[i]->source);
  } putc('\n', file);
}

inline void init_file_info(file_info* fi) {
  fi->source = NULL;
  fi->headers = NULL;
  fi->hn = 0U;
}

inline void delete_file_info(file_info* fi) {
  free(fi->source);
  for (size_t i = 0U; i != fi->hn; ++i) {
    free(fi->headers[i]);
  } free(fi->headers);
  free(fi);
}

void get_file_dependencies(file_info** fi, size_t n) {
  for (size_t i = 0U; i != n; ++i) {
    FILE* handler = fopen(fi[i]->source, "rb");
    size_t j = 0U;
    uint8_t flag = 0U;
    while (1) {
      fgets(buffer, sizeof(buffer), handler);
      buffer[strlen(buffer) - 1] = '\0';
      if (strstr(buffer, "#include") == NULL) {
	if (flag == 0U) continue;
	else break;
      } else flag = 1U;
      if (strstr(buffer, ".h\"") != NULL) {
	const char* aux = buffer;
	while (*aux++ != '"');
	const char* start = aux;
	size_t len = 0U;
	while (*aux++ != '"') ++len;
	fi[i]->headers[j] = ALLOC(len + 1, char);
	strncpy(fi[i]->headers[j++], start, len);
      }
    }
    fi[i]->hn = j;
    fclose(handler);
  }
}

inline void print_object_file(FILE* file, char* name) {
  char** base = &name;
  while (*name != '.') { putc(*name, file); ++name; }
  fprintf(file, ".o ");
}

inline void print_headers(FILE* file, char** headers, size_t n) {
  for (size_t i = 0U; i != n; ++i) {
    fprintf(file, "%s ", headers[i]);
  }
}
