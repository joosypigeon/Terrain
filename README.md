3D torus.

Compiled against raylib.

gcc -fopenmp -o terrain src/*.c -Wall -std=c99 -D_DEFAULT_SOURCE -Wno-missing-braces -Wunused-result -O2 -D_DEFAULT_SOURCE -I. -I/home/jerry/raylib/src -I/home/jerry/raylib/src/external -I/usr/local/include -I/home/jerry/raylib/src/external/glfw/include -L. -L/home/jerry/raylib/src -L/home/jerry/raylib/src -L/usr/local/lib -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -latomic -DPLATFORM_DESKTOP -DPLATFORM_DESKTOP_GLFW
