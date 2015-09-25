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
#ifndef __MPEGTSPIDFILTER__
#define __MPEGTSPIDFILTER__

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>

G_BEGIN_DECLS

/* Standard macros for defining types for this element.  */
/* #defines don't like whitespacey bits */
#define GST_TYPE_MPEGTSPIDFILTER \
  (gst_mpegtspidfilter_get_type())
#define GST_MPEGTSPIDFILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MPEGTSPIDFILTER,MpegtsPidFilter))
#define GST_MPEGTSPIDFILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MPEGTSPIDFILTER,MpegtsPidFilter))
#define GST_IS_MPEGTSPIDFILTER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MPEGTSPIDFILTER))
#define GST_IS_MPEGTSPIDFILTER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MPEGTSPIDFILTER))

typedef struct _MpegtsPidFilter      MpegtsPidFilter;
typedef struct _MpegtsPidFilterClass MpegtsPidFilterClass;

/* Definition of structure storing data for this element. */
struct _MpegtsPidFilter
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  GList *pids;

  GstBuffer *reste;

  int copy;
};

/* Standard definition defining a class for this element. */
struct _MpegtsPidFilterClass
{
  GstElementClass parent_class;
};

GType gst_mpegtspidfilter_get_type (void);

G_END_DECLS

#endif /* __MPEGTSPIDFILTER__ */
