/*#-------------------------------------------------
#
#            Color spaces conversions
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.2 - 2020/02/06
#
#  Color spaces :
#    - RGB
#    - CIE XYZ
#    - CIE xyY
#    - CIE L*a*b* and CIE LCHab
#    - CIE L*u*v* and CIE LCHuv
#    - HSL
#    - HSV
#    - HWB
#    - Hunter Lab
#    - LMS
#    - CMYK
#
#  + color utils
#
#-------------------------------------------------*/

#include <algorithm>
#include <math.h>

#include "color-spaces.h"
#include "angles.h"

/////////////////// Distances //////////////////////
// All input values are in range [0..1]

long double EuclidianDistanceSpace(const long double &x1, const long double &y1, const long double &z1,
                                   const long double &x2, const long double &y2, const long double &z2) // euclidian distance in 3D
{
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2)); // euclidian distance formula sqrt(x²+y²+z²)
}

long double EuclidianDistancePlane(const long double &x1, const long double &y1, const long double &x2, const long double &y2) // euclidian distance in 2D
{
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2)); // euclidian distance formula sqrt(x²+y²)
}

long double distanceCIEDE2000LAB(const long double &L1, const long double &A1, const long double &B1,
                                 const long double &L2, const long double &A2, const long double &B2,
                                 const long double k_L, const long double k_C, const long double k_H) // distance in CIELAB space using last and best formula (CIEDE2000) compared to CIE94 and CIE76
{
    // Adapted from Gregory Fiumara
    // source : http://github.com/gfiumara/CIEDE2000
    // Based on "The CIEDE2000 Color-Difference Formula: Implementation Notes, Supplementary Test Data, and Mathematical Observations" by Gaurav Sharma, Wencheng Wu, and Edul N. Dalal
    // Results checked against all article values page 24 : OK
    // "For these and all other numerical/graphical delta E00 values reported in this article, we set the parametric weighting factors to unity(i.e., k_L = k_C = k_H = 1.0)." (p27)
    // This function is SLOW, for speed use euclidian distance

    // Step 0 : convert values to correct ranges : L, a and b must be in [0..100]
    const long double l1 = L1 * 100.0;
    const long double l2 = L2 * 100.0;
    const long double a1 = A1 * 100.0;
    const long double a2 = A2 * 100.0;
    const long double b1 = B1 * 100.0;
    const long double b2 = B2 * 100.0;

    // Remark : all angles in equations are expressed in degrees, but atan, cos and sin need radians !

    // Step 1
    // equation 2 : chromas
    long double C1 = sqrtl((a1 * a1) + (b1 * b1));
    long double C2 = sqrtl((a2 * a2) + (b2 * b2));
    // equation 3 : chromas mean
    long double barC = (C1 + C2) / 2.0;
    // equation 4 : G
    long double G = 0.5 * (1 - sqrtl(powl(barC, 7) / (powl(barC, 7) + powl(25, 7))));
    // equation 5 : a
    long double a1Prime = (1.0 + G) * a1;
    long double a2Prime = (1.0 + G) * a2;
    // equation 6 : C' from LCH
    long double CPrime1 = sqrtl((a1Prime * a1Prime) + (b1 * b1));
    long double CPrime2 = sqrtl((a2Prime * a2Prime) + (b2 * b2));
    // equation 7 : H' from LCH
    long double hPrime1;
    if (b1 == 0 && a1Prime == 0)
        hPrime1 = 0.0;
    else {
        hPrime1 = atan2l(b1, a1Prime);
        if (hPrime1 < 0)
            hPrime1 += Pi * 2.0;
        hPrime1 = hPrime1 * 180.0 / Pi;
    }

    long double hPrime2;
    if (b2 == 0 && a2Prime == 0)
        hPrime2 = 0.0;
    else {
        hPrime2 = atan2l(b2, a2Prime);
        if (hPrime2 < 0)
            hPrime2 += Pi * 2.0;
        hPrime2 = hPrime2 * 180.0 / Pi;
    }

    // Step 2
    // equation 8 : delta L
    long double deltaLPrime = l2 - l1;
    // equation 9 : delta C'
    long double deltaCPrime = CPrime2 - CPrime1;
    // equation 10 : delta h'
    long double deltahPrime;
    long double CPrimeProduct = CPrime1 * CPrime2;

    if (CPrimeProduct == 0)
        deltahPrime = 0;
    else {
        deltahPrime = hPrime2 - hPrime1;
        if (deltahPrime < -180)
            deltahPrime += 360.0;
        else if (deltahPrime > 180)
            deltahPrime -= 360.0;
    }
    // Equation 11 : delta H'
    long double deltaHPrime = 2.0 * sqrtl(CPrimeProduct) * sinl(Angle::DegToRad(deltahPrime) / 2.0);

    // Step 3
    // equation 12 : L mean
    long double barLPrime = (l1 + l2) / 2.0;
    // equation 13 : C mean
    long double barCPrime = (CPrime1 + CPrime2) / 2.0;
    // equation 14 : bar h'
    long double barhPrime, hPrimeSum = hPrime1 + hPrime2;

    if (CPrime1 * CPrime2 == 0) {
        barhPrime = hPrimeSum;
    } else {
        if (abs(hPrime1 - hPrime2) <= 180)
            barhPrime = hPrimeSum / 2.0;
        else {
            if (hPrimeSum < 360)
                barhPrime = (hPrimeSum + 360) / 2.0;
            else
                barhPrime = (hPrimeSum - 360) / 2.0;
        }
    }
    // equation 15 : T
    long double T = 1.0 - (0.17 * cosl(Angle::DegToRad(barhPrime) - Angle::DegToRad(30.0))) +
                        (0.24 * cosl(2.0 * Angle::DegToRad(barhPrime))) +
                        (0.32 * cosl((3.0 * Angle::DegToRad(barhPrime)) + Angle::DegToRad(6.0))) -
                        (0.20 * cosl((4.0 * Angle::DegToRad(barhPrime)) - Angle::DegToRad(63.0)));
    // equation 16 : delta theta
    long double deltaTheta = Angle::DegToRad(30.0) * expl(-powl((Angle::DegToRad(barhPrime) - Angle::DegToRad(275.0)) / Angle::DegToRad(25.0), 2.0));
    // equation 17
    long double R_C = 2.0 * sqrtl(pow(barCPrime, 7.0) / (powl(barCPrime, 7.0) + powl(25, 7)));
    // equation 18
    long double S_L = 1 + ((0.015 * powl(barLPrime - 50.0, 2.0)) / sqrtl(20 + powl(barLPrime - 50.0, 2.0)));
    // equation 19
    long double S_C = 1 + (0.045 * barCPrime);
    // equation 20
    long double S_H = 1 + (0.015 * barCPrime * T);
    // equation 21
    long double R_T = (-sinl(2.0 * deltaTheta)) * R_C;

    // equation 22 : delta E (distance)
    long double deltaE = sqrtl(powl(deltaLPrime / (k_L * S_L), 2.0) +
                                powl(deltaCPrime / (k_C * S_C), 2.0) +
                                powl(deltaHPrime / (k_H * S_H), 2.0) +
                                (R_T * (deltaCPrime / (k_C * S_C)) * (deltaHPrime / (k_H * S_H))));

    return (deltaE);
}

