#ifndef DETECTOR_CONFIG_HH
#define DETECTOR_CONFIG_HH

/// Centralized detector geometry/readout configuration.
///
/// Units convention:
/// - `*Cm` fields are in centimeters
/// - `*Mm` fields are in millimeters
/// - reflectivity is unitless [0,1]
struct DetectorConfig {
    double worldRadiusCm;    ///< Radius of cylindrical world volume.
    double worldHeightCm;    ///< Full height of cylindrical world volume.
    double lappdSizeCm;      ///< Active square side of each LAPPD window.
    double driftDistanceCm;  ///< Distance between top/bottom LAPPD planes.
    double guidePitchCm;     ///< Nominal light-guide cell pitch.
    double wallThicknessMm;  ///< Wall thickness for one hollow prism cell.
    double wallReflectivity; ///< Optical wall reflectivity.
    double lappdPixelCm;     ///< Analysis pixel size used for hit binning/output.
};

#endif // DETECTOR_CONFIG_HH
