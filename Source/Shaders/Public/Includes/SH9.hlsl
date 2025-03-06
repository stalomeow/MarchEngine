#ifndef _SH9_INCLUDED
#define _SH9_INCLUDED

// Ref: https://github.com/Unity-Technologies/Graphics/blob/master/Packages/com.unity.render-pipelines.core/ShaderLibrary/SphericalHarmonics.hlsl
// Ref: https://en.wikipedia.org/wiki/Table_of_spherical_harmonics#Real_spherical_harmonics
// Ref: https://orlandoaguilar.github.io/sh/spherical/harmonics/irradiance/map/2017/02/12/SphericalHarmonics.html

// SH Basis coefs
#define SHBasis0 0.28209479177387814347f // {0, 0} : 1/2 * sqrt(1/Pi)
#define SHBasis1 0.48860251190291992159f // {1, 0} : 1/2 * sqrt(3/Pi)
#define SHBasis2 1.09254843059207907054f // {2,-2} : 1/2 * sqrt(15/Pi)
#define SHBasis3 0.31539156525252000603f // {2, 0} : 1/4 * sqrt(5/Pi)
#define SHBasis4 0.54627421529603953527f // {2, 2} : 1/4 * sqrt(15/Pi)

// Cosine coefs
#define SHCosine0 3.14159265358979323846f // Pi
#define SHCosine1 2.09439510239319549231f // 2/3 * Pi
#define SHCosine2 0.78539816339744830962f // 1/4 * Pi

void GetSH9Basis(float3 N, out float basis[9])
{
    basis[0] = SHBasis0;
    basis[1] = SHBasis1 * N.y;
    basis[2] = SHBasis1 * N.z;
    basis[3] = SHBasis1 * N.x;
    basis[4] = SHBasis2 * N.x * N.y;
    basis[5] = SHBasis2 * N.y * N.z;
    basis[6] = SHBasis3 * (3.0 * N.z * N.z - 1.0);
    basis[7] = SHBasis2 * N.x * N.z;
    basis[8] = SHBasis4 * (N.x * N.x - N.y * N.y);
}

float3 ConvolveSH9WithCosine(int basisIndex, float3 coef)
{
    static const float cosineCoefs[9] =
    {
        SHCosine0,
        SHCosine1,
        SHCosine1,
        SHCosine1,
        SHCosine2,
        SHCosine2,
        SHCosine2,
        SHCosine2,
        SHCosine2,
    };

    return cosineCoefs[basisIndex] * coef;
}

float3 SampleSH9(StructuredBuffer<float3> coefs, float3 N)
{
    float basis[9];
    GetSH9Basis(N, basis);

    float3 result = 0;

    [unroll]
    for (int i = 0; i < 9; i++)
    {
        result += coefs[i] * basis[i];
    }

    return result;
}

#endif
