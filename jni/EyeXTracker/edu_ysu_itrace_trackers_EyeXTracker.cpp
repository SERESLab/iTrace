#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "TobiiGazeSDK/tobiigaze_error_codes.h"
#include "TobiiGazeSDK/tobiigaze.h"
#include "TobiiGazeSDK/tobiigaze_discovery.h"
#include "TobiiGazeSDK/tobiigaze_calibration.h"
#include "edu_ysu_itrace_trackers_EyeXTracker.h"
#include "edu_ysu_itrace_trackers_EyeXTracker_BackgroundThread.h"
#include "edu_ysu_itrace_trackers_EyeXTracker_Calibrator.h"
#include <iostream>

struct EyeXNativeData
{
	JavaVM* jvm;
	jobject j_eye_tracker;
	jobject j_background_thread;
	tobiigaze_eye_tracker* eye_tracker;
	tobiigaze_error_code create_error_code;
};

EyeXNativeData* g_native_data_current = NULL;

void throwJException(JNIEnv* env, const char* jclass_name, const char* msg)
{
	jclass jclass = env->FindClass(jclass_name);
	env->ThrowNew(jclass, msg);
	env->DeleteLocalRef(jclass);
}

jfieldID getFieldID(JNIEnv* env, jobject obj, const char* name, const char* sig)
{
	jclass jclass = env->GetObjectClass(obj);
	if (jclass == NULL)
		return NULL;
	jfieldID jfid = env->GetFieldID(jclass, name, sig);
	if (jfid == NULL)
		return NULL;
	return jfid;
}

EyeXNativeData* getEyeXNativeData(JNIEnv* env, jobject obj)
{
	jfieldID jfid_native_data = getFieldID(env, obj, "native_data",
		"Ljava/nio/ByteBuffer;");
	if (jfid_native_data == NULL)
		return NULL;
	jobject native_data_bb = env->GetObjectField(obj, jfid_native_data);
	return (EyeXNativeData*) env->GetDirectBufferAddress(native_data_bb);
}

void on_gaze_data(const tobiigaze_gaze_data* gazedata, const tobiigaze_gaze_data_extensions* extensions, void *user_data) {

	JNIEnv* env = NULL;
	jint rs = g_native_data_current->jvm->GetEnv((void**) &env, JNI_VERSION_1_6);
	if (rs != JNI_OK || env == NULL)
		return;
	jobject obj = g_native_data_current->j_eye_tracker;

	int leftValidity;
	int rightValidity;

	if (gazedata->tracking_status == TOBIIGAZE_TRACKING_STATUS_BOTH_EYES_TRACKED ||
	        gazedata->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONLY_LEFT_EYE_TRACKED ||
	        gazedata->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONE_EYE_TRACKED_PROBABLY_LEFT) {
		leftValidity = 1;
	} else {
		leftValidity = 0;
	}

	if (gazedata->tracking_status == TOBIIGAZE_TRACKING_STATUS_BOTH_EYES_TRACKED ||
	         gazedata->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONLY_RIGHT_EYE_TRACKED ||
	         gazedata->tracking_status == TOBIIGAZE_TRACKING_STATUS_ONE_EYE_TRACKED_PROBABLY_RIGHT) {
		rightValidity = 1;
	} else {
	    rightValidity = 0;
	}

	jclass eyex_tracker_class = env->GetObjectClass(obj);
	if (eyex_tracker_class == NULL)
	    return;
	jmethodID jmid_new_gaze_point = env->GetMethodID(eyex_tracker_class,
	    "newGazePoint", "(JDDDDIIDD)V");
	//Just pretend nothing happened.
	if (jmid_new_gaze_point == NULL)
	    return;

	int pupilDiameter = 0; //EyeX does not record pupil diameter
							//jni doesn't like sending straight numbers not stored in a variable.
	//Call newGazePoint.
	env->CallVoidMethod(obj, jmid_new_gaze_point, (jlong) gazedata->timestamp,
	    gazedata->left.gaze_point_on_display_normalized.x, gazedata->left.gaze_point_on_display_normalized.y,
		gazedata->right.gaze_point_on_display_normalized.x, gazedata->right.gaze_point_on_display_normalized.y,
	    leftValidity, rightValidity,
	    pupilDiameter, pupilDiameter); //no pupil diameters for recording
}

