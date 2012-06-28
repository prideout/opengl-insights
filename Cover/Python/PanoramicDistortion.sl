/****************************************************************************
 * panorama.sl
 *
 * Procedural lens to capture a panorama around the y-axis
 * from a location in world space.
 *
 * This can mimic both cylindrical (spherical = 0.)
 * and spherical (spherical = 1., ymax == ymin)
 * panoramic lenses.
 *
 * To avoid intersecting the scene, the point P is
 * cast from the axis rather than the surface of
 * the cylinder or sphere.
 *
 * The theta angles select a range of angles around
 * the y-axis which span [0, 1] in s.
 *
 * The fovy angles set the "field of view" in the y-direction.
 * For example, a fovylo = -15; fovyhi = 15 sets a centered
 * field-of-view of 30 degrees.
 *
 * When usendc is set, the parametric coordinates are overridden
 * by screen filling normalized device coordinates, allowing
 * any clip-plane object to look through the lens.
 *
 * Reference:
 *   _Advanced RenderMan: Creating CGI for Motion Picture_, 
 *   by Anthony A. Apodaca and Larry Gritz, Morgan Kaufmann, 1999.
 *
 ****************************************************************************/


/* Converted to an Imager shader by Philip Rideout */
/* I'm not sure why RiCurves look so bad with this. */
/* Might be one of the "Curve Dicing" attributes... */

class
PanoramicDistortion (
           color background = (0,0,0);
           float usendc = 0;
	   float spherical = 0;
	   float thetamin = 125-90.; // raise X to move to left
	   float thetamax = 125+90.;

	   float worldx = 0.;
	   float worldz = 0.;

	   float ymin = -1.5; // increase to lower the top crop line
	   float ymax = -2.0; // increase to lower the bottom crop line
	   float fovylo = -80;
	   float fovyhi = 80;    )
{
   public void imager(
          output varying color Ci;
          output varying color Oi;)
   {
    varying float ss = s;
    varying float tt = t;

    if (usendc != 0.0) {
	varying point P2 = transform("NDC", P);
	ss = xcomp(P2);
	tt = 1. - ycomp(P2);
    }
   
    varying float theta = radians(thetamin + ss * (thetamax - thetamin));

    varying float angley;
    if (spherical != 0.0) {
	angley = radians(fovylo + (fovyhi - fovylo) * tt);
    } else {
	uniform float sfovylo = sin(radians(fovylo));
	uniform float sfovyhi = sin(radians(fovyhi));
	varying float sfovyt = sfovylo + (sfovyhi - sfovylo) * tt;
	angley = asin(sfovyt);
    }
    
    varying float sy = sin(angley);
    varying float cy = cos(angley);
    varying vector It = vector "world" ( cos(theta) * cy, sy, sin(theta) * cy);
    varying point Pt = point "world" ( worldx, ymin + tt * (ymax - ymin), worldz);

    float d;
    Ci = (0,0,0);
    d = trace(Pt, It); if (d > 1000.0) Ci += background; else Ci += trace(Pt, It);

    // Vignette:
    float x = s - 0.5;
    float y = t - 0.5;
    d = x * x + y * y;
    Ci *= 1.0 - d;

}
}
