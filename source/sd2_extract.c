/* sd2_extract.c -- first-run setup from the user's own Google Play APKs.
 *
 * The port ships no game files. The user copies the two APKs next to the .nro; this
 * checks they are the supported version and unpacks what the loader needs. Files
 * already unpacked at the right size are skipped, so an interrupted run resumes.
 *
 * The APKs then move to apk/. Unity copies every file at the top of this folder
 * into il2cpp/il2cpp.usym on its first boot, and copying them there is not safe:
 * the console powered off during that copy on hardware.
 *
 * MIT.
 */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <switch.h>
#include <zlib.h>

#include "sd2_extract.h"

#define IO_BUF (256 * 1024)
#define PATH_MAX_LEN 768

typedef struct {
  const char *file;
  const char *sha256;
} Package;

static const Package PACKAGES[] = {
  { "base.apk", "4f79947311560099b70cee6fab95cd9f08d6c6014fae4f822b33155bda25f0dc" },
  { "split_config.arm64_v8a.apk", "bc3ece4fd9fb6445942ede7ade0a036f36174cf9a8ae32197acc776a2b7dd151" },
};
#define PACKAGE_COUNT (sizeof(PACKAGES) / sizeof(PACKAGES[0]))

/* Libraries are last in the second package, so they also mark a finished run. */
static const char *const REQUIRED[] = {
  "libmain.so", "libil2cpp.so", "libunity.so",
  "assets/bin/Data/data.unity3d",
  "assets/bin/Data/Managed/Metadata/global-metadata.dat",
  "assets/not_atlassed", "assets/audio",
};

static const char *const ASSET_PREFIXES[] = {
  "assets/bin/Data/", "assets/not_atlassed", "assets/audio",
  "assets/StreamingAssets", "assets/UnityServicesProjectConfiguration.json",
};

#define LIB_PREFIX "lib/arm64-v8a/"

static int g_console_up;

/* Finds a package at the top of the folder, or in apk/ after an earlier setup. */
static void package_path(const char *root, const char *file, char *path, size_t cap) {
  struct stat st;
  snprintf(path, cap, "%s/%s", root, file);
  if (stat(path, &st) != 0)
    snprintf(path, cap, "%s/apk/%s", root, file);
}

static void console_up(void) {
  if (g_console_up)
    return;
  consoleInit(NULL);
  g_console_up = 1;
  printf("\n  sd2_nx first-run setup\n\n");
  consoleUpdate(NULL);
}

/* Shows the reason and waits for Plus, then leaves the app. */
static void fail(const char *fmt, const char *arg) {
  console_up();
  printf("\n  Setup stopped:\n  ");
  printf(fmt, arg);
  printf("\n\n  Copy these from your own Google Play install of\n"
         "  Soda Dungeon 2 version 1.2.21 into switch/sd2_nx/:\n"
         "    base.apk\n    split_config.arm64_v8a.apk\n\n"
         "  Press + to exit.\n");
  consoleUpdate(NULL);

  PadState pad;
  padConfigureInput(1, HidNpadStyleSet_NpadStandard);
  padInitializeDefault(&pad);
  while (appletMainLoop()) {
    padUpdate(&pad);
    if (padGetButtonsDown(&pad) & HidNpadButton_Plus)
      break;
    consoleUpdate(NULL);
  }
  consoleExit(NULL);
  exit(1);
}

static int all_present(const char *root) {
  char path[PATH_MAX_LEN];
  struct stat st;
  for (size_t i = 0; i < sizeof(REQUIRED) / sizeof(REQUIRED[0]); i++) {
    snprintf(path, sizeof path, "%s/%s", root, REQUIRED[i]);
    if (stat(path, &st) != 0)
      return 0;
  }
  return 1;
}