long double DistanceFromBlackRGB(const long double &R, const long double &G, const long double &B) // CIEDE2000 distance from RGB(0,0,0)
{
    long double X, Y, Z, L, a, b;
    RGBtoXYZ(R, G, B, X, Y, Z); // convert RGB to XYZ
    XYZtoLAB(X, Y, Z, L, a, b); // convert XYZ to CIELab
    return distanceCIEDE2000LAB(L, a, b, 0, 0, 0, 1.0, 1.0, 1.0); // CIEDE2000 distance from pure black
}

long double DistanceFromWhiteRGB(const long double &R, const long double &G, const long double &B) // CIEDE2000 distance from RGB(1,1,1)
{
    long double X, Y, Z, L, a, b;
    RGBtoXYZ(R, G, B, X, Y, Z); // convert RGB to XYZ
    XYZtoLAB(X, Y, Z, L, a, b); // convert XYZ to CIELab
    return distanceCIEDE2000LAB(L, a, b, 1, 0, 0, 1.0, 1.0, 1.0); // CIEDE2000 distance from white
}

long double DistanceFromGrayRGB(const long double &R, const long double &G, const long double &B) // CIEDE2000 distance from nearest gray (computed in CIELAB)
{
    long double X, Y, Z, L, a, b;
    RGBtoXYZ(R, G, B, X, Y, Z); // convert RGB to XYZ
    XYZtoLAB(X, Y, Z, L, a, b); // convert XYZ to CIELab
    return distanceCIEDE2000LAB(L, a, b, L, 0, 0, 1.0, 1.0, 1.0); // CIEDE2000 distance from corresponding gray (same L value with a=b=0, i.e. no chroma)
}

long double DistanceRGB(const long double &R1, const long double &G1, const long double &B1,
                        const long double &R2, const long double &G2, const long double &B2,
                        const long double k_L, const long double k_C, const long double k_H) // CIEDE2000 distance between 2 RGB values
{
    long double X, Y, Z, L1, a1, b1, L2, a2, b2;
    RGBtoXYZ(R1, G1, B1, X, Y, Z); // convert RGB to XYZ
    XYZtoLAB(X, Y, Z, L1, a1, b1); // convert XYZ to CIELab
    RGBtoXYZ(R2, G2, B2, X, Y, Z); // same for 2nd RGB value
    XYZtoLAB(X, Y, Z, L2, a2, b2);
    return distanceCIEDE2000LAB(L1, a1, b1, L2, a2, b2, k_L, k_C, k_H); // CIEDE2000 distance
}

/////////////////// RGB color space //////////////////////

void RGBMean(const long double &R1, const long double &G1, const long double &B1, const long double W1,
             const long double &R2, const long double &G2, const long double &B2, const long double W2,
             long double &R, long double &G, long double &B) // mean RGB value of 2 RGB values
{   
    long double r1, g1, b1, r2, g2, b2;
    // gamma correction - conversion to linear space - better for means
    GammaCorrectionToSRGB(R1, G1, B1, r1, g1, b1); // first RGB value
    GammaCorrectionToSRGB(R2, G2, B2, r2, g2, b2); // second RGB value

    // mean for RGB values in a linear space
    R = (r1 * W1 + r2 * W2) / (W1 + W2);
    G = (g1 * W1 + g2 * W2) / (W1 + W2);
    B = (b1 * W1 + b2 * W2) / (W1 + W2);

    // invert gamma return to RGB
    GammaCorrectionFromSRGB(R, G, B, R, G, B); // gamma correction to linear sRGB
}

void RGBtoStandard(const long double &r, const long double &g, const long double &b, int &R, int &G, int &B) // convert RGB [0..1] to RGB [0..255]
{
    R = round(r * 255.0L);
    G = round(g * 255.0L);
    B = round(b * 255.0L);
}

