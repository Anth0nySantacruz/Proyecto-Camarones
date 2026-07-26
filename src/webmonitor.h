#ifndef WEBMONITOR_H
#define WEBMONITOR_H

#include <Arduino.h>
#include <WebServer.h>

// ===================== VARIABLES EXTERNAS =====================
extern float temperatura;
extern float oxigeno;
extern bool bombaEstado;
extern bool peltierEstado;

extern WebServer server;

// ===================== PÁGINA MONITOR =====================
inline void handleMonitor()
{
    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="UTF-8">
        <title>ESP32 Monitor</title>

        <meta http-equiv="refresh" content="60">

        <style>
            body {
                font-family: Arial;
                text-align: center;
                background: #111;
                color: #00ffcc;
            }

            .card {
                background: #222;
                margin: 10px auto;
                padding: 15px;
                width: 260px;
                border-radius: 10px;
                box-shadow: 0 0 10px #00ffcc;
            }

            h1 { color: white; }
        </style>
    </head>

    <body>
        <h1>MONITOR ESP32</h1>

        <div class="card">
            <h3>Temperatura</h3>
            <p>)rawliteral";

    html += String(temperatura, 1);

    html += R"rawliteral( °C</p>
        </div>

        <div class="card">
            <h3>Oxígeno</h3>
            <p>)rawliteral";

    html += String(oxigeno, 1);

    html += R"rawliteral( %</p>
        </div>

        <div class="card">
            <h3>Bomba</h3>
            <p>)rawliteral";

    html += (bombaEstado ? "ON" : "OFF");

    html += R"rawliteral(</p>
        </div>

        <div class="card">
            <h3>Celda Peltier</h3>
            <p>)rawliteral";

    html += (peltierEstado ? "ON" : "OFF");

    html += R"rawliteral(</p>
        </div>

    </body>
    </html>
    )rawliteral";

    server.send(200, "text/html", html);
}

// ===================== REGISTRO DE RUTAS =====================
inline void iniciarWebMonitor()
{
    server.on("/monitor", handleMonitor);
    // IMPORTANTE: /wifi y / se manejan en apwifieeprommode.h
}

#endif