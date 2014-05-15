/*
#-------------------------------------------------------------------------------
# Copyright (c) 2014 Martin Marinov.
# All rights reserved. This program and the accompanying materials
# are made available under the terms of the GNU Public License v3.0
# which accompanies this distribution, and is available at
# http://www.gnu.org/licenses/gpl.html
# 
# Contributors:
#     Martin Marinov - initial API and implementation
#-------------------------------------------------------------------------------
*/

#include <jni.h>
#include <stdio.h>
#include "TSDRLibraryNDK.h"
#include "include/TSDRLibrary.h"
#include "include/TSDRCodes.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

tsdr_lib_t * tsdr_instance = NULL;

#define THROW(x) announce_jni_error(env, x)

struct java_obj_context {
	jobject obj;
} typedef java_obj_context_t;

struct java_context {
		jobject obj;
		jobject obj_pixels;
		int pic_width;
		int pic_height;
		jmethodID fixSize;
		jmethodID notifyCallbacks;
		jint * pixels;
		int pixelsize;
	} typedef java_context_t;

JavaVM *jvm;
int javaversion;
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
		case TSDR_INVALID_PARAMETER:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRInvalidParameterException");
			return;
		case TSDR_INVALID_PARAMETER_VALUE:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRInvalidParameterValueException");
			return;
		case TSDR_NOT_RUNNING:
			strcpy(exceptionclass, "martin/tempest/core/exceptions/TSDRNotRunningException");
			return;
		default:
			strcpy(exceptionclass, "java/lang/Exception");
			return;
		}
}

void announce_jni_error(JNIEnv * env, int exception_code)
{
	if (tsdr_instance == NULL) return;
	if (exception_code == TSDR_OK) return;

	char exceptionclass[256];
	error_translate(exception_code, exceptionclass);

	char * msg = tsdr_getlasterrortext(tsdr_instance);

    jclass cls = (*env)->FindClass(env, exceptionclass);
    /* if cls is NULL, an exception has already been thrown */
    if (cls != NULL) {
        (*env)->ThrowNew(env, cls, msg);
    }
    /* free the local ref */
    (*env)->DeleteLocalRef(env, cls);
}

void on_plot_ready(int plot_id, int offset, double * values, int size, uint32_t samplerate, void * ctx) {
	if (tsdr_instance == NULL) return;

	java_obj_context_t * context = (java_obj_context_t *) ctx;

	JNIEnv *env;

	jint getenv_res = (*jvm)->GetEnv(jvm, (void **)&env, javaversion);
	if (getenv_res == JNI_EDETACHED)
		assert((*jvm)->AttachCurrentThread(jvm, (void **) &env, 0) == JNI_OK);
	else if (getenv_res != JNI_OK) {
		fprintf(stderr, "GetEnv returned error %d\n", (int) getenv_res); fflush(stderr);
	}
	assert(env != NULL);
	assert(getenv_res == JNI_OK || getenv_res == JNI_EDETACHED);

	jclass cls = (*env)->GetObjectClass(env, context->obj);

	(*env)->CallVoidMethod(env, context->obj, (*env)->GetMethodID(env, cls, "onIncomingArray", "(I)V"), (jint) size);

	jobject float_array = (*env)->GetObjectField(env, context->obj, (*env)->GetFieldID(env, (*env)->GetObjectClass(env, context->obj), "double_array", "[D"));

	assert((*env)->GetArrayLength(env, float_array) >= size);

	// set elements
	(*env)->SetDoubleArrayRegion(env, float_array, 0, size, values);

	// notifyCallbacks();
	(*env)->CallVoidMethod(env, context->obj, (*env)->GetMethodID(env, cls, "onIncomingArrayNotify", "(IIIJ)V"), (jint) plot_id, (jint) offset, (jint) size, (jlong) samplerate);


	(*jvm)->DetachCurrentThread(jvm);
}

