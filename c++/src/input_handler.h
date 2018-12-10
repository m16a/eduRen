#pragma once

class CCamera;
class MyDrawController;

class CInputHandler {
 public:
  CInputHandler(MyDrawController& dc) : m_dc(dc) {}

  void OnKeyUp(float dt);
  void OnKeyDown(float dt);
  void OnKeyRight(float dt);
  void OnKeyLeft(float dt);

  void OnKeyW(float dt, bool haste);
  void OnKeyS(float dt, bool haste);
  void OnKeyA(float dt, bool haste);
  void OnKeyD(float dt, bool haste);
  void OnKeySpace(float dt);

  MyDrawController& m_dc;
};
