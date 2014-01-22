
#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER apache

#undef TRACEPOINT_INCLUDE
#define TRACEPOINT_INCLUDE "./tracepoints.h"

#if !defined(TRACEPOINTS_H) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define TRACEPOINTS_H

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(
    apache,
    apache_process_entry,
    TP_ARGS(),
    TP_FIELDS()
 )

#endif /* TRACEPOINTS_H */

#include <lttng/tracepoint-event.h>
