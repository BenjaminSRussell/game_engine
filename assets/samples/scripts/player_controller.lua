-- Sample Lua script for entity behavior
-- Location: samples/scripts/player_controller.lua

local PlayerController = {}
PlayerController.__index = PlayerController

-- Constants
local MOVE_SPEED = 5.0
local RUN_SPEED = 8.0
local JUMP_FORCE = 10.0
local GROUND_CHECK_DISTANCE = 0.1

-- Called when the script is first created
function PlayerController:Awake()
    print("PlayerController: Awake")
    self.velocity = Vec3.new(0, 0, 0)
    self.isGrounded = false
    self.isRunning = false
end

-- Called before the first frame
function PlayerController:Start()
    print("PlayerController: Start")
    -- Get components
    self.transform = self.entity:GetComponent("Transform")
    self.rigidbody = self.entity:GetComponent("Rigidbody")
    self.animator = self.entity:GetComponent("Animator")
    
    if not self.rigidbody then
        error("PlayerController requires a Rigidbody component!")
    end
end

-- Called every frame
function PlayerController:Update(dt)
    self:HandleInput(dt)
    self:UpdateAnimation()
end

-- Called at fixed timestep (physics)
function PlayerController:FixedUpdate(dt)
    self:CheckGroundStatus()
    self:ApplyMovement(dt)
end

-- Handle player input
function PlayerController:HandleInput(dt)
    -- Movement input
    local horizontal = Input.GetAxis("Horizontal") -- A/D or Left/Right
    local vertical = Input.GetAxis("Vertical")     -- W/S or Up/Down
    
    -- Calculate movement direction
    local cameraForward = Camera.GetForward()
    local cameraRight = Camera.GetRight()
    
    -- Project onto horizontal plane
    cameraForward.y = 0
    cameraForward = cameraForward:Normalize()
    cameraRight.y = 0
    cameraRight = cameraRight:Normalize()
    
    local moveDir = (cameraForward * vertical + cameraRight * horizontal):Normalize()
    
    -- Check for running
    self.isRunning = Input.GetKey("LeftShift")
    local currentSpeed = self.isRunning and RUN_SPEED or MOVE_SPEED
    
    -- Set velocity
    if moveDir:Length() > 0.1 then
        self.velocity.x = moveDir.x * currentSpeed
        self.velocity.z = moveDir.z * currentSpeed
        
        -- Rotate player to face movement direction
        local targetRotation = math.atan2(moveDir.x, moveDir.z)
        self.transform:SetRotationY(targetRotation)
    else
        self.velocity.x = 0
        self.velocity.z = 0
    end
    
    -- Jump input
    if Input.GetKeyDown("Space") and self.isGrounded then
        self.velocity.y = JUMP_FORCE
        Audio.PlayOneShot("sounds/player/jump.wav")
    end
end

-- Check if player is on ground
function PlayerController:CheckGroundStatus()
    local position = self.transform:GetPosition()
    local rayOrigin = position + Vec3.new(0, 0.1, 0)
    local rayDir = Vec3.new(0, -1, 0)
    
    local hit = Physics.Raycast(rayOrigin, rayDir, GROUND_CHECK_DISTANCE + 0.1)
    self.isGrounded = hit ~= nil
end

-- Apply movement to rigidbody
function PlayerController:ApplyMovement(dt)
    self.rigidbody:SetVelocity(self.velocity)
end

-- Update animation state
function PlayerController:UpdateAnimation()
    if not self.animator then return end
    
    -- Calculate movement speed for animation
    local moveSpeed = math.sqrt(self.velocity.x * self.velocity.x + self.velocity.z * self.velocity.z)
    
    -- Set animation parameters
    self.animator:SetFloat("Speed", moveSpeed)
    self.animator:SetBool("IsGrounded", self.isGrounded)
    self.animator:SetBool("IsRunning", self.isRunning)
    
    -- Trigger animations
    if moveSpeed < 0.1 then
        self.animator:Play("idle")
    elseif self.isRunning then
        self.animator:Play("run")
    else
        self.animator:Play("walk")
    end
    
    if not self.isGrounded then
        self.animator:Play("jump")
    end
end

-- Called when the entity is destroyed
function PlayerController:OnDestroy()
    print("PlayerController: Destroyed")
end

-- Event handlers
function PlayerController:OnCollisionEnter(collision)
    local other = collision.gameObject
    print("Player collided with: " .. other.name)
end

function PlayerController:OnTriggerEnter(trigger)
    local other = trigger.gameObject
    print("Player entered trigger: " .. other.name)
    
    -- Example: Collect items
    if other:HasTag("Collectible") then
        Audio.PlayOneShot("sounds/ui/collect.ogg")
        other:Destroy()
    end
end

return PlayerController
