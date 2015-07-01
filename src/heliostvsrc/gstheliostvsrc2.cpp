/**
 * SECTION:element-HeliosTvSource
 *
 * FIXME:Describe HeliosTvSource here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m heliostvsrc ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <cstdlib>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


extern "C"
{
#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
}

#include <msgpack.hpp>
#include <boost/thread/future.hpp>
#include <boost/asio.hpp>
#include "gstheliostvsrc2.h"


GST_DEBUG_CATEGORY_STATIC (gst_heliostvsrc_debug);
#define GST_CAT_DEFAULT gst_heliostvsrc_debug

#define MAX_READ_SIZE                   8 * 1024
#define max_length			1024

using boost::asio::ip::tcp;

//typedef msgpack::unique_ptr<msgpack::zone> unique_zone;

/* properties */
enum
{
  ARG_0,
  PROP_URI,
  PROP_HOST,
  PROP_PORT_CONTROL,
  PROP_PORT_STREAM,
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpegts, " "systemstream = (boolean) true ")
    );


#define gst_heliostvsrc_parent_class parent_class
G_DEFINE_TYPE (HeliosTvSource, gst_heliostvsrc, GST_TYPE_PUSH_SRC);

static void gst_heliostvsrc_finalize (GObject * object);

static GstCaps *gst_heliostvsrc_getcaps (GstBaseSrc * psrc,
    GstCaps * filter);

static void gst_heliostvsrc_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_heliostvsrc_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_heliostvsrc_start (GstBaseSrc * bsrc);
static gboolean gst_heliostvsrc_stop (GstBaseSrc * bsrc);

static gboolean gst_heliostvsrc_unlock (GstBaseSrc * bsrc);
static gboolean gst_heliostvsrc_unlock_stop (GstBaseSrc * bsrc);
//static gboolean gst_heliostvsrc_is_seekable (GstBaseSrc * bsrc);
//static gboolean gst_heliostvsrc_get_size (GstBaseSrc * src, guint64 * size);

static GstFlowReturn gst_heliostvsrc_create (GstPushSrc * psrc,
    GstBuffer ** outbuf);

/* GObject vmethod implementations */



