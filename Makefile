# Nombre del ejecutable
TARGET = M_Invaders

# Fuentes del proyecto
SRC = Matcom_Invaders.c

# Compilador
CC = gcc

# Opciones de compilación
CFLAGS = -Wall -g -lncurses -pthread

# Directorio de instalación
INSTALL_DIR = /usr/local/bin

# Regla por defecto
all: $(TARGET)

# Regla para compilar el ejecutable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Regla para instalar el ejecutable
install: $(TARGET)
	cp $(TARGET) $(INSTALL_DIR)
	chmod +x $(INSTALL_DIR)/$(TARGET)

# Limpieza de archivos generados
clean:
	rm -f $(TARGET) *.o
