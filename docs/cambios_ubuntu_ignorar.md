# Cambios realizados en el firmware y el teleop (30/10/2026)

**Para:** integrantes del equipo que van a flashear la ESP32 o a probar los servos.

Este documento resume los cambios de la rama `refactor/auditoria-arquitectura`
(commit `0854844`) y el entorno de la máquina donde se probaron.

---

## 1. Resumen

- El firmware controla los 6 servos con PWM directo desde GPIO. Ya no usa PCA9685.
- Cada servo tiene un cero lógico en 1500 µs (posición física de reposo).
- Cada articulación tiene su escala, su sentido y sus límites.
- El movimiento está suavizado con velocidad y aceleración limitadas.
- El teleop envía grados lógicos. El firmware se encarga de convertirlos a µs.

---

## 2. Firmware (`firmware/esp32_servos/`)

### 2.1 Pines y servos

| Servo | GPIO | Escala | Sentido | Límites (°) |
|---|---|---|---|---|
| IZQ Hip Roll | 18 | 270° | - | -15 a 90 |
| IZQ Hip Pitch | 23 | 180° | - | -90 a 90 |
| IZQ Knee | 19 | 180° | + | -90 a 90 |
| DER Hip Roll | 26 | 270° | + | -15 a 90 |
| DER Hip Pitch | 27 | 180° | + | -90 a 90 |
| DER Knee | 25 | 180° | - | -90 a 90 |

- **Sentido:** "+" lleva el servo de 1500 µs a 2500 µs. "-" lleva de 1500 µs a 500 µs.
- **Rodillas:** el sentido está invertido respecto a la versión anterior.
- **Home:** 0° lógico (1500 µs) en los seis servos.
- **Escalas:** 2000 µs / 270° para cadera roll y 2000 µs / 180° para cadera pitch y rodilla.

Todo está en `include/servo_params.h`. La conversión a µs está en `include/helpers.h`
(`angleToUs`).

### 2.2 Movimiento

- Actualización cada 20 ms (50 Hz), igual que la señal PWM.
- Velocidad máxima: 60 °/s (`SERVO_MAX_SPEED`).
- Aceleración máxima: 120 °/s² (`SERVO_ACCEL`).
- El servo frena antes del objetivo y no se pasa.

Estos valores son estimaciones. No se han medido con el hardware.

Implementado en `src/main.cpp`, en `update_servos(dt)` y `loop()`.

### 2.3 Mensaje `/servo_commands`

Sigue siendo `Float32MultiArray` de 6 valores, en grados lógicos:

- `data[0..2]`: pierna izquierda (Hip Roll, Hip Pitch, Knee).
- `data[3..5]`: pierna derecha (Hip Roll, Hip Pitch, Knee).
- Un valor `NaN` significa "no cambiar ese servo".
- Un mensaje con menos de 6 valores se ignora.

### 2.4 Conexión en la PCB

Las señales S1 a S6 son los puntos de conexión PWM en la PCB. "Left" y "Right"
indican la pierna.

| Punto PCB | Servo | GPIO de la ESP32 |
|---|---|---|
| S1 | Hip Roll - Left | 18 |
| S2 | Knee Pitch - Left | 19 |
| S3 | Hip Pitch - Left | 23 |
| S4 | Knee Pitch - Right | 25 |
| S5 | Hip Roll - Right | 26 |
| S6 | Hip Pitch - Right | 27 |

Esta tabla coincide con `include/servo_params.h`.

### 2.5 Librerías

En `platformio.ini`:

- Eliminadas: `Adafruit PWM Servo Driver` y `Adafruit BusIO`.
- Añadida: `madhephaestus/ESP32Servo`.

---

## 3. Teleop (`ros2_ws/src/robot_teleop/robot_teleop/teleop_node.py`)

- `servo_offset_deg = [0, 0, 0]`. Antes eran `[180, 135, 135]`, porque el firmware
  anterior usaba grados absolutos.
- `lower_deg[0] = -15`. El slider de cadera roll baja hasta -15°.
- Los demás rangos no cambian: cadera pitch y rodilla -90 a 90.

---

## 4. Entorno de la máquina de pruebas (Ubuntu 22.04)

Estos detalles son específicos de la máquina y no forman parte del código.

- **PlatformIO:** usar `~/.platformio/penv/bin/pio`. El `pio` del sistema
  (`/usr/bin/pio`, versión 4.3.4) no reconoce la plataforma `espressif32`.
- **Puerto de la ESP32:** `/dev/ttyUSB0`. El usuario debe estar en el grupo `dialout`.
- **micro-ROS agent:** instalado en `~/microros_ws`. Comando:
  ```bash
  source /opt/ros/humble/setup.bash
  source ~/microros_ws/install/setup.bash
  ros2 run micro_ros_agent micro_ros_agent serial --dev /dev/ttyUSB0 -v6
  ```
- **Antes de flashear:** cerrar el agente. Mantiene abierto `/dev/ttyUSB0` y el upload falla.
- **Después de flashear:** reiniciar el agente. Si la ESP32 no aparece en
  `ros2 node list`, pulsar reset (EN) con el agente en marcha.
- **Git:** la máquina no tiene `user.name` ni `user.email` configurados.

---

## 5. Verificación

- `pio run` compila el firmware sin errores.
- `colcon build` compila el paquete `robot_teleop` sin errores.
- La simulación del perfil de velocidad no tiene sobrepaso en los casos probados.
- El firmware se subió a la ESP32 el 30/10/2026.
- Falta confirmar en el robot: que cada servo se mueve en el sentido esperado,
  y que `/servo_states` publica tras la reconexión del agente.

---

## 6. Pendiente

- **RViz y rodilla:** `control_node` sigue invirtiendo el índice 2 (rodilla) en
  `/joint_states` y `/robot/joint_commands`. El servo ya usa su propio sentido, así que
  RViz y el robot pueden moverse en sentidos opuestos. Hay que decidir si se quita
  la inversión de `control_node`.
- **Calibración:** las escalas, los límites y la velocidad son nominales.
- **Paro de emergencia:** no implementado. `/robot/e_stop` no lo publica nadie.
- **WiFi:** no implementado. Requiere `set_microros_wifi_transports` y cambiar
  `board_microros_transport` en `platformio.ini`.
- **Watchdog:** si dejan de llegar comandos, los servos mantienen la última posición.
