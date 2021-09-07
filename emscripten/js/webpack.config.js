const webpack = require("webpack");
const path = require("path");

// proudly stolen from https://gist.github.com/JasonKleban/bc8446f3f33c7e3c8a8103a770db442e
class DeclarationBundlerPlugin
{
    out /* :string */;
    moduleName /* :string */;
    mode /* :string */;
    excludedReferences /* :string[] */;

    constructor(options /* :any */={})
    {
        this.out = options.out ? options.out : './build/';
        this.excludedReferences = options.excludedReferences ? options.excludedReferences : undefined;

        if(!options.moduleName)
        {
            throw new Error('please set a moduleName if you use mode:internal. new DacoreWebpackPlugin({mode:\'internal\',moduleName:...})');
        }
        this.moduleName = options.moduleName;
    }

    apply(compiler)
    {
        compiler.hooks.thisCompilation.tap('DeclarationBundlerPlugin', (compilation) => {
            compilation.hooks.optimizeAssets.tap('DeclarationBundlerPlugin', (assets) => {
                console.log(`${JSON.stringify(Object.keys(assets), void 0, '  ')}`);
            });

            compilation.hooks.processAssets.tapAsync({
                name: 'DeclarationBundlerPlugin',
                stage: compilation.PROCESS_ASSETS_STAGE_REPORT,
                assets: true
            }, (assets, callback) => {

                // ***
                console.log(`${JSON.stringify(Object.keys(assets), void 0, '  ')}`); // just .js files, no .d.ts, .d.ts.map files listed (though they are generated)
                // ***

                //collect all generated declaration files
                //and remove them from the assets that will be emited
                var declarationFiles = {};
                for (var filename in assets)
                {
                    if(filename.indexOf('.d.ts') !== -1)
                    {
                        declarationFiles[filename] = assets[filename];
                        delete assets[filename];
                    }
                }

                //combine them into one declaration file
                var combinedDeclaration = this.generateCombinedDeclaration(declarationFiles);

                //and insert that back into the assets
                assets[this.out] = {
                    source: function() {
                        return combinedDeclaration;
                    },
                    size: function() {
                        return combinedDeclaration.length;
                    }
                };

                callback();
            });
        });
    }

    generateCombinedDeclaration(declarationFiles /* :Object */) /* :string */
    {
        var declarations = '';
        for(var fileName in declarationFiles)
        {
            var declarationFile = declarationFiles[fileName];
            // The lines of the files now come as a Function inside declaration file.
            var data = declarationFile.source();
            var lines = data.split("\n");
            var i = lines.length;


            while (i--)
            {
                var line = lines[i];

                //exclude empty lines
                var excludeLine /*:boolean */ = line == "";

                //exclude export statements
                excludeLine = excludeLine || line.indexOf("export =") !== -1;

                //exclude import statements
                excludeLine = excludeLine || (/import ([a-z0-9A-Z_-]+) = require\(/).test(line);

                //if defined, check for excluded references
                if(!excludeLine && this.excludedReferences && line.indexOf("<reference") !== -1)
                {
                    excludeLine = this.excludedReferences.some(reference => line.indexOf(reference) !== -1);
                }


                if (excludeLine)
                {
                    lines.splice(i, 1);
                }
                else
                {
                    if (line.indexOf("declare ") !== -1)
                    {
                        lines[i] = line.replace("declare ", "");
                    }
                    //add tab
                    lines[i] = "\t" + lines[i];
                }
            }
            declarations += lines.join("\n") + "\n\n";
        }

        var output = "declare module "+JSON.stringify(this.moduleName)+"\n{\n" + declarations + "}";
        return output;
    }

}

module.exports = (env, argv) => {
    let rules = [
    ];

    rules.push({
        test: /pqrs-emscripten-wrapper-wasm\.wasm$/,
        type: "asset/resource",
        generator: {
          filename: 'pqrs.wasm'
        }
    });

    rules.push({
        test: /\.tsx?$/,
        use: 'ts-loader',
        exclude: /node_modules/,
    });

    const config = {
        context: path.resolve(__dirname, "."),
        entry: "./src/index.ts",
        output: {
            path: path.resolve(__dirname, "dist"),
            filename: "pqrs-js.js",
            library: {
                name: 'pqrs',
                type: 'umd',
                export: 'default'
            },
            globalObject: 'this',
            publicPath: ''
        },
        module: {
            rules
        },
        experiments: {
            topLevelAwait: true
        },
        plugins: [
            new DeclarationBundlerPlugin({
                moduleName:'pqrs-js',
                out:'bundle.d.ts',
            })
        ],
        externals: {
            'fs': 'commonjs2 fs',
            'path': 'commonjs2 path',
        },
        node: {
            __dirname: false,
            __filename: false,
        },
        performance: {
            hints: false,
            maxEntrypointSize: 512000,
            maxAssetSize: 512000
        }
    };
    
    if (argv.mode === 'production') {
        config.mode = 'production';
        config.devtool = 'source-map';
    }
    
    if (argv.mode === 'development') {
        config.mode = 'development';
    }
    
    return config;
};
