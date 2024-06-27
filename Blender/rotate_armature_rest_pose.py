import bpy
import math
from mathutils import Matrix, Vector

bl_info = {
    "name": "Rotate Armature Rest Pose",
    "author": "Snapdragon",
    "version": (1, 0),
    "blender": (2, 80, 0),
    "location": "View3D > Sidebar > Rotate Armature",
    "description": "Rotate armature bones in rest pose",
    "category": "Rigging",
}

def store_child_object_transforms(armature):
    world_transforms = {}
    for obj in bpy.data.objects:
        if obj.parent == armature and obj.parent_bone != "":
            world_transforms[obj.name] = (obj.matrix_world.copy(), obj.parent_bone)
    return world_transforms

def restore_child_object_transforms(armature, world_transforms):
    for obj_name, (world_matrix, parent_bone_name) in world_transforms.items():
        obj = bpy.data.objects.get(obj_name)
        if obj:
            parent_bone = armature.data.bones.get(parent_bone_name)
            if parent_bone:
                obj.matrix_parent_inverse = parent_bone.matrix_local.inverted()
                obj.matrix_world = world_matrix

def rotate_bone(bone, parent_matrix=None, axis='X', angle=90):
    original_length = bone.length
    
    rot_matrix = Matrix.Rotation(math.radians(angle), 4, axis)
    
    if parent_matrix:
        local_matrix = parent_matrix.inverted() @ bone.matrix
        new_matrix = local_matrix @ rot_matrix
        new_global_matrix = parent_matrix @ new_matrix
    else:
        new_global_matrix = bone.matrix @ rot_matrix
    
    bone.head = new_global_matrix.to_translation()
    
    direction = new_global_matrix.to_3x3() @ Vector((0, 1, 0))
    
    direction.normalize()
    bone.tail = bone.head + direction * original_length
    
    for child in bone.children:
        rotate_bone(child, new_global_matrix, axis, angle)

def rotate_bones_in_rest_pose(armature, axis='X', angle=90):
    world_transforms = store_child_object_transforms(armature)

    bpy.ops.object.mode_set(mode='EDIT')
    
    edit_bones = armature.data.edit_bones
    root_bones = [bone for bone in edit_bones if bone.parent is None]
    
    for root_bone in root_bones:
        rotate_bone(root_bone, axis=axis, angle=angle)
    
    bpy.ops.object.mode_set(mode='OBJECT')

    restore_child_object_transforms(armature, world_transforms)

class ARMATURE_OT_rotate_rest_pose(bpy.types.Operator):
    bl_idname = "armature.rotate_rest_pose"
    bl_label = "Rotate Rest Pose"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        armature = context.active_object
        if armature.type != 'ARMATURE':
            self.report({'ERROR'}, "Selected object is not an armature")
            return {'CANCELLED'}
        
        rotate_bones_in_rest_pose(armature, 
                                  axis=context.scene.rest_pose_rotation_axis, 
                                  angle=context.scene.rest_pose_rotation_angle)
        return {'FINISHED'}

class ARMATURE_PT_rotate_rest_pose(bpy.types.Panel):
    bl_label = "Rotate Armature Rest Pose"
    bl_idname = "ARMATURE_PT_rotate_rest_pose"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Rotate Armature'

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        layout.prop(scene, "rest_pose_rotation_axis", text="Rotation Axis")
        layout.prop(scene, "rest_pose_rotation_angle", text="Rotation Angle")
        layout.operator("armature.rotate_rest_pose")

def register():
    bpy.utils.register_class(ARMATURE_OT_rotate_rest_pose)
    bpy.utils.register_class(ARMATURE_PT_rotate_rest_pose)
    bpy.types.Scene.rest_pose_rotation_axis = bpy.props.EnumProperty(
        items=[('X', 'X', 'X axis'),
               ('Y', 'Y', 'Y axis'),
               ('Z', 'Z', 'Z axis')],
        name="Rotation Axis",
        default='Z'
    )
    bpy.types.Scene.rest_pose_rotation_angle = bpy.props.FloatProperty(
        name="Rotation Angle",
        default=-90.0,
        min=-360.0,
        max=360.0
    )

def unregister():
    bpy.utils.unregister_class(ARMATURE_OT_rotate_rest_pose)
    bpy.utils.unregister_class(ARMATURE_PT_rotate_rest_pose)
    del bpy.types.Scene.rest_pose_rotation_axis
    del bpy.types.Scene.rest_pose_rotation_angle

if __name__ == "__main__":
    register()