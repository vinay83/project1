#include <gst/gst.h>
#include <opencv2/opencv.hpp>
#include<iostream>
using namespace std;
using namespace cv;

typedef struct video
{
    GstElement *pipeline, *src,  *videoconvert1, *facedetect, *videoconvert2, *sink,*decodebin,*imagefreeze;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
 } ele;

//Hi all these are the changes

static void pad_added_handler (GstElement *src, GstPad *new_pad ,GstElement *data)
{
	GstPadLinkReturn ret;
	GstPad *sink_pad;
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;

	// g_print ("Received new pad '%s' from '%s':\n", GST_PAD_NAME (new_pad), GST_ELEMENT_NAME (src));

	  /* If our converter is already linked, we have nothing to do here */
	new_pad_caps = gst_pad_get_current_caps (new_pad);
	new_pad_struct = gst_caps_get_structure (new_pad_caps, 0);
	new_pad_type = gst_structure_get_name (new_pad_struct);
	// g_print ("It has type '%s'.\n", new_pad_type);
	sink_pad = gst_element_get_static_pad (data, "sink");
	if (gst_pad_is_linked (sink_pad)) {
	    // g_print ("We are already linked. Ignoring.\n");
	    goto exit;
	}
	ret = gst_pad_link (new_pad, sink_pad);
	if (GST_PAD_LINK_FAILED (ret)) {
	    // g_print ("Type is '%s' but link failed.\n", new_pad_type);
	}
	else
	{
	    // g_print ("Link succeeded (type '%s').\n", new_pad_type);
	}
    return;
	exit:
	  /* Unreference the new pad's caps, if we got them */
	if (new_pad_caps != NULL)
	gst_caps_unref (new_pad_caps);

	  /* Unreference the sink pad */
	gst_object_unref (sink_pad);
}



int video_opencv(int a)
{
	  VideoCapture cap(0);


    //   string path="/home/ee212503/Downloads/videos/kannada.mp4";
	// VideoCapture cap(path);


	  // Check if camera opened successfully
	  if (!cap.isOpened()) {
	      cout << "Error opening video stream or file" << endl;
	      return -1;
	  }

	  // Define the codec and create VideoWriter object
	  int codec = VideoWriter::fourcc('M', 'J', 'P', 'G');//    H264 FOR MP4
	  Size frameSize(cap.get(CAP_PROP_FRAME_WIDTH), cap.get(CAP_PROP_FRAME_HEIGHT));
	  double fps = cap.get(CAP_PROP_FPS);
	  int numFrames = a * fps;
	  VideoWriter video("output.avi", codec, fps, frameSize);

	  // Check if VideoWriter opened successfully
	  if (!video.isOpened()) {
	      cout << "Could not open the output video file for write" << std::endl;
	      return -1;
	  }

	  // Read and save frames for 10 seconds
	  for (int i = 0; i < numFrames; i++) {
	      Mat frame;
	      cap >> frame;
	      if (frame.empty()) {
	          cout << "End of video stream" << std::endl;
	          break;
	      }
	      video.write(frame);
          imwrite("image_rec.png", frame); 
          if (cv::waitKey(1) == 'q') 
          {
            break;
          }
	  }

	  // Release the VideoCapture and VideoWriter objects
	  cap.release();
	  video.release();
	  return 0;

}