//JNI FUNCTIONS

//THREADING
JNIEXPORT jboolean JNICALL Java_edu_ysu_itrace_trackers_EyeXTracker_00024BackgroundThread_jniBeginMainloop
  (JNIEnv *env, jobject obj) {

		const int urlSize = 256;
		char url[urlSize];

		tobiigaze_eye_tracker* eye_tracker;
		tobiigaze_error_code error_code;
		
	    tobiigaze_get_connected_eye_tracker(url, urlSize, &error_code);
	    if (error_code) {
	    	std::cout << "No eye tracker found." << std::endl;
	    } else {
	    	// Create an eye tracker instance.
	    	eye_tracker = tobiigaze_create(url, &error_code);
	    	if (error_code) {
	    		std::cout << tobiigaze_get_error_message(error_code) << std::endl;
	    	} else {
	    		//Get native data ByteBuffer field in EyeXTracker object.
	    		jfieldID jfid_parent = getFieldID(env, obj, "parent",
	    			"Ledu/ysu/itrace/trackers/EyeXTracker;");
	    		if (jfid_parent == NULL)
	    			return JNI_FALSE;
	   			jobject parent_eyex_tracker = env->GetObjectField(obj, jfid_parent);
	    		jfieldID jfid_native_data = getFieldID(env, parent_eyex_tracker,
	    			"native_data", "Ljava/nio/ByteBuffer;");
	    		if (jfid_native_data == NULL)
	    			return JNI_FALSE;
	    		//Create structure to hold instance-specific data.
	    		EyeXNativeData* native_data = new EyeXNativeData();
	    		jobject native_data_bb = env->NewDirectByteBuffer((void*) native_data,
	    			sizeof(EyeXNativeData));
	    		//Set java virtual machine and BackgroundThread reference.
	    		env->GetJavaVM(&native_data->jvm);
	   			native_data->j_background_thread = env->NewGlobalRef(obj);
	    		native_data->create_error_code = error_code;
	    		native_data->eye_tracker = eye_tracker;
	    		//Store structure reference in Java object.
	    		env->SetObjectField(parent_eyex_tracker, jfid_native_data, native_data_bb); 
	    		
	    		std::cout << "Starting event loop." << std::endl;
	    		// Start the event loop. This must be done before connecting.
	    		tobiigaze_run_event_loop(eye_tracker, &error_code);
	    		std::cout << "Event loop over." << std::endl;
	    		if (error_code) return JNI_FALSE;
	    		
	    		return JNI_TRUE;
	    	}
	    }

		//Get native data ByteBuffer field in EyeXTracker object.
	    jfieldID jfid_parent = getFieldID(env, obj, "parent",
	    	"Ledu/ysu/itrace/trackers/EyeXTracker;");
	    if (jfid_parent == NULL)
	    	return JNI_FALSE;
	    jobject parent_eyex_tracker = env->GetObjectField(obj, jfid_parent);
	    jfieldID jfid_native_data = getFieldID(env, parent_eyex_tracker,
	    	"native_data", "Ljava/nio/ByteBuffer;");
	    if (jfid_native_data == NULL)
	    	return JNI_FALSE;
	    //Create structure to hold instance-specific data.
	    EyeXNativeData* native_data = new EyeXNativeData();
	    jobject native_data_bb = env->NewDirectByteBuffer((void*) native_data,
	    	sizeof(EyeXNativeData));
	    //Set java virtual machine and BackgroundThread reference.
	    env->GetJavaVM(&native_data->jvm);
	    native_data->j_background_thread = env->NewGlobalRef(obj);
	    native_data->create_error_code = error_code;
	    native_data->eye_tracker = eye_tracker;
	    //Store structure reference in Java object.
	    env->SetObjectField(parent_eyex_tracker, jfid_native_data, native_data_bb);
	    
	    return JNI_TRUE;
}