static void check_hash(const char *root, const Package *pkg, uint8_t *buf) {
  char path[PATH_MAX_LEN];
  package_path(root, pkg->file, path, sizeof path);
  FILE *f = fopen(path, "rb");
  if (!f)
    fail("Missing file: %s", pkg->file);

  printf("  Checking %s...\n", pkg->file);
  consoleUpdate(NULL);

  Sha256Context ctx;
  sha256ContextCreate(&ctx);
  size_t n;
  while ((n = fread(buf, 1, IO_BUF, f)) > 0)
    sha256ContextUpdate(&ctx, buf, n);
  fclose(f);

  uint8_t hash[SHA256_HASH_SIZE];
  sha256ContextGetHash(&ctx, hash);
  char hex[SHA256_HASH_SIZE * 2 + 1];
  for (int i = 0; i < SHA256_HASH_SIZE; i++)
    snprintf(hex + i * 2, 3, "%02x", hash[i]);
  if (strcmp(hex, pkg->sha256) != 0)
    fail("%s is not the supported game version.", pkg->file);
}

/* Creates every parent folder of `path`, skipping the "sdmc:/" device root. */
static void make_parents(char *path) {
  char *p = strstr(path, ":/");
  p = p ? p + 2 : path;
  for (; *p; p++) {
    if (*p == '/') {
      *p = 0;
      mkdir(path, 0777);
      *p = '/';
    }
  }
}

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t rd32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

/* Writes to a temporary name first, so a partial file never looks finished. */
static void unpack_entry(FILE *in, uint32_t local_off, int method, uint32_t csize,
                         uint32_t usize, char *dest, uint8_t *inbuf, uint8_t *outbuf) {
  uint8_t head[30];
  if (fseek(in, (long)local_off, SEEK_SET) != 0 || fread(head, 1, 30, in) != 30 ||
      rd32(head) != 0x04034b50)
    fail("Damaged package near %s.", dest);
  if (fseek(in, (long)local_off + 30 + rd16(head + 26) + rd16(head + 28), SEEK_SET) != 0)
    fail("Damaged package near %s.", dest);

  char tmp[PATH_MAX_LEN + 8];
  snprintf(tmp, sizeof tmp, "%s.part", dest);
  make_parents(tmp);
  FILE *out = fopen(tmp, "wb");
  if (!out)
    fail("Could not write %s. Is the SD card full?", dest);

  uint32_t left = csize;
  size_t written = 0;
  if (method == 0) {
    while (left > 0) {
      size_t want = left < IO_BUF ? left : IO_BUF;
      if (fread(inbuf, 1, want, in) != want || fwrite(inbuf, 1, want, out) != want)
        fail("Could not write %s. Is the SD card full?", dest);
      left -= (uint32_t)want;
      written += want;
    }
  } else if (method == 8) {
    z_stream z;
    memset(&z, 0, sizeof z);
    if (inflateInit2(&z, -15) != Z_OK)
      fail("Could not start unpacking %s.", dest);
    int rc = Z_OK;
    while (rc != Z_STREAM_END) {
      if (z.avail_in == 0 && left > 0) {
        size_t want = left < IO_BUF ? left : IO_BUF;
        if (fread(inbuf, 1, want, in) != want)
          fail("Damaged package near %s.", dest);
        left -= (uint32_t)want;
        z.next_in = inbuf;
        z.avail_in = (uInt)want;
      }
      z.next_out = outbuf;
      z.avail_out = IO_BUF;
      rc = inflate(&z, Z_NO_FLUSH);
      if (rc != Z_OK && rc != Z_STREAM_END)
        fail("Damaged package near %s.", dest);
      size_t have = IO_BUF - z.avail_out;
      if (fwrite(outbuf, 1, have, out) != have)
        fail("Could not write %s. Is the SD card full?", dest);
      written += have;
    }
    inflateEnd(&z);
  } else {
    fail("Unsupported compression in %s.", dest);
  }
  fclose(out);
  if (written != usize)
    fail("Damaged package near %s.", dest);
  remove(dest);
  if (rename(tmp, dest) != 0)
    fail("Could not finish %s.", dest);
}

/* Maps an entry name to its place on the SD card, or returns 0 to skip it. */
static int dest_for(const char *root, const char *name, char *dest, size_t cap) {
  size_t liblen = strlen(LIB_PREFIX);
  if (!strncmp(name, LIB_PREFIX, liblen)) {
    snprintf(dest, cap, "%s/%s", root, name + liblen);
    return 1;
  }
  for (size_t i = 0; i < sizeof(ASSET_PREFIXES) / sizeof(ASSET_PREFIXES[0]); i++) {
    if (!strncmp(name, ASSET_PREFIXES[i], strlen(ASSET_PREFIXES[i]))) {
      snprintf(dest, cap, "%s/%s", root, name);
      return 1;
    }
  }
  return 0;
}

