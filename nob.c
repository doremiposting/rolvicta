#include <stdio.h>
#include <string.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define CC "gcc"
#define forceinline inline __attribute__((__always_inline__))

Cmd cmd = {0};

static forceinline
int
boilerplate() {
	cmd_append(&cmd, CC, "-Wall", "-Wextra", "-Wformat", "-Wformat=2",
		"-Wconversion", "-Wsign-conversion", "-Werror=format-security",
		"-Wimplicit-fallthrough", "-Werror=implicit",
		"-Werror=incompatible-pointer-types", "-Werror=int-conversion",
		"-Wtrampolines", "-fzero-init-padding-bits=all", "-Wbidi-chars=any",
	);
}

static forceinline
int
compilefile(const char *fn, const char *out) {
  cmd_append(&cmd,
		"-std=c2y", "-c", "-o", out, fn
  );
	if (!cmd_run(&cmd)) { return 1; }
}

static forceinline
void
pkgconfig(Cmd *c, const char *pkg) {
  char cmdline[256];
  char buf[4096];
  char *tok;
  FILE *fp;

  snprintf(cmdline, sizeof(cmdline), "pkg-config --cflags --libs %s", pkg);
  fp = popen(cmdline, "r");
  if (!fp) { return; }
  if (!fgets(buf, sizeof(buf), fp)) { pclose(fp); return; }
  pclose(fp);

  buf[strcspn(buf, "\n")] = 0;
  tok = strtok(buf, " ");
  while (tok) {
    cmd_append(c, temp_strdup(tok));
    tok = strtok(NULL, " ");
  }
}

int
main(int argc, char *argv[]) {
  int release;
	GO_REBUILD_URSELF(argc, argv); 

  if (!mkdir_if_not_exists("./build/")) { return 1; }

  if (argc > 1) {
    if (!(strncmp(argv[1], "release", 8*sizeof(char)))) {
      release = 1;
    }
  }

  boilerplate();
  cmd_append(&cmd, "-I/usr/include/freetype2");
  pkgconfig(&cmd, "gtk+-3.0");
  cmd_append(&cmd, "-lm");
  cmd_append(&cmd, "-D_GNU_SOURCE");
  if (release) { cmd_append(&cmd, "-O2"); }
  else { cmd_append(&cmd, "-g"); }
  compilefile("src/rolvicta.c", "build/rolvicta.o");


	cmd_append(&cmd, CC, "-fPIE", "-pie", "-o", "rolvicta");
  pkgconfig(&cmd, "gtk+-3.0");
  cmd_append(&cmd, "-lXft", "-lm",
    "build/rolvicta.o");
  if (!release) { cmd_append(&cmd, "-g"); }
	if (!cmd_run(&cmd)) { return 1; }
}
