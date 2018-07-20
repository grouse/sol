use std::mem::size_of;
use std::fs::File;
use std::io::Write;
use std::slice;

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

fn main()
{
    assert_eq!(40, size_of::<BitmapHeader>());
    println!("hello world!");

    let width : i32 = 1280;
    let height : i32 = 720;

    let mut pixels : Vec<BGRA8> = Vec::new();
    pixels.reserve((width * height) as usize);
    for i in 0..height {
        for _j in 0..width {
            let pixel : BGRA8;
            if i > 200 {
                pixel = BGRA8{ r: 255, g: 0, b: 0, a: 255 };
            } else {
                pixel = BGRA8{ r: 0, g: 255, b: 0, a: 255 };
            }

            pixels.push(pixel);
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
