# CMake generated Testfile for 
# Source directory: /Users/benjaminrussell/Desktop/Minecraft v2
# Build directory: /Users/benjaminrussell/Desktop/Minecraft v2
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[test_physics_ids]=] "/Users/benjaminrussell/Desktop/Minecraft v2/test_physics_ids")
set_tests_properties([=[test_physics_ids]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/benjaminrussell/Desktop/Minecraft v2/CMakeLists.txt;433;add_test;/Users/benjaminrussell/Desktop/Minecraft v2/CMakeLists.txt;437;add_engine_test;/Users/benjaminrussell/Desktop/Minecraft v2/CMakeLists.txt;0;")
add_test([=[test_network_stability]=] "/Users/benjaminrussell/Desktop/Minecraft v2/test_network_stability")
set_tests_properties([=[test_network_stability]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/benjaminrussell/Desktop/Minecraft v2/CMakeLists.txt;433;add_test;/Users/benjaminrussell/Desktop/Minecraft v2/CMakeLists.txt;440;add_engine_test;/Users/benjaminrussell/Desktop/Minecraft v2/CMakeLists.txt;0;")
subdirs("Vendor/Catch2")
