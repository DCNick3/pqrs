// @ts-ignore
import pqrs_pure from '/bin/pqrs-emscripten-wrapper-pure.js';

// it's okay to import this one directly
// @ts-ignore
import pqrs_wasm from '/bin/pqrs-emscripten-wrapper-wasm.js';
// @ts-ignore
import pqrs_wasm_wasm from '/bin/pqrs-emscripten-wrapper-wasm.wasm';

// Since webpack will change the name and potentially the path of the 
// `.wasm` file, we have to provide a `locateFile()` hook to redirect
// to the appropriate URL.
// More details: https://kripken.github.io/emscripten-site/docs/api_reference/module.html

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

async function get_module(module_base: any) {
    let module;
    // should this log be present in prod?
    console.log("started loading pqrs native code; have_wasm = " + have_wasm);
    if (!have_wasm) {
        // @ts-ignore
        //module = await import('/bin/pqrs-emscripten-wrapper-pure.js');
        //module = module.default;
        module = pqrs_pure;
    } else {
        module = pqrs_wasm;
    }
    return await module(module_base);
}

export type Vector = [ number, number ];

export interface ScannedQr {
    content: string;
    bottom_right: Vector;
    bottom_left: Vector;
    top_left: Vector;
    top_right: Vector;
}

export interface ScannedQrs {
    qrs: ScannedQr[];
    finders: Vector[];
}

export interface PqrsOptions {
    locateWasmBinary: (scriptDir: string) => string;
}

export const wasm_basename = pqrs_wasm_wasm;
export default async function(options = <Partial<PqrsOptions>>{}) {
    const module_base = {
        locateWasmBinary(scriptDirectory: string) {
            return scriptDirectory + pqrs_wasm_wasm;
        },
        locateFile(path: string, scriptDirectory: string) {
            if (path.endsWith('.wasm')) {
                return module_base.locateWasmBinary(scriptDirectory)
            }
            return path;
        },
        onRuntimeInitialized() {
            console.log("onRuntimeInitialized");
        },
        onAbort(what: any) {
            console.log(what);
        }
    };
    
    Object.assign(module_base, options);

    const module = await get_module(module_base);

    return {
        // Right now it does not need to be async, but it would be required if unloading to webworker would be attempted
        async scan_qr(image_data: ImageData): Promise<ScannedQrs> {
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
}
    
