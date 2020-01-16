/*#-------------------------------------------------
#
#            Color spaces conversions
#
#    by AbsurdePhoton - www.absurdephoton.fr
#
#                v1.1 - 2020/01/11
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

/////////////////// Color utils //////////////////////
// All input values are in range [0..1]

long double PerceivedBrightnessRGB(const long double &R, const long double &G, const long double &B) // perceived brightnessof RGB value
{
    long double P = sqrt(pow(R * 255.0, 2) * 0.299 + pow(G * 255.0, 2) * 0.587 + pow(B * 255.0, 2) * 0.114); // Perceived brightness=sqrt(0.299*R² + 0.587*G² + 0.114*B²)

    return P / 255.0; // result in [0..1]
}

void HSLfromRGB(const long double &R, const long double &G, const long double &B,
                long double &H, long double &S, long double&L) // get HSL values from RGB using HSL, CIELuv and CIELCHuv
// L and S from LCHuv are more perceptive than S and L from HSL that is derived from RGB
{
    long double X, Y, Z; // for XYZ values
    long double C, u, v; // for Luv and LCHuv

    RGBtoXYZ(R, G, B, X, Y, Z); // first step to get values in CIE spaces : convert RGB to XYZ
    XYZtoLuv(X, Y, Z, L, u, v); // first step for getting chromaticity : convert to CIELuv to get L
    LuvToLCHuv(u, v, C, H); // get chroma C
    S = C / sqrt(pow(C,2) + pow(L,2)); // saturation S from LCHuv
    long double s, l, c;
    RGBtoHSL(R, G, B, H, s, l, c); // getting H from HSL
}

long double EuclidianDistanceSpace(const long double &x1, const long double &y1, const long double &z1,
                                   const long double &x2, const long double &y2, const long double &z2) // euclidian distance in 3-dimension
{
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2) + pow(z1 - z2, 2)); // euclidian distance formula sqrt(x²+y²+z²)
}

long double EuclidianDistancePlane(const long double &x1, const long double &y1, const long double &x2, const long double &y2) // euclidian distance in 2-dimension
{
    return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2)); // euclidian distance formula sqrt(x²+y²)
}

long double distanceCIEDE2000LAB(const long double &L1, const long double &A1, const long double &B1,
                                 const long double &L2, const long double &A2, const long double &B2) // distance in CIELAB space using last and best formula (CIEDE2000) compared to CIE94 and CIE76
{
    // Adapted from Gregory Fiumara
    // source : http://github.com/gfiumara/CIEDE2000
    // Based on "The CIEDE2000 Color-Difference Formula: Implementation Notes, Supplementary Test Data, and Mathematical Observations" by Gaurav Sharma, Wencheng Wu, and Edul N. Dalal
    // Results checked against all article values page 24 : OK

    // "For these and all other numerical/graphical delta E00 values reported in this article, we set the parametric weighting factors to unity(i.e., k_L = k_C = k_H = 1.0)." (p27)
    const long double k_L = 1.0, k_C = 1.0, k_H = 1.0;

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

long double distanceCIEDE2000LCH(const long double &L1, const long double &C1, const long double &H1,
                                 const long double &L2, const long double &C2, const long double &H2) // distance in CIELCh space
{
    long double A1, A2, B1, B2;
    LCHabToLAB(H1, C1, A1, B1); // convert to CIELab
    LCHabToLAB(H2, C2, A2, B2);
    return distanceCIEDE2000LAB(L1, A1, B1, L2, A2, B2); // CIEDE2000 distance
}

long double DistanceFromBlackRGB(const long double &R, const long double &G, const long double &B) // CIEDE2000 distance from RGB(0,0,0)
{
    long double X, Y, Z, L, a, b;
    RGBtoXYZ(R, G, B, X, Y, Z); // convert RGB to XYZ
    XYZtoLAB(X, Y, Z, L, a, b); // convert XYZ to CIELab
    return distanceCIEDE2000LAB(L, a, b, 0, 0, 0); // CIEDE2000 distance from pure black
}

long double DistanceFromWhiteRGB(const long double &R, const long double &G, const long double &B) // CIEDE2000 distance from RGB(1,1,1)
{
    long double X, Y, Z, L, a, b;
    RGBtoXYZ(R, G, B, X, Y, Z); // convert RGB to XYZ
    XYZtoLAB(X, Y, Z, L, a, b); // convert XYZ to CIELab
    return distanceCIEDE2000LAB(L, a, b, 1, 0, 0); // CIEDE2000 distance from white
}

long double DistanceFromGrayRGB(const long double &R, const long double &G, const long double &B) // CIEDE2000 distance from nearest gray (computed in CIELAB)
{
    long double X, Y, Z, L, a, b;
    RGBtoXYZ(R, G, B, X, Y, Z); // convert RGB to XYZ
    XYZtoLAB(X, Y, Z, L, a, b); // convert XYZ to CIELab
    return distanceCIEDE2000LAB(L, a, b, L, 0, 0); // CIEDE2000 distance from corresponding gray (same L value with a=b=0, i.e. no chroma)
}

long double DistanceRGB(const long double &R1, const long double &G1, const long double &B1,
                        const long double &R2, const long double &G2, const long double &B2) // CIEDE2000 distance between 2 RGB values
{
    long double X, Y, Z, L1, a1, b1, L2, a2, b2;
    RGBtoXYZ(R1, G1, B1, X, Y, Z); // convert RGB to XYZ
    XYZtoLAB(X, Y, Z, L1, a1, b1); // convert XYZ to CIELab
    RGBtoXYZ(R2, G2, B2, X, Y, Z); // same for 2nd RGB value
    XYZtoLAB(X, Y, Z, L2, a2, b2);
    return distanceCIEDE2000LAB(L1, a1, b1, L2, a2, b2); // CIEDE2000 distance
}

void RGBMean(const long double &R1, const long double &G1, const long double &B1,
             const long double &R2, const long double &G2, const long double &B2,
             long double &R, long double &G, long double &B) // mean RGB value of 2 RGB values
{
    long double r1, g1, b1, r2, g2, b2;
    // gamma correction - conversion to linear space - source http://www.brucelindbloom.com/index.html?Eqn_RGB_to_XYZ.html
    // first RGB value
    if (R1 > 0.04045)
        r1 = powl((R1 + 0.055) / 1.055, 2.4);
    else
        r1 = R1 / 12.92;
    if (G1 > 0.04045)
        g1 = powl((G1 + 0.055) / 1.055, 2.4);
    else
        g1 = G1 / 12.92;
    if (B1 > 0.04045)
        b1 = powl((B1 + 0.055) / 1.055, 2.4);
    else
        b1 = B1 / 12.92;
    // second RGB value
    if (R2 > 0.04045)
        r2 = powl((R2 + 0.055) / 1.055, 2.4);
    else
        r2 = R2 / 12.92;
    if (G2 > 0.04045)
        g2 = powl((G2 + 0.055) / 1.055, 2.4);
    else
        g2 = G2 / 12.92;
    if (B2 > 0.04045)
        b2 = powl((B2 + 0.055) / 1.055, 2.4);
    else
        b2 = B2 / 12.92;
    // mean for RGB values in a linear space
    R = (r1 + r2) / 2.0;
    G = (g1 + g2) / 2.0;
    B = (b1 + b2) / 2.0;

    // invert gamma of RGB result - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    if (R > 0.0031308)
        R = 1.055 * powl(R, 1.0 / 2.4) - 0.055;
    else
        R = R * 12.92;

    if (G > 0.0031308)
        G = 1.055 * powl(G, 1.0 / 2.4) - 0.055;
    else
        G = G * 12.92;

    if (B > 0.0031308)
        B = 1.055 * powl(B, 1.0 / 2.4) - 0.055;
    else
        B = B * 12.92;
}

/////////////////// Color spaces conversions //////////////////////
//// All values are in range [0..1]

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

//// HSL
//// see https://en.wikipedia.org/wiki/HSL_and_HSV
//// All values [0..1]
//// Common range : H [0..359] S [0..100] L [0..100]

void RGBtoHSL(const long double &R, const long double &G, const long double &B, long double &H, long double &S, long double &L, long double &C) // convert RGB value to HSL
{ // function checked OK with other calculators
    long double cmax = std::max(std::max(R, G), B);    // maximum of RGB
    long double cmin = std::min(std::min(R, G), B);    // minimum of RGB
    long double diff = cmax - cmin;       // diff of cmax and cmin.

    L = (cmax + cmin) / 2.0; // middle of range

    if(cmax == cmin) // particular case : color is a gray
    {
        S = 0;
        H = 0;
    }
    else {
        if (L < .50) // result depends on Lightness
            S = diff / (cmax + cmin); // compute S
        else
            S = diff / (2 - cmax - cmin); // compute S

        // which is the dominant in R, G, B
        if (cmax == R) // red
            H = (G - B) / diff; // compute H
        if (cmax == G) // green
            H = 2 + (B - R) / diff; // compute H
        if (cmax == B) // blue
            H = 4 + (R - G) / diff; // compute H
    }

    H = H * 60; // H in degrees

    if (H < 0) // H in [0..360]
        H += 360;
    if (H >= 360)
        H -= 360;

    // Final results in range [0..1]
    H = H / 360.0; // was in degrees
    C = diff; // Chroma
}

long double HueToRGB(const long double &v1, const long double &v2, const long double &H) // Convert Hue to R, G or B value for HSLtoRGB function
{
    long double vH = H;

    if (vH < 0) vH += 1; // H must be in range [0..1]
    if (vH > 1) vH -= 1;

    if ((6 * vH) < 1) // which component (R, G, B) to compute ?
        return v1 + (v2 - v1) * 6.0 * vH;
    if ((2 * vH) < 1 )
        return v2;
    if ((3 * vH) < 2 )
        return (v1 + (v2 - v1) * ((2.0 / 3.0) - vH) * 6.0);
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
            var_2 = L * (1.0 + S);
        else
            var_2 = (L + S) - (S * L);

        var_1 = 2.0 * L - var_2; // first component based on Luminance

        R = HueToRGB(var_1, var_2, H + 1.0 / 3.0); // compute R, G, B
        G = HueToRGB(var_1, var_2, H);
        B = HueToRGB(var_1, var_2, H - 1.0 / 3.0);
    }
}

//// HWB
//// see https://en.wikipedia.org/wiki/HWB_color_model
//// All values [0..1]
//// Common range : H [0..359] W [0..100] B [0..100]

void HSVtoHWB(const long double &h, const long double &s, const long double &v, long double &H, long double &W, long double &B) // convert HSV to HWB
{ // function checked OK with other calculators
    // calculus is simple ! There is a direct relation
    H = h;
    W = (1.0 - s) * v;
    B = 1.0 - v;
}

void RGBtoHWB(const long double &r, const long double &g, const long double &b, long double &H, long double &W, long double &B) // convert RGB to HWB
{ // function checked OK with other calculators
    long double h, s, v, c;
    RGBtoHSV(r, g, b, h, s, v, c); // first find HSV
    HSVtoHWB(h, s, v, H, W, B); // then convert HSV to HWB
}

void HWBtoHSV(const long double &h, const long double &w, const long double &b, long double &H, long double &S, long double &V) // convert HWB to HSV
{ // function checked OK with other calculators
    // calculus is simple ! There is a direct relation
    H = h;
    S = 1.0 - (w / (1.0 - b));
    V = 1.0 - b;
}

void HWBtoRGB(const long double &h, const long double &w, const long double &b, long double &R, long double &G, long double &B) // convert HWB to RGB
{ // function checked OK with other calculators
    long double H, S, V;
    HWBtoHSV(h, w, b, H, S, V); // first convert to HSV
    HSVtoRGB(H, S, V, R, G, B); // then to RGB
}

//// XYZ
//// See https://en.wikipedia.org/wiki/CIE_1931_color_space
////   and https://fr.wikipedia.org/wiki/CIE_XYZ
//// All values [0..1]
//// Common range for XYZ : [0..100]

void RGBtoXYZ(const long double &R, const long double &G, const long double &B, long double &X, long double &Y, long double &Z) // convert RGB (in fact sRGB) value to CIE XYZ
{ // function checked OK with other calculators
    long double r, g, b;

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

    // Gamma profile - source http://www.brucelindbloom.com/index.html?Eqn_RGB_XYZ_Matrix.html
    if (R > 0.0031308)
        R = 1.055 * powl(R, 1.0 / 2.4) - 0.055;
    else
        R = R * 12.92;

    if (G > 0.0031308)
        G = 1.055 * powl(G, 1.0 / 2.4) - 0.055;
    else
        G = G * 12.92;

    if (B > 0.0031308)
        B = 1.055 * powl(B, 1.0 / 2.4) - 0.055;
    else
        B = B * 12.92;

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

//// L*A*B*
//// see https://en.wikipedia.org/wiki/CIELAB_color_space for LAB
//// All values [0..1]
//// Common range for L*A*B*: L [0..100] S [-128..127] V [-128..127]

void XYZtoLAB(const long double &X, const long double &Y, const long double &Z, long double &L, long double &A, long double &B) // convert CIE XYZ value to CIE L*A*B*
{ // function checked OK with other calculators
    // reference white in XYZ
    long double ref_X = 0.95047;
    long double ref_Y = 1.0;
    long double ref_Z = 1.08883;
    // CIE values
    long double E = 216.0 / 24389.0;
    long double K = 24389.0 / 27.0;

    long double Xr = X / ref_X;
    long double Yr = Y / ref_Y;
    long double Zr = Z / ref_Z;

    long double fX, fY, fZ;
    if (Xr > E)
        fX = powl(Xr, 1.0 / 3.0);
    else
        fX = (K * Xr + 16.0) / 116.0;
    if (Yr > E)
        fY = powl(Yr, 1.0 / 3.0);
    else
        fY = (K * Yr + 16.0) / 116.0;
    if (Zr > E)
        fZ = powl(Zr, 1.0 / 3.0);
    else
        fZ = (K * Zr + 16.0) / 116.0;

    L = 116.0 * fY - 16.0;
    A = 500.0 * (fX - fY);
    B = 200.0 * (fY - fZ);

    L = L / 100.0; // to stay in [0..1] range
    A = A / 127.0;
    B = B / 127.0;
}

void LABtoXYZ(const long double &L, const long double &A, const long double &B, long double &X, long double &Y, long double &Z) // convert CIE L*A*B* to CIE XYZ
{ // function checked OK with other calculators
    // reference white in XYZ
    long double ref_X = 0.95047;
    long double ref_Y = 1.0;
    long double ref_Z = 1.08883;
    // CIE values
    long double E = 216.0 / 24389.0;
    long double K = 24389.0 / 27.0;

    long double fY = (L * 100.0 + 16.0) / 116.0;
    long double fZ = fY - B * 127.0 / 200.0;
    long double fX = A * 127.0 / 500.0 + fY;

    long double Xr, Yr, Zr;

    if (powl(fX, 3.0) > E)
        Xr = powl(fX, 3.0);
    else
        Xr = (116.0 * fX -16.0) / K;
    if (L * 100.0 > K * E)
        Yr = powl((L * 100.0 + 16.0) / 116.0, 3.0);
    else
        Yr = L * 100.0 / K;
    if (powl(fZ, 3.0) > E)
        Zr = powl(fZ, 3.0);
    else
        Zr = (116.0 * fZ -16.0) / K;

    X = Xr * ref_X;
    Y = Yr * ref_Y;
    Z = Zr * ref_Z;
}

//// CIE xyY
//// see https://en.wikipedia.org/wiki/CIE_1931_color_space#CIE_xy_chromaticity_diagram_and_the_CIE_xyY_color_space
//// All values [0..1]
//// Common range for xyY: x [0..1] y [0..1] Y [0..100]
//// Note that Y is exactly equal to Y in CIE XYZ

void XYZtoxyY(const long double &X, const long double &Y, const long double &Z, long double &x, long double &y) // convert CIE XYZ value to CIE xyY
{ // function checked OK with other calculators
    if ((X == 0) and (Y == 0) and (Z == 0)) { // D65 white point
        x = 0.3127;
        y = 0.3290;
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

//// L*u*v*
//// see https://en.wikipedia.org/wiki/CIELUV
//// All values [0..1]
//// Common range for L*u*v*: L* [0..100] u* [-134..220] v* [-140..122]

void XYZtoLuv(const long double &X, const long double &Y, const long double &Z, long double &L, long double &u, long double &v) // convert CIE XYZ value to CIE L*u*v*
{ // function checked OK with other calculators
    // reference white in XYZ
    long double ref_X = 0.95047;
    long double ref_Y = 1.0;
    long double ref_Z = 1.08883;
    // CIE values
    long double E = 216.0 / 24389.0;
    long double K = 24389.0 / 27.0;

    if (Y / ref_Y > E) // two-part equation
        L = 116.0 * powl(Y / ref_Y, 1.0 / 3.0) - 16.0;
    else
        L = K * Y / ref_Y;

    long double u_prime = 4.0 * X / (X + 15.0 * Y + 3.0 * Z); // intermediate calculus
    long double u_ref = 4.0 * ref_X / (ref_X + 15.0 * ref_Y + 3.0 * ref_Z);
    long double v_prime = 9.0 * Y / (X + 15.0 * Y + 3.0 * Z);
    long double v_ref = 9.0 * ref_Y / (ref_X + 15.0 * ref_Y + 3.0 * ref_Z);

    u = 13.0 * L * (u_prime - u_ref); // final values
    v = 13.0 * L * (v_prime - v_ref);

    L = L / 100.0; // to stay in range [0..1]
    u = u / 100.0;
    v = v / 100.0;
}

void LuvToXYZ(const long double &L, const long double &U, const long double &V, long double &X, long double &Y, long double &Z) // convert CIE L*u*v* value to CIE XYZ
{ // function checked OK with other calculators
    // reference white in XYZ
    long double ref_X = 0.95047;
    long double ref_Y = 1.0;
    long double ref_Z = 1.08883;
    // CIE values
    long double E = 216.0 / 24389.0;
    long double K = 24389.0 / 27.0;

    long double l = L * 100.0;
    long double u = U * 100.0;
    long double v = V * 100.0;

    long double u0 = 4.0 * ref_X / (ref_X + 15.0 * ref_Y + 3.0 * ref_Z); // white point intermediate values
    long double v0 = 9.0 * ref_Y / (ref_X + 15.0 * ref_Y + 3.0 * ref_Z);

    long double u_prime = u / (13.0 * l) + u0;
    long double v_prime = v / (13.0 * l) + v0;

    if (l > K * E)
        Y = ref_Y * powl((l + 16.0) / 116.0, 3);
    else
        Y = ref_Y * l * (powl(3.0 / 29.0, 3));

    X = Y * 9.0 * u_prime / 4.0 / v_prime;
    Z = Y * (12.0 - 3 * u_prime - 20 * v_prime) / 4.0 / v_prime;
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
        long double ref_X = 0.95047;
        long double ref_Y = 1.0;
        long double ref_Z = 1.08883;

        long double Ka = (175.0 / 198.04) * (ref_X + ref_Y); // CIE standard values VS white point
        long double Kb = ( 70.0f / 218.11) * (ref_Y + ref_Z);

        L = sqrtl(Y / ref_Y); // final values
        A = Ka * (((X / ref_X) - (Y / ref_Y)) / sqrtl(Y / ref_Y));
        B = Kb * (((Y / ref_Y) - (Z / ref_Z)) / sqrtl(Y / ref_Y));
    }
}

void HLABtoXYZ(const long double &L, const long double &A, const long double &B, long double &X, long double &Y, long double &Z) // convert from Hunter Lab to XYZ
{ // function checked OK with other calculators
    // reference white in XYZ
    long double ref_X = 0.95047;
    long double ref_Y = 1.0;
    long double ref_Z = 1.08883;

    long double Ka = (175.0 / 198.04) * (ref_Y + ref_X); // CIE standard values VS white point
    long double Kb = ( 70.0f / 218.11) * (ref_Y + ref_Z);

    Y = powl(L / ref_Y, 2); // final values
    X =  (A / Ka * sqrtl(Y / ref_Y) + (Y / ref_Y)) * ref_X;
    Z = -(B / Kb * sqrtl(Y / ref_Y) - (Y / ref_Y)) * ref_Z;

}

//// CIE LCHab
//// See https://en.wikipedia.org/wiki/CIELAB_color_space#Cylindrical_representation:_CIELCh_or_CIEHLC
//// All values [0..1]
//// Common range : L [0..100] C [0..100] h [0..360]
//// Note that L is exactly equal to L of CIE L*a*b*

void LABtoLCHab(const long double &A, const long double &B, long double &C, long double &H) // convert from LAB to HLC - L is the same so no need to convert
{ // function checked OK with other calculators
    C = sqrtl(powl(A * 127.0, 2) + powl(B * 127.0, 2)) / 100.0; // Chroma

    H = atan2l(B, A) / 2.0 / Pi; // Hue - cartesian to polar
    while (H < 0) // Hue in range [0..1]
        H += 1;
    while (H > 1)
        H -= 1;
}

void LCHabToLAB(const long double &C, const long double &H, long double &A, long double &B) // convert from HLC to LAB - L is the same so no need to convert
{ // function checked OK with other calculators
    A = C * cosl(H * 2.0 * Pi); // polar to cartesian
    B = C * sinl(H * 2.0 * Pi);
}

//// CIE LCHuv
//// See https://en.wikipedia.org/wiki/CIELUV#Cylindrical_representation_(CIELCH)
//// All values [0..1]
//// Common range : L [0..100] C [0..100] h [0..360]
//// Note that L is exactly equal to L of CIE Luv

void LuvToLCHuv(const long double &u, const long double &v, long double &C, long double &H) // convert from Luv to LCHuv - L is the same so no need to convert
{ // function checked OK with other calculators
    C = sqrtl(powl(u * 100.0, 2) + powl(v * 100.0, 2)) / 100.0; // Chroma

    H = atan2l(v, u) / 2.0 / Pi; // Hue - cartesian to polar
    while (H < 0) // Hue in range [0..1]
        H += 1;
    while (H > 1)
        H -= 1;
}

void LCHuvToLUV(const long double &C, const long double &H, long double &u, long double &v) // convert from LCHuv to LUV - L is the same so no need to convert
{ // function checked OK with other calculators
    u = C * cosl(H * 2.0 * Pi); // polar to cartesian
    v = C * sinl(H * 2.0 * Pi);
}

//// LMS
//// See https://en.wikipedia.org/wiki/LMS_color_space
//// All values [0..1]
//// Common range : no range specified

void XYZtoLMS(const long double &X, const long double &Y, const long double &Z, long double &L, long double &M, long double &S) // convert from XYZ to LMS
{ // couldn't find any online calculator to check this function, but it is pretty straightforward
    // CIECAM02 is the successor to CIECAM97s - the best matrix so far, just coordinates change
    L = 0.7328  * X + 0.4296 * Y - 0.1624 * Z;
    M = -0.7036 * X + 1.6975 * Y + 0.0061 * Z;
    S = 0.0030  * X + 0.0136 * Y + 0.9834 * Z;
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
    C = ClampCMYK((1 - R - K) / (1 - K));
    M = ClampCMYK((1 - G - K) / (1 - K));
    Y = ClampCMYK((1 - B - K) / (1 - K));
}

void CMYKtoRGB(const long double &C, const long double &M, const long double &Y, const long double &K, long double &R, long double &G, long double &B) // convert from CMYK to RGB
{
    R = (1 - C) * (1 - K);
    G = (1 - M) * (1 - K);
    B = (1 - Y) * (1 - K);

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
