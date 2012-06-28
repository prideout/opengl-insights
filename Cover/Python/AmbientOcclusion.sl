class AmbientOcclusion(
                     string filename = "",
                     displaychannels = "",
                     coordsys = "";
                     color em = (1,0,1);
                     float samples = 64;        
                     float Ka = 0.0;
                     float Kd = 0.0;
                     float Ks = 0.0;
                     float roughness =.1;
                     color specularcolor = 1;
)
{
  public void surface(output color Ci, Oi)
  {
    normal Nf = normalize(faceforward(N, I));
    vector V = vector(0) - normalize(I);
    normal Nn = normalize(Nf);

    float occ = occlusion(P, Nn, samples,
                          "maxdist", 500.0);

    if (filename != "")
        bake3d(filename, displaychannels, P, N,
                         "coordsystem", coordsys,
                         "_occlusion", occ);

    color c = (em + Ka*ambient() + Kd*diffuse(Nf)) +
                        specularcolor * Ks * specular(Nf, V, roughness);

    Ci = (1 - occ) * Cs * c * Os;
    Oi = Os;
  }
}
