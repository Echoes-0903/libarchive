/*
 * Unified charset converter using JNI Java Charset API
 * Works on all Android API levels
 */

#include "charset_converter_jni.h"

#ifdef __ANDROID__

#include <jni.h>
#include <string.h>

/* Force enable logging for debugging */
#include <android/log.h>
#define LOG_TAG "CHARSET_CONVERTER_JNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/* Global JavaVM pointer */
static JavaVM *g_jvm = NULL;

void charset_converter_set_jvm(JavaVM *jvm) {
    g_jvm = jvm;
    LOGD("JavaVM pointer initialized: %p", jvm);
}

/* JNI method callable from Java to initialize the converter */
JNIEXPORT void JNICALL
Java_com_ponyemu_common_ArchiveUtil_initCharsetConverter(JNIEnv *env, jclass clazz) {
    if ((*env)->GetJavaVM(env, &g_jvm) != 0) {
        LOGE("Failed to get JavaVM from JNIEnv");
        return;
    }
    LOGD("JavaVM initialized via JNI call: %p", g_jvm);
}

/* Check if converter is initialized */
static int is_initialized() {
    if (g_jvm == NULL) {
        LOGE("JavaVM not initialized - charset conversion unavailable");
        return 0;
    }
    return 1;
}

/* Core JNI conversion function */
size_t charset_to_utf8(const char *charset,
                       const char *input, size_t input_len,
                       char *output, size_t output_len)
{
    if (!is_initialized()) {
        LOGE("Converter not initialized!");
        return (size_t)-1;
    }
    
    if (input == NULL || output == NULL || charset == NULL || 
        input_len == 0 || output_len == 0) {
        LOGE("Invalid parameters: charset=%p input=%p output=%p, in_len=%zu out_len=%zu",
             charset, input, output, input_len, output_len);
        return (size_t)-1;
    }
    
    LOGD("Converting %zu bytes using charset: %s", input_len, charset);
    
    JNIEnv *env = NULL;
    int getEnvResult = (*g_jvm)->GetEnv(g_jvm, (void**)&env, JNI_VERSION_1_6);
    jboolean needDetach = JNI_FALSE;
    
    if (getEnvResult == JNI_EDETACHED) {
        if ((*g_jvm)->AttachCurrentThread(g_jvm, &env, NULL) != 0) {
            LOGE("Failed to attach thread");
            return (size_t)-1;
        }
        needDetach = JNI_TRUE;
    } else if (getEnvResult != JNI_OK) {
        LOGE("GetEnv failed: %d", getEnvResult);
        return (size_t)-1;
    }
    
    /* Create byte array from input */
    jbyteArray byteArray = (*env)->NewByteArray(env, (jsize)input_len);
    if (byteArray == NULL) {
        LOGE("Failed to create byte array");
        if (needDetach) (*g_jvm)->DetachCurrentThread(g_jvm);
        return (size_t)-1;
    }
    
    (*env)->SetByteArrayRegion(env, byteArray, 0, (jsize)input_len, (const jbyte*)input);
    
    /* Create charset string */
    jstring charsetStr = (*env)->NewStringUTF(env, charset);
    if (charsetStr == NULL) {
        LOGE("Failed to create charset string");
        (*env)->DeleteLocalRef(env, byteArray);
        if (needDetach) (*g_jvm)->DetachCurrentThread(g_jvm);
        return (size_t)-1;
    }
    
    /* Get String class and constructor */
    jclass stringClass = (*env)->FindClass(env, "java/lang/String");
    if (stringClass == NULL) {
        LOGE("Failed to find String class");
        (*env)->DeleteLocalRef(env, byteArray);
        (*env)->DeleteLocalRef(env, charsetStr);
        if (needDetach) (*g_jvm)->DetachCurrentThread(g_jvm);
        return (size_t)-1;
    }
    
    jmethodID constructor = (*env)->GetMethodID(env, stringClass, "<init>", "([BLjava/lang/String;)V");
    if (constructor == NULL) {
        LOGE("Failed to find String constructor");
        (*env)->DeleteLocalRef(env, byteArray);
        (*env)->DeleteLocalRef(env, charsetStr);
        (*env)->DeleteLocalRef(env, stringClass);
        if (needDetach) (*g_jvm)->DetachCurrentThread(g_jvm);
        return (size_t)-1;
    }
    
    /* Create String object from bytes with specified charset */
    jobject stringObj = (*env)->NewObject(env, stringClass, constructor, byteArray, charsetStr);
    if (stringObj == NULL || (*env)->ExceptionCheck(env)) {
        if ((*env)->ExceptionCheck(env)) {
            LOGE("Exception creating String with charset %s", charset);
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }
        (*env)->DeleteLocalRef(env, byteArray);
        (*env)->DeleteLocalRef(env, charsetStr);
        (*env)->DeleteLocalRef(env, stringClass);
        if (needDetach) (*g_jvm)->DetachCurrentThread(g_jvm);
        return (size_t)-1;
    }
    
    /* Get UTF-8 bytes from String */
    const char *utf8Str = (*env)->GetStringUTFChars(env, (jstring)stringObj, NULL);
    if (utf8Str == NULL) {
        LOGE("Failed to get UTF-8 string");
        (*env)->DeleteLocalRef(env, byteArray);
        (*env)->DeleteLocalRef(env, charsetStr);
        (*env)->DeleteLocalRef(env, stringClass);
        (*env)->DeleteLocalRef(env, stringObj);
        if (needDetach) (*g_jvm)->DetachCurrentThread(g_jvm);
        return (size_t)-1;
    }
    
    size_t utf8Len = strlen(utf8Str);
    if (utf8Len >= output_len) {
        LOGE("Output buffer too small: need %zu, have %zu", utf8Len + 1, output_len);
        (*env)->ReleaseStringUTFChars(env, (jstring)stringObj, utf8Str);
        (*env)->DeleteLocalRef(env, byteArray);
        (*env)->DeleteLocalRef(env, charsetStr);
        (*env)->DeleteLocalRef(env, stringClass);
        (*env)->DeleteLocalRef(env, stringObj);
        if (needDetach) (*g_jvm)->DetachCurrentThread(g_jvm);
        return (size_t)-1;
    }
    
    /* Copy result */
    memcpy(output, utf8Str, utf8Len);
    output[utf8Len] = '\0';
    
    LOGD("Successfully converted %zu bytes from %s to UTF-8 (%zu bytes)", 
         input_len, charset, utf8Len);
    
    /* Cleanup */
    (*env)->ReleaseStringUTFChars(env, (jstring)stringObj, utf8Str);
    (*env)->DeleteLocalRef(env, byteArray);
    (*env)->DeleteLocalRef(env, charsetStr);
    (*env)->DeleteLocalRef(env, stringClass);
    (*env)->DeleteLocalRef(env, stringObj);
    
    if (needDetach) {
        (*g_jvm)->DetachCurrentThread(g_jvm);
    }
    
    return utf8Len;
}

