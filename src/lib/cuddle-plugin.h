#ifndef CUDDLE_PLUGIN_H
#define CUDDLE_PLUGIN_H

#include <cuddle.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Plugin .so must export this. abi must be CUDDLE_PLUGIN_ABI.
 * Newer major is refused. Soft Signal cannot be disabled.
 */
const struct cuddle_plugin_v1 *cuddle_plugin_get_v1(void);

#ifdef __cplusplus
}
#endif

#endif
