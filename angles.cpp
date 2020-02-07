/*#-------------------------------------------------
#
#                Angles utilities
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.1 - 2020/02/06
#
#-------------------------------------------------*/

#include <cmath>

#include "angles.h"

long double Angle::RadToDeg(const long double &rad) // convert radians value to degrees
{
    return ((180.0L / Pi) * rad); // 2Pi = 360°
}

long double Angle::DegToRad(const long double &deg) // convert degrees value to radians
{
    return (deg * (Pi / 180.0L)); // 360° = 2Pi
}

long double Angle::NormalizedToRad(const long double &normalized) // convert normalized radians to 2Pi rad
{
    return normalized * 2.0L * Pi; // 2Pi
}

long double Angle::NormalizedToDeg(const long double &normalized) // convert normalized degrees to 360°
{
    return normalized * 360.0L; // 360°
}

long double Angle::RadToNormalized(const long double &rad) // convert radians to [0..1]
{
    return rad / 2.0L / Pi; // 2Pi
}

long double Angle::DegToNormalized(const long double &deg) // convert degrees to [0..1]
{
    return deg / 360.0L; // 360°
}

long double Angle::DifferenceRad(const long double &a1, const long double &a2) // distance between 2 angles in radians
{
    long double angle = abs(a1 - a2); // difference
    if (angle > Pi) // 2Pi-0 rad limit (circle)
        angle = Pi - angle;

    return angle;
}

long double Angle::DifferenceDeg(const long double &a1, const long double &a2) // distance between 2 angles in degrees
{
    long double angle = abs(a1 - a2); // difference
    if (angle > 180) // 360-0° degrees limit (circle)
        angle = 360.0L - angle;

    return angle;
}
