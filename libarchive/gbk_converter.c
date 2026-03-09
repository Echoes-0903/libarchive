/*
 * GBK to UTF-8 converter
 * Priority: iconv (system native) > custom table (fallback)
 */

#include "archive_platform.h"

/* Try to use iconv - available on Android API 28+, Linux (non-Android), macOS */
#if (!defined(__ANDROID__) && (defined(__linux__) || defined(__APPLE__))) || \
    (defined(__ANDROID__) && defined(__ANDROID_API__) && __ANDROID_API__ >= 28) || \
    defined(HAVE_ICONV_H)
#include <iconv.h>
#define USE_ICONV 1
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "tab_gbk2uni.h"
#include "gbk_converter.h"

/* Convert using iconv if available */
static size_t
iconv_gbk_to_utf8(const char *input, size_t input_len, 
                 char *output, size_t output_len)
{
#ifdef USE_ICONV
    iconv_t cd = iconv_open("UTF-8", "GBK");
    if (cd == (iconv_t)-1) {
        /* iconv_open failed, try GB18030 which is a superset of GBK */
        cd = iconv_open("UTF-8", "GB18030");
        if (cd == (iconv_t)-1) {
            return (size_t)-1;
        }
    }
    
    char *inbuf = (char *)input;
    char *outbuf = output;
    size_t inbytesleft = input_len;
    size_t outbytesleft = output_len - 1; /* Reserve space for null terminator */
    
    size_t result = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    iconv_close(cd);
    
    if (result == (size_t)-1) {
        return (size_t)-1;
    }
    
    /* Null terminate */
    *outbuf = '\0';
    size_t converted_len = output_len - 1 - outbytesleft;
    
    return converted_len;
#else
    return (size_t)-1;
#endif
}

/* Fallback using custom GBK table */
static size_t
table_gbk_to_utf8(const char *input, size_t input_len, 
                 char *output, size_t output_len)
{
    int err = 0;
    char *converted = gbk2utf8((const unsigned char *)input, input_len, &err);
    
    /* If strict validation fails (err == -2), try lenient conversion */
    if (converted == NULL && err == -2) {
        converted = gbk2utf8_lenient((const unsigned char *)input, input_len, &err);
    }
    
    if (converted == NULL || err != 0) {
        return (size_t)-1;
    }
    
    size_t converted_len = strlen(converted);
    
    if (converted_len >= output_len) {
        free(converted);
        return (size_t)-1;
    }
    
    strcpy(output, converted);
    free(converted);
    
    return converted_len;
}

/* Simple wrapper for GBK to UTF-8 conversion */
size_t
simple_gbk_to_utf8(const char *input, size_t input_len, 
                   char *output, size_t output_len) 
{
    /* Try system iconv first (more reliable) */
    size_t result = iconv_gbk_to_utf8(input, input_len, output, output_len);
    if (result != (size_t)-1) {
        return result;
    }
    
    /* Fallback to custom table conversion */
    return table_gbk_to_utf8(input, input_len, output, output_len);
}

/* Check if string is valid GBK encoding */
int
is_gbk_encoding(const char *str, size_t len)
{
    return is_valid_gbk((const unsigned char *)str, len);
}