#![no_std]

pub struct WasmRunner {
    initialized: bool,
}

impl WasmRunner {
    pub const fn new() -> Self {
        Self { initialized: false }
    }

    pub fn init(&mut self) -> bool {
        self.initialized = wamr_init();
        self.initialized
    }

    pub fn run_startup(&self, wasm: &[u8]) -> i32 {
        if !self.initialized {
            return -1;
        }

        wamr_run_startup(wasm)
    }
}

unsafe extern "C" {
    fn wamr_rp2350_init() -> i32;
    fn wamr_rp2350_run_startup(wasm_ptr: *const u8, wasm_size: u32) -> i32;
}

fn wamr_init() -> bool {
    unsafe { wamr_rp2350_init() != 0 }
}

fn wamr_run_startup(wasm: &[u8]) -> i32 {
    let wasm_size = match u32::try_from(wasm.len()) {
        Ok(v) => v,
        Err(_) => return -2,
    };

    unsafe { wamr_rp2350_run_startup(wasm.as_ptr(), wasm_size) }
}
