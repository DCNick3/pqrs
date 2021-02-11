const webpack = require("webpack");
const path = require("path");

let rules = [
]

rules.push({
    test: /pqrs-emscripten-wrapper-wasm\.wasm$/,
    loader: "file-loader",
});

rules.push({
    test: /pqrs-emscripten-wrapper-pure\.js\.mem$/,
    loader: "file-loader",
})

module.exports = {
    mode: "development",
    context: path.resolve(__dirname, "."),
    entry: "./src/index.js",
    output: {
        path: path.resolve(__dirname, "dist"),
        filename: "pqrs-js.js",
        libraryTarget: "var",
        library: 'pqrs'
    },
    module: {
        rules
    },
    experiments: {
        topLevelAwait: true
    }
};