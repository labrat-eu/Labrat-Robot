cmake_minimum_required(VERSION 3.22.0)

# Set the target name from the project name.
set(TARGET_NAME ${LOCAL_PROJECT_NAME}_msg)

# Get the install path.
file(RELATIVE_PATH TARGET_RELATIVE_PATH ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR})

set(TARGET_MESSAGES
  timestamp.fbs
  timesync.fbs
  timesync_status.fbs
  test.fbs
)

set(TARGET_MESSAGES_EXTERNAL
  foxglove/ArrowPrimitive.fbs
  foxglove/ByteVector.fbs
  foxglove/CameraCalibration.fbs
  foxglove/CircleAnnotation.fbs
  foxglove/Color.fbs
  foxglove/CompressedImage.fbs
  foxglove/CompressedVideo.fbs
  foxglove/CubePrimitive.fbs
  foxglove/CylinderPrimitive.fbs
  foxglove/Duration.fbs
  foxglove/FrameTransform.fbs
  foxglove/FrameTransforms.fbs
  foxglove/GeoJSON.fbs
  foxglove/Grid.fbs
  foxglove/ImageAnnotations.fbs
  foxglove/KeyValuePair.fbs
  foxglove/LaserScan.fbs
  foxglove/LinePrimitive.fbs
  foxglove/LocationFix.fbs
  foxglove/Log.fbs
  foxglove/ModelPrimitive.fbs
  foxglove/PackedElementField.fbs
  foxglove/Point2.fbs
  foxglove/Point3.fbs
  foxglove/PointCloud.fbs
  foxglove/PointsAnnotation.fbs
  foxglove/Pose.fbs
  foxglove/PoseInFrame.fbs
  foxglove/PosesInFrame.fbs
  foxglove/Quaternion.fbs
  foxglove/RawImage.fbs
  foxglove/SceneEntityDeletion.fbs
  foxglove/SceneEntity.fbs
  foxglove/SceneUpdate.fbs
  foxglove/SpherePrimitive.fbs
  foxglove/TextAnnotation.fbs
  foxglove/TextPrimitive.fbs
  foxglove/Time.fbs
  foxglove/TriangleListPrimitive.fbs
  foxglove/Vector2.fbs
  foxglove/Vector3.fbs
)

lbot_generate_flatbuffer(TARGET ${TARGET_NAME} SCHEMAS ${TARGET_MESSAGES} TARGET_PATH ${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH})
lbot_generate_flatbuffer(TARGET ${TARGET_NAME}_external SCHEMAS ${TARGET_MESSAGES_EXTERNAL} TARGET_PATH ${LOCAL_PROJECT_PATH_FULL}/${TARGET_RELATIVE_PATH}/foxglove)
