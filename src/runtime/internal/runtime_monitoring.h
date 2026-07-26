#pragma once

#include <stdint.h>

#include "config/fs_config.h"
#include "fsc/fsid/fsid.h"
#include "fsc/namespace/namespace.h"

void runtime_monitoring_init(const FsMetricsConfig_t *config);
void runtime_monitoring_session_begin(fsc_fsid_t fsid, const char *name);
void runtime_monitoring_session_end(void);
void runtime_monitoring_deinit(void);
