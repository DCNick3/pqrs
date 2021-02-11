
import pqrs_pure from '/bin/pqrs-emscripten-wrapper-pure.js';
import pqrs_pure_mem from '/bin/pqrs-emscripten-wrapper-pure.js.mem';

import pqrs_wasm from '/bin/pqrs-emscripten-wrapper-wasm.js';
import pqrs_wasm_wasm from '/bin/pqrs-emscripten-wrapper-wasm.wasm';

// Since webpack will change the name and potentially the path of the 
// `.wasm` file, we have to provide a `locateFile()` hook to redirect
// to the appropriate URL.
// More details: https://kripken.github.io/emscripten-site/docs/api_reference/module.html
const module_base = {
    locateFile(path) {
        if(path.endsWith('.js.mem')) {
            return pqrs_pure_mem;
        }
        if(path.endsWith('.wasm')) {
            return pqrs_wasm_wasm;
        }
        return path;
    },
    onRuntimeInitialized() {
        console.log("onRuntimeInitialized");
    }
};

const have_wasm = (() => {
    try {
        if (typeof WebAssembly === "object"
            && typeof WebAssembly.instantiate === "function") {
            const module = new WebAssembly.Module(Uint8Array.of(0x0, 0x61, 0x73, 0x6d, 0x01, 0x00, 0x00, 0x00));
            if (module instanceof WebAssembly.Module)
                return new WebAssembly.Instance(module) instanceof WebAssembly.Instance;
        }
    } catch (e) {
    }
    return false;
})();

const module_promise = (have_wasm ? pqrs_wasm : pqrs_pure)(module_base);

console.log("started loading pqrs native code; have_wasm = " + have_wasm);

export default {
    async scan_qr(image_data) {
        const module = await module_promise;
        let buf = module._malloc(image_data.data.byteLength);
        let res;
        try {
            module.HEAPU8.set(image_data.data, buf);
            res = module.scan_qr_codes(buf, image_data.width, image_data.height);
        }
        catch (e) {
            if (typeof e === "number")
                throw module.get_exception_message(e);
            throw e;
        }
        finally {
            module._free(buf);
        }
        return JSON.parse(res);
    }
};