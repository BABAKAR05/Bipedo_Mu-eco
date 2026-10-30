#ifndef HELPERS_H
#define HELPERS_H

#include "servo_params.h"

// ============================================================
//  clampValue() - Limita un valor entre min y max
// ============================================================
template <typename T>
T clampValue(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// ============================================================
//  angleToUs() - Convierte ángulo (grados) a microsegundos PWM
//
//  Ángulo lógico relativo al cero (1500 us). El sentido y la
//  escala salen de SERVO_PARAMS de cada servo.
// ============================================================
inline float angleToUs(uint8_t servoId, float angleDeg) {
    if (servoId >= NUM_SERVOS) return US_CENTER;  // Neutro por defecto

    const ServoParams& p = SERVO_PARAMS[servoId];

    // Clamp al rango seguro del servo
    float angle = clampValue(angleDeg, p.angleMin, p.angleMax);

    // us = 1500 + dir * grados * us_por_grado
    return US_CENTER + p.dir * angle * p.usPerDeg;
}

#endif
