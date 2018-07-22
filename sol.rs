use std::mem::size_of;
use std::fs::File;
use std::io::Write;
use std::slice;
use std::ops::{Sub, Div, Add, Mul};
use std::f32;

#[repr(C, packed)]
struct BitmapFileHeader {
    bmp_type  : u16,
    size      : u32,
    reserved0 : u16,
    reserved1 : u16,
    offset    : u32,
}

#[repr(C, packed)]
struct BitmapHeader {
    header_size      : u32,
    width            : i32,
    height           : i32,
    planes           : u16,
    bpp              : u16,
    compression      : u32,
    bmp_size         : u32,
    res_horiz        : i32,
    res_vert         : i32,
    colors_used      : u32,
    colors_important : u32,
}

#[repr(C)]
struct BGRA8 {
    b : u8,
    g : u8,
    r : u8,
    a : u8,
}

#[derive(Copy, Clone)]
struct Vector3 {
    x : f32,
    y : f32,
    z : f32,
}

#[derive(Copy, Clone)]
struct Vector2 {
    x : f32,
    y : f32,
}

#[derive(Copy, Clone)]
struct Plane {
    n : Vector3,
    d : f32,
    material : i32,
}

#[derive(Copy, Clone)]
struct Sphere {
    p : Vector3,
    r : f32,
    material : i32,
}

#[derive(Copy, Clone)]
struct Material {
    color : Vector3,
}

impl Mul<f32> for Vector3 {
    type Output = Vector3;
    fn mul(self, rhs: f32) -> Vector3
    {
        Vector3{ x: self.x * rhs, y: self.y * rhs, z: self.z * rhs }
    }
}

impl Mul<Vector3> for f32 {
    type Output = Vector3;
    fn mul(self, rhs: Vector3) -> Vector3
    {
        Vector3{ x: self * rhs.x, y: self * rhs.y, z: self * rhs.z }
    }
}

impl Sub<Vector3> for Vector3 {
    type Output = Vector3;
    fn sub(self, rhs: Vector3) -> Vector3
    {
        Vector3{ x: self.x - rhs.x, y: self.y - rhs.y, z: self.z - rhs.z }
    }
}

impl Sub<f32> for Vector3 {
    type Output = Vector3;
    fn sub(self, rhs: f32) -> Vector3
    {
        Vector3{ x: self.x - rhs, y: self.y - rhs, z: self.z - rhs }
    }
}

impl Add for Vector3 {
    type Output = Vector3;
    fn add(self, rhs: Vector3) -> Vector3
    {
        Vector3{ x: self.x + rhs.x, y: self.y + rhs.y, z: self.z + rhs.z }
    }
}

impl Div<f32> for Vector3 {
    type Output = Vector3;
    fn div(self, rhs: f32) -> Vector3
    {
        Vector3{ x: self.x / rhs, y: self.y / rhs, z: self.z / rhs }
    }
}

fn dot(lhs : Vector3, rhs : Vector3) -> f32
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

fn cross(lhs : Vector3, rhs : Vector3) -> Vector3
{
    return Vector3 {
        x: lhs.y * rhs.z - lhs.z * rhs.y,
        y: lhs.z * rhs.x - lhs.x * rhs.z,
        z: lhs.x * rhs.y - lhs.y * rhs.x,
    };
}

fn sqrt(s : f32) -> f32
{
    return s.sqrt();
}

fn length_sq(v : Vector3) -> f32
{
    return dot(v, v);
}

fn length(v : Vector3) -> f32
{
    return sqrt(length_sq(v));
}

fn normalise(v : Vector3) -> Vector3
{
    return v / length(v);
}

fn normalise_zero(v: Vector3) -> Vector3
{
    let mut r = Vector3{ x: 0.0, y: 0.0, z: 0.0 };

    let len_sq = length_sq(v);
    if len_sq > (0.0001 * 0.0001) {
        r = v * (1.0 / sqrt(len_sq));
    }

    return r;
}

fn BGRA8_pack(v : Vector3) -> u32
{
    let c = 255.0 * v;
    return (c.z as u32) | (c.y as u32) << 8 | (c.x as u32) << 16 | 255 << 24;
}

