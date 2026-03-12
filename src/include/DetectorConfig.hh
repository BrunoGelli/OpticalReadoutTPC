#ifndef DETECTOR_CONFIG_HH
#define DETECTOR_CONFIG_HH

struct DetectorConfig {
    double worldRadiusCm;
    double worldHeightCm;
    double lappdSizeCm;
    double driftDistanceCm;
    double guidePitchCm;
    double wallThicknessMm;
    double wallReflectivity;
    double lappdPixelCm;
};

#endif // DETECTOR_CONFIG_HH
