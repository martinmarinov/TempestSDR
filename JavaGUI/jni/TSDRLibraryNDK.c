#include <jni.h>
#include <stdio.h>
#include "TSDRLibraryNDK.h"
#include "include\TSDRLibrary.h"
#include "include\TSDRCodes.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

tsdr_lib_t tsdr;

#define THROW(x) error(env, x, "")

struct java_context {
		jobject obj;
	} typedef java_context_t;

static JavaVM *jvm;
static int javaversion;

void error_translate (int exception_code, char * exceptionclass) {
	switch (exception_code) {
		case TSDR_OK:
			return;
		case TSDR_ERR_PLUGIN:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRLoadPluginException");
			return;
		case TSDR_NOT_IMPLEMENTED:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRFunctionNotImplemented");
			return;
		default:
			strcpy(exceptionclass, "java/lang/Exception");
			return;
		}
}

void error(JNIEnv * env, int exception_code, const char *inmsg, ...)
{
	if (exception_code == TSDR_OK) return;

	char exceptionclass[256];
	error_translate(exception_code, exceptionclass);

	char msg[256];

	va_list argptr;
	va_start(argptr, inmsg);
	vsprintf(msg, inmsg, argptr);
	va_end(argptr);

    jclass cls = (*env)->FindClass(env, exceptionclass);
    /* if cls is NULL, an exception has already been thrown */
    if (cls != NULL) {
        (*env)->ThrowNew(env, cls, msg);
    }
    /* free the local ref */
    (*env)->DeleteLocalRef(env, cls);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_init (JNIEnv * env, jobject obj) {
	(*env)->GetJavaVM(env, &jvm);
	javaversion = (*env)->GetVersion(env);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_nativeLoadPlugin (JNIEnv * env, jobject obj, jstring  path) {
	const char *npath = (*env)->GetStringUTFChars(env, path, 0);
	THROW(tsdr_loadplugin(&tsdr, npath));
	(*env)->ReleaseStringUTFChars(env, path, npath);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_pluginParams (JNIEnv * env, jobject obj, jstring params) {
	const char *nparams = (*env)->GetStringUTFChars(env, params, 0);
	THROW(tsdr_pluginparams(&tsdr, nparams));
	(*env)->ReleaseStringUTFChars(env, params, nparams);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setSampleRate (JNIEnv * env, jobject obj, jlong rate) {
	THROW(tsdr_setsamplerate(&tsdr, (uint32_t) rate));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setBaseFreq (JNIEnv * env, jobject obj, jlong freq) {
	THROW(tsdr_setbasefreq(&tsdr, (uint32_t) freq));
}

void read_async(float *buf, uint32_t len, void *ctx) {
	java_context_t * context = (java_context_t *) ctx;
	JNIEnv *env;

	if ((*jvm)->GetEnv(jvm, (void **)&env, javaversion) == JNI_EDETACHED)
		(*jvm)->AttachCurrentThread(jvm, (void **) &env, 0);

	const int width = 800;
	const int height = 600;

	jclass cls = (*env)->GetObjectClass(env, context->obj);

	// fixSize(200, 200);
	jmethodID fixSize = (*env)->GetMethodID(env, cls, "fixSize", "(II)V");
	(*env)->CallVoidMethod(env, context->obj, fixSize, width, height);

	jintArray pixels_obj = (*env)->GetObjectField(env, context->obj, (*env)->GetFieldID(env, cls, "pixels", "[I"));
	jint * pixels = (*env)->GetIntArrayElements(env,pixels_obj,0);

	int i;
	const int size = width * height;
	for (i = 0; i < size; i++) {
		const int x = i % width;
		const int y = i / width;

		const float rat = y / (float) height;
		const int col = (int) (buf[i] * 255.0f);
		pixels[i] = col | (col << 8) | (col << 16);
	}

	// release elements
	(*env)->ReleaseIntArrayElements(env,pixels_obj,pixels,0);

	// notifyCallbacks();
	jmethodID notifyCallbacks = (*env)->GetMethodID(env, cls, "notifyCallbacks", "()V");
	(*env)->CallVoidMethod(env, context->obj, notifyCallbacks);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_start (JNIEnv * env, jobject obj) {
	java_context_t * context = (java_context_t *) malloc(sizeof(java_context_t));
	context->obj = (*env)->NewGlobalRef(env, obj);
	(*env)->DeleteLocalRef(env, obj);

	tsdr_readasync(&tsdr, read_async, (void *) context, 800, 600);

	(*env)->DeleteGlobalRef(env, context->obj);
	free(context);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_stop (JNIEnv * env, jobject obj) {
	THROW(tsdr_stop(&tsdr));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setGain (JNIEnv * env, jobject obj, jfloat gain) {
	THROW(tsdr_setgain(&tsdr, (float) gain));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_unloadPlugin (JNIEnv * env, jobject obj) {
	THROW(tsdr_unloadplugin(&tsdr));
}