static void unpack_package(const char *root, const char *file, uint8_t *inbuf, uint8_t *outbuf) {
  char path[PATH_MAX_LEN];
  package_path(root, file, path, sizeof path);
  FILE *f = fopen(path, "rb");
  if (!f)
    fail("Missing file: %s", file);

  /* The end record sits in the last 64 KB, after a possible archive comment. */
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  long tail = size < 65557 ? size : 65557;
  uint8_t *end = malloc((size_t)tail);
  if (!end || fseek(f, size - tail, SEEK_SET) != 0 || fread(end, 1, (size_t)tail, f) != (size_t)tail)
    fail("Could not read %s.", file);
  long eocd = -1;
  for (long i = tail - 22; i >= 0; i--) {
    if (rd32(end + i) == 0x06054b50) {
      eocd = i;
      break;
    }
  }
  if (eocd < 0)
    fail("%s is not a valid package.", file);
  uint32_t count = rd16(end + eocd + 10);
  uint32_t dir_size = rd32(end + eocd + 12);
  uint32_t dir_off = rd32(end + eocd + 16);
  free(end);

  uint8_t *dir = malloc(dir_size);
  if (!dir || fseek(f, (long)dir_off, SEEK_SET) != 0 || fread(dir, 1, dir_size, f) != dir_size)
    fail("Could not read %s.", file);

  uint32_t pos = 0;
  for (uint32_t i = 0; i < count; i++) {
    if (pos + 46 > dir_size || rd32(dir + pos) != 0x02014b50)
      fail("%s is not a valid package.", file);
    int method = rd16(dir + pos + 10);
    uint32_t csize = rd32(dir + pos + 20);
    uint32_t usize = rd32(dir + pos + 24);
    uint32_t nlen = rd16(dir + pos + 28);
    uint32_t xlen = rd16(dir + pos + 30);
    uint32_t clen = rd16(dir + pos + 32);
    uint32_t local = rd32(dir + pos + 42);

    char name[512];
    if (nlen < sizeof name) {
      memcpy(name, dir + pos + 46, nlen);
      name[nlen] = 0;
      char dest[PATH_MAX_LEN];
      struct stat st;
      /* Entry names come from a verified package, but ".." stays out regardless. */
      if (name[nlen ? nlen - 1 : 0] != '/' && !strstr(name, "..") &&
          dest_for(root, name, dest, sizeof dest) &&
          !(stat(dest, &st) == 0 && (uint32_t)st.st_size == usize)) {
        printf("  %s\n", name);
        consoleUpdate(NULL);
        unpack_entry(f, local, method, csize, usize, dest, inbuf, outbuf);
      }
    }
    pos += 46 + nlen + xlen + clen;
  }
  free(dir);
  fclose(f);
}

void sd2_extract_if_needed(const char *root) {
  if (all_present(root))
    return;

  console_up();
  uint8_t *inbuf = malloc(IO_BUF);
  uint8_t *outbuf = malloc(IO_BUF);
  if (!inbuf || !outbuf)
    fail("Not enough memory. %s", "Open a game while holding R.");

  for (size_t i = 0; i < PACKAGE_COUNT; i++)
    check_hash(root, &PACKAGES[i], inbuf);
  for (size_t i = 0; i < PACKAGE_COUNT; i++)
    unpack_package(root, PACKAGES[i].file, inbuf, outbuf);

  free(inbuf);
  free(outbuf);
  if (!all_present(root))
    fail("%s", "Some game files are still missing after unpacking.");

  char from[PATH_MAX_LEN], to[PATH_MAX_LEN];
  snprintf(to, sizeof to, "%s/apk", root);
  mkdir(to, 0777);
  for (size_t i = 0; i < PACKAGE_COUNT; i++) {
    snprintf(from, sizeof from, "%s/%s", root, PACKAGES[i].file);
    snprintf(to, sizeof to, "%s/apk/%s", root, PACKAGES[i].file);
    rename(from, to);
  }

  printf("\n  Done. Starting the game...\n");
  consoleUpdate(NULL);
  consoleExit(NULL);
  g_console_up = 0;
}
