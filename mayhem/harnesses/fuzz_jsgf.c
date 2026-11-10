/* Parse a JSGF grammar and compile it to a finite-state grammar. */
#include "common.h"

static logmath_t *lmath;

int
LLVMFuzzerInitialize(int *argc, char ***argv)
{
    (void)argc;
    (void)argv;
    mayhem_quiet();
    lmath = logmath_init(1.0001, 0, 0);
    if (lmath == NULL)
        abort();
    return 0;
}

int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    char *str;
    jsgf_t *jsgf;
    jsgf_rule_iter_t *it;
    fsg_model_t *fsg;
    char *buf = NULL;
    size_t buflen = 0;
    FILE *out;

    if (size == 0)
        return -1;
    str = mayhem_cstr(data, size);
    if (str == NULL)
        return 0;
    jsgf = jsgf_parse_string(str, NULL);
    if (jsgf == NULL) {
        free(str);
        return -1;
    }
    for (it = jsgf_rule_iter(jsgf); it; it = jsgf_rule_iter_next(it)) {
        jsgf_rule_t *rule = jsgf_rule_iter_rule(it);
        (void)jsgf_rule_name(rule);
        fsg = jsgf_build_fsg(jsgf, rule, lmath, 1.0f);
        if (fsg) {
            fsg_model_accept(fsg, "go forward ten meters");
            fsg_model_free(fsg);
        }
    }
    out = open_memstream(&buf, &buflen);
    if (out) {
        jsgf_write_fsg(jsgf, jsgf_get_public_rule(jsgf), out);
        fclose(out);
        free(buf);
    }
    jsgf_grammar_free(jsgf);
    free(str);
    return 0;
}
