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
  char** dlls;
  size_t numberOfHeaders;
  size_t numberOfDlls;
} fileInfo;

/* These are the dynamic linked libraries so far.
   I can't find a list online of the dllthat there are on C.*/

typedef struct {
  uint8_t mFlag : 1;		/* math.h flag */
  uint8_t pFlag : 1;		/* pthread.h flag */
  uint8_t nFlag : 1;		/* ncurses.h flag */
} dllFlags;

const char dlls[DLL_N][2][DLL_SIZE] = {
  { "math.h", "-lm" },
  { "pthread.h", "-lpthread" },
  { "ncurses.h", "-lncurses" }
};

char buffer[1024];

/* This gets the proper compiler based on the extension of the source files
   assuming that all the source files have the same extension. That means that we can't
   have for example files with .c and files with .cpp at the same time. Maybe I sould add
   a check for that later.*/

char compiler[4];		

char* binaryName;

typedef enum extensions {
  C, CC, CPP, ERROR
} extensions;

static inline void initFileInfo(fileInfo*);
static inline void deleteFileInfo(fileInfo**, size_t);
static inline void generateMakefile(FILE*, fileInfo**, size_t);
static inline void printObjectFile(FILE*, const char*);
static inline void printAllObjFiles(FILE*, fileInfo**, size_t);
static inline void printHeaders(FILE*, char**, size_t);
static inline void checkArguments(int, char**);
fileInfo** allocAndInitFileInfo(size_t);
extensions getExtension(const char*);
uint8_t isComment(const char*);
uint8_t isSpace(const char*);
uint8_t isDirective(const char*);
void searchForDll(size_t*, fileInfo*);
void getFileDependencies(fileInfo**, size_t);

int main(int argc, char* argv[]) {
  checkArguments(argc, argv);

  int args = argc - 3;
  FILE* makefile;
  fileInfo** files = allocAndInitFileInfo(args);
  size_t j = 0U;

  while (--argc) {
    ++argv;

    if (strcmp("-n", *argv) == 0) {
      binaryName = ALLOC(strlen(*argv) + 1, char);
      strncpy(binaryName, *argv, strlen(*++argv));

      --argc;
      continue;
    }

    files[j]->source = ALLOC(strlen(*argv) + 1, char);
    strncpy(files[j++]->source, *argv, strlen(*argv));
  }

  extensions extension = getExtension(files[0]->source);

  if (extension == C) {
    strncpy(compiler, "gcc", sizeof(compiler));
  } else if (extension == CC || extension == CPP) {
    strncpy(compiler, "g++", sizeof(compiler));
  } else {
    fprintf(stderr, "Found file(s) with unknown extension\n");
    exit(EXIT_FAILURE);
  }

  getFileDependencies(files, args);
  makefile = fopen("makefile", "wb");
  generateMakefile(makefile, files, args);

  deleteFileInfo(files, args);
  fclose(makefile);

  return EXIT_SUCCESS;
}

extensions getExtension(const char* name) {
  while (*name++ != '.') {}

  if (strcmp("c", name) == 0) {
    return C;
  } else if (strcmp("cc", name) == 0) {
    return CC;
  } else if (strcmp("cpp", name) == 0) {
    return CPP;
  } else {
    return ERROR;
  }
}

inline void generateMakefile(FILE* file, fileInfo** files,
			     size_t numberOfFiles) {

  /* Printing the compiler information */
  fprintf(file, "CC = %s\nCFLAGS = -Wall -ggdb\n\n", compiler);

  /* Printing the information for the binary target */
  fprintf(file, "bin: ");
  printAllObjFiles(file, files, numberOfFiles);
  putc('\n', file);
  putc('\t', file);
  fprintf(file, "$(CC) $(CFLAGS) ");
  printAllObjFiles(file, files, numberOfFiles);
  fprintf(file, "-o %s ", binaryName);

  dllFlags flags = {0};
  size_t* info = ALLOC(DLL_N, size_t);

  for (size_t i = 0U; i != DLL_N; ++i) {
    info[i] = 0U;
  }

  for (size_t i = 0U; i != numberOfFiles; ++i) {
    searchForDll(info, files[i]);

    for (size_t j = 0U; j != DLL_N; ++j) {
      switch (info[j]) {
      case 0: break;

      case 1:
	switch (j) {
	case 0:
	  if (flags.mFlag == 0U) {
	    fprintf(file, "%s ", dlls[j][1]);
	  }

	  flags.mFlag = 1U;
	  break;

	case 1:
	  if (flags.pFlag == 0U) {
	    fprintf(file, "%s ", dlls[j][1]);
	  }

	  flags.pFlag = 1U;
	  break;

	case 2:
	  if (flags.nFlag == 0U) {
	    fprintf(file, "%s ", dlls[j][1]);
	  }

	  flags.nFlag = 1U;
	  break;
	}
      }
    }
  }
  
  free(info);
  
  putc('\n', file);
  putc('\n', file);

  /* Making the targets for object files */
  for (size_t i = 0U; i != numberOfFiles; ++i) {
    printObjectFile(file, files[i]->source);
    putc(':', file);
    putc(' ', file);
    fprintf(file, "%s ", files[i]->source);
    printHeaders(file, files[i]->headers, files[i]->numberOfHeaders);
    putc('\n', file);
    putc('\t', file);
    fprintf(file, "$(CC) $(CFLAGS) -c %s ", files[i]->source);

    for (size_t j = 0U; j != files[i]->numberOfDlls; ++j) {
      for (size_t k = 0U; k != DLL_N; ++k) {
	if (strcmp(files[i]->dlls[j], dlls[k][0]) == 0) {
	  fprintf(file, "%s ", dlls[k][1]);
	  break;
	}
      }
    }

    putc('\n', file);
    putc('\n', file);
  }

  fprintf(file, ".PHONY : clear\n\n");
  fprintf(file, "clear :\n\trm -f %s ", binaryName);
  printAllObjFiles(file, files, numberOfFiles);
  putc('\n', file);
  fprintf(file, "\n\n#Generated with makefile generator: "
	  "https://github.com/GeorgeLS/Makefile-Generator/blob/master/mfbuilder.c\n");
}

