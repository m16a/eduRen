#pragma once

class Camera
{
	public:
	void SetPos(vec3& pos);	
	void SetRot(quat& q);
	void SetYawPitchRoll(vec3& ypr) const;

	const vec3& GetPos() const;
	const quat& GetRot() const;
	const vec3& GetYawPitchRoll() const;

	const mat4& GetViewMatrix() const;
	const mat4& GetProjectionMatrix() const;

	void SetViewMatrix(const mat4& m);
	void SetProjectionMatrix(const mat4& m);
	
	void Move(vec3& v); // v = (forward, strafe, 0)
	void AddYawPitchRoll(vec3& ypr);

	private:
	vec3 m_pos;
	quat m_rot;

	mat4 m_viewM;
	mat4 m_projectionM;
};
