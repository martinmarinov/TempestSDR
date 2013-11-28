#include <jni.h>
#include <stdio.h>
#include "TSDRLibraryNDK.h"

JNIEXPORT void JNICALL Java_martin_tempest_core_TSDRLibrary_test(JNIEnv *env, jobject thisObj)
{
   printf("Hello World!\n");
   return;
}
