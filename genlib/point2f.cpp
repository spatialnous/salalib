// SPDX-FileCopyrightText: 2011-2012 Tasos Varoudis
// SPDX-FileCopyrightText: 2014-2025 Petros Koutsolampros
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "point2f.hpp"

bool Point2f::insegment(const Point2f &key, const Point2f &p2, const Point2f &p3,
                        double tolerance) {
    Point2f va = p2 - key;
    Point2f vb = p3 - key;
    Point2f vp = *this - key;
    double ap = va.det(vp);
    double bp = vb.det(vp);
    if ((va.dot(vp) > 0 && vb.dot(vp) > 0) &&
        (pafmath::sgn(ap) != pafmath::sgn(bp) || fabs(ap) < tolerance || fabs(bp) < tolerance)) {
        return true;
    }
    return false;
}

bool Point2f::intriangle(const Point2f &p1, const Point2f &p2, const Point2f &p3) {
    // touching counts
    int test = pafmath::sgn((p2 - p1).det(*this - p1));
    if (test == pafmath::sgn((p3 - p2).det(*this - p2)) &&
        test == pafmath::sgn((p1 - p3).det(*this - p3))) {
        return true;
    }
    return false;
}

// First time we have a region available to use...
void Point2f::normalScale(const Point2f &rbl, double width, double height) {
    if (width != 0)
        x = (x - rbl.x) / width;
    else
        x = 0.0;
    if (height != 0)
        y = (y - rbl.y) / height;
    else
        y = 0.0;
}

void Point2f::denormalScale(const Point2f &rbl, double width, double height) {
    x = x * width + rbl.x;
    y = y * height + rbl.y;
}

// TODO: Unused function
// gps2os: function to convert long-lat GPS coordinates to OS national grid

// n.b.: approximation only

// This algorithm is taken from:

// "A guide to coordinate systems in Great Britain"
// http://www.ordnancesurvey.co.uk/oswebsite/gps/information/coordinatesystemsinfo/guidecontents/index.html
// (v1.9 Mar 2008 D00659, Crown Copyright)
// Sourced: 21-Mar-08

// It's truly ick... and nuts... there must be an easier way...

// Outline:
// 1. take long-lat on ETRS89 ellipsoid and convert to 3d cartesian coordinates
// 2. shift 3d cartesian coordinates from ETRS89 ellipsoid to OSGB36 ellipsoid
// 3. convert 3d cartesian coordinates to long-lat on OSGB36 ellipsoid
// 4. project onto OSFB36 2d grid using a transverse Mercator projection

// According to OS, accurate to within about 5 metres

