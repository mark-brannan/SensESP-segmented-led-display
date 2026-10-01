#ifndef TEMPERATURE_H
#define TEMPERATURE_H

inline float convertDegreesKtoC(float degreesK) { return degreesK - 273.15f; }

inline float convertDegreesKtoF(float degreesK) {
  return convertDegreesKtoC(degreesK) * (9.0f / 5.0f) + 32.0f;
}

#endif /* TEMPERATURE_H */
