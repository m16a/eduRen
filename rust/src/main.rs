#[macro_use]
extern crate gfx;
extern crate gfx_window_glutin;
extern crate glutin;
#[macro_use]
extern crate imgui;
extern crate imgui_gfx_renderer;
extern crate glm;
extern crate num_traits;
extern crate assimp_sys;


use imgui::*;

mod camera;
mod support_gfx;
mod render;

const CLEAR_COLOR: [f32; 4] = [115.0/255.0, 140.0/255.0, 153.0/255.0, 1.0];


fn main() {
    support_gfx::run("eduRen".to_owned(), CLEAR_COLOR, hello_world);
}

fn hello_world(ui: &Ui) -> bool {
    true
}