void GammaCorrectionToSRGB(const long double &R, const long double &G, const long double &B, long double &r, long double &g, long double &b) // Apply linear RGB gamma correction to sRGB
{
    // Gamma correction - conversion to linear space - source http://www.brucelindbloom.com/index.html?Eqn_RGB_to_XYZ.html
    if (R > 0.04045)
        r = powl((R + 0.055) / 1.055, 2.4);
    else
        r = R / 12.92;
    if (G > 0.04045)
        g = powl((G + 0.055) / 1.055, 2.4);
    else
        g = G / 12.92;
    if (B > 0.04045)
        b = powl((B + 0.055) / 1.055, 2.4);
    else
        b = B / 12.92;
}

void GammaCorrectionFromSRGB(const long double &R, const long double &G, const long double &B, long double &r, long double &g, long double &b) // Apply linear gamma correction from sRGB
{
    // Gamma profile - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    if (R > 0.0031308)
        r = 1.055 * powl(R, 1.0 / 2.4) - 0.055;
    else
        r = R * 12.92;

    if (G > 0.0031308)
        g = 1.055 * powl(G, 1.0 / 2.4) - 0.055;
    else
        g = G * 12.92;

    if (B > 0.0031308)
        b = 1.055 * powl(B, 1.0 / 2.4) - 0.055;
    else
        b = B * 12.92;
}

long double PerceivedBrightnessRGB(const long double &R, const long double &G, const long double &B) // perceived brightness of RGB value
{
    long double P = sqrt(pow(R * 255.0, 2) * 0.299 + pow(G * 255.0, 2) * 0.587 + pow(B * 255.0, 2) * 0.114); // Perceived brightness=sqrt(0.299*R² + 0.587*G² + 0.114*B²)

    return P / 255.0; // result in [0..1]
}

void HSLChfromRGB(const long double &R, const long double &G, const long double &B,
                long double &H, long double &S, long double&L, long double &C, long double &h) // get HSL values from RGB using HSL, CIELab and CIELCHab
{
    // C, L and S from LCHab are more perceptive than S and L from HSL that is derived from RGB

    long double X, Y, Z; // for XYZ values
    long double a, b; // for CIELab and CIELCHab

    RGBtoXYZ(R, G, B, X, Y, Z); // first step to get values in CIE spaces : convert RGB to XYZ
    XYZtoLAB(X, Y, Z, L, a, b); // then to CIELAb
    LABtoLCHab(a, b, C, h); // then to CIELCHab

    if (L == 0) { // black is a particular value
        S = 0;
        C = 0;
    }
    else // if not black
        S = C / sqrtl(pow(C,2) + pow(L,2)); // saturation S from LCHab
    if (S > 1)
        S = 1;
    long double s, l, c;
    RGBtoHSL(R, G, B, H, s, l, c); // getting H from HSL
}

bool IsRGBColorDark(int red, int green, int blue) // tell if a RGB color is dark or not
{
  double brightness;
  brightness = (red * 299) + (green * 587) + (blue * 114);
  brightness = brightness / 255000;

  // values range from 0 to 1 : anything greater than 0.5 should be bright enough for dark text
  return (brightness <= 0.5);
}

/////////////////// Color spaces conversions //////////////////////
//// All values are in range [0..1]

//// Spectral colors
//// see https://en.wikipedia.org/wiki/Spectral_color

void WavelengthToXYZ(const long double &w, long double &X, long double &Y, long double &Z) // wavelength to XYZ color space
{
    int n = -1; // index of array
    for (int W = 0; W < wavelength_XYZ_nb; W++) // all values
        if (int(w) == wavelength_XYZ[W][0]) { // wavelength in array ?
            n = W; // index of line
            break; // found
        }

    if (n == -1) { // not found ?
        X = 0;
        Y = 0;
        Z = 0;
    }
    else { // found => return values
        X = wavelength_XYZ[n][1];
        Y = wavelength_XYZ[n][2];
        Z = wavelength_XYZ[n][3];
    }
}

void SpectralColorToRGB(const long double &L, long double &R, long double &G, long double &B) // roughly convert wavelength value 400-700 nm to RGB [0..1]
{
    long double t;
    R = 0.0;
    G = 0.0;
    B = 0.0;

    if      ((L >= 400.0) and (L < 410.0))  { t = (L - 400.0) / (410.0 - 400.0); R = 0.33 * t - 0.20 * t * t; }
    else if ((L >= 410.0) and (L < 475.0))  { t = (L - 410.0) / (475.0 - 410.0); R = 0.14 - 0.13 * t * t; }
    else if ((L >= 545.0) and (L < 595.0))  { t = (L - 545.0) / (595.0 - 545.0); R = 1.98 * t - t * t; }
    else if ((L >= 595.0) and (L < 650.0))  { t = (L - 595.0) / (650.0 - 595.0); R = 0.98 + 0.06 * t - 0.40 * t * t; }
    else if ((L >= 650.0) and (L < 700.0))  { t = (L - 650.0) / (700.0 - 650.0); R = 0.65 - 0.84 * t + 0.20 * t * t; }

    if      ((L >= 415.0) and (L < 475.0))  { t = (L - 415.0) / (475.0 - 415.0); G = 0.80 * t * t; }
    else if ((L >= 475.0) and (L < 590.0))  { t = (L - 475.0) / (590.0 - 475.0); G = 0.8 + 0.76 * t - 0.80 * t * t; }
    else if ((L >= 585.0) and (L < 639.0))  { t = (L - 585.0) / (639.0 - 585.0); G = 0.84 - 0.84 * t; }

    if      ((L >= 400.0) and (L < 475.0))  { t = (L - 400.0) / (475.0 - 400.0); B = 2.20 * t - 1.50 * t * t; }
    else if ((L >= 475.0) and (L < 560.0))  { t = (L - 475.0) / (560.0 - 475.0); B = 0.7 - t + 0.30 * t * t; }

    // clipping : R, G and B must stay in [0..1]
    if (R > 1)
        R = 1;
    if (G > 1)
        G = 1;
    if (B > 1)
        B = 1;
    if (R < 0)
        R = 0;
    if (G < 0)
        G = 0;
    if (B < 0)
        B = 0;
}

