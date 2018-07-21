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

struct Plane {
    n : Vector3,
    d : f32,
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
        Vector3{ x: rhs.x * self, y: rhs.y * self, z: rhs.z * self }
    }
}

impl Sub for Vector3 {
    type Output = Vector3;
    fn sub(self, rhs: Vector3) -> Vector3
    {
        Vector3{ x: self.x - rhs.x, y: self.y - rhs.y, z: self.z - rhs.z }
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
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * lhs.z;
}

fn cross(lhs : Vector3, rhs : Vector3) -> Vector3
{
    return Vector3 {
        x: lhs.y * rhs.z - lhs.z * rhs.y,
        y: lhs.z * rhs.x - lhs.x * rhs.z,
        z: lhs.x * rhs.y - lhs.y * rhs.x,
    }
}

fn sqrt(s : f32) -> f32
{
    return s.sqrt();
}

fn length(v : Vector3) -> f32
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

fn normalise(v : Vector3) -> Vector3
{
    return v / length(v);
}

fn main()
{
    assert_eq!(40, size_of::<BitmapHeader>());
    println!("hello world!");

    let width : i32 = 1280;
    let height : i32 = 720;

    let mut pixels : Vec<BGRA8> = Vec::new();
    pixels.reserve((width * height) as usize);
    for _i in 0..height*width {
        let pixel = BGRA8{ r: 0, g: 0, b: 0, a: 255 };
        pixels.push(pixel);
    }

    let camera_p = Vector3{ x: 0.0, y: 2.0, z: 5.0 };
    let look_at  = Vector3{ x: 0.0, y: 0.0, z: 0.0 };
    let camera_z = normalise(camera_p - look_at);
    let camera_x = normalise(cross(camera_z, Vector3{ x: 0.0, y: 0.0, z: 1.0 }));
    let camera_y = normalise(cross(camera_z, camera_x));

    let plane = Plane{
        n: Vector3{ x: 0.0, y: 1.0, z: 0.0 },
        d: 2.0
    };

    let film_d = 1.0;
    let film_w = 1.0;
    let film_h = 1.0;
    let film_half_w = 0.5 * film_w;
    let film_half_h = 0.5 * film_h;
    let film_c = camera_p - film_d*camera_z;

    for i in 0..height {
        for j in 0..width {
            let film = Vector3{
                x: j as f32 / width as f32 * 2.0 - 1.0,
                y: i as f32 / height as f32 * 2.0 - 1.0,
                z: camera_p.z
            };

            let film_p = film_c +
                film.x*film_half_w*camera_x +
                film.y*film_half_h*camera_y;

            let ray_o = camera_p;
            let ray_d = normalise(film_p - camera_p);

            let tolerance = 0.01;
            let denom = dot(plane.n, ray_d);

            if denom > tolerance || denom < -tolerance {
                let t = (-plane.d - dot(plane.n, ray_o)) / dot(plane.n, ray_d);
                if t > 0.0 {
                    pixels[(i*width + j) as usize] = BGRA8{ r: 255, g: 0, b: 0, a: 255 };
                }
            }
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
