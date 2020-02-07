/*#-------------------------------------------------
#
#                Angles utilities
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.1 - 2020/02/06
#
#-------------------------------------------------*/

#ifndef ANGLES_H
#define ANGLES_H

class Angle { // angles formulas in radians and degrees
    public:
        static long double RadToDeg(const long double &rad); // convert radians value to degrees
        static long double DegToRad(const long double &deg); // convert degrees value to radians
        static long double NormalizedToRad(const long double &normalized); // convert normalized radians to 2Pi rad
        static long double NormalizedToDeg(const long double &normalized); // convert normalized degrees to 360°
        static long double RadToNormalized(const long double &rad); // convert radians to [0..1]
        static long double DegToNormalized(const long double &deg); // convert degrees to [0..1]
        static long double DifferenceRad(const long double &a1, const long double &a2); // distance between 2 angles in radians
        static long double DifferenceDeg(const long double &a1, const long double &a2); // distance between 2 angles in degrees
};

const long double Pi = 3.14159265358979323846264338328L;

#endif // ANGLES_H