//TRACKER FUNCTIONS
JNIEXPORT jboolean JNICALL Java_edu_ysu_itrace_trackers_EyeXTracker_jniConnectEyeXTracker
  (JNIEnv *env, jobject obj) {

	tobiigaze_error_code error_code;
	
	//Get native data from object.
	EyeXNativeData* native_data = getEyeXNativeData(env, obj);
	if (native_data == NULL) {
		return JNI_FALSE;
	}
	//Set EyeXTracker reference.
	native_data->j_eye_tracker = env->NewGlobalRef(obj);

	//If error occured when creating the main loop
	//do no continue
	if (native_data->create_error_code) {
		return JNI_FALSE;
	}

	// Connect to the tracker.
	tobiigaze_connect(native_data->eye_tracker, &error_code);
	if (error_code) {
		return JNI_FALSE;
	}
    printf("Connected.\n");
    return JNI_TRUE;
}

JNIEXPORT void JNICALL Java_edu_ysu_itrace_trackers_EyeXTracker_close
  (JNIEnv *env, jobject obj) {

	//Get native data from object.
	EyeXNativeData* native_data = getEyeXNativeData(env, obj);
	if (native_data == NULL)
	{
		throwJException(env, "java/lang/RuntimeException",
			"Cannot find native data.");
		return;
	}
	
	if (native_data->create_error_code) { //eye_tracker is not set so get out of here
		return;
	}
	
	// Disconnect.
	tobiigaze_disconnect(native_data->eye_tracker);

	// Break the event loop
	tobiigaze_break_event_loop(native_data->eye_tracker);

	// Clean up.
	tobiigaze_destroy(native_data->eye_tracker);

	delete native_data;
}

JNIEXPORT void JNICALL Java_edu_ysu_itrace_trackers_EyeXTracker_startTracking
  (JNIEnv *env, jobject obj) {

	//Do not continue if already tracking
	if (g_native_data_current != NULL)
	{
		throwJException(env, "java/io/IOException", "Already tracking.");
		return;
	}

	//Get native data from object.
	EyeXNativeData* native_data = getEyeXNativeData(env, obj);
	if (native_data == NULL)
	{
		throwJException(env, "java/lang/RuntimeException",
			"Cannot find native data.");
		return;
	}
	//Set native data for current tracking TobiiTracker.
	g_native_data_current = native_data;

	tobiigaze_error_code error_code;

	// Now that a connection is established,
	// start tracking.
	tobiigaze_start_tracking(native_data->eye_tracker, &on_gaze_data, &error_code, 0);
	if (error_code) {
		throwJException(env, "java/lang/IOException",
				tobiigaze_get_error_message(error_code));
		return;
	}
	printf("Tracking started.\n");
}

JNIEXPORT void JNICALL Java_edu_ysu_itrace_trackers_EyeXTracker_stopTracking
  (JNIEnv *env, jobject obj) {
	
	tobiigaze_error_code error_code;

	tobiigaze_stop_tracking(g_native_data_current->eye_tracker, &error_code);
	if (error_code) {
		throwJException(env, "java/lang/IOException",
				tobiigaze_get_error_message(error_code));
		return;
	}
	printf("Tracking stopped.\n");
	g_native_data_current = NULL;
}

//CALIBRATION

//forward declaration
bool add_point_callback = false;
bool compute_and_set = false;
bool stop_calibration = false;
tobiigaze_error_code calibration_error_code;
bool error_set = false;
void stop_calibration_handler(tobiigaze_error_code error_code, void *user_data);

void handle_calibration_error(tobiigaze_error_code error_code, void *user_data, const char *error_message) {
    if (error_code) {
        if (!error_set) {
        	calibration_error_code = error_code;
        	error_set = true;
        }
        std::cout << tobiigaze_get_error_message(error_code) << std::endl;
        //this is assuming that the tracker will eventually stop calibration without error
        //the sample code also assume the same thing
        //fixed to not get state error
        tobiigaze_calibration_stop_async((tobiigaze_eye_tracker*) user_data, stop_calibration_handler, user_data);
    }
}

