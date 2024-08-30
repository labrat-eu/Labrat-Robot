cmake_minimum_required(VERSION 3.22.0)


find_package(flatbuffers MODULE REQUIRED)
find_package(yaml-cpp REQUIRED)

# Set the target name from the project name.
set(TARGET_NAME ${LOCAL_PROJECT_NAME}_core)

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_SOURCES
  clock.cpp
  manager.cpp
  topic.cpp
  service.cpp
  logger.cpp
  exception.cpp
  config.cpp
)

set(TARGET_HEADERS
  base.hpp
  clock.hpp
  manager.hpp
  message.hpp
  node.hpp
  topic.hpp
  service.hpp
  logger.hpp
  exception.hpp
  plugin.hpp
  config.hpp
  info.hpp
  filter.hpp
)

set(TARGET_MESSAGES
  msg/timestamp.fbs
  msg/test.fbs
)

set(TARGET_MESSAGES_EXTERNAL
  msg/foxglove/ArrowPrimitive.fbs
  msg/foxglove/ByteVector.fbs
  msg/foxglove/CameraCalibration.fbs
  msg/foxglove/CircleAnnotation.fbs
  msg/foxglove/Color.fbs
  msg/foxglove/CompressedImage.fbs
  msg/foxglove/CompressedVideo.fbs
  msg/foxglove/CubePrimitive.fbs
  msg/foxglove/CylinderPrimitive.fbs
  msg/foxglove/Duration.fbs
  msg/foxglove/FrameTransform.fbs
  msg/foxglove/FrameTransforms.fbs
  msg/foxglove/GeoJSON.fbs
  msg/foxglove/Grid.fbs
  msg/foxglove/ImageAnnotations.fbs
  msg/foxglove/KeyValuePair.fbs
  msg/foxglove/LaserScan.fbs
  msg/foxglove/LinePrimitive.fbs
  msg/foxglove/LocationFix.fbs
  msg/foxglove/Log.fbs
  msg/foxglove/ModelPrimitive.fbs
  msg/foxglove/PackedElementField.fbs
  msg/foxglove/Point2.fbs
  msg/foxglove/Point3.fbs
  msg/foxglove/PointCloud.fbs
  msg/foxglove/PointsAnnotation.fbs
  msg/foxglove/Pose.fbs
  msg/foxglove/PoseInFrame.fbs
  msg/foxglove/PosesInFrame.fbs
  msg/foxglove/Quaternion.fbs
  msg/foxglove/RawImage.fbs
  msg/foxglove/SceneEntityDeletion.fbs
  msg/foxglove/SceneEntity.fbs
  msg/foxglove/SceneUpdate.fbs
  msg/foxglove/SpherePrimitive.fbs
  msg/foxglove/TextAnnotation.fbs
  msg/foxglove/TextPrimitive.fbs
  msg/foxglove/Time.fbs
  msg/foxglove/TriangleListPrimitive.fbs
  msg/foxglove/Vector2.fbs
  msg/foxglove/Vector3.fbs
)

# Create library target for the project.
add_library(${TARGET_NAME} ${TARGET_HEADERS} ${TARGET_SOURCES})

# Add subtarget source directory.
add_subdirectory(utils)
add_subdirectory(plugins)

set(TARGET_NAME_MESSAGE ${TARGET_NAME}_msg)
lbot_generate_flatbuffer(TARGET ${TARGET_NAME_MESSAGE} SCHEMAS ${TARGET_MESSAGES} TARGET_PATH ${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH}/msg)

set(TARGET_NAME_MESSAGE_EXTERNAL ${TARGET_NAME}_msg_external)
lbot_generate_flatbuffer(TARGET ${TARGET_NAME_MESSAGE_EXTERNAL} SCHEMAS ${TARGET_MESSAGES_EXTERNAL} TARGET_PATH ${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH}/msg/foxglove)

# Link external libraries to the library target.
target_link_libraries(${TARGET_NAME} PRIVATE pthread yaml-cpp::yaml-cpp)
# Link internal libraries to the library target.
target_link_libraries(${TARGET_NAME} PUBLIC ${TARGET_NAME_MESSAGE} ${TARGET_NAME_MESSAGE_EXTERNAL} flatbuffers::flatbuffers)

# Add a install targets.
install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${TARGET_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
install(FILES ${TARGET_HEADERS_UTILS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${LOCAL_PROJECT_PATH_FULL}/utils/${TARGET_RELATIVE_PATH})
