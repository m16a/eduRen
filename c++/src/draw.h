
#include "vmath.h"

class MyDrawController
{
	public:
	void Init();
	void Render(int w, int h);

	//camera
	vmath::vec3 GetCamDir() const { return m_camDir;}
	
	//input handling
	void OnKeyUp();
	void OnKeyDown();
	void OnKeyRight();
	void OnKeyLeft();

	void OnKeyW();
	void OnKeyS();
	void OnKeyA();
	void OnKeyD();
	void OnKeySpace();

	private:
	vmath::vec3 m_camPos;
	vmath::vec3 m_camDir;

	vmath::mat4 m_cameraTransform;
};
