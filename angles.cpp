/*#-------------------------------------------------
#
#                Angles utilities
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.0 - 2020/01/11
#
#-------------------------------------------------*/

#include <cmath>

#include "angles.h"

long double Angle::DegToRad(const long double deg) // convert degrees value to radians
{
    return (deg * (Pi / 180.0)); // 360° = 2Pi
}

long double Angle::RadToDeg(const long double rad) // convert radians value to degrees
{
    return ((180.0 / Pi) * rad); // 2Pi = 360°
}

long double Angle::NormalizedToRad(const long double rad) // convert normalized radians to 2Pi rad
{
    return rad * 2.0 * Pi; // 2Pi = 360°
}

long double Angle::NormalizedToDeg(const long double rad) // convert normalized degrees to 360°
{
    return rad * 360.0; // 2Pi = 360°
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
        angle = 360.0 - angle;

    return angle;
}
