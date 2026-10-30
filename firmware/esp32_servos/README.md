# Firmware ESP32 - Control de 6 servos via micro-ROS

Controla los 6 servos del bípedo (3 por pierna) desde una **ESP32 NodeMCU-32S**
usando **micro-ROS** con transporte **serial (USB)**. El PWM sale directamente
de los GPIO de la ESP32, sin módulo PCA9685.

Para el estado de la última carga, las pruebas y el entorno de la máquina de
pruebas, ver `docs/cambios_ubuntu_ignorar.md`.

## Arquitectura

```
+------------------+   /servo_commands    +--------------------------------+
|   ROS2 (host)    | -------------------> |  micro-ROS Agent (USB -> host)  |
|  teleop / ik     |  Float32MultiArray   +--------------------------------+
+------------------+                                        |
                                                            | serial
                                                            v
+-------------------------------------------------------------+
|                      ESP32 (firmware)                       |
|  - Suscribe a /servo_commands (6 valores, grados lógicos)   |
|  - PWM 50 Hz directo a GPIO 18, 19, 23, 25, 26, 27          |
|  - Movimiento suave con velocidad y aceleración limitadas   |
|  - Publica estado en /servo_states                          |
+-------------------------------------------------------------+
```

## Contenido

| Archivo | Descripcion |
|---|---|
| `platformio.ini` | Configuracion de PlatformIO (board, micro-ROS, ESP32Servo) |
| `include/servo_params.h` | Pin, reposo, límites, escala y sentido de cada servo; orden del mensaje; constantes de movimiento |
| `include/helpers.h` | Conversión de grados lógicos a microsegundos |
| `include/pins.h` | LED indicador |
| `src/main.cpp` | Suscripción, perfil de movimiento y publicación de estado |

## Convención de ángulos

- El **cero lógico** de cada articulación es **1500 µs**, su posición física de reposo.
- Todos los ángulos del mensaje son **grados lógicos** respecto a ese cero.
- Cada servo tiene una **escala** y un **sentido** (`dir`):
  - Cadera roll: servo de 270°, 2000 µs / 270°.
  - Cadera pitch y rodilla: servo de 180°, 2000 µs / 180°.
  - `dir = +1`: el ángulo positivo lleva el pulso por encima de 1500 µs.
  - `dir = -1`: el ángulo positivo lleva el pulso por debajo de 1500 µs.
- Rango de pulsos: 500 a 2500 µs. Frecuencia: 50 Hz.

## Conexión de los servos

| Servo | Articulacion | GPIO | Posicion en el mensaje | Escala | Sentido |
|---|---|---|---|---|---|
| S1 | IZQ Hip Roll (cad roll) | 18 | data[0] | 270° | - |
| S3 | IZQ Hip Pitch (cad pitch) | 23 | data[1] | 180° | - |
| S2 | IZQ Knee (rod pitch) | 19 | data[2] | 180° | + |
| S5 | DER Hip Roll (cad roll) | 26 | data[3] | 270° | + |
| S6 | DER Hip Pitch (cad pitch) | 27 | data[4] | 180° | + |
| S4 | DER Knee (rod pitch) | 25 | data[5] | 180° | - |

Las señales van directo al GPIO de cada servo. La alimentación de los servos
es **externa de 5 V** (no desde el pin 5V de la ESP32, porque seis servos a la
vez consumen más de lo que da la placa). El GND de la fuente debe unirse al GND
de la ESP32.

> Verifica con el multímetro o el osciloscopio que cada GPIO llega a su servo
> correcto antes de conectar la alimentación.

## Límites de cada articulación (grados lógicos)

| Articulación | Mínimo | Máximo |
|---|---|---|
| Cadera roll (ambas piernas) | -15 | 90 |
| Cadera pitch (ambas piernas) | -90 | 90 |
| Rodilla (ambas piernas) | -90 | 90 |

Los valores fuera de rango se limitan al mínimo o máximo antes de enviarse al servo.

## Protocolo `/servo_commands`

