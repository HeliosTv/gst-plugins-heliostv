/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2015 Fabien FELIO
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

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
#include <cstring>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include <gst/gst.h>
#include <gst/base/gstbasesrc.h>
}

#include <msgpack.hpp>
#include "gstheliostvsrc2.h"


GST_DEBUG_CATEGORY_STATIC (gst_heliostvsrc_debug);
#define GST_CAT_DEFAULT gst_heliostvsrc_debug

#define MAX_READ_SIZE                   4 * 1024
#define max_length			1024

using boost::asio::ip::tcp;


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
  //gstbasesrc_class->is_seekable = GST_DEBUG_FUNCPTR (gst_heliostvsrc_is_seekable);
  //gstbasesrc_class->get_size = GST_DEBUG_FUNCPTR (gst_heliostvsrc_get_size);

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
  source->io_service = new boost::asio::io_service;
  source->socket = new boost::asio::ip::tcp::socket(*source->io_service);

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
  HeliosTvSource *src = GST_HELIOSTVSOURCE (bsrc);
  //GError *err;

  GST_DEBUG_OBJECT (src, "boost version");

  tcp::resolver resolver(*src->io_service);

/***** control connection *****/

  std::cout << "Control connection" << std::endl;

  char port[10] = "";
  sprintf(port, "%d", src->port_control);
  GST_DEBUG_OBJECT (src, "boost version new port : %s", port);

  tcp::resolver::query query_control(tcp::v4(), src->host, port);
  GST_DEBUG_OBJECT (src, "boost version");
  src->iterator = resolver.resolve(query_control);

  GST_DEBUG_OBJECT (src, "boost opening receiving client socket to %s:%d",
      src->host, src->port_stream);
  GST_DEBUG_OBJECT (src, "opened receiving client socket");
  GST_OBJECT_FLAG_SET (src, GST_HELIOSTVSRC_OPEN);


  GST_DEBUG_OBJECT (src, "connect");
  try
  {
  boost::asio::connect(*src->socket, src->iterator);
  }
  catch (boost::system::system_error const& e)
  {
    std::cout << "Warning: could not connect : " << e.what() << std::endl;
  }


    // REQUEST CONTROL
    msgpack::sbuffer sbuf_control;
    char temp_control[max_length];

    strcpy (temp_control, src->uri);

    msgpack::type::tuple<int> req_control(0);

    msgpack::pack(sbuf_control, req_control);
    boost::asio::write(*src->socket, boost::asio::buffer(sbuf_control.data(), 20));


    // REPLY CONTROL
    /*msgpack::unpacker m_pac;
    m_pac.reserve_buffer(1024);
    size_t reply_length_control = boost::asio::read(*src->socket, m_pac.buffer());
*/

    char reply_control[64];
    size_t reply_length_control = read(*src->socket, boost::asio::buffer(reply_control, 9));

    std::cout << "length : " << reply_length_control << "\n" << std::endl;
    
    msgpack::unpacked msg_control;

    msgpack::type::tuple<int, std::string> rep_control;

    msgpack::unpack(&msg_control, reply_control, reply_length_control);
    msg_control.get().convert(&rep_control);

    int ident = std::get<0>(rep_control);
    std::cout << "reply : " << std::get<0>(rep_control) << ", " << std::get<1>(rep_control) << "\n" << std::endl;

    if (strcmp(std::get<1>(rep_control).c_str(), "OK"))
    {
      std::cout << "FALSE Control\n" << std::endl;
      return FALSE;
    }

    std::cout << "TRUE Control\n" << std::endl;

/***** control disconnection *****/

    std::cout << "closing socket" << std::endl;
    if ((*src->socket).is_open())
    {
      GST_DEBUG_OBJECT (src, "closing socket");
      (*src->socket).close(); 
      src->socket = new boost::asio::ip::tcp::socket(*src->io_service);
    }

