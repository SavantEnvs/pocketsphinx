/* Shared helpers for the PocketSphinx fuzz harnesses. */
#ifndef MAYHEM_COMMON_H
#define MAYHEM_COMMON_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pocketsphinx.h>

#ifndef MODELDIR
#define MODELDIR "/mayhem/model"
#endif

/* Library messages go to stderr; keep only fatal ones so each exec stays cheap. */
static void
mayhem_quiet(void)
{
    err_set_loglevel(ERR_FATAL);
}

/* Write the input to a fresh scratch file (for parsers that take a path). */
static char *
mayhem_tmpfile(const uint8_t *data, size_t size)
{
    const char *dir = getenv("TMPDIR");
    char *path;
    size_t n;
    int fd;

    if (dir == NULL || *dir == '\0')
        dir = "/tmp";
    n = strlen(dir) + sizeof("/ps-fuzz-XXXXXX");
    path = malloc(n);
    if (path == NULL)
        return NULL;
    snprintf(path, n, "%s/ps-fuzz-XXXXXX", dir);
    fd = mkstemp(path);
    if (fd < 0) {
        free(path);
        return NULL;
    }
    while (size > 0) {
        ssize_t w = write(fd, data, size);
        if (w <= 0) {
            close(fd);
            unlink(path);
            free(path);
            return NULL;
        }
        data += w;
        size -= (size_t)w;
    }
    close(fd);
    return path;
}

static void
mayhem_rmtmp(char *path)
{
    if (path) {
        unlink(path);
        free(path);
    }
}

/* NUL-terminated copy of the input, for parsers that take a C string. */
static char *
mayhem_cstr(const uint8_t *data, size_t size)
{
    char *s = malloc(size + 1);
    if (s == NULL)
        return NULL;
    memcpy(s, data, size);
    s[size] = '\0';
    return s;
}

/* The stock en-us decoder, built once and reused across inputs. */
static ps_decoder_t *
mayhem_decoder(void)
{
    ps_config_t *config;
    ps_decoder_t *ps;

    config = ps_config_parse_json(
        NULL,
        "hmm: \"" MODELDIR "/en-us/en-us\","
        "lm: \"" MODELDIR "/en-us/en-us.lm.bin\","
        "dict: \"" MODELDIR "/en-us/cmudict-en-us.dict\","
        "fwdtree: true, fwdflat: false, bestpath: false,"
        "samprate: 16000, loglevel: \"FATAL\"");
    if (config == NULL)
        abort();
    ps = ps_init(config);
    if (ps == NULL)
        abort();
    return ps;
}

#endif
