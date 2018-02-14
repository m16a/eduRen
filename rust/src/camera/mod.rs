use glm::*;
use glm::ext::*;

pub enum CameraKey{
    Forward,
    Back,
    Left,
    Right,
    PitchUp,
    PitchDown,
    YawLeft,
    YawRight,
}

pub struct Camera{
    pos: Vector3<f32>,
    front: Vector3<f32>,
    up: Vector3<f32>,
    right: Vector3<f32>,
    world_up: Vector3<f32>,

    yaw: f32,
    pitch: f32,
}

impl Camera{
    pub fn new() -> Camera {
        let mut c = Camera {pos : vec3(0.0, 0.0, 2.0),
                front : vec3(0.0, 0.0, -1.0),
                up : vec3(0.0, 1.0, 0.0),
                world_up : vec3(0.0, 1.0, 0.0),
                right : vec3(0.0, 1.0, 0.0),
                yaw: -90.0,
                pitch: 0.0,
        };

        c.update_vectors();
        c

    }
    pub fn pos(&self) -> Vector3<f32>{
        self.pos
    }

    pub fn yaw_pitch(&self) -> (f32, f32){
        (self.yaw, self.pitch)
    }

    pub fn get_view_mat(&self) -> Matrix4<f32>{
        look_at(self.pos, self.pos + self.front, self.up)
    }
    
    pub fn process_key(&mut self, dir: CameraKey, dt: f32){
        let v = 5.0 * dt;
        let angle = 1.0;
        match dir{
            CameraKey::Forward => self.pos = self.pos + self.front*v,
            CameraKey::Back => self.pos = self.pos - self.front*v,
            CameraKey::Right => self.pos = self.pos + self.right*v,
            CameraKey::Left => self.pos = self.pos - self.right*v,
            CameraKey::PitchUp => { self.pitch += angle;  if self.pitch  > 89.0 {self.pitch = 89.0;}}, 
            CameraKey::PitchDown => { self.pitch -= angle; if self.pitch < -89.0 { self.pitch = -89.0;} },
            CameraKey::YawLeft => self.yaw -= angle, 
            CameraKey::YawRight => self.yaw += angle, 
        }

        match dir{
            CameraKey::PitchUp | CameraKey::PitchDown | CameraKey::YawLeft | CameraKey::YawRight => self.update_vectors(),
            _ => {},
        }
    }

    fn update_vectors(&mut self){
        self.front.x = cos(radians(self.yaw)) * cos(radians(self.pitch));
        self.front.y = sin(radians(self.pitch));
        self.front.z = sin(radians(self.yaw)) * cos(radians(self.pitch));
        self.front = normalize(self.front);

        self.right = normalize(cross(self.front, self.world_up));
        self.up = normalize(cross(self.right, self.front));
    }
}
