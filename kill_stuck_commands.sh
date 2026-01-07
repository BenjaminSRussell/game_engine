#!/bin/bash
#
# kill_stuck_commands.sh
# Safely terminate stuck rendering reorganization command
#

echo "=========================================="
echo "Stuck Command Termination Script"
echo "=========================================="
echo ""

echo "🔍 Searching for stuck processes..."
echo ""

# Find processes related to the stuck command
STUCK_PIDS=$(ps aux | grep -E "(mkdir.*rendering|mv.*src/engine)" | grep -v grep | awk '{print $2}')

if [ -z "$STUCK_PIDS" ]; then
    echo "✅ No stuck processes found!"
    echo ""
    echo "The command may have already terminated or is not running."
    exit 0
fi

echo "Found the following stuck processes:"
ps aux | grep -E "(mkdir.*rendering|mv.*src/engine)" | grep -v grep
echo ""

read -p "Do you want to kill these processes? (yes/no): " response

if [ "$response" = "yes" ] || [ "$response" = "y" ]; then
    echo ""
    echo "🔨 Terminating stuck processes..."
    
    for pid in $STUCK_PIDS; do
        echo "  Killing PID $pid..."
        kill -9 $pid 2>/dev/null
        
        if [ $? -eq 0 ]; then
            echo "  ✅ Process $pid terminated"
        else
            echo "  ❌ Failed to kill PID $pid (may require sudo)"
        fi
    done
    
    echo ""
    echo "✅ Cleanup complete!"
    
    # Verify directory structure
    echo ""
    echo "📁 Verifying rendering directory structure..."
    ls -la /Users/benjaminrussell/Desktop/Minecraft\ v2/src/engine/rendering/
    
    echo ""
    echo "File count in 3d_rendering:"
    find /Users/benjaminrussell/Desktop/Minecraft\ v2/src/engine/rendering/3d_rendering -type f | wc -l
    
else
    echo ""
    echo "❌ Operation cancelled."
    exit 1
fi

echo ""
echo "=========================================="
echo "Done!"
echo "=========================================="
