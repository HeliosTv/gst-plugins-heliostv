/*
*****************************************************************************
* HeliosTv
* Copyright (C) 2015  SoftAtHome
*
* Authors:
*   Fabien Felio  <fabien dot felio at softathome dot com>
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
* Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
* Boston, MA 02110-1301, USA.
******************************************************************************
*/
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gst/gst.h>
#include "gstmpegtspidfilter.h"

GST_DEBUG_CATEGORY_STATIC (gst_mpegtspidfilter_debug);
#define GST_CAT_DEFAULT gst_mpegtspidfilter_debug
#define PACKAGE_SIZE 188


/*properties */
enum
{
  ARG_0,
  PROP_PID,
};

/*the capabilities of the inputs and outputs.
 *
 *describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpegts")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpegts")
    );

#define gst_mpegtspidfilter_parent_class parent_class
G_DEFINE_TYPE (MpegtsPidFilter, gst_mpegtspidfilter, GST_TYPE_ELEMENT);


static void gst_mpegtspidfilter_finalize (GObject * object);
static void gst_mpegtspidfilter_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);
static void gst_mpegtspidfilter_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gboolean gst_mpegtspidfilter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);

static GstFlowReturn gst_mpegtspidfilter_chain (GstPad *pad, GstObject *parent, GstBuffer *buf);

static inline gchar *_csl_from_list (GList *input);

/*GObject vmethod implementations */

/*initialize the mpegtspidfilter's class */
static void
gst_mpegtspidfilter_class_init (MpegtsPidFilterClass *klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  /*define virtual function pointers */
  gobject_class->set_property = gst_mpegtspidfilter_set_property;
  gobject_class->get_property = gst_mpegtspidfilter_get_property;
  gobject_class->finalize = gst_mpegtspidfilter_finalize;

  /*define properties */
  g_object_class_install_property (gobject_class, PROP_PID, g_param_spec_string ("pids", "pids", "Set List pids : pid1,pid2,...", "0", G_PARAM_READWRITE));

  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class, gst_static_pad_template_get (&sink_factory));

  gst_element_class_set_details_simple(gstelement_class,
    "MpegtsPidFilter",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "Fabien FELIO <<user@hostname.org>>");
}

/*initialize the new element
 *instantiate pads and add them to element
 *set pad calback functions
 *initialize instance structure
 */
static void
gst_mpegtspidfilter_init (MpegtsPidFilter *filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");

  gst_pad_set_chain_function (filter->sinkpad, GST_DEBUG_FUNCPTR(gst_mpegtspidfilter_chain));
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_mpegtspidfilter_sink_event));

  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->pids = NULL;
  filter->reste = gst_buffer_new ();
  filter->copy = FALSE;
}

static void
gst_mpegtspidfilter_finalize (GObject * object)
{
  MpegtsPidFilter *mpegtspidfilter;

  mpegtspidfilter = GST_MPEGTSPIDFILTER (object);

  g_free (mpegtspidfilter->pids);

  gst_buffer_unref (mpegtspidfilter->reste);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_mpegtspidfilter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean ret;
  MpegtsPidFilter *filter;

  filter = GST_MPEGTSPIDFILTER (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;
      gst_event_parse_caps (event, &caps);
      ret = gst_pad_push_event (filter->srcpad, event);
      break;
    }
    default:
      ret = gst_pad_push_event (filter->srcpad, event);
      break;
  }
  return ret;
}

