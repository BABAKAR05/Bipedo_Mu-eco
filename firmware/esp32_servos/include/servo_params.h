#ifndef SERVO_PARAMS_H
#define SERVO_PARAMS_H

#include <cstdint>

// ============================================================
//  Parámetros de los servos
//
//  Convención: el cero lógico de cada articulación es 1500 us
//  (posición física de reposo de las piernas). Los ángulos del
//  firmware son lógicos, relativos a ese cero.
//
//  Cadera roll: servos RDS51150 de 270 grados -> 2000 us / 270
//  Cadera pitch y rodilla: servos de 180 grados -> 2000 us / 180
//  Rango de pulsos: 500-2500 us.
//  Frecuencia: 50 Hz.
//  VALORES NOMINALES - Pendientes de calibrar.
// ============================================================

#define PWM_FREQ_HZ      50
#define US_CENTER        1500.0f      // us en el cero lógico
#define US_MIN           500          // ancho de pulso mínimo [us]
#define US_MAX           2500         // ancho de pulso máximo [us]

// Movimiento suave (valores nominales, pendientes de calibrar)
#define SERVO_UPDATE_MS  20           // periodo de actualización: 50 Hz, igual que el PWM
#define SERVO_MAX_SPEED  60.0f        // velocidad máxima [grados/s]
#define SERVO_ACCEL      120.0f       // aceleración máxima [grados/s^2]

#define US_PER_DEG_270   (2000.0f / 270.0f)   // us por grado, servo de 270
#define US_PER_DEG_180   (2000.0f / 180.0f)   // us por grado, servo de 180

// ============================================================
//  Estructura de parámetros por servo
// ============================================================
struct ServoParams {
    uint8_t pin;        // GPIO de la ESP32 que genera el PWM
    float angleHome;    // Ángulo de reposo lógico (0 = 1500 us)
    float angleMin;     // Límite inferior lógico (grados)
    float angleMax;     // Límite superior lógico (grados)
    float usPerDeg;     // Escala del servo (us por grado)
    int8_t dir;         // +1 o -1: sentido del ángulo positivo
    const char* name;   // Nombre descriptivo
};

#define NUM_SERVOS 6

// ============================================================
//  Orden del mensaje /servo_commands (6 valores, en grados lógicos):
//
//    data[0..2] -> pierna IZQUIERDA: Hip Roll, Hip Pitch, Knee
//    data[3..5] -> pierna DERECHA:   Hip Roll, Hip Pitch, Knee
//
//  Un valor NaN significa "no cambiar este servo": el firmware
//  mantiene su última posición objetivo.
//
//  Sentido del ángulo positivo:
//    Derecha: positivo lleva de 1500 us a 2500 us.
//    Izquierda: positivo lleva de 1500 us a 500 us.
//
//  Cableado (GPIO de la ESP32 directo a la señal de cada servo):
//    S1 -> GPIO 18   IZQ Hip Roll   (cad roll)
//    S2 -> GPIO 19   IZQ Knee       (rod pitch)
//    S3 -> GPIO 23   IZQ Hip Pitch  (cad pitch)
//    S4 -> GPIO 25   DER Knee       (rod pitch)
//    S5 -> GPIO 26   DER Hip Roll   (cad roll)
//    S6 -> GPIO 27   DER Hip Pitch  (cad pitch)
//
//  Rodillas: sentido invertido respecto a la cadera pitch.
//    Derecha: positivo lleva de 1500 us a 500 us.
//    Izquierda: positivo lleva de 1500 us a 2500 us.
//
//  Límites lógicos:
//    Cadera roll (ambas piernas): -15 a 90 grados.
//    Cadera pitch y rodilla (ambas piernas): -90 a 90 grados.
// ============================================================

static const ServoParams SERVO_PARAMS[NUM_SERVOS] = {
    // pin, angleHome, angleMin, angleMax, usPerDeg, dir, name
    { 18, 0.0f,  -15.0f,  90.0f, US_PER_DEG_270, -1, "IZQ Hip Roll"  },
    { 23, 0.0f,  -90.0f,  90.0f, US_PER_DEG_180, -1, "IZQ Hip Pitch" },
    { 19, 0.0f,  -90.0f,  90.0f, US_PER_DEG_180, +1, "IZQ Knee"      },
    { 26, 0.0f,  -15.0f,  90.0f, US_PER_DEG_270, +1, "DER Hip Roll"  },
    { 27, 0.0f,  -90.0f,  90.0f, US_PER_DEG_180, +1, "DER Hip Pitch" },
    { 25, 0.0f,  -90.0f,  90.0f, US_PER_DEG_180, -1, "DER Knee"      },
};

#endif
