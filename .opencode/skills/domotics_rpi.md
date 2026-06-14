# Domotics RPi — MQTT → GPIO/WS2812 Bridge

## Descripción
Sistema para Raspberry Pi que recibe comandos MQTT desde la app Flutter **domotics_app** y controla LEDs por GPIO digital (libgpiod) y tiras RGB WS2812 (rpi_ws281x).

## Comandos Rápidos
```bash
cd domotics_rpi
make                    # Compilar C++
sudo ./domotics_rpi     # Ejecutar
# O alternativa Python:
python3 python/domotics_rpi.py
```

## Instalación de dependencias
```bash
bash scripts_tools/install_deps.sh
```

## Estructura
```
domotics_rpi/
├── include/              # Headers C++
│   ├── config.h          # Topics MQTT, pines GPIO, WS2812
│   ├── mqtt_handler.h
│   ├── gpio_controller.h
│   └── ws2812_controller.h
├── src/                  # Implementación C++
│   ├── main.cpp
│   ├── mqtt_handler.cpp
│   ├── gpio_controller.cpp
│   └── ws2812_controller.cpp
├── python/
│   └── domotics_rpi.py   # Alternativa Python
├── scripts_tools/
│   └── install_deps.sh   # Instalación interactiva de dependencias
├── systemd/
│   └── domotics-rpi.service
├── Makefile
└── README.md
```

## Protocolo MQTT
- Comandos (app → rpi): `domotics/{type}` → `{"command":"on"/"off", "deviceId":"<id>"}` o `{"deviceId":"<id>", "value":<num>}`
- Estado (rpi → app): `domotics/{type}/status` → `{"isOn":bool, "value":num}`
- RGB extra: `domotics/rgb` → `{"r":255,"g":0,"b":0}`
- Tipos: light, temperature, fan, lock, energy, curtain

## Mapeo GPIO por defecto
| Device ID | GPIO | Descripción |
|-----------|------|-------------|
| light_1   | 17   | LED Sala    |
| light_2   | 27   | LED Cocina  |
| light_3   | 22   | LED Exterior|
| lock_1    | 23   | Cerradura   |
| fan_1     | 24   | Ventilador  |

## Servicio systemd
```bash
sudo make install        # Instalar y arrancar
sudo make uninstall      # Detener y eliminar
```
