% Establecer la conexión Bluetooth
if exist('device')
    disp('connection already exist...');
    flush(device);
else
    disp('connecting...')
    device = bluetooth('HC-05', 1);
    disp('connected to HC-05');
    flush(device);
end

% Definir el tamaño del búfer de lectura
bufferSize = 1024;
prevchar = '';
currentchar = '';
data = '';
datasave = '';

% Bucle principal para leer datos
while true
    currentchar = read(device, 1,"char");
    if currentchar ~= '\' 
        data = [data, currentchar];
    else
        prevchar = currentchar;
        currentchar = read(device,1,"char");
        if currentchar == 'n'
            disp(data);
            datasave = data;
            data = '';
            
        end
    end
    
    % Pausa para evitar sobrecarga del procesador
    %pause(0.5);
end

% Cerrar la conexión Bluetooth
fclose(device);