# Domotics RPi Controller

Sistema para Raspberry Pi que recibe comandos MQTT desde la app Flutter **domotics_app** y controla LEDs por GPIO y tiras RGB WS2812.

## Arquitectura

```
App Flutter  ──MQTT──►  Mosquitto Broker  ──MQTT──►  domotics_rpi (C++ o Python)
                                                          │
                                                    ┌─────┴─────┐
                                                    │            │
                                               libgpiod    rpi_ws281x
                                               (GPIO)      (WS2812)
```

## Dependencias

### C++
```bash
sudo apt update
sudo apt install -y build-essential cmake git
sudo apt install -y libmosquitto-dev libgpiod-dev
```

Para WS2812 (opcional, si se quiere usar la librería real):
```bash
git clone https://github.com/jgarff/rpi_ws281x.git
cd rpi_ws281x
mkdir build && cd build
cmake .. -DBUILD_SHARED=OFF -DBUILD_TEST=OFF
make
sudo make install
```

### Python (alternativa)
```bash
sudo apt install -y python3-pip
pip3 install paho-mqtt rpi_ws281x RPi.GPIO
```

## Compilación y Ejecución (C++)

```bash
cd domotics_rpi
make
sudo ./domotics_rpi
```

Para ejecutar en background:
```bash
nohup sudo ./domotics_rpi > domotics.log 2>&1 &
```

### Como servicio systemd

```bash
sudo make install    # Instala y arranca el servicio
sudo make uninstall  # Detiene y elimina el servicio
```

O manualmente:
```bash
sudo cp systemd/domotics-rpi.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable domotics-rpi.service
sudo systemctl start domotics-rpi.service
sudo systemctl status domotics-rpi.service
```

## Ejecución (Python)

```bash
cd domotics_rpi/python
python3 domotics_rpi.py
```

## Protocolo MQTT

### Topics de comando (App -> RPi)

| Tipo | Topic | Payload |
|------|-------|---------|
| Luz | `domotics/light` | `{"command":"on"/"off", "deviceId":"light_1"}` |
| Temperatura | `domotics/temperature` | `{"deviceId":"temp_1", "value":22.5}` |
| Ventilador | `domotics/fan` | `{"command":"on", "deviceId":"fan_1"}` |
| Cerradura | `domotics/lock` | `{"command":"on", "deviceId":"lock_1"}` |
| Cortina | `domotics/curtain` | `{"command":"on", "deviceId":"curtain_1"}` |
| Energía | `domotics/energy` | `{"deviceId":"energy_1", "value":500}` |
| RGB (extra) | `domotics/rgb` | `{"r":255, "g":0, "b":0}` |

### Topics de estado (RPi -> App)

| Tipo | Topic | Payload |
|------|-------|---------|
| Luz | `domotics/light/status` | `{"isOn":true, "value":75}` |
| Temperatura | `domotics/temperature/status` | `{"isOn":true, "value":22.5}` |
| ... | `domotics/{type}/status` | `{"isOn":bool, "value":number}` |
| RGB | `domotics/rgb/status` | `{"isOn":true, "value":128}` |

### Mapeo de dispositivos

| Device ID | GPIO Pin | Descripción |
|-----------|----------|-------------|
| `light_1` | GPIO 17 | LED Sala |
| `light_2` | GPIO 27 | LED Cocina |
| `light_3` | GPIO 22 | LED Exterior |
| `lock_1`  | GPIO 23 | Cerradura |
| `fan_1`   | GPIO 24 | Ventilador |

Los dispositivos `light_1`, `light_2` y `light_3` también se mapean a los píxeles 0, 1 y 2 de la tira WS2812 respectivamente.

## Pruebas

### 1. Verificar que el broker MQTT funciona
```bash
mosquitto_sub -h test.mosquitto.org -t "domotics/#" -v
```

### 2. Probar encendido de un LED
```bash
mosquitto_pub -h test.mosquitto.org -t "domotics/light" \
  -m '{"command":"on", "deviceId":"light_1"}'
```

### 3. Probar ajuste de brillo
```bash
mosquitto_pub -h test.mosquitto.org -t "domotics/light" \
  -m '{"deviceId":"light_1", "value":50}'
```

### 4. Probar tira RGB
```bash
mosquitto_pub -h test.mosquitto.org -t "domotics/rgb" \
  -m '{"r":255, "g":0, "b":0}'
```

### 5. Usar con la app Flutter
1. Abrir domotics_app en el teléfono
2. Configurar el broker MQTT (por defecto `test.mosquitto.org:1883`)
3. Los comandos desde la app impactarán directamente en los GPIO/WS2812
4. El estado de los dispositivos se actualizará en tiempo real en la app

## Personalización

Editar `include/config.h` (C++) o las constantes al inicio de `python/domotics_rpi.py` para:
- Cambiar broker MQTT
- Reasignar pines GPIO
- Configurar cantidad de LEDs WS2812
- Ajustar rangos de valores por dispositivo
