use std::path::Path;

fn build_wamr_bridge() {
    for pat in &[
        "src/wamr_bridge_stub.c",
        "src/wamr_platform.c",
        "src/wamr_platform_rp2350",
        "third_party/wasm-micro-runtime/core",
    ] {
        println!("cargo:rerun-if-changed={pat}");
    }

    let wamr = Path::new("third_party/wasm-micro-runtime");
    let core = wamr.join("core");
    let iwasm = core.join("iwasm");
    let shared = core.join("shared");
    let platform_inc = shared.join("platform").join("include");
    let mem_alloc = shared.join("mem-alloc");
    let utils_dir = shared.join("utils");
    let common_dir = iwasm.join("common");
    let interp_dir = iwasm.join("interpreter");
    let invoke_dir = common_dir.join("arch");
    let platform_rp = Path::new("src/wamr_platform_rp2350");

    let mut b = cc::Build::new();
    b.warnings(false);
    b.compiler("arm-none-eabi-gcc")
        .target("thumbv8m.main-none-eabihf")
        .flag("-mthumb")
        .flag("-mfloat-abi=hard")
        .flag("-Os")
        .flag("-std=gnu11")
        .flag("-ffunction-sections")
        .flag("-fdata-sections")
        .flag("-Wno-implicit-function-declaration")
        .flag("-Wno-implicit-int")
        .flag("-Wno-unused-parameter")
        .flag("-Wno-missing-field-initializers")
        .flag("-include")
        .flag("stdlib.h")
        .flag("-include")
        .flag("string.h")
        .define("BUILD_TARGET_THUMB_VFP", None)
        .define("WASM_ENABLE_INTERP", "1")
        .define("WASM_ENABLE_FAST_INTERP", "0")
        .define("WASM_ENABLE_AOT", "0")
        .define("WASM_ENABLE_JIT", "0")
        .define("WASM_ENABLE_FAST_JIT", "0")
        .define("WASM_ENABLE_LIBC_BUILTIN", "0")
        .define("WASM_ENABLE_LIBC_WASI", "0")
        .define("WASM_ENABLE_THREAD_MGR", "0")
        .define("WASM_ENABLE_LIB_PTHREAD", "0")
        .define("WASM_ENABLE_LIB_WASI_THREADS", "0")
        .define("WASM_ENABLE_BULK_MEMORY", "0")
        .define("WASM_ENABLE_BULK_MEMORY_OPT", "0")
        .define("WASM_ENABLE_SHARED_MEMORY", "0")
        .define("WASM_ENABLE_MULTI_MODULE", "0")
        .define("WASM_ENABLE_REF_TYPES", "0")
        .define("WASM_ENABLE_TAIL_CALL", "0")
        .define("WASM_ENABLE_SIMD", "0")
        .define("WASM_ENABLE_DEBUG_INTERP", "0")
        .define("WASM_ENABLE_GC", "0")
        .define("WASM_ENABLE_MINI_LOADER", "0")
        .define("WASM_DISABLE_HW_BOUND_CHECK", "1")
        .define("WASM_DISABLE_STACK_HW_BOUND_CHECK", "1")
        .define("WASM_DISABLE_WAKEUP_BLOCKING_OP", "1")
        .define("BH_MALLOC", "wasm_runtime_malloc")
        .define("BH_FREE", "wasm_runtime_free")
        .include(platform_rp)
        .include(&platform_inc)
        .include(&mem_alloc)
        .include(&utils_dir)
        .include(&common_dir)
        .include(&interp_dir)
        .include(iwasm.join("include"))
        .include(&core);

    b.file("src/wamr_platform.c");

    b.file(mem_alloc.join("ems/ems_alloc.c"))
        .file(mem_alloc.join("ems/ems_gc.c"))
        .file(mem_alloc.join("ems/ems_hmu.c"))
        .file(mem_alloc.join("ems/ems_kfc.c"))
        .file(mem_alloc.join("mem_alloc.c"));

    for name in &[
        "bh_assert.c",
        "bh_bitmap.c",
        "bh_common.c",
        "bh_hashmap.c",
        "bh_leb128.c",
        "bh_list.c",
        "bh_log.c",
        "bh_queue.c",
        "bh_vector.c",
        "runtime_timer.c",
    ] {
        b.file(utils_dir.join(name));
    }

    for name in &[
        "wasm_exec_env.c",
        "wasm_loader_common.c",
        "wasm_memory.c",
        "wasm_native.c",
        "wasm_runtime_common.c",
    ] {
        b.file(common_dir.join(name));
    }

    b.file(invoke_dir.join("invokeNative_thumb_vfp.s"));

    b.file(interp_dir.join("wasm_loader.c"))
        .file(interp_dir.join("wasm_runtime.c"))
        .file(interp_dir.join("wasm_interp_classic.c"));

    b.file("src/wamr_bridge_stub.c");

    b.compile("wamr_rp2350");
}

fn main() {
    build_wamr_bridge();
}
