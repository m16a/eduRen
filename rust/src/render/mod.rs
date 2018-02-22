use gfx::{Resources, Factory, CommandBuffer, Encoder};
use camera::Camera;
mod model;

use self::model::Model;

use std::collections::LinkedList;

pub struct Render<R: Resources>{
    res: R,
    m_models: LinkedList<Box<Model>>,
}

impl<R: Resources> Render<R>{

 pub fn render<F: Factory<R>, C: CommandBuffer<R>>(&self, factory : &mut F,  encoder : &mut Encoder<R,C>, camera : Camera){

    for m in self.m_models.iter(){
/*
        encoder.update_buffer(&data.transform, &[t], 0).expect("Update buffer failed",);
        encoder.draw(&slice, &pso, &data);
*/
    }
 }

 //pub fn add_model(m : & Model)
}