void compute_calibration_handler(tobiigaze_error_code error_code, void *user_data) {
	
    if (error_code) {
        if (error_code == TOBIIGAZE_FW_ERROR_OPERATION_FAILED) {
            std::cout << "Compute calibration FAILED due to insufficient gaze data.\n" << std::endl;
        }
		
        handle_calibration_error(error_code, user_data, "compute_calibration_handler");
        compute_and_set = true;
        return;
    }

    std::cout << "compute_calibration_handler: OK\n" << std::endl;
	compute_and_set = true;
}

void add_calibration_point_handler(tobiigaze_error_code error_code, void *user_data) {
	
	if (error_code) {
		handle_calibration_error(error_code, user_data, "add_calibration_point_handler");
	}
	
	add_point_callback = true;
}

void stop_calibration_handler(tobiigaze_error_code error_code, void *user_data) {
	stop_calibration = true;
	
	if (error_code) {
        handle_calibration_error(error_code, user_data, "stop_calibration_handler");
        return;
    }

    std::cout << "stop_calibration_handler: OK\n" << std::endl;
}

JNIEXPORT void JNICALL Java_edu_ysu_itrace_trackers_EyeXTracker_00024Calibrator_jniAddPoint
  (JNIEnv *env, jobject obj, jdouble x, jdouble y) {
	add_point_callback = false;

	//Get native data from parent EyeXTracker
	jfieldID jfid_parent = getFieldID(env, obj, "parent",
		"Ledu/ysu/itrace/trackers/EyeXTracker;");
	if (jfid_parent == NULL)
	{
		throwJException(env, "java/lang/RuntimeException",
			"Parent EyeXTracker not found.");
		return;
	}
	jobject parent_eyex_tracker = env->GetObjectField(obj, jfid_parent);
	EyeXNativeData* native_data = getEyeXNativeData(env, parent_eyex_tracker);
	
	tobiigaze_point_2d point;
	point.x = x;
	point.y = y;
	
	// The call to tobiigaze_calibration_add_point_async starts collecting calibration data at the specified point.
	// Make sure to keep the stimulus (i.e., the calibration dot) on the screen until the tracker is finished, that
	// is, until the callback function is invoked.
	tobiigaze_calibration_add_point_async(native_data->eye_tracker, &point, add_calibration_point_handler, native_data->eye_tracker);
	
	while(add_point_callback != true);
	if (error_set) {
		error_set = false;
		throwJException(env, "java/lang/RuntimeException",
			tobiigaze_get_error_message(calibration_error_code));
		return;
	}
}

JNIEXPORT void JNICALL Java_edu_ysu_itrace_trackers_EyeXTracker_00024Calibrator_jniStartCalibration
  (JNIEnv *env, jobject obj) {
	add_point_callback = false;
	
	//Get native data from parent EyeXTracker
	jfieldID jfid_parent = getFieldID(env, obj, "parent",
		"Ledu/ysu/itrace/trackers/EyeXTracker;");
	if (jfid_parent == NULL)
	{
		throwJException(env, "java/lang/RuntimeException",
			"Parent EyeXTracker not found.");
		return;
	}
	jobject parent_eyex_tracker = env->GetObjectField(obj, jfid_parent);
	EyeXNativeData* native_data = getEyeXNativeData(env, parent_eyex_tracker);
	
	// calibration
	tobiigaze_calibration_start_async(native_data->eye_tracker, add_calibration_point_handler, native_data->eye_tracker);
	
	while(add_point_callback != true);
	if (error_set) {
		error_set = false;
		throwJException(env, "java/lang/RuntimeException",
			tobiigaze_get_error_message(calibration_error_code));
		return;
	}
}

