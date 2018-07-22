#include <stdio.h>
#include <stdlib.h>
#include <math.h>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using i32 = signed int;

using f32 = float;

#define F32_MAX 3.402823466e+38F

#define PACKED(decl) decl __attribute__((__packed__))

PACKED(struct BitmapFileHeader {
    u16 bmp_type  ;
    u32 size      ;
    u16 reserved0 ;
    u16 reserved1 ;
    u32 offset    ;
});

PACKED(struct BitmapHeader {
    u32 header_size      ;
    i32 width            ;
    i32 height           ;
    u16 planes           ;
    u16 bpp              ;
    u32 compression      ;
    u32 bmp_size         ;
    i32 res_horiz        ;
    i32 res_vert         ;
    u32 colors_used      ;
    u32 colors_important ;
});

struct BGRA8 {
    u8 b;
    u8 g;
    u8 r;
    u8 a;
};

struct Vector3 {
    f32 x;
    f32 y;
    f32 z;
};

struct Vector2 {
    f32 x;
    f32 y;
};

struct Plane {
    Vector3 n ;
    f32 d ;
};

struct Sphere {
    Vector3 p ;
    f32 r ;
};

Vector3 operator*(Vector3 self, f32 rhs)
{
    return Vector3{ self.x * rhs, self.y * rhs, self.z * rhs };
}

Vector3 operator*(f32 self, Vector3 rhs)
{
    return Vector3{ self * rhs.x, self * rhs.y, self * rhs.z };
}

Vector3 operator-(Vector3 self, Vector3 rhs)
{
    return Vector3{ self.x - rhs.x, self.y - rhs.y, self.z - rhs.z };
}

Vector3 operator-(Vector3 self, f32 rhs)
{
    return Vector3{ self.x - rhs, self.y - rhs, self.z - rhs };
}

Vector3 operator+(Vector3 self, Vector3 rhs)
{
    return Vector3{ self.x + rhs.x, self.y + rhs.y, self.z + rhs.z };
}

Vector3 operator/(Vector3 self, f32 rhs)
{
    return Vector3{ self.x / rhs, self.y / rhs, self.z / rhs };
}

f32 dot(Vector3 lhs, Vector3 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * lhs.z;
}

Vector3 cross(Vector3 lhs, Vector3 rhs)
{
    return Vector3 {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x,
    };
}

f32 length(Vector3 v)
{
    return sqrt(dot(v, v));
}

f32 length_sq(Vector3 v)
{
    return dot(v, v);
}

Vector3 normalise(Vector3 v)
{
    return v / length(v);
}

Vector3 normalise_zero(Vector3 v)
{
    Vector3 r = {};

    f32 len_sq = length_sq(v);
    if (len_sq > (0.0001f * 0.0001f)) {
        r = v * (1.0f / sqrt(len_sq));
    }

    return r;
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    i32 width = 1920;
    i32 height = 1080;

    i32 pixels_size = width*height*sizeof(BGRA8);
    BGRA8 *pixels = (BGRA8*)malloc(pixels_size);

    for (i32 i = 0; i < height*width; i++) {
        auto pixel = BGRA8{ 0, 0, 0, 255 };
        pixels[i] = pixel;
    }

    Plane plane = Plane{
        Vector3{ 0.0, 0.0, 1.0 },
        0.0
    };

    Sphere sphere = Sphere{
        Vector3{ 0.0, 0.0f, -1.0f },
        1.0f
    };

    Vector3 camera_p = Vector3{ 0.0f, -10.0f, 1.0f };
    Vector3 camera_z = normalise_zero(camera_p);
    Vector3 camera_x = normalise_zero(cross(Vector3{ 0.0f, 0.0f, 1.0f}, camera_z));
    Vector3 camera_y = normalise_zero(cross(camera_z, camera_x));

    f32 film_d = 1.0f;
    f32 film_w = 1.0f;
    f32 film_h = 1.0f;
    f32 film_half_w = 0.5f * film_w;
    f32 film_half_h = 0.5f * film_h;
    Vector3 film_c = camera_p - film_d*camera_z;

    for (i32 i = 0; i < height; i++) {
        f32 film_y = -1.0f + 2.0f*((f32)i / (f32)height);

        for (i32 j = 0; j < width; j++) {
            f32 film_x = -1.0f + 2.0f*((f32)j / (f32)width);

            Vector3 film_p = film_c +
                film_x*film_half_w*camera_x +
                film_y*film_half_h*camera_y;

            Vector3 ray_o = camera_p;
            Vector3 ray_d = normalise_zero(film_p - camera_p);

            i32 mat   = 0;
            f32 hit_d = F32_MAX;

            f32 tolerance = 0.0001f;
            f32 min_hit_distance = 0.001f;

            // planes
            {
                f32 denom = dot(plane.n, ray_d);
                if ((denom < -tolerance) || (denom > tolerance)) {
                    f32 t = (-plane.d - dot(plane.n, ray_o)) / denom;
                    if (t > min_hit_distance && t < hit_d) {
                        hit_d = t;
                        mat   = 1;
                        pixels[(i*width + j)] = BGRA8{ 0, 0, 255, 255 };
                    }
                }
            }

            // spheres
            if (false) {
                //Vector3 l = sphere.p - ray_o;
                Vector3 l = ray_o - sphere.p;
                f32 a = dot(ray_d, ray_d);
                f32 b = 2.0f * dot(ray_d, l);
                f32 c = dot(l, l) - sphere.r * sphere.r;

                f32 root_term = sqrt(b*b - 4.0f*a*c);
                f32 denom = 2.0f * a;

                if (root_term > 0.0) {
                    f32 tp = (-b + root_term) / denom;
                    f32 tn = (-b - root_term) / denom;

                    f32 t = tp;
                    if (tn > 0.0f && tn < tp) {
                        t = tn;
                    }

                    if (t > 0.0f && t < hit_d) {
                        hit_d = t;
                        mat   = 2;
                    }
                }
            }

            // TODO(jesper): move into material array
            if (mat == 2) {
                pixels[(i*width + j)] = BGRA8{ 255, 0, 0, 255 };
            } else if (mat == 1) {
                pixels[(i*width + j)] = BGRA8{ 0, 0, 255, 255 };
            } else {
                pixels[(i*width + j)] = BGRA8{ 50, 50, 50, 255 };
            }
        }
    }

    auto bmfh = BitmapFileHeader{
        0x4d42,
        (u32)(sizeof(BitmapFileHeader) + sizeof(BitmapHeader) + pixels_size),
        0,
        0,
        (u32)(sizeof(BitmapFileHeader) + sizeof(BitmapHeader))
    };

    auto bmh = BitmapHeader{
        sizeof(BitmapHeader),
        width,
        height,
        1,
        32,
        0,
        0,
        width,
        height,
        0,
        0,
    };

    FILE *f = fopen("test.bmp", "wb");
    fwrite(&bmfh, sizeof bmfh, 1, f);
    fwrite(&bmh, sizeof bmh, 1, f);
    fwrite(pixels, pixels_size, 1, f);
    fflush(f);

    return 0;
}