/***** Stream connection *****/

  std::cout << "Stream connection" << std::endl;
  sprintf(port, "%d", src->port_stream);
  GST_DEBUG_OBJECT (src, "boost version new port : %s", port);

  tcp::resolver::query query_stream(tcp::v4(), src->host, port);
  GST_DEBUG_OBJECT (src, "boost version");
  src->iterator = resolver.resolve(query_stream);

  /* create receiving client socket */
  GST_DEBUG_OBJECT (src, "boost opening receiving client socket to %s:%d",
      src->host, src->port_stream);
  GST_DEBUG_OBJECT (src, "opened receiving client socket");
  GST_OBJECT_FLAG_SET (src, GST_HELIOSTVSRC_OPEN);


  GST_DEBUG_OBJECT (src, "connect");
  try
  {
  boost::asio::connect(*src->socket, src->iterator);
  }
  catch (boost::system::system_error const& e)
  {
    std::cout << "Warning: could not connect : " << e.what() << std::endl;
  }


    // REQUEST STREAM
    msgpack::sbuffer sbuf_stream;
    char temp_stream[max_length];

    strcpy (temp_stream, src->uri);

    msgpack::type::tuple<int, int, std::string> req_stream(1, ident, temp_stream);

    msgpack::pack(sbuf_stream, req_stream);
    boost::asio::write(*src->socket, boost::asio::buffer(sbuf_stream.data(), sbuf_stream.size()));


    // REPLY STREAM
    char reply_stream[10];
    size_t reply_length_stream = boost::asio::read(*src->socket, boost::asio::buffer(reply_stream, 4));

    std::cout << "length : " << reply_length_stream << "\n" << std::endl;

    msgpack::unpacked msg_stream;
    msgpack::type::tuple<std::string> rep_stream;

    msgpack::unpack(&msg_stream, reply_stream, reply_length_stream);
    msg_stream.get().convert(&rep_stream);

    std::cout << "reply : " << std::get<0>(rep_stream) << "\n" << std::endl;

    if (strcmp(std::get<0>(rep_stream).c_str(), "OK"))
    {
      std::cout << "FALSE Stream\n" << std::endl;
      return FALSE;
    }

    std::cout << "TRUE Stream\n" << std::endl;
    return TRUE;

/*no_socket:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
        ("Failed to create socket: %s", err->message));
    g_clear_error (&err);
    return FALSE;
  }*/

    GST_DEBUG_OBJECT (src, "end of start");
}
/***********************************************************************************/



/***********************************************************************************/static gboolean
gst_heliostvsrc_stop (GstBaseSrc * bsrc)
{
  HeliosTvSource *src;

  src = GST_HELIOSTVSOURCE (bsrc);

  if ((*src->socket).is_open()) {
    GST_DEBUG_OBJECT (src, "closing socket");
    (*src->socket).close(); 
  }

  GST_OBJECT_FLAG_UNSET (src, GST_HELIOSTVSRC_OPEN);

  return TRUE;
}
/***********************************************************************************/



