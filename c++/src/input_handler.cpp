
#include "input_handler.h"
#include "draw.h"

void CInputHandler::OnKeyW(float dt, bool haste)
{
	m_dc.GetCam().ProcessKeyboard(eCameraMovement::FORWARD, dt, haste);
}

void CInputHandler::OnKeyS(float dt, bool haste)
{
	m_dc.GetCam().ProcessKeyboard(eCameraMovement::BACKWARD, dt, haste);
}

void CInputHandler::OnKeyA(float dt, bool haste)
{
	m_dc.GetCam().ProcessKeyboard(eCameraMovement::LEFT, dt, haste);
}

void CInputHandler::OnKeyD(float dt, bool haste)
{
	m_dc.GetCam().ProcessKeyboard(eCameraMovement::RIGHT, dt, haste);
}

void CInputHandler::OnKeyUp(float dt)
{
	m_dc.GetCam().ProcessRotation(eCameraMovement::PITCH_UP, dt);
}

void CInputHandler::OnKeyDown(float dt)
{
	m_dc.GetCam().ProcessRotation(eCameraMovement::PITCH_DOWN, dt);
}

void CInputHandler::OnKeyRight(float dt)
{
	m_dc.GetCam().ProcessRotation(eCameraMovement::YAW_RIGHT, dt);
}

void CInputHandler::OnKeyLeft(float dt)
{
	m_dc.GetCam().ProcessRotation(eCameraMovement::YAW_LEFT, dt);
}

void CInputHandler::OnKeySpace(float dt)
{
}
