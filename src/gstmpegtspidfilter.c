/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2015 Fabien FELIO <<user@hostname.org>>
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
 * SECTION:element-mpegtspidfilter
 *
 * FIXME:Describe mpegtspidfilter here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! mpegtspidfilter ! fakesink silent=TRUE
 * ]|
 * </refsect2>
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


/* properties */
enum
{
  ARG_0,
  PROP_PID,
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpegts, " "systemstream = (boolean) true ")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/mpegts, " "systemstream = (boolean) true ")
    );

#define gst_my_filter_parent_class parent_class
G_DEFINE_TYPE (MpegtsPidFilter, gst_my_filter, GST_TYPE_ELEMENT);

static void gst_my_filter_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_my_filter_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);

static gboolean gst_my_filter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_my_filter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

static GstStateChangeReturn gst_my_filter_change_state (GstElement *element, GstStateChange transition);

static inline gchar * _csl_from_list (GList * input);

/* GObject vmethod implementations */

/* initialize the mpegtspidfilter's class */
static void
gst_my_filter_class_init (MpegtsPidFilterClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gstelement_class->change_state = gst_my_filter_change_state;

  /* define virtual function pointers */
  gobject_class->set_property = gst_my_filter_set_property;
  gobject_class->get_property = gst_my_filter_get_property;

  /* define properties */ //--------> corriger
  g_object_class_install_property (gobject_class, PROP_PID, g_param_spec_string ("pids", "pids", "Set List pids in decimal : pid1,pid2,...", "0", G_PARAM_READWRITE));

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));

  gst_element_class_set_details_simple(gstelement_class,
    "MpegtsPidFilter",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "Fabien FELIO <<user@hostname.org>>");
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_my_filter_init (MpegtsPidFilter * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_my_filter_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_my_filter_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->reste = NULL;

  //filter->silent = FALSE;
}

static void
gst_my_filter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  MpegtsPidFilter *filter = GST_MPEGTSPIDFILTER (object);

  gchar * s;
  gchar * temp;
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
        temp = strtok (NULL, ",");
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_my_filter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  MpegtsPidFilter *filter = GST_MPEGTSPIDFILTER (object);

  gchar * output;

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

/* GstElement vmethod implementations */

/* this function handles sink events */
static gboolean 
gst_my_filter_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean ret;
  //MpegtsPidFilter *filter;

  //filter = GST_MPEGTSPIDFILTER (parent);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps * caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    case GST_EVENT_EOS:
      /* end-of-stream, we should close down all stream leftovers here */
      //gst_my_filter_stop_processing (filter);

      ret = gst_pad_event_default (pad, parent, event);
      break;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

