#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using i32 = signed int;

using f32 = float;

#define F32_MAX 3.402823466e+38F
#define U32_MAX 0xFFFFFFFF

#define MAX(a, b) (a) > (b) ? (a) : (b)
#if defined(__linux__)
#include "linux_sol.cpp"
#elif defined(_WIN32)
#include "win32_sol.cpp"
#else
#error "unsupported platform"
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

struct Tile {
    i32 start_x, end_x;
    i32 start_y, end_y;
};

struct World {
    Plane *planes;
    i32 planes_count;

    Sphere *spheres;
    i32 spheres_count;

    Material *materials;
    i32 materials_count;
};

struct Settings {
    f32 tolerance;
    f32 min_hit_distance;
    i32 max_ray_bounce;
    i32 rays_per_pixel;
    f32 inv_rays_per_pixel;
};

struct Camera {
    f32 half_pixel_w;
    f32 half_pixel_h;
    f32 film_half_w;
    f32 film_half_h;

    Vector3 film_c;

    Vector3 p;
    Vector3 x_axis;
    Vector3 y_axis;
    Vector3 z_axis;
};

struct Image {
    i32 width;
    i32 height;
    u32 *pixels;
};

void ray_cast(
    Tile tile,
    Image image,
    Camera camera,
    World world,
    Settings settings,
    RandomSeries random_series)
{
    for (i32 i = tile.start_y; i < tile.end_y; i++) {
        f32 film_y = -1.0f + 2.0f*((f32)i / (f32)image.height);

        for (i32 j = tile.start_x; j < tile.end_x; j++) {
            f32 film_x = -1.0f + 2.0f*((f32)j / (f32)image.width);

            f32 off_x = film_x + camera.half_pixel_w;
            f32 off_y = film_y + camera.half_pixel_h;

            Vector3 film_p = camera.film_c +
                off_x*camera.film_half_w*camera.x_axis +
                off_y*camera.film_half_h*camera.y_axis;

            Vector3 final_color = {};

            for (i32 k = 0; k < settings.rays_per_pixel; k++) {
                Vector3 ray_o = camera.p;
                Vector3 ray_d = normalise_zero(film_p - camera.p);

                Vector3 color = {};
                Vector3 attenuation = { 1.0f, 1.0f, 1.0f };

                Vector3 next_ray_n = {};
                for (i32 l = 0; l < settings.max_ray_bounce; l++) {
                    i32 hit_mat = 0;
                    f32 hit_d   = F32_MAX;

                    for (i32 p = 0; p < world.planes_count; p++) {
                        Plane plane = world.planes[p];

                        f32 denom = dot(plane.n, ray_d);
                        if (denom > settings.tolerance ||
                            denom < -settings.tolerance)
                        {
                            f32 t = (-plane.d - dot(plane.n, ray_o)) / denom;
                            if (t > settings.min_hit_distance && t < hit_d) {
                                hit_d   = t;
                                hit_mat = plane.material;
                                next_ray_n = plane.n;
                            }
                        }
                    }

                    for (i32 s = 0; s < world.spheres_count; s++) {
                        Sphere sphere = world.spheres[s];

                        Vector3 o = ray_o - sphere.p;
                        f32 a = dot(ray_d, ray_d);
                        f32 b = 2.0f * dot(ray_d, o);
                        f32 c = dot(o, o) - sphere.r * sphere.r;

                        f32 root_term = b*b - 4.0f*a*c;
                        f32 denom = 2.0f * a;

                        if (root_term >= 0.0f &&
                            (denom > settings.tolerance ||
                             denom < -settings.tolerance))
                        {
                            f32 root = sqrtf(root_term);
                            f32 tp = (-b + root) / denom;
                            f32 tn = (-b - root) / denom;

                            f32 t = tp;
                            if (tn > settings.min_hit_distance && tn < tp) {
                                t = tn;
                            }

                            if (t > settings.min_hit_distance && t < hit_d) {
                                hit_d   = t;
                                hit_mat = sphere.material;
                                next_ray_n = normalise_zero(t*ray_d + o);
                            }
                        }
                    }

                    Material mat = world.materials[hit_mat];
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

                final_color += color * settings.inv_rays_per_pixel;
            }

            Vector3 srgb = Vector3{
                sRGB_from_linear(final_color.x),
                    sRGB_from_linear(final_color.y),
                    sRGB_from_linear(final_color.z)
            };

            image.pixels[(i*image.width + j)] = BGRA8_pack(srgb);
        }
    }
}

struct ThreadData {
    Tile *tiles;
    World world;
    Camera camera;
    Image image;
    Settings settings;

    u32 jobs_count;
    volatile u32 job_index;
};

void* worker_thread_proc(void *data)
{
    ThreadData *thread_data = (ThreadData*)data;

    Tile *tiles       = thread_data->tiles;
    World world       = thread_data->world;
    Camera camera     = thread_data->camera;
    Image image       = thread_data->image;
    Settings settings = thread_data->settings;
    u32 jobs_count    = thread_data->jobs_count;

    RandomSeries random_series = { 23528812 };

    u32 tile_index = interlocked_fetch_and_add(&thread_data->job_index, 1);
    while (tile_index < jobs_count) {
        ray_cast(tiles[tile_index], image, camera, world, settings, random_series);
        tile_index = interlocked_fetch_and_add(&thread_data->job_index, 1);
    }

    return nullptr;
}

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    Image image = {};
    image.width = 1280;
    image.height = 720;

    Settings settings = {};
    settings.tolerance = 0.0001f;
    settings.min_hit_distance = 0.001f;
    settings.max_ray_bounce = 8;
    settings.rays_per_pixel = 128;
    settings.inv_rays_per_pixel = 1.0f / settings.rays_per_pixel;

    constexpr i32 num_threads   = 8;
    i32 tiles_count_x = image.width / 128;
    i32 tiles_count_y = image.height / 128;
    i32 tiles_count   = tiles_count_x*tiles_count_y;

    i32 pixels_size = image.width*image.height*sizeof(u32);
    image.pixels = (u32*)malloc(pixels_size);
    memset(image.pixels, 0xFF0000FF, pixels_size);

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

    World world = {};
    world.spheres = spheres;
    world.spheres_count = sizeof spheres / sizeof spheres[0];

    world.planes = planes;
    world.planes_count = sizeof planes / sizeof planes[0];

    world.materials = materials;
    world.materials_count = sizeof materials / sizeof materials[0];

    Camera camera = {};
    camera.p = Vector3{ 0.0f, 2.0f, 10.0f };
    camera.z_axis = normalise_zero(camera.p);
    camera.y_axis = normalise_zero(cross(camera.z_axis, Vector3{ 1.0f, 0.0f, 0.0f }));
    camera.x_axis = normalise_zero(cross(camera.y_axis, camera.z_axis));

    f32 film_d = 1.0f;
    f32 film_w = 1.0f;
    f32 film_h = 1.0f;

    if (image.width > image.height) {
        film_h = film_w * (f32)image.height / (f32)image.width;
    } else if (image.height > image.width) {
        film_w = film_h * (f32)image.width / (f32)image.height;
    }

    camera.film_half_w = 0.5f * film_w;
    camera.film_half_h = 0.5f * film_h;
    camera.film_c = camera.p - film_d*camera.z_axis;
    camera.half_pixel_w = 0.5f / image.width;
    camera.half_pixel_h = 0.5f / image.height;

    Tile *tiles = (Tile*)malloc(tiles_count * sizeof *tiles);
    for (i32 i = 0; i < tiles_count_y; i++) {
        for (i32 j = 0; j < tiles_count_x; j++) {
            i32 index = i*tiles_count_x + j;

            tiles[index].start_x = j * (image.width / tiles_count_x);
            tiles[index].start_y = i * (image.height / tiles_count_y);

            tiles[index].end_x = j < tiles_count_x-1 ?
                tiles[index].start_x + image.width / tiles_count_x :
                image.width;
            tiles[index].end_y = i < tiles_count_y-1 ?
                tiles[index].start_y + image.height / tiles_count_y :
                image.height;
        }
    }

    ThreadData thread_data = {};
    thread_data.tiles    = tiles;
    thread_data.world    = world;
    thread_data.image    = image;
    thread_data.settings = settings;
    thread_data.camera   = camera;

    thread_data.jobs_count = tiles_count;
    thread_data.job_index  = 0;

    if (num_threads == 0) {
        worker_thread_proc(&thread_data);
    } else {
        void *threads[num_threads];
        for (i32 i = 0; i < num_threads; i++) {
            threads[i] = create_thread(&worker_thread_proc, &thread_data);
        }

        for (i32 i = 0; i < num_threads; i++) {
            wait_for_thread(threads[i]);
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
        image.width,
        image.height,
        1,
        32,
        0,
        0,
        image.width,
        image.height,
        0,
        0,
    };

    FILE *f = fopen("test.bmp", "wb");
    fwrite(&bmfh, sizeof bmfh, 1, f);
    fwrite(&bmh, sizeof bmh, 1, f);
    fwrite(image.pixels, pixels_size, 1, f);
    fflush(f);

    return 0;
}
