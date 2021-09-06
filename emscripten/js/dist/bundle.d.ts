declare module "pqrs-js"
{
	export type Vector = [number, number];
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
	export const wasm_basename: any;
	export default function (options?: Partial<PqrsOptions>): Promise<{
	    scan_qr(image_data: ImageData): Promise<ScannedQrs>;
	}>;

}