//// HSV
//// see https://en.wikipedia.org/wiki/HSL_and_HSV
//// All values [0..1]
//// Common range : H [0..359] S [0..100] V [0..100]

void RGBtoHSV(const long double &R, const long double &G, const long double &B, long double& H, long double& S, long double &V, long double &C) // convert RGB value to HSV+C
{ // function checked OK with other calculators
    long double cmax = std::max(std::max(R, G), B);    // maximum of RGB
    long double cmin = std::min(std::min(R, G), B);    // minimum of RGB
    long double diff = cmax-cmin;       // diff of cmax and cmin.

    if (diff > 0) { // not particular case of diff = 0 -> find if R G or B is dominant
        if (cmax == R) // R is dominant
            H = 60.0 * (fmod(((G - B) / diff), 6)); // compute H
        else if (cmax == G) // G is dominant
            H = 60 * (((B - R) / diff) + 2); // compute H
        else if (cmax == B) // B is dominant
            H = 60 * (((R - G) / diff) + 4); // compute H

        if (cmax > 0) // compute S
            S = diff / cmax;
        else
            S = 0;

        V = cmax; // compute V
    }
    else { // particular case -> H = red (convention)
        H = 0;
        S = 0;
        V = cmax;
    }

    if (H < 0) // H must be in [0..360] range
        H += 360;
    if (H >= 360)
        H -= 360;

    // Final results are in range [0..1]
    H = H / 360.0; // was in degrees
    C = diff; // chroma
}

void HSVtoRGB(const long double &H, const long double &S, const long double &V, long double &R, long double &G, long double &B) // convert HSV value to RGB
{ // function checked OK with other calculators
  long double C = V * S; // chroma
  long double HPrime = H * 360.0 / 60.0; // dominant 6th part of H - H must be in [0..360]
  long double X = C * (1 - abs(fmod(HPrime, 2) - 1)); // intermediate value
  long double M = V - C; // difference to add at the end

  // for each part its calculus
  if (0 <= HPrime && HPrime < 1) {
    R = C;
    G = X;
    B = 0;
  }
  else if (1 <= HPrime && HPrime < 2) {
    R = X;
    G = C;
    B = 0;
  }
  else if (2 <= HPrime && HPrime < 3) {
    R = 0;
    G = C;
    B = X;
  }
  else if (3 <= HPrime && HPrime < 4) {
    R = 0;
    G = X;
    B = C;
  }
  else if (4 <= HPrime && HPrime < 5) {
    R = X;
    G = 0;
    B = C;
  }
  else if(5 <= HPrime && HPrime < 6) {
    R = C;
    G = 0;
    B = X;
  } else {
    R = 0;
    G = 0;
    B = 0;
  }

  R += M; // final results
  G += M;
  B += M;
}

void HSVtoStandard(const long double &h, const long double &s, const long double &v, int &H, int &S, int &V) // convert HSV [0..1] to HSV H [0..359] S and V [0..100]
{
    H = round(h * 360.0L);
    S = round(s * 100.0L);
    V = round(v * 100.0L);
}

//// HSL
//// see https://en.wikipedia.org/wiki/HSL_and_HSV
//// All values [0..1]
//// Common range : H [0..359] S [0..100] L [0..100]

void RGBtoHSL(const long double &R, const long double &G, const long double &B, long double &H, long double &S, long double &L, long double &C) // convert RGB value to HSL
{ // function checked OK with other calculators
    long double cmax = std::max(std::max(R, G), B);    // maximum of RGB
    long double cmin = std::min(std::min(R, G), B);    // minimum of RGB
    long double diff = cmax - cmin;       // diff of cmax and cmin.

    L = (cmax + cmin) / 2.0L; // middle of range

    if(cmax == cmin) // particular case : color is a gray
    {
        S = 0;
        H = 0;
    }
    else {
        if (L < 0.5) // result depends on Lightness
            S = diff / (cmax + cmin); // compute S
        else
            S = diff / (2.0L - cmax - cmin); // compute S

        // which is the dominant in R, G, B
        if (cmax == R) // red
            H = (G - B) / diff; // compute H
        if (cmax == G) // green
            H = 2.0L + (B - R) / diff; // compute H
        if (cmax == B) // blue
            H = 4.0L + (R - G) / diff; // compute H
    }

    H = H * 60; // H in degrees

    if (H < 0) // H in [0..360]
        H += 360.0L;
    if (H >= 360.0L)
        H -= 360.0L;

    // Final results in range [0..1]
    H = H / 360.0L; // was in degrees
    C = diff; // Chroma
}

