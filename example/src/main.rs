#![no_std]
#![no_main]

use cortex_m_rt::entry;
use wamr_rp2350::WasmRunner;
use defmt_rtt as _;
use panic_probe as _;

// Minimal wasm module exporting `run() -> i32` that returns 42.
const EXAMPLE_WASM: &[u8] = &[
    0x00, 0x61, 0x73, 0x6d, // wasm magic
    0x01, 0x00, 0x00, 0x00, // wasm version 1
    0x01, 0x05, 0x01, 0x60, 0x00, 0x01, 0x7f, // type section
    0x03, 0x02, 0x01, 0x00, // function section
    0x07, 0x07, 0x01, 0x03, 0x72, 0x75, 0x6e, 0x00, 0x00, // export section (`run`)
    0x0a, 0x06, 0x01, 0x04, 0x00, 0x41, 0x2a, 0x0b, // code section
];


#[entry]
fn main() -> ! {
    let _p = embassy_rp::init(Default::default());
    let mut runner = WasmRunner::new();
    let result = if runner.init() {
        runner.run_startup(EXAMPLE_WASM)
    }
    else {
        -1
    };

    defmt::info!("WASM result: {}", result);

    loop {
        core::hint::spin_loop();
    }
}
