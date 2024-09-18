//////////////////////////////////////////////////
//                                              //
// Emu64                                        //
// von Thorsten Kattanek                        //
//                                              //
// #file: mos6581_8085_calc.h                   //
//                                              //
// Dieser Sourcecode ist Copyright geschützt!   //
// Geistiges Eigentum von Th.Kattanek           //
//                                              //
// Letzte Änderung am 18.05.2014                //
// www.emu64.de                                 //
//                                              //
//////////////////////////////////////////////////

#ifndef MOS_6581_8085_CALC_H
#define MOS_6581_8085_CALC_H

#if SPLINE_BRUTE_FORCE
#define interpolate_segment interpolate_brute_force
#else
#define interpolate_segment interpolate_forward_difference
#endif

static void PointPlotter(int* f, double x, double y)
{
    // Clamp negative values to zero.
    if (y < 0) {
      y = 0;
    }
    f[(int)x] = (int)y;
}

static void interpolate_forward_difference(
            double x1, double y1, double x2, double y2,
				    double k1, double k2,
				    int* plot, double res)
{
  double a, b, c, d;
  double dx = x2 - x1, dy = y2 - y1;

  a = ((k1 + k2) - 2*dy/dx)/(dx*dx);
  b = ((k2 - k1)/dx - 3*(x1 + x2)*a)/2;
  c = k1 - (3*x1*a + 2*b)*x1;
  d = y1 - ((x1*a + b)*x1 + c)*x1;
  
  double y = ((a*x1 + b)*x1 + c)*x1 + d;
  dy = (3*a*(x1 + res) + 2*b)*x1*res + ((a*res + b)*res + c)*res;
  double d2y = (6*a*(x1 + res) + 2*b)*res*res;
  double d3y = 6*a*res*res*res;
    
  // Calculate each point.
  for (double x = x1; x <= x2; x += res) {
    PointPlotter(plot, x, y);
    y += dy; dy += d2y; d2y += d3y;
  }
}

#define x(p) (*p[0])
#define y(p) (*p[1])

static void interpolate(fc_point* p0, fc_point* pn, int* plot, double res)
{
  double k1, k2;

  // Set up points for first curve segment.
  fc_point* p1 = p0; ++p1;
  fc_point* p2 = p1; ++p2;
  fc_point* p3 = p2; ++p3;

  // Draw each curve segment.
  for (; p2 != pn; ++p0, ++p1, ++p2, ++p3) {
    // p1 and p2 equal; single point.
    if (x(p1) == x(p2)) {
      continue;
    }
    // Both end points repeated; straight line.
    if (x(p0) == x(p1) && x(p2) == x(p3)) {
      k1 = k2 = (y(p2) - y(p1))/(x(p2) - x(p1));
    }
    // p0 and p1 equal; use f''(x1) = 0.
    else if (x(p0) == x(p1)) {
      k2 = (y(p3) - y(p1))/(x(p3) - x(p1));
      k1 = (3*(y(p2) - y(p1))/(x(p2) - x(p1)) - k2)/2;
    }
    // p2 and p3 equal; use f''(x2) = 0.
    else if (x(p2) == x(p3)) {
      k1 = (y(p2) - y(p0))/(x(p2) - x(p0));
      k2 = (3*(y(p2) - y(p1))/(x(p2) - x(p1)) - k1)/2;
    }
    // Normal curve.
    else {
      k1 = (y(p2) - y(p0))/(x(p2) - x(p0));
      k2 = (y(p3) - y(p1))/(x(p3) - x(p1));
    }

    interpolate_segment(x(p1), y(p1), x(p2), y(p2), k1, k2, plot, res);
  }
}
#undef x
#undef y

#endif // MOS_6581_8085_CALC_H
