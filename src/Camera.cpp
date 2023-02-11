/*!
  @file Camera.cpp @author Joel Barrett @date 01/01/12 @brief A basic first-person camera.
*/

#include <cassert>
#include "Camera.h"

namespace OceanWaves
{
  Camera::Camera()
  {
    SetViewMatrix(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f));
    SetProjectionMatrix(XM_PIDIV4, 1.0f, 0.1f, 1000.0f);

    velocity_ = XMFLOAT3(0.0f, 0.0f, 0.0f);
    velocityDrag_ = XMFLOAT3(0.0f, 0.0f, 0.0f);
    rotVelocity_ = XMFLOAT2(0.0f, 0.0f);

    // Keyboard
    ZeroMemory(keys_, sizeof(BYTE) * CAM_NUM_KEYS);
    keyboardDirection_ = XMFLOAT3(0.0f, 0.0f, 0.0f);
    keysDown_ = 0;

    // Mouse
    GetCursorPos(&prevMousePosition_);
    mouseDelta_ = XMFLOAT2(0.0f, 0.0f);
    mouseLButtonDown_ = mouseMButtonDown_ = mouseRButtonDown_ = false;
    currentButtonMask_ = mouseWheelDelta_ = 0;
    framesToSmoothMouseData_ = 2.0f;

