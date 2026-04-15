#include <stdint.h>
#include <string.h>

#include "wasm_export.h"

#define WAMR_RP2350_GLOBAL_HEAP_SIZE (96 * 1024)
#define WAMR_RP2350_STACK_SIZE (8 * 1024)
#define WAMR_RP2350_APP_HEAP_SIZE (8 * 1024)

static uint8_t g_wamr_heap[WAMR_RP2350_GLOBAL_HEAP_SIZE];
static uint8_t g_runtime_initialized;

static wasm_function_inst_t
wamr_rp2350_lookup_startup_func(wasm_module_t module,
                                wasm_module_inst_t module_inst)
{
    static const char *preferred_names[] = { "run", "_start", "main" };
    uint32_t i;

    for (i = 0; i < (uint32_t)(sizeof(preferred_names) / sizeof(preferred_names[0])); i++) {
        wasm_function_inst_t func =
            wasm_runtime_lookup_function(module_inst, preferred_names[i]);
        if (func) {
            return func;
        }
    }

    {
        int32_t export_count = wasm_runtime_get_export_count(module);
        if (export_count > 0) {
            for (i = 0; i < (uint32_t)export_count; i++) {
                wasm_export_t export_type;
                memset(&export_type, 0, sizeof(export_type));
                wasm_runtime_get_export_type(module, (int32_t)i, &export_type);
                if (export_type.kind == WASM_IMPORT_EXPORT_KIND_FUNC
                    && export_type.name) {
                    wasm_function_inst_t func = wasm_runtime_lookup_function(
                        module_inst, export_type.name);
                    if (func) {
                        return func;
                    }
                }
            }
        }
    }

    return NULL;
}

int32_t
wamr_rp2350_init(void)
{
    RuntimeInitArgs init_args;

    if (g_runtime_initialized) {
        return 1;
    }

    memset(&init_args, 0, sizeof(init_args));
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = g_wamr_heap;
    init_args.mem_alloc_option.pool.heap_size = sizeof(g_wamr_heap);
    init_args.running_mode = Mode_Interp;

    if (!wasm_runtime_full_init(&init_args)) {
        return 0;
    }

    g_runtime_initialized = 1;
    return 1;
}

int32_t
wamr_rp2350_run_startup(const uint8_t *wasm_ptr, uint32_t wasm_size)
{
    char error_buf[128];
    wasm_module_t module = NULL;
    wasm_module_inst_t module_inst = NULL;
    wasm_exec_env_t exec_env = NULL;
    wasm_function_inst_t func = NULL;
    uint32_t argv[1] = { 0 };
    int32_t return_value = -1;

    if (!g_runtime_initialized || !wasm_ptr || wasm_size == 0) {
        return -1;
    }

    module = wasm_runtime_load((uint8_t *)wasm_ptr, wasm_size, error_buf,
                               sizeof(error_buf));
    if (!module) {
        return -2;
    }

    module_inst = wasm_runtime_instantiate(module, WAMR_RP2350_STACK_SIZE,
                                           WAMR_RP2350_APP_HEAP_SIZE, error_buf,
                                           sizeof(error_buf));
    if (!module_inst) {
        return_value = -3;
        goto done;
    }

    exec_env = wasm_runtime_create_exec_env(module_inst, WAMR_RP2350_STACK_SIZE);
    if (!exec_env) {
        return_value = -4;
        goto done;
    }

    func = wamr_rp2350_lookup_startup_func(module, module_inst);
    if (!func) {
        int32_t export_count = wasm_runtime_get_export_count(module);
        if (export_count < 0) {
            return_value = -50;
        }
        else {
            return_value = -50 - export_count;
        }
        goto done;
    }

    if (!wasm_runtime_call_wasm(exec_env, func, 0, argv)) {
        return_value = -6;
        goto done;
    }

    return_value = (int32_t)argv[0];

done:
    if (exec_env) {
        wasm_runtime_destroy_exec_env(exec_env);
    }
    if (module_inst) {
        wasm_runtime_deinstantiate(module_inst);
    }
    if (module) {
        wasm_runtime_unload(module);
    }

    return return_value;
}
