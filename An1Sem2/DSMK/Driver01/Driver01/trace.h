#ifndef _TRACE_H_
#define _TRACE_H_
//
// Copyright (C) 2015 BitDefender S.R.L.
// Author(s) : Radu PORTASE(rportase@bitdefender.com)
//
//
// WPP Control GUID for MyDriver: bb6301c6-4e62-4bbf-b869-33b533287481
//
#define WPP_CONTROL_GUIDS \
WPP_DEFINE_CONTROL_GUID( \
MyDriverTraceGuid, (bb6301c6,4e62,4bbf,b869,33b533287481), \
WPP_DEFINE_BIT(TRACE_FLAG_DRIVER) /* bit 0 = 0x00000001 */ \
WPP_DEFINE_BIT(TRACE_FLAG_FILE_FILTER) /* bit 1 = 0x00000002 */ \
)
//
// Because we are using our custom logging function DrvLogMessage(Flags, Level, Message, ...) we need to define the flowing macros
//
#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl,flags) (WPP_LEVEL_ENABLED(flags) && \
WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// Here we define our logging functions
//
// begin_wpp config
//
// Functions for logging driver related events
//
// FUNC DrvLogTrace{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS=TRACE_FLAG_DRIVER}(MSG, ...);
// FUNC DrvLogInfo{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS=TRACE_FLAG_DRIVER}(MSG, ...);
// FUNC DrvLogWarning{LEVEL=TRACE_LEVEL_WARNING, FLAGS=TRACE_FLAG_DRIVER}(MSG, ...);
// FUNC DrvLogError{LEVEL=TRACE_LEVEL_ERROR, FLAGS=TRACE_FLAG_DRIVER}(MSG,...);
// FUNC DrvLogCritical{LEVEL=TRACE_LEVEL_CRITICAL, FLAGS=TRACE_FLAG_DRIVER}(MSG, ...);
//
// Functions for logging file filter related events
//
// FUNC FltfLogTrace{LEVEL=TRACE_LEVEL_VERBOSE, FLAGS = TRACE_FLAG_FILE_FILTER}(MSG, ...);
// FUNC FltfLogInfo{LEVEL=TRACE_LEVEL_INFORMATION, FLAGS = TRACE_FLAG_FILE_FILTER}(MSG, ...);
// FUNC FltfLogWarning{LEVEL=TRACE_LEVEL_WARNING, FLAGS = TRACE_FLAG_FILE_FILTER}(MSG, ...);
// FUNC FltfLogError{LEVEL=TRACE_LEVEL_ERROR, FLAGS=TRACE_FLAG_FILE_FILTER}(MSG, ...);
// FUNC FltfLogCritical{LEVEL=TRACE_LEVEL_CRITICAL, FLAGS = TRACE_FLAG_REGISTRY_FILTER}(MSG, ...);
//
//
// end_wpp
//

#endif//_TRACE_H_