long double HueToRGB(const long double &v1, const long double &v2, const long double &H) // Convert Hue to R, G or B value for HSLtoRGB function
{
    long double vH = H;

    if (vH < 0) vH += 1; // H must be in range [0..1]
    if (vH > 1) vH -= 1;

    if ((6 * vH) < 1) // which component (R, G, B) to compute ?
        return v1 + (v2 - v1) * 6.0L * vH;
    if ((2 * vH) < 1 )
        return v2;
    if ((3 * vH) < 2 )
        return (v1 + (v2 - v1) * ((2.0L / 3.0L) - vH) * 6.0L);
    return (v1);
}

void HSLtoRGB(const long double &H, const long double &S, const long double &L, long double &R, long double &G, long double &B) // convert HSL to RGB value - H is in degrees
{ // function checked OK with other calculators
    if ( S == 0 ) { // color is a gray
        R = L;
        G = L;
        B = L;
    }
    else {
        long double var_1, var_2;
        if (L < 0.5) // Result depends on Luminance
            var_2 = L * (1.0L + S);
        else
            var_2 = (L + S) - (S * L);

        var_1 = 2.0L * L - var_2; // first component based on Luminance

        R = HueToRGB(var_1, var_2, H + 1.0L / 3.0L); // compute R, G, B
        G = HueToRGB(var_1, var_2, H);
        B = HueToRGB(var_1, var_2, H - 1.0L / 3.0L);
    }
}

void HSLtoStandard(const long double &h, const long double &s, const long double &l, int &H, int &S, int &L) // convert HSL [0..1] to HSL H [0..359] S and L [0..100]
{
    H = round(h * 360.0L);
    S = round(s * 100.0L);
    L = round(l * 100.0L);
}

//// HWB
//// see https://en.wikipedia.org/wiki/HWB_color_model
//// All values [0..1]
//// Common range : H [0..359] W [0..100] B [0..100]

void HSVtoHWB(const long double &h, const long double &s, const long double &v, long double &H, long double &W, long double &B) // convert HSV to HWB
{ // function checked OK with other calculators
    // calculus is simple ! There is a direct relation
    H = h;
    W = (1.0L - s) * v;
    B = 1.0L - v;
}

void RGBtoHWB(const long double &r, const long double &g, const long double &b, long double &H, long double &W, long double &B) // convert RGB to HWB
{ // function checked OK with other calculators
    long double h, s, v, c;
    RGBtoHSV(r, g, b, h, s, v, c); // first find HSV
    HSVtoHWB(h, s, v, H, W, B); // then convert HSV to HWB
}

void HWBtoHSV(const long double &h, const long double &w, const long double &b, long double &H, long double &S, long double &V) // convert HWB to HSV
{ // function checked OK with other calculators
    // calculus is simple ! This is a direct relation
    H = h;
    S = 1.0L - (w / (1.0L - b));
    V = 1.0L - b;
}

void HWBtoRGB(const long double &h, const long double &w, const long double &b, long double &R, long double &G, long double &B) // convert HWB to RGB
{ // function checked OK with other calculators
    long double H, S, V;
    HWBtoHSV(h, w, b, H, S, V); // first convert to HSV
    HSVtoRGB(H, S, V, R, G, B); // then to RGB
}

void HWBtoStandard(const long double &h, const long double &w, const long double &b, int &H, int &W, int &B) // convert HWB [0..1] to HWB H [0..359] W and B [0..100]
{
    H = round(h * 360.0L);
    W = round(w * 100.0L);
    B = round(b * 100.0L);
}

//// XYZ
//// See https://en.wikipedia.org/wiki/CIE_1931_color_space
////   and https://fr.wikipedia.org/wiki/CIE_XYZ
//// All values [0..1]
//// Common range for XYZ : [0..100]

void RGBtoXYZ(const long double &R, const long double &G, const long double &B, long double &X, long double &Y, long double &Z) // convert RGB (in fact sRGB) value to CIE XYZ
{ // function checked OK with other calculators
    long double r, g, b;

    GammaCorrectionToSRGB(R, G, B, r, g, b); // gamma correction to linear sRGB

    // Gammut conversion to sRGB - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    X = r * 0.4124564 + g * 0.3575761 + b * 0.1804375;
    Y = r * 0.2126729 + g * 0.7151522 + b * 0.0721750;
    Z = r * 0.0193339 + g * 0.1191920 + b * 0.9503041;
}

void XYZtoRGB(const long double &X, const long double &Y, const long double &Z, long double &R, long double &G, long double &B) // convert from XYZ to RGB (in fact sRGB)
{ // function checked OK with other calculators
    // Gammut conversion - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    R = X *  3.2404542 + Y * -1.5371385 + Z * -0.4985314;
    G = X * -0.9692660 + Y *  1.8760108 + Z *  0.0415560;
    B = X *  0.0556434 + Y * -0.2040259 + Z *  1.0572252;

    GammaCorrectionFromSRGB(R, G, B, R, G, B); // gamma correction from linear sRGB

    // clipping : R, G and B must be in [0..1]
    if (R < 0)
        R = 0;
    if (R > 1)
        R = 1;
    if (G < 0)
        G = 0;
    if (G > 1)
        G = 1;
    if (B < 0)
        B = 0;
    if (B > 1)
        B = 1;
}

