/*
 * Unified charset converter using JNI
 * Supports GBK, CP932, Shift-JIS and other encodings via Java Charset API
 */

#ifndef ARCHIVE_CHARSET_CONVERTER_JNI_H
#define ARCHIVE_CHARSET_CONVERTER_JNI_H

#include "archive_platform.h"

#ifdef __ANDROID__
#include <jni.h>

/* Initialize converter with JavaVM pointer (call from JNI_OnLoad or Java static init) */
void charset_converter_set_jvm(JavaVM *jvm);

/* JNI native method to initialize from Java side */
JNIEXPORT void JNICALL
Java_com_ponyemu_common_ArchiveUtil_initCharsetConverter(JNIEnv *env, jclass clazz);

/* Convert from any charset to UTF-8 using Java Charset API */
size_t charset_to_utf8(const char *charset, 
                       const char *input, size_t input_len,
                       char *output, size_t output_len);

/* Convenience wrappers for common encodings */
size_t gbk_to_utf8(const char *input, size_t input_len,
                   char *output, size_t output_len);

size_t cp932_to_utf8(const char *input, size_t input_len,
                     char *output, size_t output_len);

size_t shiftjis_to_utf8(const char *input, size_t input_len,
                        char *output, size_t output_len);

#endif /* __ANDROID__ */

#endif /* ARCHIVE_CHARSET_CONVERTER_JNI_H */
