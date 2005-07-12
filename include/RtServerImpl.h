/* DO NOT EDIT THIS FILE - it is machine generated */

#include "common.h"
#ifdef HAVE_JAVAVM_JNI_H
#  include <JavaVM/jni.h>
#elif defined(HAVE_JNI_H)
#  include <jni.h>
#else
#  error ERROR: jni.h could not be found
#endif

/* Header for class mil_army_arl_services_RtService */

#ifndef _Included_mil_army_arl_services_RtService
#define _Included_mil_army_arl_services_RtService

__BEGIN_DECLS

#undef mil_army_arl_services_RtService_minimumVersion
#define mil_army_arl_services_RtService_minimumVersion 14.1
/*
 * Class:     mil_army_arl_services_RtService
 * Method:    rtsInit
 * Signature: ([Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_mil_army_arl_services_RtService_rtsInit
  (JNIEnv *, jobject, jobjectArray);

/*
 * Class:     mil_army_arl_services_RtService
 * Method:    getDbTitle
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_mil_army_arl_services_RtService_getDbTitle
  (JNIEnv *, jobject);

/*
 * Class:     mil_army_arl_services_RtService
 * Method:    getLibraryVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_mil_army_arl_services_RtService_getLibraryVersion
  (JNIEnv *, jclass);

/*
 * Class:     mil_army_arl_services_RtService
 * Method:    openSession
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_mil_army_arl_services_RtService_openSession
  (JNIEnv *, jobject);

/*
 * Class:     mil_army_arl_services_RtService
 * Method:    closeSession
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_mil_army_arl_services_RtService_closeSession
  (JNIEnv *, jobject, jint);

/*
 * Class:     mil_army_arl_services_RtService
 * Method:    shutdownNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_mil_army_arl_services_RtService_shutdownNative
  (JNIEnv *, jobject);

/*
 * Class:     mil_army_arl_services_RtService
 * Method:    shootRay
 * Signature: (Lmil/army/arl/muves/math/Point;Lmil/army/arl/muves/math/Vector3;I)Lmil/army/arl/muves/rtserver/RayResult;
 */
JNIEXPORT jobject JNICALL Java_mil_army_arl_services_RtService_shootRay
  (JNIEnv *, jobject, jobject, jobject, jint);

/*
 * Class:     mil_army_arl_services_RtService
 * Method:    shootArray
 * Signature: (Lmil/army/arl/muves/math/Point;Lmil/army/arl/muves/math/Vector3;Lmil/army/arl/muves/math/Vector3;Lmil/army/arl/muves/math/Vector3;IIII)[Lmil/army/arl/muves/rtserver/RayResult;
 */
JNIEXPORT jobjectArray JNICALL Java_mil_army_arl_services_RtService_shootArray
  (JNIEnv *, jobject, jobject, jobject, jobject, jobject, jint, jint, jint, jint);

/*
 * Class:     mil_army_arl_services_RtService
 * Method:    getItemTree
 * Signature: (I)Lmil/army/arl/muves/rtserver/ItemTree;
 */
JNIEXPORT jobject JNICALL Java_mil_army_arl_services_RtService_getItemTree
  (JNIEnv *, jobject, jint);

/*
 * Class:     mil_army_arl_services_RtService
 * Method:    getBoundingBox
 * Signature: (I)Lmil/army/arl/muves/math/BoundingBox;
 */
JNIEXPORT jobject JNICALL Java_mil_army_arl_services_RtService_getBoundingBox
  (JNIEnv *, jobject, jint);

__END_DECLS

#endif

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
