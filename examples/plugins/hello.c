#include <cuddle-plugin.h>

static int hello_init(void) {
  return 0;
}

static void hello_fini(void) {
}

static const struct cuddle_plugin_v1 hello = {
    .abi = CUDDLE_PLUGIN_ABI,
    .id = "hello",
    .title = "hello",
    .protocol_id = NULL,
    .protocol_flag = NULL,
    .init = hello_init,
    .fini = hello_fini,
};

const struct cuddle_plugin_v1 *cuddle_plugin_get_v1(void) {
  return &hello;
}
