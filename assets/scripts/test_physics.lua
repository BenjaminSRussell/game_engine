-- Test Script for Next-Gen Engine Features
-- This script tests the Physics and Audio bindings exposed to Lua.

-- Print to console to verify script is running
print("Lua: Test script loaded!")

-- Spawn a box when the script starts
print("Lua: Spawning test box at (0, 10, 0)")
if Physics and Physics.SpawnBox then
    Physics.SpawnBox(0.0, 10.0, 0.0)
else
    print("Lua Error: Physics.SpawnBox not found!")
end

-- Play an impact sound
print("Lua: Playing impact sound")
if Audio and Audio.PlayImpact then
    Audio.PlayImpact("metal", "stone", 5.0)
else
    print("Lua Error: Audio.PlayImpact not found!")
end

-- Define an update function that the engine could call (future proofing)
function Update(dt)
    -- continuous logic could go here
end