- **Tipo:** `std_msgs/Float32MultiArray`, con 6 valores en grados lógicos.
- **Orden:** `[IZQ Hip Roll, IZQ Hip Pitch, IZQ Knee, DER Hip Roll, DER Hip Pitch, DER Knee]`.
- **NaN:** un valor `NaN` significa "no cambiar este servo". El firmware conserva
  su última posición objetivo. El teleop lo usa para la pierna que no controla.
- **Mensajes cortos:** un mensaje con menos de 6 valores se ignora.

Ejemplo (mueve solo la pierna izquierda, deja la derecha quieta):

```bash
ros2 topic pub --once /servo_commands std_msgs/msg/Float32MultiArray \
  "{data: [30.0, 0.0, -45.0, nan, nan, nan]}"
```

## Movimiento

- El firmware actualiza los servos cada 20 ms (50 Hz).
- Velocidad máxima: 60 °/s. Aceleración máxima: 120 °/s².
- Cada servo acelera, mantiene la velocidad y frena a tiempo para llegar al objetivo
  sin pasarse.
- Al arrancar, los seis servos van a su reposo (0° lógico = 1500 µs), de uno en uno.
- Si dejan de llegar comandos, los servos mantienen la última posición.

Estos valores son nominales y no se han medido en el robot.

## Pasos

### 1. Compilar y cargar

La compilación se hace en Ubuntu. Con la ESP32 conectada al puerto `/dev/ttyUSB0`:

```bash
cd firmware/esp32_servos
pio run -t upload --upload-port /dev/ttyUSB0
```

Antes de cargar, cierra el micro-ROS agent: mantiene abierto el puerto y la
carga falla. Ver `docs/cambios_ubuntu_ignorar.md`, sección 4.

### 2. Levantar el micro-ROS Agent (serial)

En una terminal aparte, desactiva cualquier monitor serial y lanza:

```bash
source /opt/ros/humble/setup.bash
source ~/microros_ws/install/setup.bash
ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -v6
```

Deberia verse algo como `Client connected` cuando la ESP32 se conecte.

### 3. Verificar nodo en ROS2

En otra terminal:

```bash
source /opt/ros/humble/setup.bash
ros2 node list            # deberia aparecer /esp32_servo_controller
ros2 topic list           # deberia estar /servo_commands y /servo_states
ros2 topic echo /servo_states
```

### 4. Mover los servos

```bash
# Lleva los 6 servos a su cero lógico (1500 µs)
ros2 topic pub --once /servo_commands std_msgs/msg/Float32MultiArray \
  "{data: [0.0, 0.0, 0.0, 0.0, 0.0, 0.0]}"
```

## Tipos de mensaje

El firmware usa `std_msgs/Float32MultiArray`, que ya viene incluido en la librería
micro-ROS estandar, por lo que no requiere generar mensajes custom.

## Notas

- Las escalas, los límites, el reposo y el perfil de velocidad son **nominales,
  pendientes de calibrar** en el robot. Ver `include/servo_params.h` y `src/main.cpp`.
- El firmware publica `/servo_states` cada 0.5 s para confirmar al host que la
  ESP32 esta viva y conectada.
- **IMPORTANTE:** no uses un `extra_micro_ros.meta` con opciones del host
  (`-DUHANDLER_AGENT_URL`, `-DMICROROS_TRANSPORT`), rompe el build del firmware y
  la ESP32 no bootea la app (se queda en modo descarga dando ruido `0x80`).
  El transporte se configura solo con `board_microros_transport = serial`.
- Despues de flashear, si la ESP32 queda en modo descarga (ruido constante en el
  puerto), haz un reset manual cerrando y abriendo el puerto (DTR/RTS) antes de
  lanzar el agente, para que bootee la app.
- El transporte serial usa el mismo puerto USB que la consola. Cierra el monitor
  serial (`pio device monitor`) antes de lanzar el agent serial.

## Integracion con los sliders del teleop

El `teleop_node` (lanzado por `bringup.launch.py`) envía en `/servo_commands` los
tres valores de su pierna, en grados lógicos, y `NaN` para la otra pierna. Su
rango de sliders coincide con los límites de la tabla de arriba.
