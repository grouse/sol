#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using i32 = signed int;

using f32 = float;

#define F32_MAX 3.402823466e+38F
#define U32_MAX 0xFFFFFFFF

#define MAX(a, b) (a) > (b) ? (a) : (b)

#ifdef _WIN32
#define PACKED(decl) __pragma(pack(push, 1)) decl __pragma(pack(pop))
#else
#define PACKED(decl) decl __attribute__((__packed__))
#endif

PACKED(struct BitmapFileHeader {
    u16 bmp_type;
    u32 size;
    u16 reserved0;
    u16 reserved1;
    u32 offset;
});

PACKED(struct BitmapHeader {
    u32 header_size;
    i32 width;
    i32 height;
    u16 planes;
    u16 bpp;
    u32 compression;
    u32 bmp_size;
    i32 res_horiz;
    i32 res_vert;
    u32 colors_used;
    u32 colors_important;
});

struct RandomSeries {
    u32 state;
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
    Vector3 n;
    f32 d;
    i32 material;
};

struct Sphere {
    Vector3 p;
    f32 r;
    i32 material;
};

struct Material {
    Vector3 emit;
    Vector3 reflect;
    f32 specularity;
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

Vector3& operator+=(Vector3 &lhs, f32 rhs)
{
    lhs.x += rhs;
    lhs.y += rhs;
    lhs.z += rhs;
    return lhs;
}

Vector3 operator-(Vector3 v)
{
    return { -v.x, -v.y, -v.z };
}

Vector3& operator+=(Vector3 &lhs, Vector3 rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    return lhs;
}

Vector3 operator/(Vector3 self, f32 rhs)
{
    return Vector3{ self.x / rhs, self.y / rhs, self.z / rhs };
}

f32 dot(Vector3 lhs, Vector3 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

f32 lerp(f32 a, f32 b, f32 t)
{
    return (1.0f-t) * a + t*b;
}

Vector3 lerp(Vector3 a, Vector3 b, f32 t)
{
    Vector3 r;

    r.x = lerp(a.x, b.x, t);
    r.y = lerp(a.y, b.y, t);
    r.z = lerp(a.z, b.z, t);

    return r;
}

Vector3 hadamard(Vector3 lhs, Vector3 rhs)
{
    return Vector3{ lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
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
    return sqrtf(dot(v, v));
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
        r = v * (1.0f / (f32)sqrt(len_sq));
    }

    return r;
}

f32 sRGB_from_linear(f32 l)
{
    if (l > 1.0f) {
        return 1.0f;
    } else if (l < 0.0f) {
        return 0.0f;
    }

    f32 s = l*12.92f;
    if (l > 0.0031308f) {
        s = 1.055f*powf(l, 1.0f/2.4f) - 0.055f;
    }

    return s;
}

u32 BGRA8_pack(Vector3 v)
{
    Vector3 c = 255.0f * v;
    return (u32)c.z | (u32)c.y << 8 | (u32)c.x << 16 | 255 << 24;
}

f32 rand_f32_uni(RandomSeries *r)
{
    u32 s = r->state;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    r->state = s;

    return (f32)(s >> 1) / (f32)(U32_MAX >> 1);
}

f32 rand_f32_bi(RandomSeries *r)
{
    return -1.0f + 2.0f * rand_f32_uni(r);
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    i32 width = 1280;
    i32 height = 720;

    i32 pixels_size = width*height*sizeof(u32);
    u32 *pixels = (u32*)malloc(pixels_size);
    memset(pixels, 0x000000FF, pixels_size);

    Material materials[] = {
        //        emit                         reflect                      specularity
        Material{ Vector3{ 0.4f, 0.4f, 0.9f }, Vector3{ 0.0f, 0.0f, 0.0f }, 0.0f },
        Material{ Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.3f, 0.9f, 0.3f }, 0.0f },
        Material{ Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.2f, 0.2f, 0.2f }, 0.0f },
        Material{ Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.8f, 0.95f, 0.8f }, 0.8f },
        Material{ Vector3{ 5.0f, 1.0f, 1.0f }, Vector3{ 0.0f, 0.0f, 0.0f }, 0.0f },
    };

    Plane planes[] = {
        //     position                     normal   material
        Plane{ Vector3{ 0.0f, 1.0f, 0.0f }, 0.0f,    1 },
    };

    Sphere spheres[] = {
        //      position                     radius   material
        Sphere{ Vector3{ 0.0f, 0.0f, 0.0f }, 1.0f,    2 },
        Sphere{ Vector3{ 3.0f, 0.0f, 2.0f }, 1.0f,    3 },
        Sphere{ Vector3{ 2.5f, 2.0f, -5.0f }, 1.0f,   4 },
    };

    i32 planes_count  = sizeof planes / sizeof planes[0];
    i32 spheres_count = sizeof spheres / sizeof spheres[0];

    Vector3 camera_p = Vector3{ 0.0f, 2.0f, 10.0f };
    Vector3 camera_z = normalise_zero(camera_p);
    Vector3 camera_y = normalise_zero(cross(camera_z, Vector3{ 1.0f, 0.0f, 0.0f }));
    Vector3 camera_x = normalise_zero(cross(camera_y, camera_z));

    f32 film_d = 1.0f;
    f32 film_w = 1.0f;
    f32 film_h = 1.0f;

    if (width > height) {
        film_h = film_w * (f32)height / (f32)width;
    } else if (height > width) {
        film_w = film_h * (f32)width / (f32)height;
    }

    f32 film_half_w = 0.5f * film_w;
    f32 film_half_h = 0.5f * film_h;
    Vector3 film_c = camera_p - film_d*camera_z;

    f32 half_pixel_w = 0.5f / width;
    f32 half_pixel_h = 0.5f / height;

    f32 tolerance = 0.0001f;
    f32 min_hit_distance = 0.001f;

    i32 max_ray_bounce = 4;
    i32 rays_per_pixel = 16;
    f32 inv_rays_per_pixel = 1.0f / rays_per_pixel;
    RandomSeries random_series = { 23528812 };



    for (i32 i = 0; i < height; i++) {
        f32 film_y = -1.0f + 2.0f*((f32)i / (f32)height);

        for (i32 j = 0; j < width; j++) {
            f32 film_x = -1.0f + 2.0f*((f32)j / (f32)width);

            f32 off_x = film_x + half_pixel_w;
            f32 off_y = film_y + half_pixel_h;

            Vector3 film_p = film_c +
                off_x*film_half_w*camera_x +
                off_y*film_half_h*camera_y;

            Vector3 final_color = {};

            for (i32 k = 0; k < rays_per_pixel; k++) {
                Vector3 ray_o = camera_p;
                Vector3 ray_d = normalise_zero(film_p - camera_p);

                Vector3 color = {};
                Vector3 attenuation = { 1.0f, 1.0f, 1.0f };

                Vector3 next_ray_n;
                for (i32 l = 0; l < max_ray_bounce; l++) {
                    i32 hit_mat = 0;
                    f32 hit_d   = F32_MAX;

                    for (i32 p = 0; p < planes_count; p++) {
                        Plane plane = planes[p];

                        f32 denom = dot(plane.n, ray_d);
                        if (denom > tolerance || denom < -tolerance) {
                            f32 t = (-plane.d - dot(plane.n, ray_o)) / denom;
                            if (t > min_hit_distance && t < hit_d) {
                                hit_d   = t;
                                hit_mat = plane.material;
                                next_ray_n = plane.n;
                            }
                        }
                    }

                    for (i32 s = 0; s < spheres_count; s++) {
                        Sphere sphere = spheres[s];

                        Vector3 l = ray_o - sphere.p;
                        f32 a = dot(ray_d, ray_d);
                        f32 b = 2.0f * dot(ray_d, l);
                        f32 c = dot(l, l) - sphere.r * sphere.r;

                        f32 root_term = b*b - 4.0f*a*c;
                        f32 denom = 2.0f * a;

                        if (root_term >= 0.0f &&
                            (denom > tolerance || denom < -tolerance))
                        {
                            f32 root = sqrtf(root_term);
                            f32 tp = (-b + root) / denom;
                            f32 tn = (-b - root) / denom;

                            f32 t = tp;
                            if (tn > min_hit_distance && tn < tp) {
                                t = tn;
                            }

                            if (t > min_hit_distance && t < hit_d) {
                                hit_d   = t;
                                hit_mat = sphere.material;
                                next_ray_n = normalise_zero(t*ray_d + l);
                            }
                        }
                    }

                    Material mat = materials[hit_mat];
                    color += hadamard(attenuation, mat.emit);

                    if (hit_mat != 0) {
                        f32 cos_attenuation = dot(-ray_d, next_ray_n);
                        cos_attenuation = MAX(cos_attenuation, 0.0f);

                        attenuation = hadamard(attenuation, cos_attenuation*mat.reflect);

                        ray_o = ray_o + ray_d * hit_d;

                        f32 x = rand_f32_bi(&random_series);
                        f32 y = rand_f32_bi(&random_series);
                        f32 z = rand_f32_bi(&random_series);
                        Vector3 rvec = Vector3{ x, y, z };

                        Vector3 pure_bounce = ray_d - 2.0f * dot(ray_d, next_ray_n) * next_ray_n;
                        Vector3 random_bounce = normalise_zero(next_ray_n + rvec);

                        ray_d = normalise_zero(lerp(random_bounce, pure_bounce, mat.specularity));
                    } else {
                        break;
                    }
                }

                final_color += color * inv_rays_per_pixel;
            }

            Vector3 srgb = Vector3{
                sRGB_from_linear(final_color.x),
                sRGB_from_linear(final_color.y),
                sRGB_from_linear(final_color.z)
            };

            pixels[(i*width + j)] = BGRA8_pack(srgb);
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
