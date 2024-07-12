import bpy
import os
from bpy_extras.io_utils import ImportHelper
from bpy.props import StringProperty, PointerProperty
from bpy.types import Operator, Panel, PropertyGroup

bl_info = {
    "name": "NAF Export Helper",
    "author": "Snapdragon",
    "version": (1, 0),
    "blender": (3, 60, 0),
    "location": "View3D > Sidebar > Starfield Tools",
    "description": "Export animations for Starfield",
    "category": "Animation",
}

class NAFHelperProperties(PropertyGroup):
    root_object: PointerProperty(
        name="Root Object",
        type=bpy.types.Object,
        description="Select the root object of the existing skeleton"
    )
    gltf_file: StringProperty(
        name="Skeleton GLTF",
        description="Path to the GLTF/GLB file containing the game skeleton",
        default="",
        maxlen=1024,
        subtype='FILE_PATH'
    )
    output_file: StringProperty(
        name="Output GLB",
        description="Path to output the final result GLB file to",
        default="",
        maxlen=1024,
        subtype='FILE_PATH'
    )

class OBJECT_OT_NAFExportHelper(Operator, ImportHelper):
    bl_idname = "object.naf_export_animation"
    bl_label = "Export Animation"
    bl_options = {'REGISTER', 'UNDO'}
    
    filter_glob: StringProperty(default="*.gltf;*.glb", options={'HIDDEN'})
    
    def execute(self, context):
        root_object = context.scene.naf_helper_props.root_object
        gltf_file = bpy.path.abspath(context.scene.naf_helper_props.gltf_file)
        output_file = self.filepath

        print(gltf_file)
        
        if not root_object:
            self.report({'ERROR'}, "Please select a root object")
            return {'CANCELLED'}
        
        if not gltf_file:
            self.report({'ERROR'}, "Please select a Skeleton GLTF file")
            return {'CANCELLED'}

        if not output_file:
            self.report({'ERROR'}, "Please select a Output GLTF file")
            return {'CANCELLED'}
        
        # Import GLTF file
        bpy.ops.import_scene.gltf(
            filepath=gltf_file,
            import_pack_images=True,
            bone_heuristic='BLENDER',
            guess_original_bind_pose=False
        )
        
        # Get the newly imported armature
        imported_armature = None
        for obj in bpy.context.selected_objects:
            imported_armature = obj
            break
        
        if not imported_armature:
            self.report({'ERROR'}, "No objects found in imported GLTF file")
            return {'CANCELLED'}

        def no_numbers_name(obj_name):
            idx = obj_name.rfind(".")
            if idx >= 0:
                obj_name = obj_name[:idx]
            return obj_name
        
        # Function to recursively process objects and bones
        def process_hierarchy(existing_obj, imported_obj):
            if existing_obj.type == 'ARMATURE':
                found_armature = None
                for child in imported_obj.children_recursive:
                    if child.type == 'ARMATURE':
                        child["original_name"] = existing_obj.name
                        found_armature = child
                        break

                if not found_armature:
                    return

                for bone in existing_obj.pose.bones:
                    for imported_bone in found_armature.pose.bones:
                        if imported_bone.name == bone.name:
                            constraint = imported_bone.constraints.new('COPY_TRANSFORMS')
                            constraint.target = existing_obj
                            constraint.subtarget = bone.name
                            constraint.target_space = 'LOCAL_OWNER_ORIENT'
                            constraint.owner_space = 'LOCAL'
                            break
                
            else:
                for child in imported_obj.children_recursive:
                    if child.name == existing_obj.name or no_numbers_name(child.name) == existing_obj.name:
                        child["original_name"] = existing_obj.name
                        constraint = child.constraints.new('COPY_TRANSFORMS')
                        constraint.target = existing_obj
                        constraint.target_space = 'WORLD'
                        constraint.owner_space = 'WORLD'
                        break
            
            for child in existing_obj.children:
                process_hierarchy(child, imported_obj)
        
        # Start processing from the root object
        process_hierarchy(root_object, imported_armature)
        
        # Bake the animation
        bpy.ops.object.select_all(action='DESELECT')
        
        def select_hierarchy(obj):
            obj.select_set(True)
            for child in obj.children:
                select_hierarchy(child)
        
        select_hierarchy(imported_armature)
        bpy.context.view_layer.objects.active = imported_armature
        
        bpy.ops.nla.bake(
            frame_start=bpy.context.scene.frame_start,
            frame_end=bpy.context.scene.frame_end,
            step=1,
            only_selected=False,
            visual_keying=True,
            clear_constraints=False,
            clear_parents=False,
            use_current_action=False,
            bake_types={'OBJECT'}
        )

        # Select all objects in the imported skeleton except meshes
        bpy.ops.object.select_all(action='DESELECT')
        def select_non_mesh_objects(obj):
            if obj.type != 'MESH':
                obj.select_set(True)
            for child in obj.children:
                select_non_mesh_objects(child)
        
        select_non_mesh_objects(imported_armature)
        
        # Select only meshes in the existing skeleton
        def select_mesh_objects(obj):
            if obj.type == 'MESH':
                obj.select_set(True)
            for child in obj.children:
                select_mesh_objects(child)
        
        select_mesh_objects(root_object)
        
        # Invoke the GLTF exporter
        bpy.ops.export_scene.gltf(
            filepath=output_file,
            check_existing=True,
            export_import_convert_lighting_mode='SPEC',
            export_format='GLB',
            export_image_format='NONE',
            export_materials='NONE',
            use_selection=True,
            export_animations=True,
            export_animation_mode='ACTIVE_ACTIONS',
            export_nla_strips_merged_animation_name='Animation',
            export_extras=True,
            export_morph=True,
            export_morph_normal=False,
            export_morph_tangent=False,
            export_morph_animation=True
        )
        
        # Delete the imported skeleton
        bpy.ops.object.select_all(action='DESELECT')
        select_hierarchy(imported_armature)
        bpy.ops.object.delete()
        
        self.report({'INFO'}, "Export successful.")
        return {'FINISHED'}

class VIEW3D_PT_NAFExportHelper(Panel):
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = "Starfield Tools"
    bl_label = "NAF Export Helper"

    def draw(self, context):
        layout = self.layout
        props = context.scene.naf_helper_props

        layout.prop(props, "root_object")
        layout.prop(props, "gltf_file")
        layout.operator("object.naf_export_animation")

def register():
    bpy.utils.register_class(NAFHelperProperties)
    bpy.types.Scene.naf_helper_props = PointerProperty(type=NAFHelperProperties)
    bpy.utils.register_class(OBJECT_OT_NAFExportHelper)
    bpy.utils.register_class(VIEW3D_PT_NAFExportHelper)

def unregister():
    bpy.utils.unregister_class(VIEW3D_PT_NAFExportHelper)
    bpy.utils.unregister_class(OBJECT_OT_NAFExportHelper)
    del bpy.types.Scene.naf_helper_props
    bpy.utils.unregister_class(NAFHelperProperties)

if __name__ == "__main__":
    register()