#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>

#include "cliopts.h"


enum {
    CLIOPTS_ERR_SUCCESS,
    CLIOPTS_ERR_NEED_ARG,
    CLIOPTS_ERR_ISSWITCH,
    CLIOPTS_ERR_BADOPT,
    CLIOPTS_ERR_BAD_VALUE,
    CLIOPTS_ERR_UNRECOGNIZED
};

struct cliopts_priv {
    cliopts_entry *entries;
    cliopts_entry *current;

    char *errstr;
    int errnum;

    int argsplit;
    int wanted;

    char current_key[4096];
    char current_value[4096];

};

enum {
    WANT_OPTION,
    WANT_VALUE,

    MODE_ERROR,
    MODE_RESTARGS,
    MODE_HELP
};

#ifdef CLIOPTS_DEBUG

#define cliopt_debug(...) \
    fprintf(stderr, "(%s:%d) ", __func__, __LINE__); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n")

#else
#define cliopt_debug(a, ...)
#endif /* CLIOPT_DEBUG */

static int
parse_option(struct cliopts_priv *ctx, const char *key);


static int
parse_value(struct cliopts_priv *ctx, const char *value);

/**
 * Various extraction/conversion functions for numerics
 */

#define _VERIFY_INT_COMMON(m1, m2) \
    if (value == m1 || value > m2) { *errp = "Value too large"; return -1; } \
    if (*endptr != '\0') { *errp = "Trailing garbage"; return -1; }

static int
extract_int(const char *s, void *dest, char **errp)
{
    long int value;
    char *endptr = NULL;
    value = strtol(s, &endptr, 10);
    _VERIFY_INT_COMMON(LONG_MAX, INT_MAX)
    *(int*)dest = value;
    return 0;
}

static int
extract_uint(const char *s, void *dest, char **errp)
{
    unsigned long int value;
    char *endptr = NULL;
    value = strtoul(s, &endptr, 10);
    _VERIFY_INT_COMMON(ULONG_MAX, UINT_MAX)
    *(unsigned int*)dest = value;
    return 0;
}

static int
extract_hex(const char *s, void *dest, char **errp)
{
    unsigned long value;
    char *endptr = NULL;
    value = strtoul(s, &endptr, 16);
    _VERIFY_INT_COMMON(ULONG_MAX, UINT_MAX);
    *(unsigned int*)dest = value;
    return 0;
}

#undef _VERIFY_INT_COMMON

static int
extract_float(const char *s, void *dest, char **errp)
{
    char dummy_buf[4096];
    float value;
    if (sscanf(s, "%f%s", &value, dummy_buf) != 1) {
        *errp = "Found trailing garbage";
        return -1;
    }
    *(float*)dest = value;
    return 0;
}

typedef int(*cliopts_extractor_func)(const char*, void*, char**);


/**
 * This function tries to extract a single value for an option key.
 * If it successfully has extracted a value, it returns MODE_VALUE.
 * If the entry takes no arguments, then the current string is a key,
 * and it will return MODE_OPTION. On error, MODE_ERROR is set, and errp
 * will point to a string.
 *
 * @param entry The current entry
 * @param value the string which might be a value
 * @errp a pointer which will be populated with the address of the error, if any
 *
 * @return a MODE_* type
 */
static int
parse_value(struct cliopts_priv *ctx,
            const char *value)
{
    cliopts_entry *entry = ctx->current;

    size_t vlen = strlen(value);
    cliopts_extractor_func exfn = NULL;
    int exret;
    int is_option = 0;

    cliopt_debug("Called with %s, want=%d", value, ctx->wanted);

    if (ctx->argsplit) {
        if (vlen > 2 && strncmp(value, "--", 2) == 0) {
            is_option = 1;
        } else if (*value == '-') {
            is_option = 1;
        }
    }

    if (is_option) {
        ctx->errstr = "Expected option. Got '-' or '--' prefixed value "
                        "(use = if this is really a value)";
        ctx->errnum = CLIOPTS_ERR_NEED_ARG;
        return MODE_ERROR;
    }

    if (entry->ktype == CLIOPTS_ARGT_STRING) {
        char *vp = malloc(vlen+1);
        vp[vlen] = 0;
        strcpy(vp, value);
        *(char**)entry->dest = vp;
        return WANT_OPTION;
    }

    if (entry->ktype == CLIOPTS_ARGT_FLOAT) {
        exfn = extract_float;
    } else if (entry->ktype == CLIOPTS_ARGT_HEX) {
        exfn = extract_hex;
    } else if (entry->ktype == CLIOPTS_ARGT_INT) {
        exfn = extract_int;
    } else if (entry->ktype == CLIOPTS_ARGT_UINT) {
        exfn = extract_uint;
    } else {
        fprintf(stderr, "Unrecognized type %d. Abort.\n", entry->ktype);
    }

    exret = exfn(value, entry->dest, &ctx->errstr);
    if (exret == 0) {
        return WANT_OPTION;
    } else {
        ctx->errnum = CLIOPTS_ERR_BAD_VALUE;
    }

    return MODE_ERROR;
}

