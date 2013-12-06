#include <jni.h>
#include <stdio.h>
#include "TSDRLibraryNDK.h"
#include "include\TSDRLibrary.h"
#include "include\TSDRCodes.h"
#include <string.h>

tsdr_lib_t tsdr;

#define THROW(x) error(env, x, "")

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

	JavaVM* jvm;
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

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_nativeLoadPlugin (JNIEnv * env, jobject obj, jstring  path) {
	const char *npath = (*env)->GetStringUTFChars(env, path, 0);
	THROW(tsdr_loadplugin(&tsdr, npath));
	(*env)->ReleaseStringUTFChars(env, path, npath);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_pluginParams (JNIEnv * env, jobject obj, jstring params) {
	error(env, TSDR_NOT_IMPLEMENTED, "%s not implemented", "pluginParams");
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setSampleRate (JNIEnv * env, jobject obj, jlong rate) {
	error(env, TSDR_NOT_IMPLEMENTED, "%s not implemented", "setSampleRate");
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setBaseFreq (JNIEnv * env, jobject obj, jlong freq) {
	error(env, TSDR_NOT_IMPLEMENTED, "%s not implemented", "setBaseFreq");
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_start (JNIEnv * env, jobject obj) {
	error(env, TSDR_NOT_IMPLEMENTED, "%s not implemented", "start");
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_stop (JNIEnv * env, jobject obj) {
	error(env, TSDR_NOT_IMPLEMENTED, "%s not implemented", "stop");
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setGain (JNIEnv * env, jobject obj, jfloat gain) {
	error(env, TSDR_NOT_IMPLEMENTED, "%s not implemented", "setGain");
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_readAsync (JNIEnv * env, jobject obj) {
	error(env, TSDR_NOT_IMPLEMENTED, "%s not implemented", "readAsync");
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_unloadPlugin (JNIEnv * env, jobject obj) {
	error(env, TSDR_NOT_IMPLEMENTED, "%s not implemented", "unloadPlugin");
}
