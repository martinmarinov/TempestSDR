#include <jni.h>
#include <stdio.h>
#include "TSDRLibraryNDK.h"
#include "include/TSDRLibrary.h"
#include "include/TSDRCodes.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

tsdr_lib_t tsdr_instance;

#define THROW(x) announce_jni_error(env, x)

struct java_context {
		jobject obj;
		jobject obj_pixels;
		jclass cls;
		jfieldID fid_pixels;
		int pic_width;
		int pic_height;
		jmethodID fixSize;
		jmethodID notifyCallbacks;
		jint * pixels;
		int pixelsize;
	} typedef java_context_t;

static JavaVM *jvm;
static int javaversion;
int inverted = 0;

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
		case TSDR_WRONG_VIDEOPARAMS:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRVideoParamsException");
			return;
		case TSDR_ALREADY_RUNNING:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRAlreadyRunningException");
			return;
		case TSDR_PLUGIN_PARAMETERS_WRONG:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRPluginParametersException");
			return;
		case TSDR_SAMPLE_RATE_WRONG:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRSampleRateWrongException");
			return;
		case TSDR_CANNOT_OPEN_DEVICE:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRCannotOpenDeviceException");
			return;
		case TSDR_INCOMPATIBLE_PLUGIN:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRIncompatiblePluginException");
			return;
		default:
			strcpy(exceptionclass, "java/lang/Exception");
			return;
		}
}