/* GBK conversion with fallback to GB18030 */
size_t gbk_to_utf8(const char *input, size_t input_len,
                   char *output, size_t output_len)
{
    LOGD("gbk_to_utf8 called: input_len=%zu, output_len=%zu", input_len, output_len);
    
    if (!is_initialized()) {
        LOGE("Cannot convert GBK - JavaVM not initialized");
        return (size_t)-1;
    }
    
    size_t result = charset_to_utf8("GBK", input, input_len, output, output_len);
    if (result != (size_t)-1) {
        LOGD("GBK conversion succeeded: %zu bytes", result);
        return result;
    }
    
    LOGD("GBK failed, trying GB18030");
    /* Try GB18030 (superset of GBK) */
    result = charset_to_utf8("GB18030", input, input_len, output, output_len);
    if (result != (size_t)-1) {
        LOGD("GB18030 conversion succeeded: %zu bytes", result);
    } else {
        LOGE("Both GBK and GB18030 conversion failed");
    }
    return result;
}

/* CP932 conversion with fallback to Shift_JIS variants */
size_t cp932_to_utf8(const char *input, size_t input_len,
                     char *output, size_t output_len)
{
    LOGD("cp932_to_utf8 called: input_len=%zu, output_len=%zu", input_len, output_len);
    
    if (!is_initialized()) {
        LOGE("Cannot convert CP932 - JavaVM not initialized");
        return (size_t)-1;
    }
    
    size_t result = charset_to_utf8("CP932", input, input_len, output, output_len);
    if (result != (size_t)-1) {
        LOGD("CP932 conversion succeeded: %zu bytes", result);
        return result;
    }
    
    LOGD("CP932 failed, trying Windows-31J");
    /* Try Windows-31J (another name for CP932) */
    result = charset_to_utf8("Windows-31J", input, input_len, output, output_len);
    if (result != (size_t)-1) {
        LOGD("Windows-31J conversion succeeded: %zu bytes", result);
        return result;
    }
    
    LOGD("Windows-31J failed, trying Shift_JIS");
    /* Try Shift_JIS */
    result = charset_to_utf8("Shift_JIS", input, input_len, output, output_len);
    if (result != (size_t)-1) {
        LOGD("Shift_JIS conversion succeeded: %zu bytes", result);
    } else {
        LOGE("All CP932 variants conversion failed");
    }
    return result;
}

/* Shift-JIS conversion */
size_t shiftjis_to_utf8(const char *input, size_t input_len,
                        char *output, size_t output_len)
{
    size_t result = charset_to_utf8("Shift_JIS", input, input_len, output, output_len);
    if (result != (size_t)-1) {
        return result;
    }
    
    /* Try SJIS variant */
    return charset_to_utf8("SJIS", input, input_len, output, output_len);
}

#endif /* __ANDROID__ */