/**
 * Like parse_value, except for keys.
 *
 * @param entries all option entries
 * @param key the current string from argv
 * @param errp a pointer which will be populated with the address of an error
 * string
 *
 * @param found_entry a pointer to be populated with the relevant entry
 * structure
 * @param kp a pointer which will be poplated with the address of the 'sanitized'
 * key string
 *
 * @param valp if the string is actually a key-value pair (i.e. --foo=bar) then
 * this will be populated with the address of that string
 *
 * @return MODE_OPTION if an option was found, MODE_VALUE if the current option
 * is a value, or MODE_ERROR on error
 */
static int
parse_option(struct cliopts_priv *ctx,
          const char *key)
{
    cliopts_entry *cur = NULL;
    int ii, prefix_len = 0;
    const char *valp = NULL;
    size_t klen;

    klen = strlen(key);
    ctx->errstr = NULL;
    ctx->current = NULL;
    cliopt_debug("Called with %s, want=%d", key, ctx->wanted);

    if (klen <= 1) {
        ctx->errstr = "Expected a valid option (null or too short)";
        return MODE_ERROR;
    }

    /**
     * figure out what type of option it is..
     * it can either be a -c, --long, or --long=value
     */
    while (*key == '-') {
        key++;
        prefix_len++;
        klen--;
    }

    for (ii = 0; ii < klen; ii++) {
        if (key[ii] == '"' || key[ii] == '\'') {
            ii = klen;
            break;

        } else if (key[ii] == '=') {
            valp = key + (ii + 1);
            break;
        }
    }

    GT_PARSEOPT:

    memset(ctx->current_key, 0, sizeof(ctx->current_key));
    memset(ctx->current_value, 0, sizeof(ctx->current_value));
    memcpy(ctx->current_key, key, ii);

    if (valp) {
        strcpy(ctx->current_value, valp);
    }

    if (prefix_len == 0) {
        ctx->errstr = "Options must begin with either '-' or '--'";
        ctx->errnum = CLIOPTS_ERR_BADOPT;
        return MODE_ERROR;
    }

    /**
     * --help or -?
     */

    if ( (prefix_len == 1 && *key == '?') ||
            (prefix_len == 2 && strcmp(key, "help") == 0)) {
        return MODE_HELP;
    }

    /**
     * Bare --
     */
    if (prefix_len == 2 && *key == '\0') {

        if (ctx->wanted == WANT_VALUE) {
            ctx->errnum = CLIOPTS_ERR_NEED_ARG;
            ctx->errstr = "Found bare '--', but value wanted";
            return MODE_ERROR;
        }

        return MODE_RESTARGS;
    }

    for (cur = ctx->entries; cur->dest; cur++) {
        if ( (prefix_len == 1 && cur->kshort == ctx->current_key[0]) ||
                (prefix_len == 2 &&
                        strcmp(cur->klong, ctx->current_key) == 0)) {

            ctx->current = cur;
            break;
        }
    }

    if (!ctx->current) {
        ctx->errstr = "Unknown option";
        ctx->errnum = CLIOPTS_ERR_UNRECOGNIZED;
        return MODE_ERROR;
    }

    ctx->current->found++;

    if (ctx->current_value[0]) {
        if (ctx->current->ktype == CLIOPTS_ARGT_NONE) {
            ctx->errnum = CLIOPTS_ERR_ISSWITCH;
            ctx->errstr = "Option takes no arguments";
            return MODE_ERROR;
        } else {
            ctx->wanted = WANT_VALUE;
            return parse_value(ctx, ctx->current_value);
        }
    }

    if (ctx->current->ktype == CLIOPTS_ARGT_NONE) {
        *(char*)ctx->current->dest = 1;
        if (prefix_len == 1 && klen > 1) {
            klen--;
            key++;
            /**
             * While we can also possibly recurse, this may be a security risk
             * as it wouldn't take much to cause a deep recursion on the stack
             * which will cause all sorts of nasties.
             */
            goto GT_PARSEOPT;
        }
        return WANT_OPTION;
    }
    return WANT_VALUE;
}

