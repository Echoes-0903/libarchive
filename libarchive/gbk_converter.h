/*
 * GBK converter header
 */

#ifndef ARCHIVE_GBK_CONVERTER_H
#define ARCHIVE_GBK_CONVERTER_H

#include "archive_platform.h"

/* Simple wrapper for GBK to UTF-8 conversion */
size_t simple_gbk_to_utf8(const char *input, size_t input_len, 
                          char *output, size_t output_len);

/* Check if string is valid GBK encoding */
int is_gbk_encoding(const char *str, size_t len);

#endif /* ARCHIVE_GBK_CONVERTER_H */