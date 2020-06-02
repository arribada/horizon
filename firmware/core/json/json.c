/* This is free and unencumbered software released into the public domain.
 * 
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * 
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 *
 * by jeremie miller - 2014
 * contributions/improvements welcome via github at https://github.com/quartzjer/js0n
 */

#include <string.h> // one strncmp() is used to do key comparison, and a strlen(key) if no len passed in
#include <assert.h>

// only at depth 1, track start pointers to match key/value
#define PUSH(i)                 \
if(depth == 1)                  \
{                               \
    if (!index)                 \
    {                           \
        val = cur + i;          \
    }                           \
    else                        \
    {                           \
        if (klen && index == 1) \
            start = cur + i;    \
        else                    \
            index--;            \
    }                           \
}

// determine if key matches or value is complete
#define CAP(i)                                                                              \
if (depth == 1)                                                                             \
{                                                                                           \
    if (val && !index)                                                                      \
    {                                                                                       \
        *vlen = (size_t)((cur + i + 1) - val);                                              \
        return val;                                                                         \
    };                                                                                      \
    if (klen && start)                                                                      \
    {                                                                                       \
        index = (klen == (size_t)(cur - start) && strncmp(key, start, klen) == 0) ? 0 : 2;  \
        start = 0;                                                                          \
    }                                                                                       \
}

enum state
{
    S_STRUCT,
    S_BARE,
    S_STRING,
    S_UTF8,
    S_ESC,
};

#define range(x,s,e) ((x) >= (s) && (x) <= (e))

// this makes a single pass across the json bytes, using each byte as an index into a jump table to build an index and transition state
// key = string to match or null
// klen = key length (or 0), or if null key then len is the array offset value
// json = json object or array
// jlen = length of json
// vlen = where to store return value length
// returns pointer to value and sets len to value length, or 0 if not found
// any parse error will set vlen to the position of the error
const char * json_parse(const char * key, size_t klen, const char * json, size_t jlen, size_t * vlen)
{
    const char *val = 0;
    const char *cur, *end, *start;
    size_t index = 1;
    int depth = 0;
    int utf8_remain = 0;
    enum state state = S_STRUCT;

    if (!json || !jlen || !vlen)
        return NULL;

    *vlen = 0;

    // no key is array mode, klen provides requested index
    if (!key)
    {
        index = klen;
        klen = 0;
    }
    else
    {
        if (!klen)
            klen = strlen(key); // convenience
    }

    for (start = cur = json, end = cur + jlen; cur < end; ++cur)
    {
again:
        switch (state)
        {
            case S_STRUCT:
                switch (*cur)
                {
                    case '\t':
                    case ' ':
                    case '\r':
                    case '\n':
                    case ':':
                    case ',':
                        continue;

                    case '"': goto l_qup;
                    case '[': goto l_up;
                    case ']': goto l_down;
                    case '{': goto l_up;
                    case '}': goto l_down;

                    case '-': goto l_bare;
                    default:
                    {
                        if (range(*cur, '0', '9') ||
                            range(*cur, 'A', 'Z') ||
                            range(*cur, 'a', 'z'))
                            goto l_bare;
                        else
                            goto l_bad;
                    }
                }
                assert(0);
            case S_BARE:
                switch (*cur)
                {
                    case '\t':
                    case ' ':
                    case '\r':
                    case '\n':
                    case ',':
                    case ']':   // correct? not [ ?
                    case '}':   // correct? not { ?
                    case ':':
                        goto l_unbare;
                    default:
                    {
                        // could be more pedantic/validation-checking
                        if (range(*cur, 32, 126))
                            continue;
                        goto l_bad;
                    }
                }
                assert(0);
            case S_STRING:
                if (*cur == '\\')
                {
                    state = S_ESC;
                    continue;
                }
                if (*cur == '"')
                    goto l_qdown;
                if (range(*cur, 32, 126))
                    continue;
                if ((*cur & 224) == 192)   // range(*cur, 192, 223))
                {
                    state = S_UTF8;
                    utf8_remain = 1;
                    continue;
                }
                if ((*cur & 240) == 224)   // range(*cur, 224, 239)
                {
                    state = S_UTF8;
                    utf8_remain = 2;
                    continue;
                }
                if ((*cur & 248) == 240)   // range(*cur, 240, 247)
                {
                    state = S_UTF8;
                    utf8_remain = 3;
                    continue;
                }
                goto l_bad;
            // XXX no utf8 outside strings?
            case S_UTF8:
                if ((*cur & 192) == 128)   // range(*cur, 128, 191)
                {
                    if (!--utf8_remain)
                        state = S_STRING;
                    continue;
                }
                goto l_bad;
            case S_ESC:
                switch (*cur)
                {
                    case '"':
                    case '\\':
                    case '/':
                    case 'b':
                    case 'f':
                    case 'n':
                    case 'r':
                    case 't':
                    case 'u':
                        state = S_STRING;
                        continue;
                    default:
                        goto l_bad;
                }
        }
        assert (0);
l_bad:
        *vlen = cur - json; // where error'd
        return NULL;

l_up:
        PUSH(0);
        ++depth;
        continue;

l_down:
        --depth;
        CAP(0);
        continue;

l_qup:
        PUSH(1);
        state = S_STRING;
        continue;

l_qdown:
        CAP(-1);
        state = S_STRUCT;
        continue;

l_bare:
        PUSH(0);
        state = S_BARE;
        continue;

l_unbare:
        CAP(-1);
        state = S_STRUCT;
        goto again;

    }

    if (depth)
    {
        *vlen = jlen; // incomplete
    }
    return NULL;
}
