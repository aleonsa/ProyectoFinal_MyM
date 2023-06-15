clear; clf;

disp('connecting...')
device = bluetooth('HC-05', 1);
disp('connected to HC-05');
flush(device);


% Definir el tamaño del búfer de lectura
bufferSize = 1024;
prevchar = '';
currentchar = '';
data = '';
datasave = '';
% Inicializar los arreglos de datos
distancia = [];
motor_pos = [];
inclinacion = [];

% Bucle principal para leer datos

while true
    currentchar = read(device, 1, "char");
    if currentchar ~= '\' 
        data = [data, currentchar];
    else
        prevchar = currentchar;
        currentchar = read(device, 1, "char");
        if currentchar == 'd'
            disp(data);
            distancia = [distancia, str2double(data)]; % Convertir los datos a número si es necesario
            data = '';
        end
        if currentchar == 'm'
            disp(data);
            motor_pos = [motor_pos, str2double(data)]; % Convertir los datos a número si es necesario
            data = '';
        end
        if currentchar == 'i'
            disp(data);
            inclinacion = [inclinacion, str2double(data)]; % Convertir los datos a número si es necesario
            data = '';
        end

    end
    
    % Subplot 1
    subplot(3,1,1);
    plot(distancia(:),'r'); drawnow;
    title('Distancia');

    % Subplot 2
    subplot(3,1,2);
    plot(motor_pos(:),'b'); drawnow;
    title('Posicion de Motor');

    % Subplot 3
    subplot(3,1,3);
    plot(inclinacion(:),'g'); drawnow;
    title('Angulo de inclinacion');    
    

    % Pausa para evitar sobrecarga del procesador
    %pause(0.001);
end

% Cerrar la conexión Bluetooth
fclose(device);