    SetRect(&dragRect_, LONG_MIN, LONG_MIN, LONG_MAX, LONG_MAX);
    applyVelocityDrag_ = false;
    dragTimer_ = 0.0f;
    totalDragTimeToZero_ = 0.25f;
    invertPitch_ = false;
    enablePositionMovement_ = true;
    enableYAxisMovement_ = true;
    resetCursorAfterMove_ = false;
    clipToBoundary_ = false;
    minBoundary_ = XMFLOAT3(-1.0f, -1.0f, -1.0f);
    maxBoundary_ = XMFLOAT3(1.0f, 1.0f, 1.0f);
    yaw_ = pitch_ = 0.0f;
    rotationScaler_ = 0.01f;
    moveScaler_ = 5.0f;
  }

  void Camera::Reset()
  {
    SetViewMatrix(defaultPosition_, defaultLookAt_);
  }

  void Camera::SetViewMatrix(const XMFLOAT3& position, const XMFLOAT3& lookAt)
  {
    defaultPosition_ = position_ = position;
    defaultLookAt_ = lookAt_ = lookAt;

    // Calculate view and inverse view
    XMVECTOR p = XMLoadFloat3(&position);
    XMVECTOR l = XMLoadFloat3(&lookAt);
    XMVECTOR u = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(p, l, u);
    XMStoreFloat4x4(&view_, view);
    XMVECTOR invDet;
    XMMATRIX invView = XMMatrixInverse(&invDet, view);

    // The axis basis vectors and camera position are stored inside the 
    // position matrix in the 4 rows of the camera's world matrix.
    // To figure out the yaw/pitch of the camera, we just need the Z basis vector
    XMFLOAT3 zBasis(invView._31, invView._32, invView._33);

    yaw_ = atan2f(zBasis.x, zBasis.z);
    pitch_ = -atan2f(zBasis.y, sqrtf(zBasis.x * zBasis.x + zBasis.z * zBasis.z));
  }

  void Camera::SetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane)
  {
    assert(nearPlane < farPlane);

    fov_ = fov;
    aspectRatio_ = aspectRatio;
    nearPlane_ = nearPlane;
    farPlane_ = farPlane;

    XMMATRIX projection = XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane);
    XMStoreFloat4x4(&projection_, projection);
  }

  LRESULT Camera::HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
  {
    switch (uMsg)
    {
    case WM_KEYDOWN:
    {
      // Map this key to a CameraKeys enum and update the
      // state of keys_[] by adding the KEY_WAS_DOWN_MASK|KEY_IS_DOWN_MASK mask
      // only if the key is not down
      CameraKeys mappedKey = MapKey((UINT)wParam);
      if (mappedKey != CAM_UNKNOWN)
      {
        if (FALSE == IsKeyDown(keys_[mappedKey]))
        {
          keys_[mappedKey] = KEY_WAS_DOWN_MASK | KEY_IS_DOWN_MASK;
          ++keysDown_;
        }
      }
      break;
    }

    case WM_KEYUP:
    {
      // Map this key to a CameraKeys enum and update the
      // state of keys_[] by removing the KEY_IS_DOWN_MASK mask
      CameraKeys mappedKey = MapKey((UINT)wParam);
      if (mappedKey != CAM_UNKNOWN && (DWORD)mappedKey < 8)
      {
        keys_[mappedKey] &= ~KEY_IS_DOWN_MASK;
        --keysDown_;
      }
      break;
    }

    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_LBUTTONDBLCLK:
    {
      // Compute the drag rectangle in screen coord
      POINT cursor = { (short)LOWORD(lParam), (short)HIWORD(lParam) };

      // Update member var state
      if ((uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK) && PtInRect(&dragRect_, cursor))
      {
        mouseLButtonDown_ = true; currentButtonMask_ |= MOUSE_LEFT_BUTTON;
      }

      if ((uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONDBLCLK) && PtInRect(&dragRect_, cursor))
      {
        mouseMButtonDown_ = true; currentButtonMask_ |= MOUSE_MIDDLE_BUTTON;
      }

      if ((uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONDBLCLK) && PtInRect(&dragRect_, cursor))
      {
        mouseRButtonDown_ = true; currentButtonMask_ |= MOUSE_RIGHT_BUTTON;
      }

      // Capture the mouse, so if the mouse button is 
      // released outside the window, we'll get the WM_LBUTTONUP message
      SetCapture(hWnd);
      GetCursorPos(&prevMousePosition_);

      return TRUE;
    }

    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_LBUTTONUP:
    {
      // Update member variable state
      if (uMsg == WM_LBUTTONUP)
      {
        mouseLButtonDown_ = false; currentButtonMask_ &= ~MOUSE_LEFT_BUTTON;
      }

      if (uMsg == WM_MBUTTONUP)
      {
        mouseMButtonDown_ = false; currentButtonMask_ &= ~MOUSE_MIDDLE_BUTTON;
      }

      if (uMsg == WM_RBUTTONUP)
      {
        mouseRButtonDown_ = false; currentButtonMask_ &= ~MOUSE_RIGHT_BUTTON;
      }

      // Release the capture if no mouse buttons down
      if (!mouseLButtonDown_ && !mouseRButtonDown_ && !mouseMButtonDown_)
      {
        ReleaseCapture();
      }
      break;
    }

    case WM_CAPTURECHANGED:
    {
      if ((HWND)lParam != hWnd)
      {
        if ((currentButtonMask_ & MOUSE_LEFT_BUTTON) ||
          (currentButtonMask_ & MOUSE_MIDDLE_BUTTON) ||
          (currentButtonMask_ & MOUSE_RIGHT_BUTTON))
        {
          mouseLButtonDown_ = false;
          mouseMButtonDown_ = false;
          mouseRButtonDown_ = false;

          currentButtonMask_ &= ~MOUSE_LEFT_BUTTON;
          currentButtonMask_ &= ~MOUSE_MIDDLE_BUTTON;
          currentButtonMask_ &= ~MOUSE_RIGHT_BUTTON;

          ReleaseCapture();
        }
      }
      break;
    }

    case WM_MOUSEWHEEL:
    {
      // Update member var state
      mouseWheelDelta_ += (short)HIWORD(wParam);
      break;
    }
    }
    return FALSE;
  }

  void Camera::SetDrag(bool movementDrag, float totalDragTimeToZero)
  {
    applyVelocityDrag_ = movementDrag;
    totalDragTimeToZero_ = totalDragTimeToZero;
  }

  void Camera::SetClipToBoundary(bool clipToBoundary, const XMFLOAT3& minBoundary, const XMFLOAT3& maxBoundary)
  {
    clipToBoundary_ = clipToBoundary;

    minBoundary_ = minBoundary;
    maxBoundary_ = maxBoundary;
  }

  void Camera::SetScalers(float rotationScaler, float moveScaler)
  {
    rotationScaler_ = rotationScaler;
    moveScaler_ = moveScaler;
  }

  void Camera::SetNumberOfFramesToSmoothMouseData(int numFrames)
  {
    if (numFrames > 0) {
      framesToSmoothMouseData_ = (float)numFrames;
    }
  }

  CameraKeys Camera::MapKey(UINT key)
  {
    switch (key)
    {
    case VK_CONTROL:
      return CAM_CONTROLDOWN;

    case VK_LEFT:
      return CAM_STRAFE_LEFT;

    case VK_RIGHT:
      return CAM_STRAFE_RIGHT;

    case VK_UP:
      return CAM_MOVE_FORWARD;

    case VK_DOWN:
      return CAM_MOVE_BACKWARD;

    case VK_PRIOR: // pgup
      return CAM_MOVE_UP;

    case VK_NEXT: // pgdn
      return CAM_MOVE_DOWN;

    case 'A':
      return CAM_STRAFE_LEFT;

    case 'D':
      return CAM_STRAFE_RIGHT;

    case 'W':
      return CAM_MOVE_FORWARD;

    case 'S':
      return CAM_MOVE_BACKWARD;

    case 'Q':
      return CAM_MOVE_DOWN;

    case 'E':
      return CAM_MOVE_UP;

    case VK_NUMPAD4:
      return CAM_STRAFE_LEFT;

    case VK_NUMPAD6:
      return CAM_STRAFE_RIGHT;

    case VK_NUMPAD8:
      return CAM_MOVE_FORWARD;

    case VK_NUMPAD2:
      return CAM_MOVE_BACKWARD;

    case VK_NUMPAD9:
      return CAM_MOVE_UP;

    case VK_NUMPAD3:
      return CAM_MOVE_DOWN;

    case VK_HOME:
      return CAM_RESET;
    }
    return CAM_UNKNOWN;
  }

  void Camera::UpdateMouseDelta()
  {
    POINT mouseDelta;
    POINT mousePos;

    // Get current position of mouse
    GetCursorPos(&mousePos);

    // Calc how far it's moved since last frame
    mouseDelta.x = mousePos.x - prevMousePosition_.x;
    mouseDelta.y = mousePos.y - prevMousePosition_.y;

    // Record current position for next time
    prevMousePosition_ = mousePos;

    // Smooth the relative mouse data over a few frames so it isn't 
    // jerky when moving slowly at low frame rates
    float percentOfNew = 1.0f / framesToSmoothMouseData_;
    float percentOfOld = 1.0f - percentOfNew;

    mouseDelta_.x = mouseDelta_.x * percentOfOld + mouseDelta.x * percentOfNew;
    mouseDelta_.y = mouseDelta_.y * percentOfOld + mouseDelta.y * percentOfNew;

    XMVECTOR r = XMVectorScale(XMLoadFloat2(&mouseDelta_), rotationScaler_);
    XMStoreFloat2(&rotVelocity_, r);
  }

  void Camera::UpdateVelocity(float elapsedTime)
  {
    XMVECTOR r = XMVectorScale(XMLoadFloat2(&mouseDelta_), rotationScaler_);
    XMStoreFloat2(&rotVelocity_, r);

    XMVECTOR acceleration = XMLoadFloat3(&keyboardDirection_);

    // Normalize vector so if moving 2 dirs (left & forward), 
    // the camera doesn't move faster than if moving in 1 dir
    acceleration = XMVector3Normalize(acceleration);
    acceleration = XMVectorScale(acceleration, moveScaler_);

    if (applyVelocityDrag_)
    {
      // Is there any acceleration this frame?
      if (XMVectorGetX(XMVector3LengthSq(acceleration)) > 0.0f)
      {
        // If so, then this means the user has pressed a movement key,
        // so change the velocity immediately to acceleration 
        // upon keyboard input.  This isn't normal physics
        // but it will give a quick response to keyboard input
        XMStoreFloat3(&velocity_, acceleration);
        dragTimer_ = totalDragTimeToZero_;
        XMStoreFloat3(&velocityDrag_, XMVectorDivide(acceleration, XMVectorReplicate(dragTimer_)));
      }
      else {
        // If no key being pressed, then slowly decrease velocity to 0
        if (dragTimer_ > 0.0f)
        {
          // Drag until timer is <= 0
          XMVECTOR v = XMLoadFloat3(&velocity_);
          XMVECTOR vd = XMLoadFloat3(&velocityDrag_);

          v = XMVectorSubtract(v, XMVectorScale(vd, elapsedTime));
          XMStoreFloat3(&velocity_, v);
          dragTimer_ -= elapsedTime;
        }
        else {
          // Zero velocity
          velocity_ = XMFLOAT3(0.0f, 0.0f, 0.0f);
        }
      }
    }
    else {
      // No drag, so immediately change the velocity
      XMStoreFloat3(&velocity_, acceleration);
    }
  }

  void Camera::GetInput(bool bGetKeyboardInput, bool bGetMouseInput, bool resetCursorAfterMove)
  {
    keyboardDirection_ = XMFLOAT3(0.0f, 0.0f, 0.0f);

    // Update acceleration vector based on keyboard state
    if (bGetKeyboardInput)
    {
      if (IsKeyDown(keys_[CAM_MOVE_FORWARD])) {
        keyboardDirection_.z += 1.0f;
      }
      if (IsKeyDown(keys_[CAM_MOVE_BACKWARD])) {
        keyboardDirection_.z -= 1.0f;
      }
      if (enableYAxisMovement_)
      {
        if (IsKeyDown(keys_[CAM_MOVE_UP])) {
          keyboardDirection_.y += 1.0f;
        }
        if (IsKeyDown(keys_[CAM_MOVE_DOWN])) {
          keyboardDirection_.y -= 1.0f;
        }
      }
      if (IsKeyDown(keys_[CAM_STRAFE_RIGHT])) {
        keyboardDirection_.x += 1.0f;
      }
      if (IsKeyDown(keys_[CAM_STRAFE_LEFT])) {
        keyboardDirection_.x -= 1.0f;
      }
    }
    if (bGetMouseInput) {
      UpdateMouseDelta();
    }
  }

  void Camera::ConstrainToBoundary(XMFLOAT3& v)
  {
    // Constrain vector to a bounding box 
    v.x = XMMax(v.x, minBoundary_.x);
    v.y = XMMax(v.y, minBoundary_.y);
    v.z = XMMax(v.z, minBoundary_.z);

    v.x = XMMin(v.x, maxBoundary_.x);
    v.y = XMMin(v.y, maxBoundary_.y);
    v.z = XMMin(v.z, maxBoundary_.z);
  }

  FirstPersonCamera::FirstPersonCamera() : activeButtonMask_(0x07),
    rotateWithoutButtonDown_(false)
  {}

  void FirstPersonCamera::SetRotateButtons(bool left, bool middle, bool right, bool rotateWithoutButtonDown)
  {
    activeButtonMask_ = (left ? MOUSE_LEFT_BUTTON : 0)
      | (middle ? MOUSE_MIDDLE_BUTTON : 0)
      | (right ? MOUSE_RIGHT_BUTTON : 0);

    rotateWithoutButtonDown_ = rotateWithoutButtonDown;
  }

  void FirstPersonCamera::Update(float elapsedTime)
  {
    if (IsKeyDown(keys_[CAM_RESET])) {
      Reset();
    }
    // Get keyboard/mouse/gamepad input
    GetInput(enablePositionMovement_, (activeButtonMask_ & currentButtonMask_)
      || rotateWithoutButtonDown_, resetCursorAfterMove_);

    // Get amount of velocity based on the keyboard input and drag (if any)
    UpdateVelocity(elapsedTime);

    // If rotating the camera
    if ((activeButtonMask_ & currentButtonMask_) ||
      rotateWithoutButtonDown_)
    {
      // Update the pitch & yaw angle based on mouse movement
      float yawDelta = rotVelocity_.x;
      float pitchDelta = rotVelocity_.y;

      // Invert pitch if requested
      if (invertPitch_) {
        pitchDelta = -pitchDelta;
      }
      yaw_ += yawDelta;
      pitch_ += pitchDelta;

      // Limit pitch to straight up or straight down
      pitch_ = XMMax(-XM_PIDIV2, pitch_);
      pitch_ = XMMin(XM_PIDIV2, pitch_);
    }

    // Make a rotation matrix based on the camera's yaw and pitch
    XMMATRIX cameraRot = XMMatrixRotationRollPitchYaw(pitch_, yaw_, 0.0f);

    // Transform vectors based on camera's rotation matrix
    XMVECTOR localUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR localForward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    XMVECTOR worldUp = XMVector3TransformCoord(localUp, cameraRot);
    XMVECTOR worldForward = XMVector3TransformCoord(localForward, cameraRot);

    // Transform the position delta by the camera's rotation
    if (!enableYAxisMovement_)
    {
      // If restricting Y movement, do not include pitch
      // when transforming position delta vector.
      cameraRot = XMMatrixRotationRollPitchYaw(0.0f, yaw_, 0.0f);
    }

    // Simple euler method to calculate position delta
    XMVECTOR posDelta = XMVectorScale(XMLoadFloat3(&velocity_), elapsedTime);
    XMVECTOR posDeltaWorld = XMVector3TransformCoord(posDelta, cameraRot);

    // Move the eye position
    XMVECTOR p = XMLoadFloat3(&position_);
    p = XMVectorAdd(p, posDeltaWorld);
    XMStoreFloat3(&position_, p);

    if (clipToBoundary_) {
      ConstrainToBoundary(position_);
    }
    // Update the lookAt position based on the camera position
    XMVECTOR l = XMVectorAdd(p, worldForward);
    XMStoreFloat3(&lookAt_, l);

    // Update the view and world matrices
    XMMATRIX v = XMMatrixLookAtLH(p, l, worldUp);
    XMStoreFloat4x4(&view_, v);

    XMVECTOR invDet;
    XMMATRIX w = XMMatrixInverse(&invDet, v);
    XMStoreFloat4x4(&world_, w);
  }
}
