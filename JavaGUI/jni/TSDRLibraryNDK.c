#include <jni.h>
#include <stdio.h>
#include "TSDRLibraryNDK.h"
#include "include\TSDRLibrary.h"

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_test(JNIEnv *env, jobject thisObj)
{
   test();
}