int video(int argc, char *argv[])
{
    ele e;
    int a;
    cout<<"enter how many seconds to recored video"<<endl;
    cin>>a;

    // Open the default camera
    video_opencv(a);

    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    /* Create elements */
    e.pipeline = gst_pipeline_new(NULL);
    e.src = gst_element_factory_make("filesrc", "src");
    g_object_set(G_OBJECT(e.src), "location", "output.avi", NULL);
    e.decodebin = gst_element_factory_make("decodebin", "decodebin");
    e.videoconvert1 = gst_element_factory_make("videoconvert", "videoconvert1");
    e.facedetect = gst_element_factory_make("facedetect", "facedetect");
    g_object_set(G_OBJECT(e.facedetect), "min-size-width", 60, "min-size-height", 60, "profile", "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml", NULL);
    e.videoconvert2 = gst_element_factory_make("videoconvert", "videoconvert2");
    e.sink = gst_element_factory_make("autovideosink", "sink");
    /* Add elements to the pipeline */
    gst_bin_add_many(GST_BIN(e.pipeline), e.src, e.decodebin,e.facedetect,e.videoconvert1, e.videoconvert2, e.sink, NULL);

    /* Link elements */
    gst_element_link_many(e.src,e.decodebin,   NULL);

    g_signal_connect(e.decodebin,"pad-added",G_CALLBACK(pad_added_handler),e.videoconvert1);

    gst_element_link_many(e.videoconvert1,e.facedetect,e.videoconvert2,e.sink,  NULL);


    /* Set the pipeline state to playing */
    e.ret = gst_element_set_state(e.pipeline, GST_STATE_PLAYING);

    if (e.ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(e.pipeline);
        return -1;
    }
    /* Wait until error or EOS */
    e.bus = gst_element_get_bus(e.pipeline);
    e.msg = gst_bus_timed_pop_filtered(e.bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    /* Parse message */
    if (e.msg != NULL)
    {
        GError *err;
        gchar *debug_info;
        switch (GST_MESSAGE_TYPE(e.msg))
        {
            case GST_MESSAGE_ERROR:
            gst_message_parse_error(e.msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(e.msg->src), err->message);
            g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
            g_clear_error(&err);
            g_free(debug_info);
            break;
            case GST_MESSAGE_EOS:
            g_print("End-Of-Stream reached.\n");
            break;
            default:
            /* We should not reach here because we only asked for ERRORs and EOS */
            g_printerr("Unexpected message received.\n");
            break;
        }
        gst_message_unref(e.msg);
    }

    /* Free resources */
    gst_object_unref(e.bus);
    gst_element_set_state(e.pipeline, GST_STATE_NULL);
    gst_object_unref(e.pipeline);

    return 0;
}

int image(int argc, char *argv[])
{
    ele e;
    // Open the default camera
    video_opencv(1);

    /* Initialize GStreamer */
    gst_init(&argc, &argv);
    /* Create elements */
    e.pipeline = gst_pipeline_new(NULL);
    e.src = gst_element_factory_make("filesrc", "src");
    g_object_set(G_OBJECT(e.src), "location", "image_rec.png", NULL);
    e.decodebin = gst_element_factory_make("decodebin", "decodebin");
    e.imagefreeze = gst_element_factory_make("imagefreeze", "imagefreeze");
    e.videoconvert1 = gst_element_factory_make("videoconvert", "videoconvert1");
    e.facedetect = gst_element_factory_make("facedetect", "facedetect");
    g_object_set(G_OBJECT(e.facedetect), "min-size-width", 60, "min-size-height", 60, "profile", "/usr/share/opencv/haarcascades/haarcascade_frontalface_default.xml", NULL);
    e.videoconvert2 = gst_element_factory_make("videoconvert", "videoconvert2");
    e.sink = gst_element_factory_make("autovideosink", "sink");
    /* Add elements to the pipeline */
    gst_bin_add_many(GST_BIN(e.pipeline), e.src, e.decodebin,e.imagefreeze,e.facedetect, e.videoconvert1,  e.videoconvert2, e.sink, NULL);

    /* Link elements */
    gst_element_link_many(e.src,e.decodebin,   NULL);

    g_signal_connect(e.decodebin,"pad-added",G_CALLBACK(pad_added_handler),e.imagefreeze);

    gst_element_link_many(e.imagefreeze,e.videoconvert1,e.facedetect,e.videoconvert2,e.sink,  NULL);


    /* Set the pipeline state to playing */
    e.ret = gst_element_set_state(e.pipeline, GST_STATE_PLAYING);

    if (e.ret == GST_STATE_CHANGE_FAILURE)
    {
        g_printerr("Unable to set the pipeline to the playing state.\n");
        gst_object_unref(e.pipeline);
        return -1;
    }
    /* Wait until error or EOS */
    e.bus = gst_element_get_bus(e.pipeline);
    e.msg = gst_bus_timed_pop_filtered(e.bus, GST_CLOCK_TIME_NONE, GstMessageType(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
    /* Parse message */
    if (e.msg != NULL)
    {
        GError *err;
        gchar *debug_info;
        switch (GST_MESSAGE_TYPE(e.msg))
        {
            case GST_MESSAGE_ERROR:
            gst_message_parse_error(e.msg, &err, &debug_info);
            g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(e.msg->src), err->message);
            g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
            g_clear_error(&err);
            g_free(debug_info);
            break;
            case GST_MESSAGE_EOS:
            g_print("End-Of-Stream reached.\n");
            break;
            default:
            /* We should not reach here because we only asked for ERRORs and EOS */
            g_printerr("Unexpected message received.\n");
            break;
        }
        gst_message_unref(e.msg);
    }

    /* Free resources */
    gst_object_unref(e.bus);

    gst_element_set_state(e.pipeline, GST_STATE_NULL);
    gst_object_unref(e.pipeline);

    return 0;
}

int main(int argc, char *argv[])
{
    int a;
    
    for(;;)
    {
        cout<<"enter 1 for recording video and do FACE DETECTION"<<endl;
        cout<<"enter 2 for capturing image and do FACE DETECTION"<<endl;
        cout<<"enter 3 for quiting"<<endl;
        cin>>a;
        switch(a)
        {
            case 1 : video(argc, argv);
            break;
            case 2 : image(argc, argv);
            break;
            case 3 : exit(0);
            break;
            default :
            cout<<"wrong input"<<endl;
            break;
        }
    }
}