void XYZtoRGBNoClipping(const long double &X, const long double &Y, const long double &Z, long double &R, long double &G, long double &B) // convert from XYZ to RGB without clipping to [0..1]
{ // function checked OK with other calculators
    // Gammut conversion - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    R = X *  3.2404542 + Y * -1.5371385 + Z * -0.4985314;
    G = X * -0.9692660 + Y *  1.8760108 + Z *  0.0415560;
    B = X *  0.0556434 + Y * -0.2040259 + Z *  1.0572252;

    GammaCorrectionFromSRGB(R, G, B, R, G, B); // gamma correction from linear sRGB

    if ((R > 1) or (R < 0) or (G > 1) or (G < 0) or (B > 1) or (B < 0)) { // R, G and B must be in [0..1] - else return black (0,0,0) - no clipping
        R = 0;
        G = 0;
        B = 0;
    }
}

void XYZtoStandard(const long double &x, const long double &y, const long double &z, int &X, int &Y, int &Z) // convert XYZ [0..1] to XYZ [0..100]
{
    X = round(x * 100.0L);
    Y = round(y * 100.0L);
    Z = round(z * 100.0L);
}

//// CIE xyY
//// see https://en.wikipedia.org/wiki/CIE_1931_color_space#CIE_xy_chromaticity_diagram_and_the_CIE_xyY_color_space
//// All values [0..1]
//// Common range for xyY: x [0..1] y [0..1] Y [0..100]
//// Note that Y is exactly equal to Y in CIE XYZ

void XYZtoxyY(const long double &X, const long double &Y, const long double &Z, long double &x, long double &y) // convert CIE XYZ value to CIE xyY
{ // function checked OK with other calculators
    if ((X == 0) and (Y == 0) and (Z == 0)) { // D65 white point
        x = 0.3127L;
        y = 0.3290L;
    }
    else { // from formula X + Y + Z = 1
        x = X / (X + Y +Z);
        y = Y / (X + Y +Z);
    }
}

void xyYtoXYZ(const long double &x, const long double &y, const long double &Y, long double &X, long double &Z) // convert CIE xyY value to CIE XYZ
{ // function checked OK with other calculators
    if (Y == 0) { // Y is lightness so if 0 => no color
        X = 0;
        Z = 0;
    }
    else { // from formula X + Y + Z = 1
        X = x * Y / y;
        Z = (1 - x - y) * Y / y;
    }
}

//// CIE L*A*B*
//// see https://en.wikipedia.org/wiki/CIELAB_color_space for LAB
//// All values [0..1]
//// Common range for L*A*B*: L [0..100] S [-128..127] V [-128..127]

void XYZtoLAB(const long double &X, const long double &Y, const long double &Z, long double &L, long double &A, long double &B) // convert CIE XYZ value to CIE L*A*B*
{ // function checked OK with other calculators
    // reference white in XYZ
    long double ref_X = 0.95047L;
    long double ref_Y = 1.0L;
    long double ref_Z = 1.08883L;
    // CIE values
    long double E = 216.0L / 24389.0L;
    long double K = 24389.0L / 27.0L;

    long double Xr = X / ref_X;
    long double Yr = Y / ref_Y;
    long double Zr = Z / ref_Z;

    long double fX, fY, fZ;
    if (Xr > E)
        fX = powl(Xr, 1.0L / 3.0L);
    else
        fX = (K * Xr + 16.0L) / 116.0L;
    if (Yr > E)
        fY = powl(Yr, 1.0L / 3.0L);
    else
        fY = (K * Yr + 16.0L) / 116.0L;
    if (Zr > E)
        fZ = powl(Zr, 1.0L / 3.0L);
    else
        fZ = (K * Zr + 16.0L) / 116.0L;

    L = 116.0L * fY - 16.0L;
    A = 500.0L * (fX - fY);
    B = 200.0L * (fY - fZ);

    L = L / 100.0L; // to stay in [0..1] range
    A = A / 127.0L;
    B = B / 127.0L;
}

void LABtoXYZ(const long double &L, const long double &A, const long double &B, long double &X, long double &Y, long double &Z) // convert CIE L*A*B* to CIE XYZ
{ // function checked OK with other calculators
    if (L == 0) { // black
        X = 0;
        Y = 0;
        Z = 0;
        return;
    }

    // reference white in XYZ
    long double ref_X = 0.95047L;
    long double ref_Y = 1.0L;
    long double ref_Z = 1.08883L;
    // CIE values
    long double E = 216.0L / 24389.0L;
    long double K = 24389.0L / 27.0L;

    long double fY = (L * 100.0L + 16.0L) / 116.0L;
    long double fZ = fY - B * 127.0L / 200.0L;
    long double fX = A * 127.0L / 500.0L + fY;

    long double Xr, Yr, Zr;

    if (powl(fX, 3.0) > E)
        Xr = powl(fX, 3.0L);
    else
        Xr = (116.0L * fX -16.0L) / K;
    if (L * 100.0L > K * E)
        Yr = powl((L * 100.0L + 16.0L) / 116.0L, 3.0L);
    else
        Yr = L * 100.0L / K;
    if (powl(fZ, 3.0L) > E)
        Zr = powl(fZ, 3.0L);
    else
        Zr = (116.0L * fZ -16.0L) / K;

    X = Xr * ref_X;
    Y = Yr * ref_Y;
    Z = Zr * ref_Z;
}

void LABtoStandard(const long double &l, const long double &a, const long double &b, int &L, int &A, int &B) // convert CIELab [0..1] to CIELab L [0..100] a and b [-128..127]
{
    L = round(l * 100.0L);
    A = round(a * 127.0L);
    B = round(b * 127.0L);
}

//// CIE LCHab
//// See https://en.wikipedia.org/wiki/CIELAB_color_space#Cylindrical_representation:_CIELCh_or_CIEHLC
//// All values [0..1] except C
//// Common range : L [0..100] C [0..100+] h [0..360]
//// Note that L is exactly equal to L of CIE L*a*b*