/***********************************************************************************/
/********************************* chain function **********************************/
static GstFlowReturn
gst_my_filter_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  //variable declaration and init
  MpegtsPidFilter * filter;
  filter = GST_MPEGTSPIDFILTER (parent);

  GstBuffer * buf_out;
  buf_out = gst_buffer_new ();

  //GstIterator * iter;
  //iter = gst_iterator_new_list (G_TYPE_INT, GMutex *lock, guint32 *master_cookie, &iter, GObject *owner, GstIteratorItemFunction item);
  //GstIteratorResult iter_res = GST_ITERATOR_OK; 

  int i, j, k, cnt = 0, cmp_pid = 0, offset =0;
  guint16 test = 0x0000, buf_pid = 1; 
  int buf_offset = 0;

  gboolean pid_ok = 0;

  GList * p_pids = filter->pids;

  GstMapInfo info, info_out, info_reste;

  gst_buffer_map (buf, &info, GST_MAP_READ);
  gst_buffer_map (buf_out, &info_out, GST_MAP_READ);
  
  if (filter->reste != NULL)
  {
    gst_buffer_map (filter->reste, &info_reste, GST_MAP_READ);
  }
 
  buf_pid = GPOINTER_TO_INT ((filter->pids)->data);

  buf_offset -= filter->diff;
  filter->buf_number++;


  //merge package parts if need to copy
  if (filter->diff!=0 && filter->copy)
  {
    //data from buffer n-1
    gst_buffer_copy_into (buf_out, filter->reste, GST_BUFFER_COPY_MEMORY, 0, filter->diff);

    //data from buffer n
    gst_buffer_copy_into (buf_out, buf, GST_BUFFER_COPY_MEMORY, 0, (PACKAGE_SIZE-filter->diff));
    gst_buffer_map (buf_out, &info_out, GST_MAP_READ);

    //gst_buffer_remove_all_memory (filter->reste);
    cnt+=PACKAGE_SIZE;
  }

  if (filter->diff!=0)
  {
    filter->diff=PACKAGE_SIZE-filter->diff;
  }

  offset=filter->diff;



  /************** filtrage PID **************/
  for (i=0; i<(info.size-offset);i+=PACKAGE_SIZE)
  {
    if (buf_offset < 0)
    {
      buf_offset += PACKAGE_SIZE;
    }

    test = info.data[buf_offset+1]<<8;
    test += info.data[buf_offset+2];
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

    filter->pids = p_pids;

/*
    while (iter_res == GST_ITERATOR_OK && g_list_length (filter->pids) != 0 && !pid_ok) {
      gint pids_list;
      iter_res = gst_iterator_next(iter, pids_list);
      if (pids_list == test)
      {
        pid_ok = 1;
      }
    }
*/

    //Parsing buffer
    if (pid_ok && buf_offset<=(info.size-PACKAGE_SIZE))
    {
      gst_buffer_copy_into (buf_out, buf, GST_BUFFER_COPY_MEMORY, buf_offset, PACKAGE_SIZE);
      gst_buffer_map (buf_out, &info_out, GST_MAP_READ);

      pid_ok = 0;
    }
  buf_offset += PACKAGE_SIZE;
  }
  
  buf_offset -= PACKAGE_SIZE;

  filter->diff = info.size-buf_offset;
  

  //Print end of buffer
  if (filter->diff!=0)
  {
    filter->reste = gst_buffer_new (); //_allocate (NULL, filter->diff, NULL);
    gst_buffer_copy_into (filter->reste, buf, GST_BUFFER_COPY_MEMORY, buf_offset, filter->diff);
    gst_buffer_map (filter->reste, &info_reste, GST_MAP_READ);
  }


  //test copy end of buffer
  if (pid_ok)
    filter->copy = TRUE;
  else
    filter->copy = FALSE;


  //Unmapbuffer
  gst_buffer_unmap (buf, &info);
  gst_buffer_unmap (buf_out, &info_out);
  
/*
  g_list_free (p_pids);
*/

  return gst_pad_push (filter->srcpad, buf_out);
}
/***********************************************************************************/


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
mpegtspidfilter_init (GstPlugin * mpegtspidfilter)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template mpegtspidfilter' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_mpegtspidfilter_debug, "mpegtspidfilter",
      0, "Template mpegtspidfilter");

  return gst_element_register (mpegtspidfilter, "mpegtspidfilter", GST_RANK_NONE,
      GST_TYPE_MPEGTSPIDFILTER);
}

/* change_state function
 */
static GstStateChangeReturn
gst_my_filter_change_state (GstElement *element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  MpegtsPidFilter *filter = GST_MPEGTSPIDFILTER (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      //if (!gst_my_filter_allocate_memory (filter))
      //  return GST_STATE_CHANGE_FAILURE;
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
     // gst_my_filter_free_memory (filter);
      break;
    default:
      break;
  }

  return ret;
}



/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "mpegtspidfilter"
#endif

/* gstreamer looks for this structure to register mpegtspidfilters
 *
 * exchange the string 'Template mpegtspidfilter' with your mpegtspidfilter description
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

static inline gchar * _csl_from_list (GList * input)
{
  gchar * csl = NULL;
  gchar *tmp, * field;
  
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