JNIEXPORT void JNICALL Java_edu_ysu_itrace_trackers_EyeXTracker_00024Calibrator_jniStopCalibration
  (JNIEnv *env, jobject obj) {
	compute_and_set = false;
	stop_calibration = false;
	
	//Get native data from parent EyeXTracker
	jfieldID jfid_parent = getFieldID(env, obj, "parent",
		"Ledu/ysu/itrace/trackers/EyeXTracker;");
	if (jfid_parent == NULL)
	{
		throwJException(env, "java/lang/RuntimeException",
			"Parent EyeXTracker not found.");
		return;
	}
	jobject parent_eyex_tracker = env->GetObjectField(obj, jfid_parent);
	EyeXNativeData* native_data = getEyeXNativeData(env, parent_eyex_tracker);

	std::cout << "Computing calibration...\n" << std::endl;
	tobiigaze_calibration_compute_and_set_async(native_data->eye_tracker, compute_calibration_handler, native_data->eye_tracker);
	
	while(compute_and_set != true);
	if (error_set) {
		error_set = false;
		if (calibration_error_code == TOBIIGAZE_FW_ERROR_OPERATION_FAILED) {
            throwJException(env, "java/lang/RuntimeException",
				"Compute calibration FAILED due to insufficient gaze data.");
			return;
        }
		throwJException(env, "java/lang/RuntimeException",
			tobiigaze_get_error_message(calibration_error_code));
		return;
	}
	
	tobiigaze_calibration_stop_async(native_data->eye_tracker, stop_calibration_handler, native_data->eye_tracker);
	while(stop_calibration != true);
}

JNIEXPORT jdoubleArray JNICALL Java_edu_ysu_itrace_trackers_EyeXTracker_00024Calibrator_jniGetCalibration
  (JNIEnv *env, jobject obj) {
  
	//Get native data from parent EyeXTracker
	jfieldID jfid_parent = getFieldID(env, obj, "parent",
		"Ledu/ysu/itrace/trackers/EyeXTracker;");
	if (jfid_parent == NULL)
	{
		throwJException(env, "java/lang/RuntimeException",
			"Parent EyeXTracker not found.");
		return NULL;
	}
	jobject parent_eyex_tracker = env->GetObjectField(obj, jfid_parent);
	EyeXNativeData* native_data = getEyeXNativeData(env, parent_eyex_tracker);
	
    tobiigaze_calibration *calibration = new tobiigaze_calibration;
	tobiigaze_calibration_point_data point_data_items[TOBIIGAZE_MAX_CALIBRATION_POINT_DATA_ITEMS];
	uint32_t point_data_items_size;
	tobiigaze_error_code error_code;
	
	//Get calibration
	tobiigaze_get_calibration(native_data->eye_tracker, calibration, &error_code);
	if (error_code) {
		std::cout << "Cannot retrieve calibration data." << std::endl;
		delete calibration;
		return NULL;
	}
	
	tobiigaze_get_calibration_point_data_items(calibration, point_data_items,
			TOBIIGAZE_MAX_CALIBRATION_POINT_DATA_ITEMS, &point_data_items_size, &error_code);
	if (error_code) {
		std::cout << tobiigaze_get_error_message(error_code) << std::endl;
		delete calibration;
		return NULL;
	}

	delete calibration;
	
	jdoubleArray calibrationPoints = env->NewDoubleArray(4 * (point_data_items_size));  // allocate
		
   	if (NULL == calibrationPoints) return NULL;
   	jdouble *points = env->GetDoubleArrayElements(calibrationPoints, 0);
   		
   	tobiigaze_calibration_point_data item;

   	for (int i = 0; i < point_data_items_size; i++) {
   		item = point_data_items[i];
        
        if (item.left_status == TOBIIGAZE_CALIBRATION_POINT_STATUS_VALID_AND_USED_IN_CALIBRATION) {
        	points[i] = item.left_map_position.x;
        	points[point_data_items_size+i] = item.left_map_position.y;
        } else {
        	points[i] = -1;
        	points[point_data_items_size+i] = -1;
        }
        if (item.right_status == TOBIIGAZE_CALIBRATION_POINT_STATUS_VALID_AND_USED_IN_CALIBRATION) {
        	points[2*point_data_items_size+i] = item.right_map_position.x;
        	points[3*point_data_items_size+i] = item.right_map_position.y;
        } else {
        	points[2*point_data_items_size+i] = 2;
        	points[3*point_data_items_size+i] = 2;
       	}
    }
    env->ReleaseDoubleArrayElements(calibrationPoints, points, 0);
    return calibrationPoints;
}