/***********************************************************************************/
/* initialize the HeliosTvSource's class */
static void
gst_heliostvsrc_class_init (HeliosTvSourceClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesrc_class = GST_BASE_SRC_CLASS (klass);
  gstpushsrc_class = (GstPushSrcClass *) klass;

  /* define virtual function pointers */
  gobject_class->set_property = gst_heliostvsrc_set_property;
  gobject_class->get_property = gst_heliostvsrc_get_property;
  gobject_class->finalize = gst_heliostvsrc_finalize;

  /* define properties */
  g_object_class_install_property (gobject_class, PROP_URI, g_param_spec_string ("uri", "uri", "Set channel uri", "0", (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HOST,
      g_param_spec_string ("host", "host",
          "The host IP address to receive packets from", "localhost",
          (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PORT_CONTROL,
      g_param_spec_int ("port_control", "port_control", "The port to receive packets from", 0,
          10000, 6000,
          (GParamFlags) G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PORT_STREAM,
      g_param_spec_int ("port_stream", "port_stream", "The port to receive packets from", 0,
          10000, 6000,
          (GParamFlags) G_PARAM_READWRITE));



  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&srctemplate));

  gst_element_class_set_details_simple(gstelement_class,
    "HeliosTvSource",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "Fabien FELIO <<user@hostname.org>>");

  gstbasesrc_class->get_caps = gst_heliostvsrc_getcaps;
  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_heliostvsrc_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gst_heliostvsrc_stop);
  gstbasesrc_class->unlock = GST_DEBUG_FUNCPTR (gst_heliostvsrc_unlock);
  gstbasesrc_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_heliostvsrc_unlock_stop);

  gstpushsrc_class->create = gst_heliostvsrc_create;
}
/***********************************************************************************/



/***********************************************************************************/
/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_heliostvsrc_init (HeliosTvSource * source)
{
  source->port_control = 6005;
  source->port_stream = 6006;
  source->host = g_strdup ("localhost");
  source->uri = NULL;
  source->stream = NULL;

  GST_OBJECT_FLAG_UNSET (source, GST_HELIOSTVSRC_OPEN);
}
/***********************************************************************************/



/***********************************************************************************/static void
gst_heliostvsrc_finalize (GObject * object)
{
  if (G_OBJECT_CLASS (parent_class)->finalize)
    G_OBJECT_CLASS (parent_class)->finalize (object);
}
/***********************************************************************************/



/***********************************************************************************/
static GstCaps *
gst_heliostvsrc_getcaps (GstBaseSrc * bsrc, GstCaps * filter)
{
  HeliosTvSource *src;
  GstCaps *caps = NULL;

  src = GST_HELIOSTVSOURCE (bsrc);

  caps = (filter ? gst_caps_ref (filter) : gst_caps_new_any ());

  GST_DEBUG_OBJECT (src, "returning caps %" GST_PTR_FORMAT, caps);
  g_assert (GST_IS_CAPS (caps));
  return caps;
}
/***********************************************************************************/



/***********************************************************************************/
static gboolean
gst_heliostvsrc_start (GstBaseSrc * bsrc)
{
  std::cout << "TcpClient :" << std::endl;
  HeliosTvSource *src = GST_HELIOSTVSOURCE (bsrc);

  //GError *err;

/***** control connection *****/

  std::cout << "Control :" << std::endl;
  HeliosTv::TcpChannelFactory *tcpChannel;
  tcpChannel = HeliosTv::TcpChannelFactory::Instantiate();

  HeliosTv::ControlChannel *controlChannel;
  controlChannel = tcpChannel->newControlChannel(0);

  boost::unique_future<int> f = controlChannel->connect();
  f.wait();

  f = controlChannel->write();
  f.wait();

  f = controlChannel->read();
  f.wait();
  int token = f.get();
  std::cout << "Token :" << token << std::endl;

/***** Stream connection *****/

  std::cout << "Stream :" << std::endl;

  std::string temp(src->uri);
  boost::unique_future<HeliosTv::Stream*> f_stream = controlChannel->newStream(temp, token);
  f_stream.wait();
  src->stream = f_stream.get();

  if (src->stream==NULL)
	std::cout << "NULL :" << std::endl;

  GST_OBJECT_FLAG_SET (src, GST_HELIOSTVSRC_OPEN);

  return TRUE;

  GST_DEBUG_OBJECT (src, "end of start");
}
/***********************************************************************************/



/***********************************************************************************/static gboolean
gst_heliostvsrc_stop (GstBaseSrc * bsrc)
{
  HeliosTvSource *src;

  src = GST_HELIOSTVSOURCE (bsrc);

  GST_OBJECT_FLAG_UNSET (src, GST_HELIOSTVSRC_OPEN);

  return TRUE;
}
/***********************************************************************************/



/***********************************************************************************/
static GstFlowReturn
gst_heliostvsrc_create (GstPushSrc * psrc, GstBuffer ** outbuf)
{
  HeliosTvSource *src;
  src = GST_HELIOSTVSOURCE (psrc);

  GstFlowReturn ret = GST_FLOW_OK;
  GstMapInfo map;
  size_t size;

  boost::asio::mutable_buffers_1 buffer = (src->stream)->get_data_buffer();

  size = boost::asio::buffer_size(buffer);

  while (size <= 0) {
    usleep(10000);
    buffer = (src->stream)->get_data_buffer();
    size = boost::asio::buffer_size(buffer);
  }
  if (size > 0) {
    *outbuf = gst_buffer_new_and_alloc (size);
    gst_buffer_map (*outbuf, &map, (GstMapFlags)GST_MAP_READWRITE);
    memcpy(map.data, boost::asio::buffer_cast<const void *>(buffer),  boost::asio::buffer_size(buffer));
  } else {
    *outbuf = NULL;
  }

  if (size == 0) {
    ret = GST_FLOW_EOS;
    if (*outbuf) {
      gst_buffer_unmap (*outbuf, &map);
      gst_buffer_unref (*outbuf);
    }
    *outbuf = NULL;
  } else if (size < 0) {
    gst_buffer_unmap (*outbuf, &map);
    gst_buffer_unref (*outbuf);
    *outbuf = NULL;
  } else {
    ret = GST_FLOW_OK;
    gst_buffer_unmap (*outbuf, &map);
    gst_buffer_resize (*outbuf, 0, size);
  }

  free(boost::asio::buffer_cast<void *>(buffer));

  return ret;
}
/***********************************************************************************/



/***********************************************************************************/
/* will be called only between calls to start() and stop() */
static gboolean
gst_heliostvsrc_unlock (GstBaseSrc * bsrc)
{
  HeliosTvSource *src = GST_HELIOSTVSOURCE (bsrc);

  GST_DEBUG_OBJECT (src, "set to flushing");

  return TRUE;
}
/***********************************************************************************/



/***********************************************************************************/
/* will be called only between calls to start() and stop() */
static gboolean
gst_heliostvsrc_unlock_stop (GstBaseSrc * bsrc)
{
  HeliosTvSource *src = GST_HELIOSTVSOURCE (bsrc);

  GST_DEBUG_OBJECT (src, "unset flushing");

  return TRUE;
}
/***********************************************************************************/



/***********************************************************************************/
static void
gst_heliostvsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  HeliosTvSource *source = GST_HELIOSTVSOURCE (object);

  switch (prop_id) {
    case PROP_URI:
      source->uri = g_value_dup_string (value); 
      break;
    case PROP_HOST:
      source->host = g_value_dup_string (value); 
      break;
    case PROP_PORT_CONTROL:
      source->port_control = g_value_get_int (value); 
    case PROP_PORT_STREAM:
      source->port_stream = g_value_get_int (value); 
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
/***********************************************************************************/



/***********************************************************************************/
static void
gst_heliostvsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  HeliosTvSource *source = GST_HELIOSTVSOURCE (object);

  switch (prop_id) {
    case PROP_URI:
      g_value_set_string (value, source->uri);
      break;
    case PROP_HOST:
      g_value_set_string (value, source->host);
      break;
    case PROP_PORT_CONTROL:
      g_value_set_int (value, source->port_control);
    case PROP_PORT_STREAM:
      g_value_set_int (value, source->port_stream);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}
/***********************************************************************************/



/***********************************************************************************/static gboolean
heliostvsrc_init (GstPlugin * heliostvsrc)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template HeliosTvSource' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_heliostvsrc_debug, "heliostvsrc",
      0, "Template heliostvsrc");

  return gst_element_register (heliostvsrc, "heliostvsrc", GST_RANK_NONE,
      GST_TYPE_HELIOSTVSOURCE);
}
/***********************************************************************************/



/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "heliostvsrc"
#endif

/* gstreamer looks for this structure to register HeliosTvSources
 *
 * exchange the string 'Template HeliosTvSource' with your HeliosTvSource description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    heliostvsrc,
    "Template heliostvsrc",
    heliostvsrc_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)


