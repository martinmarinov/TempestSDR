#include <jni.h>
#include <stdio.h>
#include "TSDRLibraryNDK.h"
#include "include\TSDRLibrary.h"
#include "include\TSDRCodes.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

tsdr_lib_t tsdr_instance;

#define THROW(x) error(env, x, "")

struct java_context {
		jobject obj;
		jclass cls;
		jfieldID fid_pixels;
		jfieldID fid_width;
		jfieldID fid_height;
		jmethodID fixSize;
		jmethodID notifyCallbacks;
		jint * pixels;
		int pixelsize;
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

	jint i_width = (*env)->GetIntField(env, context->obj, context->fid_width);
	jint i_height = (*env)->GetIntField(env, context->obj, context->fid_height);

	if (i_width != width || i_height != height) {
		// fixSize(200, 200);
		(*env)->CallVoidMethod(env, context->obj, context->fixSize, width, height);

		context->pixelsize = width * height;
		context->pixels = (jint *) realloc((void *) context->pixels, sizeof(jint) * context->pixelsize);
	}

	jint * data = context->pixels;

	int i;
	for (i = 0; i < context->pixelsize; i++) {
		const int col = (int) (*(buf++) * 255.0f);
		*(data++) = col | (col << 8) | (col << 16);
	}

	// release elements
	(*env)->SetIntArrayRegion(env, (*env)->GetObjectField(env, context->obj, context->fid_pixels), 0, context->pixelsize, context->pixels);

	// notifyCallbacks();
	(*env)->CallVoidMethod(env, context->obj, context->notifyCallbacks);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_nativeStart (JNIEnv * env, jobject obj, jstring path, jstring params) {

	java_context_t * context = (java_context_t *) malloc(sizeof(java_context_t));

	context->obj = (*env)->NewGlobalRef(env, obj);
	(*env)->DeleteLocalRef(env, obj);
	context->cls = (jclass) (*env)->NewGlobalRef(env, (*env)->GetObjectClass(env, context->obj));
	context->fid_pixels = (*env)->GetFieldID(env, context->cls, "pixels", "[I");
	context->fid_width = (*env)->GetFieldID(env, context->cls, "width", "I");
	context->fid_height = (*env)->GetFieldID(env, context->cls, "height", "I");
	context->fixSize = (*env)->GetMethodID(env, context->cls, "fixSize", "(II)V");
	context->notifyCallbacks = (*env)->GetMethodID(env, context->cls, "notifyCallbacks", "()V");

	jint i_width = (*env)->GetIntField(env, context->obj, context->fid_width);
	jint i_height = (*env)->GetIntField(env, context->obj, context->fid_height);

	context->pixelsize = i_width * i_height;
	context->pixels = (jint *) malloc(sizeof(jint) * context->pixelsize);

	const char *npath = (*env)->GetStringUTFChars(env, path, 0);
	const char *nparams = (*env)->GetStringUTFChars(env, params, 0);

	THROW(tsdr_readasync(&tsdr_instance, npath, read_async, (void *) context, nparams));

	(*env)->ReleaseStringUTFChars(env, path, npath);
	(*env)->ReleaseStringUTFChars(env, params, nparams);
	(*env)->DeleteGlobalRef(env, context->obj);
	(*env)->DeleteGlobalRef(env, context->cls);
	free(context);
	free(context->pixels);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_stop (JNIEnv * env, jobject obj) {
	THROW(tsdr_stop(&tsdr_instance));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setGain (JNIEnv * env, jobject obj, jfloat gain) {
	THROW(tsdr_setgain(&tsdr_instance, (float) gain));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setResolution (JNIEnv * env, jobject obj, jint width, jint height, jdouble refreshrate) {
	THROW(tsdr_setresolution(&tsdr_instance, (int) width, (int) height, (double) refreshrate));
}
