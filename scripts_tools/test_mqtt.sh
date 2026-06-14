#!/usr/bin/env bash
# =============================================================================
# test_mqtt.sh — Script de prueba para verificar comunicación MQTT
# con Domotics RPi desde otra PC.
#
# Uso:
#   bash test_mqtt.sh                    # Prueba todo
#   bash test_mqtt.sh monitor            # Solo monitorear
#   bash test_mqtt.sh light              # Solo probar luces
#   bash test_mqtt.sh rgb               # Solo probar RGB
#   bash test_mqtt.sh all               # Prueba completa
# =============================================================================

set -euo pipefail

BROKER="${1:-all}"
HOST="raspberry.local"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

info()  { echo -e "${CYAN}[INFO]${NC}  $1"; }
ok()    { echo -e "${GREEN}[OK]${NC}    $1"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $1"; }
cmd()   { echo -e "  ${BOLD}>${NC} $1"; }

if ! command -v mosquitto_pub &>/dev/null || ! command -v mosquitto_sub &>/dev/null; then
    echo -e "${RED}ERROR: mosquitto_pub/sub no instalados.${NC}"
    echo "  Instalar: sudo apt install mosquitto-clients"
    exit 1
fi

banner() {
    echo ""
    echo -e "${CYAN}╔══════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${NC}  ${BOLD}Domotics RPi — Test MQTT${NC}                        ${CYAN}║${NC}"
    echo -e "${CYAN}║${NC}  Broker: ${HOST}:1883${NC}                                   ${CYAN}║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════════╝${NC}"
    echo ""
}

test_light() {
    echo -e "\n${BOLD}--- Luces (domotics/light) ---${NC}\n"

    info "Encendiendo light_1..."
    cmd "mosquitto_pub -h $HOST -t domotics/light -m '{\"command\":\"on\",\"deviceId\":\"light_1\"}'"
    mosquitto_pub -h "$HOST" -t "domotics/light" -m '{"command":"on","deviceId":"light_1"}'
    sleep 1

    info "Ajustando brillo light_1 a 50%..."
    cmd "mosquitto_pub -h $HOST -t domotics/light -m '{\"deviceId\":\"light_1\",\"value\":50}'"
    mosquitto_pub -h "$HOST" -t "domotics/light" -m '{"deviceId":"light_1","value":50}'
    sleep 1

    info "Apagando light_1..."
    cmd "mosquitto_pub -h $HOST -t domotics/light -m '{\"command\":\"off\",\"deviceId\":\"light_1\"}'"
    mosquitto_pub -h "$HOST" -t "domotics/light" -m '{"command":"off","deviceId":"light_1"}'
    sleep 1

    info "Encendiendo light_2..."
    cmd "mosquitto_pub -h $HOST -t domotics/light -m '{\"command\":\"on\",\"deviceId\":\"light_2\"}'"
    mosquitto_pub -h "$HOST" -t "domotics/light" -m '{"command":"on","deviceId":"light_2"}'
    sleep 1

    ok "Luces listo."
}

test_rgb() {
    echo -e "\n${BOLD}--- Tira RGB (domotics/rgb) ---${NC}\n"

    info "Rojo..."
    cmd "mosquitto_pub -h $HOST -t domotics/rgb -m '{\"r\":255,\"g\":0,\"b\":0}'"
    mosquitto_pub -h "$HOST" -t "domotics/rgb" -m '{"r":255,"g":0,"b":0}'
    sleep 1.5

    info "Verde..."
    cmd "mosquitto_pub -h $HOST -t domotics/rgb -m '{\"r\":0,\"g\":255,\"b\":0}'"
    mosquitto_pub -h "$HOST" -t "domotics/rgb" -m '{"r":0,"g":255,"b":0}'
    sleep 1.5

    info "Azul..."
    cmd "mosquitto_pub -h $HOST -t domotics/rgb -m '{\"r\":0,\"g\":0,\"b\":255}'"
    mosquitto_pub -h "$HOST" -t "domotics/rgb" -m '{"r":0,"g":0,"b":255}'
    sleep 1.5

    info "Blanco..."
    cmd "mosquitto_pub -h $HOST -t domotics/rgb -m '{\"r\":255,\"g\":255,\"b\":255}'"
    mosquitto_pub -h "$HOST" -t "domotics/rgb" -m '{"r":255,"g":255,"b":255}'
    sleep 1.5

    info "Apagando RGB..."
    cmd "mosquitto_pub -h $HOST -t domotics/rgb -m '{\"r\":0,\"g\":0,\"b\":0}'"
    mosquitto_pub -h "$HOST" -t "domotics/rgb" -m '{"r":0,"g":0,"b":0}'

    ok "RGB listo."
}

test_other_devices() {
    echo -e "\n${BOLD}--- Otros dispositivos ---${NC}\n"

    info "Ventilador -> ON..."
    mosquitto_pub -h "$HOST" -t "domotics/fan" -m '{"command":"on","deviceId":"fan_1"}'
    sleep 0.5

    info "Ventilador -> velocidad 3..."
    mosquitto_pub -h "$HOST" -t "domotics/fan" -m '{"deviceId":"fan_1","value":3}'
    sleep 0.5

    info "Cerradura -> ON..."
    mosquitto_pub -h "$HOST" -t "domotics/lock" -m '{"command":"on","deviceId":"lock_1"}'
    sleep 0.5

    info "Cortina -> 80%..."
    mosquitto_pub -h "$HOST" -t "domotics/curtain" -m '{"deviceId":"curtain_1","value":80}'
    sleep 0.5

    info "Temperatura -> 24.5°C..."
    mosquitto_pub -h "$HOST" -t "domotics/temperature" -m '{"deviceId":"temp_1","value":24.5}'
    sleep 0.5

    info "Energía -> 450W..."
    mosquitto_pub -h "$HOST" -t "domotics/energy" -m '{"deviceId":"energy_1","value":450}'

    ok "Otros dispositivos listo."
}

monitor() {
    echo -e "\n${BOLD}--- Monitoreando tráfico MQTT (domotics/#) ---${NC}"
    echo "  Presiona Ctrl+C para salir"
    echo ""
    mosquitto_sub -h "$HOST" -t "domotics/#" -v
}

full_test() {
    banner
    test_light
    test_rgb
    test_other_devices

    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║${NC}  ${BOLD}Prueba completada${NC}                                  ${GREEN}║${NC}"
    echo -e "${GREEN}║${NC}  Revisá los logs en la RPi:${NC}                               ${GREEN}║${NC}"
    echo -e "${GREEN}║${NC}    tail -f logs/domotics_rpi.log${NC}                          ${GREEN}║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════╝${NC}"
    echo ""
}

case "${BROKER}" in
    monitor)
        monitor
        ;;
    light)
        banner
        test_light
        ;;
    rgb)
        banner
        test_rgb
        ;;
    other)
        banner
        test_other_devices
        ;;
    all|full)
        full_test
        ;;
    *)
        echo "Uso: bash test_mqtt.sh [monitor|light|rgb|other|all]"
        echo ""
        echo "  monitor   - Solo monitorear tráfico MQTT"
        echo "  light     - Solo probar luces"
        echo "  rgb       - Solo probar tira RGB"
        echo "  other     - Solo probar otros dispositivos"
        echo "  all       - Prueba completa (default)"
        ;;
esac
