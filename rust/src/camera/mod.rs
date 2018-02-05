use glm::*;
use glm::ext::*;
use num_traits::identities::One;

enum Key{
    Forward,
    Back,
    Left,
    Right
}

struct Camera{
    pos: Vector3<f32>,
    front: Vector3<f32>,
    up: Vector3<f32>,
    right: Vector3<f32>,

    yaw: f32,
    pitch: f32,
}

impl Camera{
    fn new() -> Camera {
        Camera {pos : vec3(0.0, 0.0, -1.0),
                front : vec3(1.0, 0.0, 0.0),
                up : vec3(0.0, 1.0, 0.0),
                right : vec3(1.0, 1.0, 0.0),
                yaw: 0.0,
                pitch: 0.0,
        }
    }

    fn get_view_mat(&self) -> Matrix4<f32>{
        look_at(self.pos, self.pos + self.front, self.up)
    }
    
    fn process_key(&mut self, dir: Key, dt: f32){
        let v = 5.0 * dt;
        match dir{
            Key::Forward => self.pos = self.pos + self.front*v,
            Key::Back => self.pos = self.pos - self.front*v,
            Key::Right => self.pos = self.pos + self.right*v,
            Key::Left => self.pos = self.pos - self.right*v,
        }
    }
}