void LABtoLCHab(const long double &A, const long double &B, long double &C, long double &H) // convert from LAB to HLC - L is the same so no need to convert
{ // function checked OK with other calculators
    C = sqrtl(powl(A, 2.0L) + powl(B, 2.0L)); // chroma : divided by maximum of formula with a=1 and b=1 to have C in [0..1]

    H = atan2l(B, A) / 2.0L / Pi; // Hue - cartesian to polar
    while (H < 0) // Hue in range [0..1]
        H += 1.0L;
    while (H > 1)
        H -= 1.0L;
}

void LCHabToLAB(const long double &C, const long double &H, long double &A, long double &B) // convert from HLC to LAB - L is the same so no need to convert
{ // function checked OK with other calculators
    A = C * cosl(H * 2.0L * Pi); // polar to cartesian
    B = C * sinl(H * 2.0L * Pi);
}

void LCHabtoStandard(const long double &l, const long double &c, const long double &h, int &L, int &C, int &H) // convert CIE LCHab [0..1] to CIE LCHab L [0..100] C [0..100+] H [0..359]
{
    L = round(l * 100.0L);
    C = round(c * 127.0L); // because a and b from CIELab are in [-128..127]
    H = round(h * 360.0L);
}

//// CIE L*u*v*
//// see https://en.wikipedia.org/wiki/CIELUV
//// All values [0..1]
//// Common range for L*u*v*: L* [0..100] u* [-100..100] v* [-100..100]

void XYZtoLuv(const long double &X, const long double &Y, const long double &Z, long double &L, long double &u, long double &v) // convert CIE XYZ value to CIE L*u*v*
{ // function checked OK with other calculators
    // reference white in XYZ
    long double ref_X = 0.95047L;
    long double ref_Y = 1.0L;
    long double ref_Z = 1.08883L;
    // CIE values
    long double E = 216.0L / 24389.0L;
    long double K = 24389.0L / 27.0L;

    if (Y / ref_Y > E) // two-part equation
        L = 116.0L * powl(Y / ref_Y, 1.0L / 3.0L) - 16.0L;
    else
        L = K * Y / ref_Y;

    long double u_prime = 4.0L * X / (X + 15.0L * Y + 3.0L * Z); // intermediate calculus
    long double u_ref = 4.0L * ref_X / (ref_X + 15.0L * ref_Y + 3.0L * ref_Z);
    long double v_prime = 9.0L * Y / (X + 15.0L * Y + 3.0L * Z);
    long double v_ref = 9.0L * ref_Y / (ref_X + 15.0L * ref_Y + 3.0L * ref_Z);

    u = 13.0L * L * (u_prime - u_ref); // final values
    v = 13.0L * L * (v_prime - v_ref);

    L = L / 100.0L; // to stay in range [0..1]
    u = u / 100.0L;
    v = v / 100.0L;
    if (isnan(u)) // division by zero is bad so default value
        u = 0;
    if (isnan(v))
        v = 0;
}

void LuvToXYZ(const long double &L, const long double &U, const long double &V, long double &X, long double &Y, long double &Z) // convert CIE L*u*v* value to CIE XYZ
{ // function checked OK with other calculators
    // reference white in XYZ
    long double ref_X = 0.95047L;
    long double ref_Y = 1.0L;
    long double ref_Z = 1.08883L;
    // CIE values
    long double E = 216.0L / 24389.0L;
    long double K = 24389.0L / 27.0L;

    long double l = L * 100.0L;
    long double u = U * 100.0L;
    long double v = V * 100.0L;

    long double u0 = 4.0L * ref_X / (ref_X + 15.0L * ref_Y + 3.0L * ref_Z); // white point intermediate values
    long double v0 = 9.0L * ref_Y / (ref_X + 15.0L * ref_Y + 3.0L * ref_Z);

    long double u_prime = u / (13.0L * l) + u0;
    long double v_prime = v / (13.0L * l) + v0;

    if (l > K * E)
        Y = ref_Y * powl((l + 16.0L) / 116.0L, 3.0L);
    else
        Y = ref_Y * l * (powl(3.0L / 29.0L, 3.0L));

    X = Y * 9.0L * u_prime / 4.0L / v_prime;
    Z = Y * (12.0L - 3.0L * u_prime - 20.0L * v_prime) / 4.0L / v_prime;
}

void LuvToStandard(const long double &l, const long double &u, const long double &v, int &L, int &U, int &V) // convert CIELab [0..1] to CIELab L [0..100] u and v [-100..100]
{
    L = round(l * 100.0L);
    U = round(u * 100.0L);
    V = round(v * 100.0L);
}

//// CIE LCHuv
//// See https://en.wikipedia.org/wiki/CIELUV#Cylindrical_representation_(CIELCH)
//// All values [0..1]
//// Common range : L [0..100] C [0..100+] h [0..360]
//// Note that L is exactly equal to L of CIE Luv

void LuvToLCHuv(const long double &u, const long double &v, long double &C, long double &H) // convert from Luv to LCHuv - L is the same so no need to convert
{ // function checked OK with other calculators
    C = sqrtl(powl(u * 100.0L, 2.0L) + powl(v * 100.0L, 2.0L)) / 100.0; // Chroma

    H = atan2l(v, u) / 2.0L / Pi; // Hue - cartesian to polar
    while (H < 0) // Hue in range [0..1]
        H += 1.0L;
    while (H > 1)
        H -= 1.0L;
}