inline void initFileInfo(fileInfo* file) {
  file->source = NULL;
  file->headers = NULL;
  file->dlls = NULL;
  file->numberOfHeaders = 0U;
  file->numberOfDlls = 0U;
}

inline void deleteFileInfo(fileInfo** files, size_t numberOfFiles) {
  for (size_t i = 0U; i != numberOfFiles; ++i) {
    free(files[i]->source);

    for (size_t j = 0U; j != files[i]->numberOfHeaders; ++j) {
      free(files[i]->headers[j]);
    } free(files[i]->headers);

    for (size_t j = 0U; j != files[i]->numberOfDlls; ++j) {
      free(files[i]->dlls[j]);
    } free(files[i]->dlls);

    free(files[i]);
  }

  free(files);
}

void getFileDependencies(fileInfo** files, size_t numberOfFiles) {
  for (size_t i = 0U; i != numberOfFiles; ++i) {
    FILE* handler = fopen(files[i]->source, "rb");
    size_t j = 0U;
    size_t k = 0U;

    while (1) {
      fgets(buffer, sizeof(buffer), handler);
      buffer[strlen(buffer) - 1] = '\0';

      if (strstr(buffer, "#include") == NULL) {
	if (isComment(buffer) ||
	    isSpace(buffer) ||
	    isDirective(buffer)) {

	  continue;
	}

	break;
      }

      if (strstr(buffer, ".h\"") != NULL) {
	const char* aux = buffer;

	while (*aux++ != '"') {}

	const char* start = aux;
	size_t length = 0U;

	while (*aux++ != '"') {
	  ++length;
	}

	files[i]->headers[j] = ALLOC(length + 1, char);
	strncpy(files[i]->headers[j++], start, length);
      } else if (strstr(buffer, ".h>") != NULL) {
	const char* aux = buffer;

	while (*aux++ != '<') {}
	
	const char* start = aux;
	size_t length = 0U;

	while (*aux++ != '>') {
	  ++length;
	}

	for (size_t j = 0U; j != DLL_N; ++j) {
	  if (strncmp(start, dlls[j][0], length) == 0) {
	    files[i]->dlls[k] = ALLOC(length + 1, char);
	    strncpy(files[i]->dlls[k++], start, length);
	    break;
	  }
	}
      }
    }

    files[i]->numberOfHeaders = j;
    files[i]->numberOfDlls = k;
    fclose(handler);
  }
}

inline void printObjectFile(FILE* file, const char* name) {
  while (*name != '.') {
    putc(*name, file);
    ++name;
  }

  fprintf(file, ".o ");
}

inline void printHeaders(FILE* file, char** headers,
			 size_t numberOfHeaders) {

  for (size_t i = 0U; i != numberOfHeaders; ++i) {
    fprintf(file, "%s ", headers[i]);
  }
}

uint8_t isComment(const char* buffer) {
  while (*buffer != '\0') {
    if (*buffer++ == '/') {
      if (*buffer == '*' || *buffer == '/') {
	return 1U;
      }
    }
  }

  return 0U;
}

uint8_t isSpace(const char* buffer) {
  while (*buffer != '\0') {
    if (!isspace(*buffer++)) {
      return 0U;
    }
  }

  return 1U;
}

uint8_t isDirective(const char* buffer) {
  if (strstr(buffer, "#if") != NULL ||
      strstr(buffer, "else") != NULL ||
      strstr(buffer, "#end") != NULL ||
      strstr(buffer, "#define") != NULL ||
      strstr(buffer, "undef") != NULL) {

    return 1U;
  }

  return 0U;
}

void searchForDll(size_t* info, fileInfo* file) {
  for (size_t i = 0U; i != file->numberOfDlls; ++i) {
    for (size_t j = 0U; j != DLL_N; ++j) {
      if (strcmp(file->dlls[i], dlls[j][0]) == 0) {
	info[j] = 1U;
      }
    }
  }
}

fileInfo** allocAndInitFileInfo(size_t numberOfFiles) {
  fileInfo** newFiles = ALLOC(numberOfFiles, fileInfo*);

  for (size_t i = 0U; i != numberOfFiles; ++i) {
    newFiles[i] = ALLOC(1, fileInfo);
    initFileInfo(newFiles[i]);
    newFiles[i]->headers = ALLOC(numberOfFiles, char*);
    newFiles[i]->dlls = ALLOC(DLL_N, char*);
  }

  return newFiles;
}

inline void checkArguments(int argc, char** argv) {
  if ((argc - 3U) <= 0U) {
    fprintf(stderr, "You should provide more arguments!\n");
    fprintf(stderr, "Program usage: ./mfbuilder -n exec_name source_files\n");
    exit(EXIT_FAILURE);
  }

  uint8_t seen = 0U;

  while (--argc) {
    if (strcmp("-n", argv[argc]) == 0) {
      seen = 1U;
    }
  }

  if (!seen) {
    fprintf(stderr, "You should provide an executable name with the parameter -n\n");
    exit(EXIT_FAILURE);
  }
}

inline void printAllObjFiles(FILE* file, fileInfo** files,
			     size_t numberOfFiles) {

  for (size_t i = 0U; i != numberOfFiles; ++i) {
    printObjectFile(file, files[i]->source);
  }
}
