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
#ifndef __HELIOSTVSOURCE__
#define __HELIOSTVSOURCE__

#include <gst/gst.h>
#include <boost/asio.hpp>
#include <gst/base/gstpushsrc.h>
#include "heliostv/TcpClient.h"
#include "heliostv/ControlChannel.h"
#include "heliostv/TcpChannelFactory.h"
#include "heliostv/Stream.h"

G_BEGIN_DECLS

/* Standard macros for defining types for this element.  */
/* #defines don't like whitespacey bits */
#define GST_TYPE_HELIOSTVSOURCE \
  (gst_heliostvsrc_get_type())
#define GST_HELIOSTVSOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_HELIOSTVSOURCE,HeliosTvSource))
#define GST_HELIOSTVSOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_HELIOSTVSOURCE,HeliosTvSource))
#define GST_IS_HELIOSTVSOURCE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_HELIOSTVSOURCE))
#define GST_IS_HELIOSTVSOURCE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_HELIOSTVSOURCE))

typedef struct _HeliosTvSource      HeliosTvSource;
typedef struct _HeliosTvSourceClass HeliosTvSourceClass;

typedef enum {
  GST_HELIOSTVSRC_OPEN       = (GST_BASE_SRC_FLAG_LAST << 0),

  GST_HELIOSTVSRC_FLAG_LAST  = (GST_BASE_SRC_FLAG_LAST << 2)
} GstHeliosTvSrcFlags;

/* Definition of structure storing data for this element. */
struct _HeliosTvSource
{
  GstPushSrc element;

  GstPad *sinkpad;

  char *uri;

  /* server information */
  int port_control;
  int port_stream;
  char *host;

  HeliosTv::Stream *stream;
};

/* Standard definition defining a class for this element. */
struct _HeliosTvSourceClass
{
  GstPushSrcClass parent_class;
};

GType gst_heliostvsrc_get_type (void);

G_END_DECLS

#endif /* __HeliosTvSource__ */