Point2f Point2f::gps2os() const {
    // *first*, we have ETRS89 data...

    // Convert it to 3D Cartesian Coordinates
    double lambda = M_PI * x / 180.0;
    double phi = M_PI * y / 180.0;

    // GRS80 ellipsoid
    double a = 6378137.0000;
    double b = 6356752.3141;
    double eSq = (pafmath::sqr(a) - pafmath::sqr(b)) / pafmath::sqr(a);

    double nu = a / sqrt(1.0 - eSq * pafmath::sqr(sin(phi)));

    double lx = nu * cos(phi) * cos(lambda);
    double ly = nu * cos(phi) * sin(lambda);
    double lz = (1 - eSq) * nu * sin(phi);

    // Now we have the ETRS89 location, convert it to a rough OSGB36 location:

    // rough conversion chart

    // t_x (m)     t_y (m)   t_z (m)    s (ppm)    r_x (sec)  r_y (sec)  r_z (sec)
    // -446.448    +125.157  -542.060   +20.4894   -0.1502    -0.2470    -0.8421 = (in radians: )

    // nb, seconds converted to radians:
    double rX = -0.7281901490265230623720509817416e-6;
    double rY = -1.1974897923405539041670878328241e-6;
    double rZ = -4.0826160086234026020206666559563e-6;

    lx = -446.448 + (1.0 + 2.04894e-5) * lx - rZ * ly + rY * lz;
    ly = +125.157 + rZ * lx + (1.0 + 2.04894e-5) * ly - rX * lz;
    lz = -542.060 - rY * lx + rX * ly + (1.0 + 2.04894e-5) * lz;

    double p = sqrt(pafmath::sqr(lx) + pafmath::sqr(y));

    // now place it back in long lat on the OSGB36 ellipsoid:

    // Airy 1830 (OSGB36) ellipsoid
    a = 6377563.396;
    b = 6356256.910;
    eSq = (pafmath::sqr(a) - pafmath::sqr(b)) / pafmath::sqr(a);

    lambda = atan(ly / lx);
    phi = atan(lz / (p * (1.0 - eSq)));
    double lastphi = phi;

    nu = a / sqrt(1.0 - eSq * pafmath::sqr(sin(phi)));
    do {
        phi = atan((lz + eSq * nu * sin(phi)) / p);
    } while (fabs(lastphi - phi) > 1e-6);

    // now, it's on the ellipsoid, project it onto the OSGB36 grid:

    // E_0 easting of true origin                400 000m
    double e0 = 400000;
    // N_0 northing of true origin              -100 000m
    double n0 = -100000;
    // F_0 scaling factor on central meridian    0.9996012717
    double f0 = 0.9996012717;
    // lambda_0 longitude of true origin         -2.0 radians: -0.034906585039886591538473815369772
    double lambda0 = -0.034906585039886591538473815369772;
    // phi_0 latitude of true origin             49.0 radians:
    double phi0 = 0.85521133347722149269260847655942;

    nu = a * f0 * pow((1 - eSq * pafmath::sqr(sin(phi))), -0.5);

    double n = (a - b) / (a + b);
    double rho = a * f0 * (1.0 - eSq) * pow((1 - eSq * pafmath::sqr(sin(phi))), -1.5);
    double etaSq = nu / rho - 1;

    double nSq = pow(n, 2);
    double nCubed = pow(n, 3);
    double m =
        b * f0 *
        ((1.0 + n + 0.25 * 5 * (nSq + nCubed)) * (phi - phi0) -
         (3.0 * (n + nSq + 0.125 * 7 * nCubed)) * sin(phi - phi0) * cos(phi + phi0) +
         (0.125 * 15.0 * (nSq + nCubed)) * sin(2.0 * (phi - phi0)) * cos(2.0 * (phi + phi0)) -
         (35.0 / 24.0 * nCubed) * sin(3.0 * (phi - phi0)) * cos(3.0 * (phi + phi0)));
    double i = m + n0;
    double ii = 0.5 * nu * sin(phi) * cos(phi);
    double tanphi = tan(phi);
    double iii =
        nu * sin(phi) * pow(cos(phi), 3.0) * (5.0 - pafmath::sqr(tanphi) + 9.0 * etaSq) / 24.0;
    double iiia = nu * sin(phi) * pow(cos(phi), 5.0) *
                  (61.0 - 58.0 * pafmath::sqr(tanphi) + pow(tanphi, 4.0)) / 720.0;
    double iv = nu * cos(phi);
    double v = nu * pow(cos(phi), 3.0) * (nu / rho - pafmath::sqr(tanphi)) / 6.0;
    double vi = nu * pow(cos(phi), 5.0) *
                (5.0 - 18.0 * pafmath::sqr(tanphi) + pow(tanphi, 4) + 14.0 * etaSq -
                 58.0 * pafmath::sqr(tanphi) * etaSq) /
                120.0;

    double e = e0 + iv * (lambda - lambda0) + v * pow((lambda - lambda0), 3) +
               vi * pow((lambda - lambda0), 5);
    double nn = i + ii * pow((lambda - lambda0), 2) + iii * pow((lambda - lambda0), 4) +
                iiia * pow((lambda - lambda0), 6);

    return Point2f(e, nn);
}