/***********************************************************************************/
static GstFlowReturn
gst_heliostvsrc_create (GstPushSrc * psrc, GstBuffer ** outbuf)
{
  HeliosTvSource *src;
  GstFlowReturn ret = GST_FLOW_OK;
  gssize rret;
  GError *err = NULL;
  GstMapInfo map;
  gssize avail, read;

  src = GST_HELIOSTVSOURCE (psrc);

  GST_DEBUG_OBJECT (src, "create function");

  if (!GST_OBJECT_FLAG_IS_SET (src, GST_HELIOSTVSRC_OPEN))
    goto wrong_state;

  GST_LOG_OBJECT (src, "asked for a buffer");

  // read the buffer header
  avail = (*src->socket).available();
  while (avail <= 0) {
    usleep(10000);
    avail = (*src->socket).available();
  }
    /*goto get_available_error;
  } else if (avail == 0) {
    GIOCondition condition;

    if (!g_socket_condition_wait (*src->socket,
            G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP, src->cancellable, &err))
      goto select_error;

    condition =
        g_socket_condition_check (*src->socket,
        G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP);

    if ((condition & G_IO_ERR)) {
      GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
          ("Socket in error state"));
      *outbuf = NULL;
      ret = GST_FLOW_ERROR;
      goto done;
    } else if ((condition & G_IO_HUP)) {
      GST_DEBUG_OBJECT (src, "Connection closed");
      *outbuf = NULL;
      ret = GST_FLOW_EOS;
      goto done;
    }
    avail = g_socket_get_available_bytes (*src->socket);
    if (avail < 0)
      goto get_available_error;
  }*/

  if (avail > 0) {
    read = MIN (avail, MAX_READ_SIZE);
    *outbuf = gst_buffer_new_and_alloc (read);
    gst_buffer_map (*outbuf, &map, (GstMapFlags)GST_MAP_READWRITE);
    rret = boost::asio::read(*src->socket, boost::asio::buffer(map.data, map.size));
  } else {
    // Connection closed
    *outbuf = NULL;
    read = 0;
    rret = 0;
  }

  if (rret == 0) {
    GST_DEBUG_OBJECT (src, "Connection closed");
    ret = GST_FLOW_EOS;
    if (*outbuf) {
      gst_buffer_unmap (*outbuf, &map);
      gst_buffer_unref (*outbuf);
    }
    *outbuf = NULL;
  } else if (rret < 0) {
    /*if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      ret = GST_FLOW_FLUSHING;
      GST_DEBUG_OBJECT (src, "Cancelled reading from socket");
    } else {
      ret = GST_FLOW_ERROR;
      GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
          ("Failed to read from socket: %s", err->message));
    }*/
    gst_buffer_unmap (*outbuf, &map);
    gst_buffer_unref (*outbuf);
    *outbuf = NULL;
  } else {
    ret = GST_FLOW_OK;
    gst_buffer_unmap (*outbuf, &map);
    gst_buffer_resize (*outbuf, 0, rret);

    GST_LOG_OBJECT (src,
        "Returning buffer from _get of size %" G_GSIZE_FORMAT ", ts %"
        GST_TIME_FORMAT ", dur %" GST_TIME_FORMAT
        ", offset %" G_GINT64_FORMAT ", offset_end %" G_GINT64_FORMAT,
        gst_buffer_get_size (*outbuf),
        GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (*outbuf)),
        GST_TIME_ARGS (GST_BUFFER_DURATION (*outbuf)),
        GST_BUFFER_OFFSET (*outbuf), GST_BUFFER_OFFSET_END (*outbuf));
  }
  g_clear_error (&err);

//done:
  return ret;

/*select_error:
  {
    if (g_error_matches (err, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      GST_DEBUG_OBJECT (src, "Cancelled");
      ret = GST_FLOW_FLUSHING;
    } else {
      GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
          ("Select failed: %s", err->message));
      ret = GST_FLOW_ERROR;
    }
    g_clear_error (&err);
    return ret;
  }
get_available_error:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("Failed to get available bytes from socket"));
    return GST_FLOW_ERROR;
  }*/
wrong_state:
  {
    GST_DEBUG_OBJECT (src, "connection to closed, cannot read data");
    return GST_FLOW_FLUSHING;
  }
}
/***********************************************************************************/



/***********************************************************************************/
/* will be called only between calls to start() and stop() */
static gboolean
gst_heliostvsrc_unlock (GstBaseSrc * bsrc)
{
  HeliosTvSource *src = GST_HELIOSTVSOURCE (bsrc);

  GST_DEBUG_OBJECT (src, "set to flushing");
  //g_cancellable_cancel (src->cancellable);

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
  //g_cancellable_reset (src->cancellable);

  return TRUE;
}
/***********************************************************************************/



/***********************************************************************************/
/*
static gboolean
gst_file_src_set_location (HeliosTvSource * src, const gchar * location)
{
  GstState state;

  GST_OBJECT_LOCK (src);
  state = GST_STATE (src);
  if (state != GST_STATE_READY && state != GST_STATE_NULL)
    goto wrong_state;
  GST_OBJECT_UNLOCK (src);


  if (location == NULL) {
    src->uri = NULL;
  } else {

    src->uri = gst_filename_to_uri (location, NULL);
    GST_INFO ("uri      : %s", src->uri);
  }
  g_object_notify (G_OBJECT (src), "location");


  return TRUE;


wrong_state:
  {
    g_warning ("Changing the `location' property on filesrc when a file is "
        "open is not supported.");
    GST_OBJECT_UNLOCK (src);
    return FALSE;
  }
}*/
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


