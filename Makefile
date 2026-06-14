CC = g++
CFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lmosquitto -lgpiod -lpthread

SRCDIR = src
OBJDIR = obj
BINDIR = bin
TARGET = $(BINDIR)/domotics_rpi

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

.PHONY: all clean install uninstall

all: $(TARGET)

$(OBJDIR) $(BINDIR):
	mkdir -p $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "============================================"
	@echo " Build complete: $(TARGET)"
	@echo " Run with: sudo ./$(TARGET)"
	@echo "============================================"

clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Install as systemd service
install: $(TARGET)
	@echo "Installing systemd service..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo cp systemd/domotics-rpi.service /etc/systemd/system/
	sudo systemctl daemon-reload
	sudo systemctl enable domotics-rpi.service
	sudo systemctl start domotics-rpi.service
	@echo "Service installed and started."

uninstall:
	@echo "Stopping and removing service..."
	-sudo systemctl stop domotics-rpi.service
	-sudo systemctl disable domotics-rpi.service
	-sudo rm -f /etc/systemd/system/domotics-rpi.service
	sudo systemctl daemon-reload
	-sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Service removed."
