/* DO NOT EDIT THIS FILE - it is machine generated */

/* Header for class mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper */

#ifndef _Included_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
#define _Included_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
#ifdef __cplusplus
extern "C" {
#endif
#undef mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_minimumVersion
#define mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_minimumVersion 14.1
/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    rtsInit
 * Signature: ([Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_rtsInit
  (JNIEnv *, jobject, jobjectArray);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    getDbTitle
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_getDbTitle
  (JNIEnv *, jobject);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    getLibraryVersion
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_getLibraryVersion
  (JNIEnv *, jclass);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    openSession
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_openSession
  (JNIEnv *, jobject);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    closeSession
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_closeSession
  (JNIEnv *, jobject, jint);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    shootRay
 * Signature: (Lmil/army/muves/math/Point;Lmil/army/muves/math/Vector3;I)[B
 */
JNIEXPORT jbyteArray JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_shootRay
  (JNIEnv *, jobject, jobject, jobject, jint);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    shootArray
 * Signature: (Lmil/army/muves/math/Point;Lmil/army/muves/math/Vector3;Lmil/army/muves/math/Vector3;Lmil/army/muves/math/Vector3;IIII)[B
 */
JNIEXPORT jbyteArray JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_shootArray
  (JNIEnv *, jobject, jobject, jobject, jobject, jobject, jint, jint, jint, jint);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    getItemTree
 * Signature: (I)Lmil/army/muves/geometryservice/datatypes/ItemTree;
 */
JNIEXPORT jobject JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_getItemTree
  (JNIEnv *, jobject, jint);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    getRegionNames
 * Signature: (I)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_getRegionNames
  (JNIEnv *, jobject, jint);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    getBoundingBox
 * Signature: (I)Lmil/army/muves/math/BoundingBox;
 */
JNIEXPORT jobject JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_getBoundingBox
  (JNIEnv *, jobject, jint);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    shootList
 * Signature: ([Lmil/army/muves/math/Ray;II)[B
 */
JNIEXPORT jbyteArray JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_shootList
  (JNIEnv *, jobject, jobjectArray, jint, jint);

/*
 * Class:     mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper
 * Method:    shutdownNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_mil_army_muves_brlcadservice_impl_BrlcadJNIWrapper_shutdownNative
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