fn main()
{
    assert_eq!(40, size_of::<BitmapHeader>());
    println!("hello world!");

    let width : i32 = 1920;
    let height : i32 = 1080;

    let mut pixels : Vec<u32> = Vec::new();
    pixels.reserve((width * height) as usize);
    for _i in 0..height*width {
        pixels.push(0x000000FF);
    }

    let camera_p = Vector3{ x: 0.0, y: 2.0, z: 10.0 };
    let camera_z = normalise_zero(camera_p);
    let camera_y = normalise_zero(cross(camera_z, Vector3{ x: 1.0, y: 0.0, z: 0.0 }));
    let camera_x = normalise_zero(cross(camera_y, camera_z));

    let mut materials : [Material; 3] = [Material{ color: Vector3{ x: 0.0, y: 0.0, z: 0.0 } } ; 3];
    materials[0].color = Vector3{ x: 0.0, y: 0.0, z: 0.0 };
    materials[1].color = Vector3{ x: 1.0, y: 0.0, z: 0.0 };
    materials[2].color = Vector3{ x: 0.0, y: 0.0, z: 1.0 };

    let plane = Plane{
        n: Vector3{ x: 0.0, y: 1.0, z: 0.0 },
        d: 0.0,
        material: 1,
    };

    let sphere = Sphere{
        p: Vector3{ x: 0.0, y: 0.0, z: 0.0 },
        r: 1.0,
        material: 2,
    };

    let film_d = 1.0;
    let mut film_w = 1.0;
    let mut film_h = 1.0;

    if width > height {
        film_h = film_w * (height as f32 / width as f32);
    } else if height > width {
        film_w = film_h * (width as f32 / height as f32);
    }

    let film_half_w = 0.5 * film_w;
    let film_half_h = 0.5 * film_h;
    let film_c = camera_p - film_d*camera_z;

    let half_pixel_w = 0.5 / width as f32;
    let half_pixel_h = 0.5 / height as f32;

    for i in 0..height {
        let film_y = -1.0 + 2.0 * (i as f32 / height as f32);
        let offset_y = film_y + half_pixel_h;

        for j in 0..width {
            let film_x = -1.0 + 2.0 * (j as f32 / width as f32);
            let offset_x = film_x + half_pixel_w;

            let film_p = film_c +
                offset_x*film_half_w*camera_x +
                offset_y*film_half_h*camera_y;

            let ray_o = camera_p;
            let ray_d = normalise_zero(film_p - camera_p);

            let mut hit_mat = 0;
            let mut hit_d   = f32::MAX;

            let tolerance = 0.0001;
            let min_distance = 0.001;

            // planes
            {
                let denom = dot(plane.n, ray_d);
                if denom > tolerance || denom < -tolerance {
                    let t = (-plane.d - dot(plane.n, ray_o)) / denom;
                    if t > min_distance && t < hit_d {
                        hit_d   = t;
                        hit_mat = plane.material;
                    }
                }
            }

            // spheres
            {
                let l : Vector3 = ray_o - sphere.p;
                let a = dot(ray_d, ray_d);
                let b = 2.0 * dot(ray_d, l);
                let c = dot(l, l) - sphere.r * sphere.r;

                let root_term = b*b - 4.0*a*c;
                let denom = 2.0 * a;

                if root_term >= 0.0 && (denom > tolerance || denom < -tolerance) {
                    let tp = (-b + sqrt(root_term)) / denom;
                    let tn = (-b - sqrt(root_term)) / denom;

                    let mut t = tp;
                    if tn > min_distance && tn < tp {
                        t = tn;
                    }

                    if t > min_distance && t < hit_d {
                        hit_d   = t;
                        hit_mat = sphere.material;
                    }
                }
            }

            let mat = materials[hit_mat as usize];
            pixels[(i*width + j) as usize] = BGRA8_pack(mat.color);
        }
    }

    let bmfh = BitmapFileHeader{
        bmp_type  : 0x4d42,
        size      :
            (size_of::<BitmapFileHeader>() +
             size_of::<BitmapHeader>() +
             pixels.len() * size_of::<BGRA8>()) as u32,
        reserved0 : 0,
        reserved1 : 0,
        offset    :
            (size_of::<BitmapFileHeader>() +
             size_of::<BitmapHeader>()) as u32
    };

    let bmh = BitmapHeader{
        header_size      : size_of::<BitmapHeader>() as u32,
        width            : width,
        height           : height,
        planes           : 1,
        bpp              : 32,
        compression      : 0,
        bmp_size         : 0,
        res_horiz        : width,
        res_vert         : height,
        colors_used      : 0,
        colors_important : 0,
    };

    let mut f = File::create("test.bmp").unwrap();

    unsafe {
        let dst = &bmfh as *const BitmapFileHeader as *const u8;
        let slice = slice::from_raw_parts(dst, size_of::<BitmapFileHeader>() );
        f.write(slice);

        let dst = &bmh as *const BitmapHeader as *const u8;
        let slice = slice::from_raw_parts(dst, size_of::<BitmapHeader>() );
        f.write(slice);

        let dst = pixels.as_ptr() as *const u8;
        let slice = slice::from_raw_parts(dst, size_of::<BGRA8>() * pixels.len() );
        f.write(slice);
    }
}
