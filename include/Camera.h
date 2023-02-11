/*!
  @file Camera.h @author Joel Barrett @date 01/01/12 @brief A basic first-person camera.
*/

#pragma once

#include <windows.h>
#include <xnamath.h>

#define KEY_WAS_DOWN_MASK   0x80
#define KEY_IS_DOWN_MASK    0x01
#define MOUSE_LEFT_BUTTON   0x01
#define MOUSE_MIDDLE_BUTTON 0x02
#define MOUSE_RIGHT_BUTTON  0x04
#define MOUSE_WHEEL         0x08

namespace OceanWaves
{
  // Used by Camera to map WM_KEYDOWN keys
  enum CameraKeys
  {
    CAM_STRAFE_LEFT = 0,
    CAM_STRAFE_RIGHT,
    CAM_MOVE_FORWARD,
    CAM_MOVE_BACKWARD,
    CAM_MOVE_UP,
    CAM_MOVE_DOWN,
    CAM_RESET,
    CAM_CONTROLDOWN,
    CAM_NUM_KEYS,
    CAM_UNKNOWN = 0xFF
  };

  /*
    Simple base camera class that moves and rotates. The base class
    records mouse and keyboard input for use by a derived class, and
    keeps common state. Adapted from DXUT.
  */
  class Camera
  {
  public:
    Camera();

    virtual void Reset();
    virtual void SetViewMatrix(const XMFLOAT3& position, const XMFLOAT3& lookAt);
    virtual void SetProjectionMatrix(float fov, float aspectRatio, float nearPlane, float farPlane);
    virtual void SetDragRect(const RECT& dragRect) { dragRect_ = dragRect; }
    virtual void Update(float elapsedTime) = 0;
    virtual LRESULT HandleMessages(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void SetDrag(bool movementDrag, float totalDragTimeToZero = 0.25f);
    void SetClipToBoundary(bool clipToBoundary, const XMFLOAT3& minBoundary, const XMFLOAT3& maxBoundary);
    void SetScalers(float rotationScaler = 0.01f, float moveScaler = 5.0f);
    void SetNumberOfFramesToSmoothMouseData(int numFrames);
    void SetInvertPitch(bool invertPitch) { invertPitch_ = invertPitch; }
    void SetEnableYAxisMovement(bool enableYAxisMovement) { enableYAxisMovement_ = enableYAxisMovement; }
    void SetEnablePositionMovement(bool enablePositionMovement) { enablePositionMovement_ = enablePositionMovement; }
    void SetResetCursorAfterMove(bool resetCursorAfterMove) { resetCursorAfterMove_ = resetCursorAfterMove; }

    bool IsBeingDragged() const { return mouseLButtonDown_ || mouseMButtonDown_ || mouseRButtonDown_; }
    bool IsMouseLButtonDown() const { return mouseLButtonDown_; }
    bool IsMouseMButtonDown() const { return mouseMButtonDown_; }
    bool IsMouseRButtonDown() const { return mouseRButtonDown_; }

    const XMFLOAT4X4& GetViewMatrix() const { return view_; }
    const XMFLOAT4X4& GetProjectionMatrix() const { return projection_; }
    const XMFLOAT3& GetPosition() const { return position_; }
    const XMFLOAT3& GetLookAt() const { return lookAt_; }

    float GetNearPlane() const { return nearPlane_; }
    float GetFarPlane() const { return farPlane_; }

  protected:
    virtual CameraKeys MapKey(unsigned int key);

    bool IsKeyDown(BYTE key) const { return (key & KEY_IS_DOWN_MASK) == KEY_IS_DOWN_MASK; }
    bool WasKeyDown(BYTE key) const { return (key & KEY_WAS_DOWN_MASK) == KEY_WAS_DOWN_MASK; }

    void UpdateMouseDelta();
    void UpdateVelocity(float elapsedTime);
    void GetInput(bool getKeyboardInput, bool getMouseInput, bool resetCursorAfterMove);
    void ConstrainToBoundary(XMFLOAT3& v);

  protected:
    XMFLOAT4X4 view_, projection_;

    XMFLOAT3 position_, lookAt_;
    XMFLOAT3 defaultPosition_, defaultLookAt_;
    XMFLOAT3 velocity_, velocityDrag_;
    XMFLOAT2 rotVelocity_;

    // Keyboard
    BYTE keys_[CAM_NUM_KEYS]; // State of input, KEY_WAS_DOWN_MASK | KEY_IS_DOWN_MASK
    XMFLOAT3 keyboardDirection_; // Direction vector of keyboard input
    int keysDown_; // Number of camera keys that are down

    // Mouse
    POINT prevMousePosition_; // Last absolute position of mouse cursor
    XMFLOAT2 mouseDelta_; // Mouse relative delta smoothed over a few frames
    bool mouseLButtonDown_, mouseMButtonDown_, mouseRButtonDown_; // True if mouse button is down 
    int currentButtonMask_, mouseWheelDelta_; // Mask of which buttons are down and amount of middle wheel scroll (+/-) 
    float framesToSmoothMouseData_; // Number of frames to smooth mouse data over

    RECT dragRect_; // Rectangle within which a drag can be initiated
    bool applyVelocityDrag_; // If true, then camera movement will slow to a stop otherwise movement is instant
    float dragTimer_, totalDragTimeToZero_; // Countdown timer to apply drag and time it takes for velocity to go from full to 0
    bool invertPitch_, enablePositionMovement_, enableYAxisMovement_,
      resetCursorAfterMove_, clipToBoundary_; // If true, then the camera will be clipped to the boundary
    XMFLOAT3 minBoundary_, maxBoundary_; // Min/Max point in clip boundary

    float yaw_, pitch_;
    float rotationScaler_, moveScaler_;
    float fov_, aspectRatio_, nearPlane_, farPlane_;
  };

  /*
    Simple first person camera class that moves and rotates.
    It allows yaw and pitch but not roll. It uses WM_KEYDOWN and
    GetCursorPos() to respond to keyboard and mouse input and updates
    the view matrix based on input. Adapted from DXUT.
  */
  class FirstPersonCamera : public Camera
  {
  public:
    FirstPersonCamera();

    // Enable or disable each of the mouse buttons for rotation drag
    void SetRotateButtons(bool left, bool middle, bool right, bool rotateWithoutButtonDown = false);

    // Functions to get state
    const XMFLOAT4X4& GetWorldMatrix() const { return world_; }

    const XMFLOAT3 GetWorldRight() const { return XMFLOAT3(world_._11, world_._12, world_._13); }
    const XMFLOAT3 GetWorldUp() const { return XMFLOAT3(world_._21, world_._22, world_._23); }
    const XMFLOAT3 GetWorldForward() const { return XMFLOAT3(world_._31, world_._32, world_._33); }
    const XMFLOAT3 GetWorldPosition() const { return XMFLOAT3(world_._41, world_._42, world_._43); }

    // Update the view matrix based on user input & elapsed time
    void Update(float elapsedTime);

  private:
    // World matrix of the camera (inverse of the view matrix)
    XMFLOAT4X4 world_;

    // Mask to determine which button to enable for rotation
    int activeButtonMask_;
    bool rotateWithoutButtonDown_;
  };
}
