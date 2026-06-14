#!/usr/bin/env bash
# =============================================================================
# install_deps.sh — Instalación interactiva de dependencias para Domotics RPi
# =============================================================================
# Uso: bash install_deps.sh
# =============================================================================

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

log_info()  { echo -e "${CYAN}[INFO]${NC}  $1"; }
log_ok()    { echo -e "${GREEN}[OK]${NC}    $1"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC}  $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

banner() {
    echo ""
    echo -e "${CYAN}╔══════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║${NC}  ${BOLD}Domotics RPi — Instalación de Dependencias${NC}        ${CYAN}║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════════╝${NC}"
    echo ""
}

check_root() {
    if [[ $EUID -ne 0 ]]; then
        log_warn "Algunas instalaciones requieren sudo. Se pedirá cuando sea necesario."
    fi
}

prompt_yes_no() {
    local prompt="$1"
    local default="${2:-y}"
    local answer
    while true; do
        read -r -p "$prompt (Y/n) " answer
        answer="${answer:-$default}"
        case "$answer" in
            [Yy]*) return 0 ;;
            [Nn]*) return 1 ;;
            *) echo "  Responde Y o n." ;;
        esac
    done
}

# ---------------------------------------------------------------------------
# Detectar sistema
# ---------------------------------------------------------------------------
detect_system() {
    log_info "Detectando sistema..."
    if [[ ! -f /etc/os-release ]]; then
        log_error "No se puede detectar el sistema operativo."
        exit 1
    fi
    source /etc/os-release
    echo "  OS:       $NAME $VERSION_ID"
    echo "  Arq:      $(uname -m)"
    echo "  Kernel:   $(uname -r)"

    if [[ "$ID" != "raspbian" && "$ID" != "debian" && "$ID" != "ubuntu" ]]; then
        log_warn "Sistema no probado. Se recomienda Raspberry Pi OS (Debian/Ubuntu)."
        if ! prompt_yes_no "¿Continuar de todas formas?"; then
            exit 1
        fi
    fi
}

# ---------------------------------------------------------------------------
# Paquetes base del sistema
# ---------------------------------------------------------------------------
install_base_packages() {
    echo ""
    log_info "Paso 1/4 — Paquetes base del sistema"
    echo "  Se instalarán: build-essential, cmake, git, libmosquitto-dev, libgpiod-dev"
    echo ""

    if prompt_yes_no "¿Instalar paquetes base?"; then
        sudo apt update
        sudo apt install -y \
            build-essential \
            cmake \
            git \
            libmosquitto-dev \
            libgpiod-dev
        log_ok "Paquetes base instalados."
    else
        log_warn "Omitido."
    fi
}

# ---------------------------------------------------------------------------
# rpi_ws281x (librería WS2812 para C++)
# ---------------------------------------------------------------------------
install_rpi_ws281x() {
    echo ""
    log_info "Paso 2/4 — Librería rpi_ws281x (WS2812 para C++)"
    echo "  Repo: https://github.com/jgarff/rpi_ws281x.git"
    echo "  Se clonará y compilará en /tmp/rpi_ws281x"
    echo ""

    if ! prompt_yes_no "¿Instalar rpi_ws281x?"; then
        log_warn "Omitido. El código C++ usará un stub (sin control real de WS2812)."
        return
    fi

    local tmpdir
    tmpdir=$(mktemp -d)
    echo "  Clonando en $tmpdir..."
    git clone --depth=1 https://github.com/jgarff/rpi_ws281x.git "$tmpdir"

    pushd "$tmpdir" > /dev/null
    mkdir -p build && cd build
    cmake .. -DBUILD_SHARED=OFF -DBUILD_TEST=OFF
    make -j"$(nproc)"
    sudo make install
    popd > /dev/null

    rm -rf "$tmpdir"
    log_ok "rpi_ws281x instalada."
}

# ---------------------------------------------------------------------------
# Python y paquetes pip
# ---------------------------------------------------------------------------
install_python_deps() {
    echo ""
    log_info "Paso 3/4 — Dependencias de Python (alternativa)"
    echo "  Se instalarán: python3-pip, paho-mqtt, rpi_ws281x (python), RPi.GPIO"
    echo ""

    if ! prompt_yes_no "¿Instalar dependencias de Python?"; then
        log_warn "Omitido."
        return
    fi

    sudo apt install -y python3-pip python3-dev

    pip3 install --upgrade pip
    pip3 install paho-mqtt

    # rpi_ws281x python y RPi.GPIO pueden fallar fuera de una RPi → no critical
    pip3 install rpi_ws281x RPi.GPIO 2>/dev/null || \
        log_warn "rpi_ws281x/RPi.GPIO python no disponibles (esperado si no es una RPi real)."

    log_ok "Dependencias Python instaladas."
}

# ---------------------------------------------------------------------------
# Servicio systemd (opcional)
# ---------------------------------------------------------------------------
install_systemd_service() {
    echo ""
    log_info "Paso 4/4 — Servicio systemd (opcional)"
    echo "  Instala domotics-rpi como servicio para inicio automático."
    echo ""

    if ! prompt_yes_no "¿Configurar servicio systemd?"; then
        log_warn "Omitido."
        return
    fi

    local service_file="../systemd/domotics-rpi.service"
    if [[ ! -f "$service_file" ]]; then
        log_error "No se encontró $service_file. Asegúrate de ejecutar este script desde scripts_tools/."
        return
    fi

    sudo cp "$service_file" /etc/systemd/system/
    sudo systemctl daemon-reload
    sudo systemctl enable domotics-rpi.service
    log_ok "Servicio habilitado (inicia con el sistema)."
    echo ""
    log_info "Para arrancarlo ahora: sudo systemctl start domotics-rpi"
    log_info "Para ver su estado:   sudo systemctl status domotics-rpi"
}

# ---------------------------------------------------------------------------
# Post-instalación: compilar C++
# ---------------------------------------------------------------------------
build_cpp() {
    echo ""
    log_info "Post-instalación — Compilar el proyecto C++"
    echo ""

    if ! prompt_yes_no "¿Compilar el proyecto C++ ahora?"; then
        log_warn "Omitido. Puedes compilar luego con: make"
        return
    fi

    if [[ -f ../Makefile ]]; then
        pushd .. > /dev/null
        make clean 2>/dev/null || true
        make
        popd > /dev/null
        log_ok "Compilación completa."
    else
        log_error "Makefile no encontrado."
    fi
}

# ---------------------------------------------------------------------------
# Resumen final
# ---------------------------------------------------------------------------
summary() {
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║${NC}  ${BOLD}Instalación completada${NC}                          ${GREEN}║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "  ${BOLD}Ejecutar C++:${NC}    sudo ./domotics_rpi"
    echo -e "  ${BOLD}Ejecutar Python:${NC}  python3 python/domotics_rpi.py"
    echo -e "  ${BOLD}Ver ayuda:${NC}        cat README.md"
    echo ""
}

# =============================================================================
# Main
# =============================================================================
main() {
    banner
    check_root
    detect_system

    echo ""
    echo -e "${YELLOW}Este script instalará las dependencias necesarias para ejecutar${NC}"
    echo -e "${YELLOW}Domotics RPi en una Raspberry Pi.${NC}"
    echo ""

    install_base_packages
    install_rpi_ws281x
    install_python_deps
    install_systemd_service
    build_cpp
    summary
}

main "$@"