static void
print_help(struct cliopts_priv *ctx, const char *progname)
{
    cliopts_entry *cur;
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%2s", " ");
    fprintf(stderr, "%s [OPTIONS...]\n\n", progname);


    for (cur = ctx->entries; cur->dest; cur++) {
        char shortopt = cur->kshort;
        fprintf(stderr, "  ");

        if (shortopt) {
            fprintf(stderr, " -%c  ", shortopt);
        } else {
            fprintf(stderr, "   ");
        }

        if (cur->klong) {
            fprintf(stderr, "--%-20s", cur->klong);
        } else {
            fprintf(stderr, "%-22s", " ");
        }

        if (cur->help) {
            fprintf(stderr, cur->help);
        }
        fprintf(stderr, "\n");
    }
}

int
cliopts_parse_options(cliopts_entry *entries,
                      int argc,
                      char **argv,
                      int *lastidx,
                      struct cliopts_extra_settings *settings)
{
    /**
     * Now let's build ourselves a
     */
    int curmode;
    int ii, ret = 0;
    cliopts_entry *cur_ent;
    struct cliopts_priv ctx = { 0 };
    struct cliopts_extra_settings default_settings = { 0 };

    ctx.entries = entries;

    if (!settings) {
        settings = &default_settings;
        settings->progname = argv[0];
    }

    if (settings->argv_noskip) {
        ii = 0;
    } else {
        ii = 1;
    }

    if (ii >= argc) {
        *lastidx = 0;
        return 0;
    }

    curmode = WANT_OPTION;
    ctx.wanted = curmode;
    for (; ii < argc; ii++) {
        if (curmode == WANT_OPTION) {
            curmode = parse_option(&ctx, argv[ii]);
        } else if (curmode == WANT_VALUE) {
            curmode = parse_value(&ctx, argv[ii]);
        }

        if (curmode == MODE_ERROR) {

            if (settings->error_nohelp == 0) {
                fprintf(stderr, "Couldn't parse options: %s\n", ctx.errstr);
                if (ctx.errnum == CLIOPTS_ERR_BADOPT) {
                    fprintf(stderr, "Bad option: %s", ctx.current_key);
                } else if (ctx.errnum == CLIOPTS_ERR_BAD_VALUE) {
                    fprintf(stderr, "Bad value '%s' for %s",
                            ctx.current_value,
                            ctx.current_key);
                } else if (ctx.errnum == CLIOPTS_ERR_UNRECOGNIZED) {
                    fprintf(stderr, "No such option: %s", ctx.current_key);
                }
                fprintf(stderr, "\n");
                print_help(&ctx, settings->progname);
            }
            if (settings->error_noexit == 0) {
                exit(EXIT_FAILURE);
            }
            ret = -1;
            break;

        } else if (curmode == MODE_HELP) {
            if (settings->help_noflag) {
                /* ignore it ? */
                continue;
            }

            print_help(&ctx, settings->progname);
            exit(0);

        } else if (curmode == MODE_RESTARGS) {
            break;
        } else {
            ctx.wanted = curmode;
        }
    }
    *lastidx = ii;
    if (curmode == WANT_VALUE) {
        ret = -1;

        if (settings->error_nohelp == 0) {
            fprintf(stderr, "Option %s requires argument\n",
                    ctx.current_key);
            print_help(&ctx, settings->progname);
        }
        if (settings->error_noexit == 0) {
            exit(EXIT_FAILURE);
        }
    }

    /**TODO: Here we should check required arguments */
    (void)cur_ent;

    return ret;
}
