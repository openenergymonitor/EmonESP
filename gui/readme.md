# OpenEVSE WiFi GUI

This is the Web UI for OpenEVSE WiFi module. It is intended to be served via the [ESP8266](https://github.com/OpenEVSE/ESP8266_WiFi_v2.x), [ESP32](https://github.com/OpenEVSE/ESP32_WiFi_v3.x) or the [Node.JS](https://github.com/openevse/openevse_wifi_server)

## Building the UI

```shell
npm install
npm run build
```

## Dev server

To allow for easier development of the GUI there is a development server built in to Webpack. This can be configured to pass on calls to the backend to
a real device or the simulator.

### Config

You can configure the dev server using [dotenv](https://www.npmjs.com/package/dotenv). An example `.env` file can be found [here](.env.example).

`OPENEVSE_ENDPOINT` - URL of the OpenEVSE to test against.

`DEV_HOST` - By default the dev server is only available to localhost. If you want to expose the server to the local network then set this to the IP address or hostname of the external network interface.

### Starting

To start the dev server use the command:

`npm start`
