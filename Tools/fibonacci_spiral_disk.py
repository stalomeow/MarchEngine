import numpy as np

# https://people.irisa.fr/Ricardo.Marques/articles/2013/SF_CGF.pdf
# 论文里的算法是三维的，我们只需要 XOY 平面上的投影

def get_points1(n):
    delta_phi = np.pi * (3 - np.sqrt(5))
    points = []
    for i in range(n):
        phi = np.fmod(i * delta_phi, 2 * np.pi)
        points.append((np.cos(phi), np.sin(phi)))
    return ',\n'.join(map(lambda p: f"float2({p[0]}, {p[1]})", points))

# 提前给 delta_phi 除以 2 * np.pi 的版本
def get_points2(n):
    delta_phi = (3 - np.sqrt(5)) * 0.5
    points = []
    for i in range(n):
        phi = np.modf(i * delta_phi)[0] * 2 * np.pi
        points.append((np.cos(phi), np.sin(phi)))
    return ',\n'.join(map(lambda p: f"float2({p[0]}, {p[1]})", points))

if __name__ == '__main__':
    print(get_points2(64))
