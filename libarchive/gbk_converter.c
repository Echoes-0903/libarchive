/*
 * Simple GBK to UTF-8 converter using complete mapping table
 * Uses tab_gbk2uni.h for accurate GBK conversion
 */

#include "archive_platform.h"

#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "tab_gbk2uni.h"
#include "gbk_converter.h"

/* Simple wrapper for GBK to UTF-8 conversion */
size_t
simple_gbk_to_utf8(const char *input, size_t input_len, 
                   char *output, size_t output_len) 
{
    int err = 0;
    char *converted = gbk2utf8((const unsigned char *)input, input_len, &err);
    
    /* If strict validation fails (err == -2), try lenient conversion
     * This handles files with slightly malformed GBK or mixed encodings */
    if (converted == NULL && err == -2) {
        /* Retry without strict validation by calling the converter directly
         * Some Windows-created ZIPs have GBK filenames that fail strict validation
         * but can still be converted successfully */
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

/* Check if string is valid GBK encoding */
int
is_gbk_encoding(const char *str, size_t len)
{
    return is_valid_gbk((const unsigned char *)str, len);
}