void LCHuvToLUV(const long double &C, const long double &H, long double &u, long double &v) // convert from LCHuv to LUV - L is the same so no need to convert
{ // function checked OK with other calculators
    u = C * cosl(H * 2.0L * Pi); // polar to cartesian
    v = C * sinl(H * 2.0L * Pi);
}

void LCHuvtoStandard(const long double &l, const long double &c, const long double &h, int &L, int &C, int &H) // convert CIE LCHuv [0..1] to CIE LCHuv L [0..100] C [0..100+] H [0..359]
{
    L = round(l * 100.0L);
    C = round(c * 100.0L);
    H = round(h * 360.0L);
}

//// Hunter Lab (HLAB)
//// See https://en.wikipedia.org/wiki/CIELAB_color_space#Hunter_Lab
//// All values [0..1]
//// Common range : L [0..100] a [-100..100] b [-100..100]

void XYZtoHLAB(const long double &X, const long double &Y, const long double &Z, long double &L, long double &A, long double &B) // convert from XYZ to Hunter Lab
{ // function checked OK with other calculators
    if (Y == 0) { // lightness is 0 => no color
        L = 0;
        A = 0;
        B = 0;
    }
    else {
        // reference white in XYZ
        long double ref_X = 0.95047L;
        long double ref_Y = 1.0L;
        long double ref_Z = 1.08883L;

        long double Ka = (175.0L / 198.04L) * (ref_X + ref_Y); // CIE standard values VS white point
        long double Kb = ( 70.0L / 218.11L) * (ref_Y + ref_Z);

        L = sqrtl(Y / ref_Y); // final values
        A = Ka * (((X / ref_X) - (Y / ref_Y)) / sqrtl(Y / ref_Y));
        B = Kb * (((Y / ref_Y) - (Z / ref_Z)) / sqrtl(Y / ref_Y));
    }
}

void HLABtoXYZ(const long double &L, const long double &A, const long double &B, long double &X, long double &Y, long double &Z) // convert from Hunter Lab to XYZ
{ // function checked OK with other calculators
    // reference white in XYZ
    long double ref_X = 0.95047L;
    long double ref_Y = 1.0L;
    long double ref_Z = 1.08883L;

    long double Ka = (175.0L / 198.04L) * (ref_Y + ref_X); // CIE standard values VS white point
    long double Kb = ( 70.0L / 218.11L) * (ref_Y + ref_Z);

    Y = powl(L / ref_Y, 2.0L); // final values
    X =  (A / Ka * sqrtl(Y / ref_Y) + (Y / ref_Y)) * ref_X;
    Z = -(B / Kb * sqrtl(Y / ref_Y) - (Y / ref_Y)) * ref_Z;

}

void HLABtoStandard(const long double &l, const long double &a, const long double &b, int &L, int &A, int &B) // convert Hunter Lab [0..1] to Hunter Lab L [0..100] a and b [-100..100]
{
    L = round(l * 100.0L);
    A = round(a * 100.0L);
    B = round(b * 100.0L);
}

//// CIE CAM02 LMS
//// See https://en.wikipedia.org/wiki/LMS_color_space
//// All values [0..1]
//// Common range : no range specified

void XYZtoLMS(const long double &X, const long double &Y, const long double &Z, long double &L, long double &M, long double &S) // convert from XYZ to LMS
{ // couldn't find any online calculator to check this function, but it is pretty straightforward
    // CIECAM02 is the successor to CIECAM97s - the best matrix so far, just coordinates change
    L =  0.7328L * X + 0.4296L * Y - 0.1624L * Z;
    M = -0.7036L * X + 1.6975L * Y + 0.0061L * Z;
    S =  0.0030L * X + 0.0136L * Y + 0.9834L * Z;
}

//// CMYK
//// See https://en.wikipedia.org/wiki/CMYK_color_model
//// All values [0..1]
//// Common range : C [0..100] M [0..100] Y [0..100] K [0..100]

long double ClampCMYK(const long double &value) // don't divide by 0 !
{
    if (value < 0 or isnan(value))
        return 0;
    else
        return value;
}

void RGBtoCMYK(const long double &R, const long double &G, const long double &B, long double &C, long double &M, long double &Y, long double &K) // convert from RGB to CYMK
{
    K = ClampCMYK(1 - std::max(std::max(R, G), B));
    C = ClampCMYK((1.0L - R - K) / (1.0L - K));
    M = ClampCMYK((1.0L - G - K) / (1.0L - K));
    Y = ClampCMYK((1.0L - B - K) / (1.0L - K));
}

void CMYKtoRGB(const long double &C, const long double &M, const long double &Y, const long double &K, long double &R, long double &G, long double &B) // convert from CMYK to RGB
{
    R = (1.0L - C) * (1.0L - K);
    G = (1.0L - M) * (1.0L - K);
    B = (1.0L - Y) * (1.0L - K);

    // R, G and B must be in [0..1]
    if (R > 1)
        R = 1;
    if (G > 1)
        G = 1;
    if (B > 1)
        B = 1;
    if (R < 0)
        R = 0;
    if (G < 0)
        G = 0;
    if (B < 0)
        B = 0;
}

void CMYKtoStandard(const long double &c, const long double &m, const long double &y, const long double &k, int &C, int &M, int &Y, int &K) // convert CMYK [0..1] to CMYK [0..100]
{
    C = round(c * 100.0L);
    M = round(m * 100.0L);
    Y = round(y * 100.0L);
    K = round(k * 100.0L);
}
