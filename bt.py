import bluetooth

# Dirección MAC del HC-05
address = '00:21:13:01:E7:E2'  # Reemplaza con la dirección MAC real

# Canal del HC-05
channel = 1

# Tamaño máximo del búfer de recepción
buffer_size = 1024

# Conexión al HC-05
sock = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
sock.connect((address, channel))

print("Conexión establecida con éxito.")

while True:
    try:
        data = sock.recv(buffer_size)  # Leer datos recibidos desde el HC-05
        if data:
            data_str = data.decode()
            print("Datos recibidos:", data_str)
    except KeyboardInterrupt:
        break

# Cerrar la conexión
sock.close()
print("Conexión cerrada.")