void announce_jni_error(JNIEnv * env, int exception_code)
{
	if (exception_code == TSDR_OK) return;

	char exceptionclass[256];
	error_translate(exception_code, exceptionclass);

	char * msg = tsdr_getlasterrortext(&tsdr_instance);

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
	tsdr_init(&tsdr_instance);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setSampleRate (JNIEnv * env, jobject obj, jlong rate) {
	THROW(tsdr_setsamplerate(&tsdr_instance, (uint32_t) rate));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setBaseFreq (JNIEnv * env, jobject obj, jlong freq) {
	THROW(tsdr_setbasefreq(&tsdr_instance, (uint32_t) freq));
}

void read_async(float *buf, int width, int height, void *ctx) {
	java_context_t * context = (java_context_t *) ctx;
	JNIEnv *env;

	if ((*jvm)->GetEnv(jvm, (void **)&env, javaversion) == JNI_EDETACHED)
		(*jvm)->AttachCurrentThread(jvm, (void **) &env, 0);

	if (context->pic_width != width || context->pic_height != height) {

		if (context->obj_pixels != NULL) (*env)->DeleteLocalRef(env, context->obj_pixels);

		(*env)->CallVoidMethod(env, context->obj, context->fixSize, width, height);
		context->obj_pixels = (*env)->GetObjectField(env, context->obj, context->fid_pixels);

		context->pixelsize = width * height;
		context->pixels = (jint *) realloc((void *) context->pixels, sizeof(jint) * context->pixelsize);
		context->pic_width = width;
		context->pic_height = height;
	}

	jint * data = context->pixels;

	int i;
	for (i = 0; i < context->pixelsize; i++) {
		int col = (inverted) ? (255 - (int) (*(buf++) * 255.0f)) : ((int) (*(buf++) * 255.0f));
		if (col > 255) col = 255; else if (col < 0) col = 0;
		*(data++) = col | (col << 8) | (col << 16);
	}

	// release elements
	(*env)->SetIntArrayRegion(env, context->obj_pixels, 0, context->pixelsize, context->pixels);

	// notifyCallbacks();
	(*env)->CallVoidMethod(env, context->obj, context->notifyCallbacks);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_loadPlugin (JNIEnv * env, jobject obj, jstring path, jstring params) {
	const char *npath = (*env)->GetStringUTFChars(env, path, 0);
	const char *nparams = (*env)->GetStringUTFChars(env, params, 0);

	THROW(tsdr_loadplugin(&tsdr_instance, npath, nparams));

	(*env)->ReleaseStringUTFChars(env, path, npath);
	(*env)->ReleaseStringUTFChars(env, params, nparams);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_nativeStart (JNIEnv * env, jobject obj) {

	java_context_t * context = (java_context_t *) malloc(sizeof(java_context_t));

	context->obj = (*env)->NewGlobalRef(env, obj);
	(*env)->DeleteLocalRef(env, obj);
	context->cls = (jclass) (*env)->NewGlobalRef(env, (*env)->GetObjectClass(env, context->obj));
	context->fid_pixels = (*env)->GetFieldID(env, context->cls, "pixels", "[I");
	context->fixSize = (*env)->GetMethodID(env, context->cls, "fixSize", "(II)V");
	context->notifyCallbacks = (*env)->GetMethodID(env, context->cls, "notifyCallbacks", "()V");
	context->obj_pixels = NULL;

	context->pic_width = 0;
	context->pic_height = 0;
	context->pixelsize = 1;
	context->pixels = (jint *) malloc(sizeof(jint) * context->pixelsize);

	(*jvm)->DetachCurrentThread(jvm);

	int status = tsdr_readasync(&tsdr_instance, read_async, (void *) context);

	if ((*jvm)->GetEnv(jvm, (void **)&env, javaversion) == JNI_EDETACHED)
		(*jvm)->AttachCurrentThread(jvm, (void **) &env, 0);

	THROW(status);

	(*env)->DeleteGlobalRef(env, context->obj);
	(*env)->DeleteGlobalRef(env, context->cls);

	free(context);
	free(context->pixels);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_stop (JNIEnv * env, jobject obj) {
	THROW(tsdr_stop(&tsdr_instance));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_unloadPlugin (JNIEnv * env, jobject obj) {
	THROW(tsdr_unloadplugin(&tsdr_instance));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_free (JNIEnv * env, jobject obj) {
	tsdr_free(&tsdr_instance);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setGain (JNIEnv * env, jobject obj, jfloat gain) {
	THROW(tsdr_setgain(&tsdr_instance, (float) gain));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setMotionBlur(JNIEnv * env, jobject obj, jfloat coeff) {
	THROW(tsdr_motionblur(&tsdr_instance, (float) coeff));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setResolution (JNIEnv * env, jobject obj, jint width, jint height, jdouble refreshrate) {
	THROW(tsdr_setresolution(&tsdr_instance, (int) width, (int) height, (double) refreshrate));
}

JNIEXPORT jboolean JNICALL Java_martin_tempest_core_TSDRLibrary_isRunning  (JNIEnv * env, jobject obj) {
	if (tsdr_isrunning(&tsdr_instance))
		return JNI_TRUE;
	else
		return JNI_FALSE;
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setInvertedColors  (JNIEnv * env, jobject obj, jboolean invertedEnabled) {
	inverted = invertedEnabled == JNI_TRUE;
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_sync (JNIEnv * env, jobject obj, jint pixels, jobject dir) {
	jclass enumClass = (*env)->FindClass(env, "martin/tempest/core/TSDRLibrary$SYNC_DIRECTION");
	jmethodID getNameMethod = (*env)->GetMethodID(env, enumClass, "name", "()Ljava/lang/String;");
	jstring value = (jstring)(*env)->CallObjectMethod(env, dir, getNameMethod);
	const char* valueNative = (*env)->GetStringUTFChars(env, value, 0);

	if (strcmp(valueNative, "ANY") == 0)
		THROW(tsdr_sync(&tsdr_instance, (int) pixels, DIRECTION_CUSTOM));
	else if (strcmp(valueNative, "UP") == 0)
		THROW(tsdr_sync(&tsdr_instance, (int) pixels, DIRECTION_UP));
	else if (strcmp(valueNative, "DOWN") == 0)
		THROW(tsdr_sync(&tsdr_instance, (int) pixels, DIRECTION_DOWN));
	else if (strcmp(valueNative, "LEFT") == 0)
		THROW(tsdr_sync(&tsdr_instance, (int) pixels, DIRECTION_LEFT));
	else if (strcmp(valueNative, "RIGHT") == 0)
		THROW(tsdr_sync(&tsdr_instance, (int) pixels, DIRECTION_RIGHT));

	(*env)->ReleaseStringUTFChars(env, value, valueNative);
}