void on_value_changed(int value_id, double arg0, double arg1, void * ctx) {
	if (tsdr_instance == NULL) return;

	java_obj_context_t * context = (java_obj_context_t *) ctx;

	JNIEnv *env;

	jint getenv_res = (*jvm)->GetEnv(jvm, (void **)&env, javaversion);
	if (getenv_res == JNI_EDETACHED)
		assert((*jvm)->AttachCurrentThread(jvm, (void **) &env, 0) == JNI_OK);
	else if (getenv_res != JNI_OK) {
		fprintf(stderr, "GetEnv returned error %d\n", (int) getenv_res); fflush(stderr);
	}
	assert(env != NULL);
	assert(getenv_res == JNI_OK || getenv_res == JNI_EDETACHED);


	jclass cls = (*env)->GetObjectClass(env, context->obj);
	jmethodID msghandler = (*env)->GetMethodID(env, cls, "onValueChanged", "(IDD)V");

	(*env)->CallVoidMethod(env, context->obj, msghandler, value_id, arg0, arg1);

	(*jvm)->DetachCurrentThread(jvm);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_init (JNIEnv * env, jobject obj) {
	(*env)->GetJavaVM(env, &jvm);
	javaversion = (*env)->GetVersion(env);

	java_obj_context_t * ctx = malloc(sizeof(java_obj_context_t));

	ctx->obj = (*env)->NewGlobalRef(env, obj);

	tsdr_init(&tsdr_instance, on_value_changed, on_plot_ready, ctx);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setBaseFreq (JNIEnv * env, jobject obj, jlong freq) {
	if (tsdr_instance == NULL) return;
	THROW(tsdr_setbasefreq(tsdr_instance, (uint32_t) freq));
}

void read_async(float *buf, int width, int height, void *ctx) {
	java_context_t * context = (java_context_t *) ctx;
	JNIEnv *env;

	jint getenv_res = (*jvm)->GetEnv(jvm, (void **)&env, javaversion);
	if (getenv_res == JNI_EDETACHED)
		assert((*jvm)->AttachCurrentThread(jvm, (void **) &env, 0) == JNI_OK);
	else if (getenv_res != JNI_OK) {
		fprintf(stderr, "GetEnv returned error %d\n", (int) getenv_res); fflush(stderr);
	}
	assert(env != NULL);
	assert(getenv_res == JNI_OK || getenv_res == JNI_EDETACHED);

	if (context->pic_width != width || context->pic_height != height) {

		if (context->obj_pixels != NULL) (*env)->DeleteGlobalRef(env, context->obj_pixels);

		(*env)->CallVoidMethod(env, context->obj, context->fixSize, width, height);

		context->obj_pixels = (*env)->NewGlobalRef(env, (*env)->GetObjectField(env, context->obj, (*env)->GetFieldID(env, (*env)->GetObjectClass(env, context->obj), "pixels", "[I")));

		const int newpixelsize = width * height;
		if (newpixelsize != context->pixelsize) {
			if (context->pixels == NULL)
				context->pixels = (jint *) malloc(sizeof(jint) * newpixelsize);
			else
				context->pixels = (jint *) realloc((void *) context->pixels, sizeof(jint) * newpixelsize);
		}

		assert(context->pixels != NULL);

		context->pixelsize = newpixelsize;
		context->pic_width = width;
		context->pic_height = height;

		assert((*env)->GetArrayLength(env, context->obj_pixels) >= context->pixelsize);
	}

	jint * data = context->pixels;

	int i;
	for (i = 0; i < context->pixelsize; i++) {
		const float val = *(buf++);
		if (val > 0.0f && val <= 1.0f) {
			const int col = (inverted) ? (255 - (int) (val * 255.0f)) : ((int) (val * 255.0f));
			*(data++) = col | (col << 8) | (col << 16);
		} else if (val <= 0.0f) {
			*(data++) = (inverted) ? (255 | (255 << 8) | (255 << 16)) : 0;
		}
#if PIXEL_SPECIAL_COLOURS_ENABLED
		else if (val == PIXEL_SPECIAL_VALUE_R) {
			*(data++) = 255 << 16;
		} else if (val == PIXEL_SPECIAL_VALUE_G) {
			*(data++) = 255 << 8;
		} else if (val == PIXEL_SPECIAL_VALUE_B) {
			*(data++) = 255;
		} else if (val == PIXEL_SPECIAL_VALUE_TRANSPARENT) {
			data++;
		}
#endif
		else {
			*(data++) = (inverted) ? 0 : (255 | (255 << 8) | (255 << 16));
		}

	}

	assert(data == context->pixels + context->pixelsize);

	//assert((*env)->GetArrayLength(env, context->obj_pixels) >= context->pixelsize);

	// release elements#
	(*env)->SetIntArrayRegion(env, context->obj_pixels, 0, context->pixelsize, context->pixels);

	// notifyCallbacks();
	(*env)->CallVoidMethod(env, context->obj, context->notifyCallbacks);

	//assert (!(*env)->ExceptionCheck(env));

	(*jvm)->DetachCurrentThread(jvm);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_loadPlugin (JNIEnv * env, jobject obj, jstring path, jstring params) {
	if (tsdr_instance == NULL) return;

	const char *npath = (*env)->GetStringUTFChars(env, path, 0);
	const char *nparams = (*env)->GetStringUTFChars(env, params, 0);

	int status = tsdr_loadplugin(tsdr_instance, npath, nparams);

	THROW(status);

	(*env)->ReleaseStringUTFChars(env, path, npath);
	(*env)->ReleaseStringUTFChars(env, params, nparams);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_nativeStart (JNIEnv * env, jobject obj) {
	if (tsdr_instance == NULL) return;

	java_context_t * context = (java_context_t *) malloc(sizeof(java_context_t));

	context->obj = (*env)->NewGlobalRef(env, obj);
	(*env)->DeleteLocalRef(env, obj);
	context->fixSize = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context->obj), "fixSize", "(II)V");
	context->notifyCallbacks = (*env)->GetMethodID(env, (*env)->GetObjectClass(env, context->obj), "notifyCallbacks", "()V");
	context->obj_pixels = NULL;

	context->pic_width = 0;
	context->pic_height = 0;
	context->pixelsize = 0;
	context->pixels = NULL;

	int status = tsdr_readasync(tsdr_instance, read_async, (void *) context);

	THROW(status);

	if (context->obj_pixels != NULL) {
		if ((*env)->GetObjectRefType(env, context->obj_pixels) == JNIGlobalRefType)
			(*env)->DeleteGlobalRef(env, context->obj_pixels);
	}

	if ((*env)->GetObjectRefType(env, context->obj) == JNIGlobalRefType)
		(*env)->DeleteGlobalRef(env, context->obj);

	free(context);
	if (context->pixels != NULL) free(context->pixels);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_stop (JNIEnv * env, jobject obj) {
	if (tsdr_instance == NULL) return;
	THROW(tsdr_stop(tsdr_instance));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_unloadPlugin (JNIEnv * env, jobject obj) {
	if (tsdr_instance == NULL) return;
	THROW(tsdr_unloadplugin(tsdr_instance));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_free (JNIEnv * env, jobject obj) {
	if (tsdr_instance == NULL) return;

	java_obj_context_t * ctx = (java_obj_context_t *) tsdr_getctx(tsdr_instance);
	if (ctx != NULL) {
		(*env)->DeleteGlobalRef(env, ctx->obj);
		free(ctx);
	}

	tsdr_free(&tsdr_instance);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setGain (JNIEnv * env, jobject obj, jfloat gain) {
	if (tsdr_instance == NULL) return;
	THROW(tsdr_setgain(tsdr_instance, (float) gain));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setMotionBlur(JNIEnv * env, jobject obj, jfloat coeff) {
	if (tsdr_instance == NULL) return;
	THROW(tsdr_motionblur(tsdr_instance, (float) coeff));
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setResolution (JNIEnv * env, jobject obj, jint height, jdouble refreshrate) {
	if (tsdr_instance == NULL) return;
	THROW(tsdr_setresolution(tsdr_instance, (int) height, (double) refreshrate));
}

JNIEXPORT jboolean JNICALL Java_martin_tempest_core_TSDRLibrary_isRunning  (JNIEnv * env, jobject obj) {
	if (tsdr_instance == NULL) return JNI_FALSE;
	if (tsdr_isrunning(tsdr_instance))
		return JNI_TRUE;
	else
		return JNI_FALSE;
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setInvertedColors  (JNIEnv * env, jobject obj, jboolean invertedEnabled) {
	inverted = invertedEnabled == JNI_TRUE;
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_sync (JNIEnv * env, jobject obj, jint pixels, jobject dir) {
	if (tsdr_instance == NULL) return;

	jclass enumClass = (*env)->FindClass(env, "martin/tempest/core/TSDRLibrary$SYNC_DIRECTION");
	jmethodID getNameMethod = (*env)->GetMethodID(env, enumClass, "name", "()Ljava/lang/String;");
	jstring value = (jstring)(*env)->CallObjectMethod(env, dir, getNameMethod);
	const char* valueNative = (*env)->GetStringUTFChars(env, value, 0);

	if (strcmp(valueNative, "ANY") == 0)
		THROW(tsdr_sync(tsdr_instance, (int) pixels, DIRECTION_CUSTOM));
	else if (strcmp(valueNative, "UP") == 0)
		THROW(tsdr_sync(tsdr_instance, (int) pixels, DIRECTION_UP));
	else if (strcmp(valueNative, "DOWN") == 0)
		THROW(tsdr_sync(tsdr_instance, (int) pixels, DIRECTION_DOWN));
	else if (strcmp(valueNative, "LEFT") == 0)
		THROW(tsdr_sync(tsdr_instance, (int) pixels, DIRECTION_LEFT));
	else if (strcmp(valueNative, "RIGHT") == 0)
		THROW(tsdr_sync(tsdr_instance, (int) pixels, DIRECTION_RIGHT));

	(*env)->ReleaseStringUTFChars(env, value, valueNative);
}

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setParam  (JNIEnv * env, jobject obj, jobject name, jlong value) {
	if (tsdr_instance == NULL) return;

	jclass enumClass = (*env)->FindClass(env, "martin/tempest/core/TSDRLibrary$PARAM");
	jmethodID getOrdinalMethod = (*env)->GetMethodID(env, enumClass, "ordinal", "()I");
	jint id = (int)(*env)->CallIntMethod(env, name, getOrdinalMethod);

	THROW(tsdr_setparameter_int(tsdr_instance, id, value));
}


JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_setParamDouble  (JNIEnv * env, jobject obj, jobject name, jdouble value) {
	if (tsdr_instance == NULL) return;

	jclass enumClass = (*env)->FindClass(env, "martin/tempest/core/TSDRLibrary$PARAM_DOUBLE");
	jmethodID getOrdinalMethod = (*env)->GetMethodID(env, enumClass, "ordinal", "()I");
	jint id = (int)(*env)->CallIntMethod(env, name, getOrdinalMethod);

	THROW(tsdr_setparameter_double(tsdr_instance, id, value));
}
