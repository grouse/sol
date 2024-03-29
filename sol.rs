use std::mem::size_of;
use std::fs::File;
use std::io::Write;
use std::slice;
use std::ops::{Sub, Div, Add, Mul, AddAssign, Neg};
use std::f32;
use std::u32;
use std::cmp::max;
use std::sync::{Arc, Mutex};
use std::thread;


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
    emit        : Vector3,
    reflect     : Vector3,
    specularity : f32,
}

struct RandomSeries {
    state : u32
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

impl Neg for Vector3 {
    type Output = Vector3;
    fn neg(self) -> Vector3
    {
        Vector3{ x: -self.x, y: -self.y, z: -self.z }
    }
}

impl AddAssign for Vector3 {
    fn add_assign(&mut self, rhs: Vector3)
    {
        *self = Vector3{
            x: self.x + rhs.x,
            y: self.y + rhs.y,
            z: self.z + rhs.z
        };
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

fn hadamard(lhs : Vector3, rhs : Vector3) -> Vector3
{
    return Vector3{
        x: lhs.x * rhs.x,
        y: lhs.y * rhs.y,
        z: lhs.z * rhs.z
    };
}

trait Lerp {
    type Output;
    fn lerp(self, b: Self, t: f32) -> Self;
}

impl Lerp for f32 {
    type Output = f32;
    fn lerp(self, b: f32, t: f32) -> f32
    {
        return (1.0-t) * self + t*b;
    }
}

impl Lerp for Vector3 {
    type Output = Vector3;
    fn lerp(self, b: Vector3, t: f32) -> Vector3
    {
        return Vector3{
            x: self.x.lerp(b.x, t),
            y: self.y.lerp(b.y, t),
            z: self.z.lerp(b.z, t)
        };
    }
}

fn rand_f32_uni(r : &mut RandomSeries) -> f32
{
    let mut s = r.state;
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    r.state = s;

    return (s >> 1) as f32 / (u32::MAX >> 1) as f32;
}

fn rand_f32_bi(r : &mut RandomSeries) -> f32
{
    return -1.0 + 2.0 * rand_f32_uni(r);
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

fn sRGB_from_linear(l : f32) -> f32
{
    if l > 1.0 {
        return 1.0;
    } else if l < 0.0 {
        return 0.0;
    }

    let mut s = l*12.92;
    if l > 0.0031308 {
        s = 1.055*l.powf(1.0/2.4) - 0.055;
    }

    return s;
}

struct Tile {
    start_x : i32,
    start_y : i32,
    end_x : i32,
    end_y : i32,
}

fn main()
{
    assert_eq!(40, size_of::<BitmapHeader>());

    //let width : i32 = 7680;
    //let height : i32 = 4320;
    //let width : i32 = 3840;
    //let height : i32 = 2160;
    //let width : i32 = 1920;
    //let height : i32 = 1080;
    let width : i32 = 1280;
    let height : i32 = 720;

    let camera_p = Vector3{ x: 0.0, y: 1.83, z: 10.0 };
    let camera_z = normalise_zero(camera_p);
    let camera_y = normalise_zero(cross(camera_z, Vector3{ x: 1.0, y: 0.0, z: 0.0 }));
    let camera_x = normalise_zero(cross(camera_y, camera_z));

    let mut materials : [Material;5] = [
        Material{
            emit: Vector3{ x: 0.0, y: 0.0, z: 0.0 },
            reflect: Vector3{ x: 0.0, y: 0.0, z: 0.0 },
            specularity: 0.0,
        }; 5];

    materials[0].emit = Vector3{ x: 0.4, y: 0.4, z: 0.9 };

    materials[1].reflect = Vector3{ x: 0.3, y: 0.9, z: 0.3 };
    materials[2].reflect = Vector3{ x: 0.2, y: 0.2, z: 0.2 };

    materials[3].reflect = Vector3{ x: 0.8, y: 0.95, z: 0.8 };
    materials[3].specularity = 0.94;

    materials[4].emit = Vector3{ x: 5.0, y: 1.0, z: 1.0 };

    let mut planes : [Plane;1] = [
        Plane{
            n: Vector3{ x: 0.0, y: 1.0, z: 0.0 },
            d: 0.0,
            material: 0,
        }; 1];

    planes[0] = Plane{
        n: Vector3{ x: 0.0, y: 1.0, z: 0.0 },
        d: 0.0,
        material: 1,
    };

    let mut spheres : [Sphere;3] = [
         Sphere{
            p: Vector3{ x: 0.0, y: 0.0, z: 0.0 },
            r: 1.0,
            material: 0
         }; 3];

    spheres[0] = Sphere{
        p: Vector3{ x: 0.0, y: 0.0, z: 0.0 },
        r: 1.0,
        material: 2,
    };
    spheres[1] = Sphere{
        p: Vector3{ x: 3.0, y: 0.0, z: 2.0 },
        r: 1.0,
        material: 3,
    };
    spheres[2] = Sphere{
        p: Vector3{ x: 2.5, y: 2.0, z: -5.0 },
        r: 1.0,
        material: 4,
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

    let tolerance = 0.0001;
    let min_distance = 0.001;

    let max_ray_bounce = 8;
    let rays_per_pixel = 1;
    let inv_rays_per_pixel = 1.0 / rays_per_pixel as f32;

    let num_tiles = 200;

    let mut tiles : Vec<Tile> = Vec::new();
    if false {
        for i in 0..num_tiles {
            for j in 0..num_tiles {
                let start_x = j * ( width / num_tiles );
                let start_y = i * ( height / num_tiles );
                let end_x = max(start_x + ( width / num_tiles ), width);
                let end_y = max(start_y + ( height / num_tiles ), height);

                let tile = Tile{ start_x, start_y, end_x, end_y };
                tiles.push(tile);
            }
        }
    } else {
        for i in 0..num_tiles {
            let start_x = 0;
            let start_y = i * ( height / num_tiles );
            let end_x = width;
            let end_y = max(start_y + ( height / num_tiles ), height);

            let tile = Tile{ start_x, start_y, end_x, end_y };
            tiles.push(tile);
        }
    }

    let num_cores = 12;
    let mut worker_threads : Vec<thread::JoinHandle<_>> = Vec::new();
    let work_queue = Arc::new(Mutex::new(tiles));

    let mut pixels : Vec<u32> = Vec::new();
    pixels.reserve((width * height) as usize);
    for _i in 0..height*width {
        pixels.push(0x000000FF);
    }

    for i in 0..num_cores {
        let tl_index  = i;
        let tl_queue  = work_queue.clone();

        let handle = thread::spawn(move || {
            let mut random_series = RandomSeries{ state: 23528812 };
            loop {
                let mut d = tl_queue.lock().unwrap();

                if d.len() != 0 {
                    let tile = d.pop().unwrap();
                    let mut tl_pixels = pixels.chunks_mut((tile.start_y * width) as usize);
                    let mut iter = tl_pixels.iter();

                    println!("thread {:?} has work", thread::current().id());


                    for i in 0..(tile.end_y - tile.start_y) {
                        let film_y = -1.0 + 2.0 * (i as f32 / height as f32);

                        for j in 0..width {
                            let film_x = -1.0 + 2.0 * (j as f32 / width as f32);

                            let mut final_color = Vector3{ x: 0.0, y: 0.0, z: 0.0 };

                            for _k in 0..rays_per_pixel {
                                let offset_x = (film_x + half_pixel_w);
                                let offset_y = (film_y + half_pixel_h);

                                let film_p = film_c +
                                    offset_x*film_half_w*camera_x +
                                    offset_y*film_half_h*camera_y;

                                let mut ray_o = camera_p;
                                let mut ray_d = normalise_zero(film_p - camera_p);

                                let mut color       = Vector3{ x: 0.0, y: 0.0, z: 0.0 };
                                let mut attenuation = Vector3{ x: 1.0, y: 1.0, z: 1.0 };

                                for _l in 0..max_ray_bounce {
                                    let mut hit_mat = 0;
                                    let mut hit_d   = f32::MAX;

                                    let mut next_ray_n = Vector3{ x: 0.0, y: 0.0, z: 0.0 };

                                    for plane in &planes {
                                        let denom = dot(plane.n, ray_d);
                                        if denom > tolerance || denom < -tolerance {
                                            let t = (-plane.d - dot(plane.n, ray_o)) / denom;
                                            if t > min_distance && t < hit_d {
                                                hit_d   = t;
                                                hit_mat = plane.material;

                                                next_ray_n = plane.n;
                                            }
                                        }
                                    }

                                    for sphere in &spheres {
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

                                                next_ray_n = normalise_zero(t*ray_d + l);
                                            }
                                        }
                                    }

                                    let mat = materials[hit_mat as usize];
                                    color += hadamard(attenuation, mat.emit);

                                    if hit_mat != 0 {
                                        let mut cos_attenuation = dot(-ray_d, next_ray_n);
                                        if cos_attenuation < 0.0 {
                                            cos_attenuation = 0.0;
                                        }
                                        attenuation = hadamard(attenuation, cos_attenuation*mat.reflect);

                                        ray_o = ray_o + ray_d * hit_d;

                                        let x = rand_f32_bi(&mut random_series);
                                        let y = rand_f32_bi(&mut random_series);
                                        let z = rand_f32_bi(&mut random_series);
                                        let rvec = Vector3{ x, y, z };

                                        let pure_bounce = ray_d - 2.0 * dot(ray_d, next_ray_n) * next_ray_n;
                                        let random_bounce = normalise_zero(next_ray_n + rvec);

                                        ray_d = normalise_zero(
                                            random_bounce.lerp(
                                                pure_bounce,
                                                mat.specularity));
                                    } else {
                                        break;
                                    }
                                }

                                final_color += color * inv_rays_per_pixel;
                            }

                            let srgb = Vector3{
                                x: sRGB_from_linear(final_color.x),
                                y: sRGB_from_linear(final_color.y),
                                z: sRGB_from_linear(final_color.z)
                            };

                            iter = BGRA8_pack(srgb);
                            iter = iter.next();
                        }
                    }

                    println!("thread {:?} finished work", thread::current().id());
                } else {
                    println!("no work to be done for {}", tl_index);

                }
            }
        });

        worker_threads.push(handle);
    }

    for handle in worker_threads {
        handle.join();
    }

    println!(" done!");

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
