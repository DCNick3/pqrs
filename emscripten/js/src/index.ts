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

const ENVIRONMENT_IS_WEB = typeof window === "object";
const ENVIRONMENT_IS_WORKER = typeof importScripts === "function";
const ENVIRONMENT_IS_NODE = typeof process === "object" && typeof process.versions === "object" && typeof process.versions.node === "string";

const scriptDirectory = (() => {
    // @ts-ignore
	var _scriptDir = typeof document !== 'undefined' && document.currentScript ? document.currentScript.src : undefined;
	if (typeof __filename !== 'undefined') _scriptDir = _scriptDir || __filename;
    var scriptDirectory = "";

    if (ENVIRONMENT_IS_NODE) {
        if (ENVIRONMENT_IS_WORKER) {
            scriptDirectory = require("path").dirname(scriptDirectory) + "/"
        } else {
            scriptDirectory = __dirname + "/"
        }
    } else if (ENVIRONMENT_IS_WEB || ENVIRONMENT_IS_WORKER) {
        if (ENVIRONMENT_IS_WORKER) {
            scriptDirectory = self.location.href
        } else if (typeof document !== "undefined" && document.currentScript) {
            // @ts-ignore
            scriptDirectory = document.currentScript.src
        }
        if (_scriptDir) {
            scriptDirectory = _scriptDir
        }
        if (scriptDirectory.indexOf("blob:") !== 0) {
            scriptDirectory = scriptDirectory.substr(0, scriptDirectory.lastIndexOf("/") + 1)
        } else {
            scriptDirectory = ""
        }
    } else {}

    return scriptDirectory;
})();
const readAsync: (path: string) => Promise<string> = (() => {
    let fun: (filename: string, onload: (data: string) => void, onerror: (error: any) => void) => void;
    
    if (ENVIRONMENT_IS_NODE) {
        fun = function(filename, onload, onerror) {
            const nodeFS = require("fs");
            const nodePath = require("path");
            filename = nodePath["normalize"](filename);
            nodeFS["readFile"](filename, "utf8", function(err: any, data: any) {
                if (err) onerror(err);
                else onload(data)
            });
        };
    } else if (ENVIRONMENT_IS_WEB || ENVIRONMENT_IS_WORKER) {
        fun = function(url, onload, onerror) {
            var xhr = new XMLHttpRequest;
            xhr.open("GET", url, true);
            xhr.onload = function() {
                if (xhr.status == 200 || xhr.status == 0 && xhr.response) {
                    onload(xhr.response);
                    return
                }
                onerror("Bad status:" + xhr.status);
            };
            xhr.onerror = onerror;
            xhr.send(null)
        }
    } else {
        throw "Don't know how to load files";
    }
    return (filename: string) => new Promise<string>((res, rej) => fun(filename, res, rej));
})();

async function get_module(module_base: any) {
    let module;
    // should this log be present in prod?
    console.log("started loading pqrs native code; have_wasm = " + have_wasm);
    if (!have_wasm) {
        //module = await import('/bin/pqrs-emscripten-wrapper-pure.js');
        //module = module.default;
        const path = module_base.locateFile(pqrs_pure, scriptDirectory);
        console.log("Loading " + path);
        let contents = await readAsync(path);
        contents = `var __dirname=d;var module={};var exports={};${contents}; return module.exports;`;
        module = new Function('d', contents)(__dirname);
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
    locateFile: (path: string, scriptDirectory: string) => string;
}

export const wasm_basename = pqrs_wasm_wasm;
export default async function(options = <Partial<PqrsOptions>>{}) {
    const module_base = {
        locateFile(path: string, scriptDirectory: string) {
            if (path.endsWith('.wasm')) {
                return scriptDirectory + pqrs_wasm_wasm
            }
            if (path.endsWith('.asm.js')) {
                return scriptDirectory + pqrs_pure;
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
    
