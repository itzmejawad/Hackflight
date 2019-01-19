#!/usr/bin/env python
'''
rstateviz.py: ROS rviz script for visualizing quadcopter state in 3D space

Copyright (C) 2018 Simon D. Levy

Adapted from 

  https://github.com/ros-visualization/visualization_tutorials/blob/indigo-devel/interactive_marker_tutorials/scripts/basic_controls.py

This file is part of Hackflight.

Hackflight is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as 
published by the Free Software Foundation, either version 3 of the 
License, or (at your option) any later version.
This code is distributed in the hope that it will be useful,     
but WITHOUT ANY WARRANTY without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU Lesser General Public License 
along with this code.  If not, see <http:#www.gnu.org/licenses/>.
'''

MARKER_RESOURCE = 'package://stateviz/arrowhead.stl'
MARKER_SCALE    = .02

import rospy
import copy

from interactive_markers.interactive_marker_server import InteractiveMarkerServer, InteractiveMarker
from visualization_msgs.msg import Marker, InteractiveMarkerControl
from geometry_msgs.msg import Point
from tf.broadcaster import TransformBroadcaster
from tf.transformations import quaternion_from_euler

from math import sin, pi

server = None
br = None
counter = 0
marker = None

def frameCallback(msg):

    global counter, br, marker
    time = rospy.Time.now()

    cycle = sin(counter/140.0)

    rospy.loginfo('%+3.3f' % cycle)

    roll,pitch,yaw = 0,0,0
    rotation_quaternion = quaternion_from_euler(roll, pitch, yaw)
   
    translation = (0, 0, cycle*2.0)

    br.sendTransform(
            translation,
            rotation_quaternion,            
            time,   
            'map',                          # child (sender)
            'moving_frame')                 # parent (recipient)
    counter += 1

def processFeedback(feedback):

    server.applyChanges()

def makeBox(msg):

    marker = Marker()

    #marker.type = Marker.CUBE
    marker.type = Marker.MESH_RESOURCE
    marker.mesh_resource = MARKER_RESOURCE
    marker.scale.x = msg.scale * MARKER_SCALE
    marker.scale.y = msg.scale * MARKER_SCALE
    marker.scale.z = msg.scale * MARKER_SCALE
    marker.color.r = 1.0
    marker.color.g = 0.0
    marker.color.b = 0.0
    marker.color.a = 1.0

    return marker

def makeBoxControl(msg):

    control =  InteractiveMarkerControl()
    control.always_visible = True
    control.markers.append(makeBox(msg))
    msg.controls.append(control)
    return control

def normalizeQuaternion(quaternion_msg):

    norm = quaternion_msg.x**2 + quaternion_msg.y**2 + quaternion_msg.z**2 + quaternion_msg.w**2
    s = norm**(-0.5)
    quaternion_msg.x *= s
    quaternion_msg.y *= s
    quaternion_msg.z *= s
    quaternion_msg.w *= s

def makeQuadcopterMarker(position):

    marker = InteractiveMarker()
    marker.header.frame_id = 'moving_frame'
    marker.pose.position = position
    marker.scale = 1

    marker.name = 'quadcopter'

    makeBoxControl(marker)

    control = InteractiveMarkerControl()
    control.orientation.w = 1
    control.orientation.x = 0
    control.orientation.y = 1
    control.orientation.z = 0
    normalizeQuaternion(control.orientation)
    marker.controls.append(copy.deepcopy(control))
    marker.controls.append(control)

    server.insert(marker, processFeedback)

if __name__=='__main__':

    rospy.init_node('basic_controls')

    br = TransformBroadcaster()
    
    # create a timer to update the published transforms
    rospy.Timer(rospy.Duration(0.01), frameCallback)

    server = InteractiveMarkerServer('basic_controls')

    position = Point(0, -3, 0)
    makeQuadcopterMarker(position)

    server.applyChanges()

    rospy.spin()