import numpy as np

def gaussian(sigma, x):
    return np.exp(-x**2 / (2.0 * sigma**2))

def print_weights(sigma, r):
    w = np.array([gaussian(sigma, x) for x in range(-r, r+1)])
    print(',\n'.join(map(str, (w / np.sum(w)))))

if __name__ == '__main__':
    print_weights(2.0, 5)
