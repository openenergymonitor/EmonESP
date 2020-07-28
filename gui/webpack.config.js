/* jslint node: true, esversion: 6 */
/* eslint-env node */

const HtmlWebpackPlugin = require("html-webpack-plugin");
const MiniCssExtractPlugin = require("mini-css-extract-plugin");
const UglifyJsPlugin = require("uglifyjs-webpack-plugin");
const CleanWebpackPlugin = require("clean-webpack-plugin");
const OptimizeCssAssetsPlugin = require("optimize-css-assets-webpack-plugin");
const ExtractTextPlugin = require("extract-text-webpack-plugin");
const MergeIntoSingleFilePlugin = require("webpack-merge-and-include-globally");
const webpack = require("webpack"); //to access built-in plugins
const path = require("path");
const UglifyJS = require("uglify-js");
const babel = require("@babel/core");
const CopyPlugin = require("copy-webpack-plugin");

require("dotenv").config();
const emonespEndpoint = process.env.EMONESP_ENDPOINT || "http://emonesp.local";
const devHost = process.env.DEV_HOST || "localhost";

var htmlMinify = {
  removeComments: true,
  collapseWhitespace: true,
  conservativeCollapse: true
};

module.exports = {
  mode: "production",
  entry: {
    assets: "./src/assets.js"
  },
  output: {
    path: path.resolve(__dirname, "dist"),
    filename: "[name].js"
  },
  devServer: {
    host: devHost,
    contentBase: "./dist",
    index: "home.html",
    proxy: [{
      context: [
        "/config",
        "/status",
        "/update",
        "/scan",
        "/emoncms",
        "/savenetwork",
        "/saveemoncms",
        "/savemqtt",
        "/saveadmin",
        "/saveohmkey",
        "/settime",
        "/reset",
        "/restart",
        "/apoff"
      ],
      target: emonespEndpoint
    },
    {
      context: [ "/ws" ],
      target: emonespEndpoint,
      ws: true
    }]
  },
  module: {
    rules: [
      {
        test: /\.css$/,
        use: [
          MiniCssExtractPlugin.loader,
          "css-loader"
        ]
      }
    ]
  },
  plugins: [
    //new CleanWebpackPlugin(['dist']),
    new HtmlWebpackPlugin({
      filename: "home.html",
      template: "./src/home.html",
      inject: false,
      minify: htmlMinify
    }),
    new MiniCssExtractPlugin({
      // Options similar to the same options in webpackOptions.output
      // both options are optional
      filename: "style.css",
      chunkFilename: "[id].css"
    }),
    new MergeIntoSingleFilePlugin({
      files: {
        "lib.js": [
          "node_modules/jquery/dist/jquery.js",
          "node_modules/knockout/build/output/knockout-latest.js",
          "node_modules/knockout-mapping/dist/knockout.mapping.js"
        ],
        "config.js": [
          "src/config.js"
        ]
      },
      transform: {
        "lib.js": code => uglify("lib.js", code),
        "config.js": code => uglify("config.js", code)
      }
    }) 
  ],
  optimization: {
    splitChunks: {},
    minimizer: [
      new UglifyJsPlugin({}),
      new OptimizeCssAssetsPlugin({})
    ]
  }
};

function uglify(name, code)
{
  var compiled = babel.transformSync(code, {
    presets: ["@babel/preset-env"],
    sourceMaps: true
  });
  var ugly = UglifyJS.minify(compiled.code, {
    warnings: true,
    sourceMap: {
      content: compiled.map,
      url: name+".map"
    }
  });
  if(ugly.error) {
    console.log(ugly.error);
    return code;
  }
  return ugly.code;
}