static void
gst_mpegtspidfilter_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  MpegtsPidFilter *filter = GST_MPEGTSPIDFILTER (object);

  gchar *s=NULL;
  gchar *temp=NULL;
  gint temp_int;

  switch (prop_id) {
    case PROP_PID:
      s = g_value_get_string (value);
      //parsing
      temp = strtok (s, ",;:");
      while (temp != NULL)
      {
        temp_int = atoi(temp);
        filter->pids = g_list_append (filter->pids, GINT_TO_POINTER (temp_int));
        temp = strtok (NULL, ",;:");
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_mpegtspidfilter_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
  MpegtsPidFilter *filter = GST_MPEGTSPIDFILTER (object);

  gchar *output=NULL;

  switch (prop_id) {
    case PROP_PID:
      output = _csl_from_list (filter->pids);
      g_value_set_string (value, output);
      g_free (output);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}



/***********************************************************************************/
/*********************************chain function **********************************/
static GstFlowReturn
gst_mpegtspidfilter_chain (GstPad *pad, GstObject *parent, GstBuffer *buf)
{


  //variable declaration and init
  MpegtsPidFilter *filter;
  filter = GST_MPEGTSPIDFILTER (parent);

  GstBuffer *buf_out;
  buf_out = gst_buffer_new ();

  int i, cmp_pid = 0, offset = 0;
  guint16 test = 0;
  int buf_offset = 0;
  gboolean pid_ok = 0;

  if(filter->pids==NULL)
  {
    pid_ok = 1;
  }

  GList *pointer_pids = filter->pids;

  GstMapInfo info, info_reste;

  gst_buffer_map (buf, &info, GST_MAP_READ);

  //Get Size of reste data
  gst_buffer_map (filter->reste, &info_reste, GST_MAP_READ);
  buf_offset = info_reste.size;
  gst_buffer_unmap (filter->reste, &info_reste);

  //merge package parts if need to copy
  if (buf_offset>0 && filter->copy)
  {
    gst_buffer_copy_into (buf_out, filter->reste, GST_BUFFER_COPY_MEMORY, 0, buf_offset);

    gst_buffer_copy_into (buf_out, buf, GST_BUFFER_COPY_MEMORY, 0, (PACKAGE_SIZE-buf_offset));
  }

  if (buf_offset>0)
    buf_offset = PACKAGE_SIZE-buf_offset;

  offset = buf_offset;

  /**************filtrage PID **************/
  for (i=offset; i<info.size;i+=PACKAGE_SIZE)
  {

    test = info.data[i+1]<<8;
    test += info.data[i+2];
    test &= 0x1FFF;

    //Test on PID
    while ((filter->pids) != NULL && g_list_length (filter->pids) != 0 && !pid_ok) {
      if (GPOINTER_TO_INT ((filter->pids)->data) == test)
      {
        pid_ok = 1;
      }
      cmp_pid++;
      filter->pids = g_list_next (filter->pids);
      if (pid_ok)
      {
        filter->pids = g_list_previous (filter->pids);
      }
    }

    filter->pids = pointer_pids;

    //Parsing buffer
    if (pid_ok && i<(info.size-PACKAGE_SIZE))
    {
      gst_buffer_copy_into (buf_out, buf, GST_BUFFER_COPY_MEMORY, i, PACKAGE_SIZE);

      pid_ok = 0;
    }
  }

  i -= PACKAGE_SIZE;

  //Copy end of buffer
  if ((info.size-i)>0)
  {
    gst_buffer_unref (filter->reste);
    filter->reste = gst_buffer_new ();
    gst_buffer_copy_into (filter->reste, buf, GST_BUFFER_COPY_MEMORY, i, info.size-i);
  }

  //test copy end of buffer
  if (pid_ok)
    filter->copy = 1;
  else
    filter->copy = 0;


  //Unmapbuffer
  gst_buffer_unmap (buf, &info);
  gst_buffer_unref(buf);

  return gst_pad_push (filter->srcpad, buf_out);
}
/***********************************************************************************/


/*entry point to initialize the plug-in
 *initialize the plug-in itself
 *register the element factories and other features
 */
static gboolean
mpegtspidfilter_init (GstPlugin *mpegtspidfilter)
{
  /*debug category for fltering log messages
   *
   *exchange the string 'Template mpegtspidfilter' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_mpegtspidfilter_debug, "mpegtspidfilter",
      0, "Template mpegtspidfilter");

  return gst_element_register (mpegtspidfilter, "mpegtspidfilter", GST_RANK_NONE,
      GST_TYPE_MPEGTSPIDFILTER);
}


/*PACKAGE: this is usually set by autotools depending on some _INIT macro
 *in configure.ac and then written into and defined in config.h, but we can
 *just set it ourselves here in case someone doesn't use autotools to
 *compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "mpegtspidfilter"
#endif

/*gstreamer looks for this structure to register mpegtspidfilters
 *
 *exchange the string 'Template mpegtspidfilter' with your mpegtspidfilter description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    mpegtspidfilter,
    "Template mpegtspidfilter",
    mpegtspidfilter_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)

static inline gchar *_csl_from_list (GList *input)
{
  gchar *csl = NULL;
  gchar *tmp, *field;

  if (!input || !g_list_length (input)) {
    goto beach;
  }

  csl = g_strdup_printf ("%d", GPOINTER_TO_INT (input->data));

  input = g_list_next (input);

  while (input != NULL) {
    field = g_strdup_printf ("%d", GPOINTER_TO_INT (input->data));

    tmp = g_strjoin (",", csl, field, NULL);

    g_free (csl);
    g_free (field);

    csl = tmp;

    input = g_list_next (input);
  }

  beach:

  return csl;
}

