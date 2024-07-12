import bpy
import ctypes
import os
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, PointerProperty, EnumProperty
from bpy.types import Operator, Panel, PropertyGroup

bl_info = {
    "name": "NAF Export Helper",
    "author": "Snapdragon",
    "version": (1, 1, 0),
    "blender": (3, 6, 0),
    "location": "View3D > Sidebar > Starfield Tools",
    "description": "Export animations for Starfield",
    "category": "Animation",
}

def get_dll_path():
    current_dir = os.path.dirname(os.path.abspath(__file__))
    dll_path = os.path.join(current_dir, "AnimationOptimizer.dll")
    return dll_path

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
        description="Path to output the result GLB file to",
        default="",
        maxlen=1024,
        subtype='FILE_PATH'
    )
    optimization_level: EnumProperty(
        name="Compression Level",
        description="Select the level of animation compression",
        items=[
            ('0', "Lossless", "All animation data is kept perfectly as-is"),
            ('1', "Normal (Curve Compression)", "Animation curves are decimated with a zero tolerance"),
            ('2', "High (Curve Compression)", "Animation curves are decimated with a 1e-7 tolerance"),
            ('3', "Maximum (Curve Compression)", "Animation curves are decimated with a 1e-6 tolerance"),
            ('4', "Ultra (Curve Compression)", "Animation curves are decimated with a 1e-5 tolerance")
        ],
        default='0'
    )

class OBJECT_OT_NAFExportHelper(Operator, ExportHelper):
    bl_idname = "object.naf_export_animation"
    bl_label = "Export Animation"
    bl_options = {'REGISTER', 'UNDO'}
    
    filename_ext = ".glb"
    filter_glob: StringProperty(default="*.glb", options={'HIDDEN'})
    
    def execute(self, context):
        root_object = context.scene.naf_helper_props.root_object
        gltf_file = bpy.path.abspath(context.scene.naf_helper_props.gltf_file)
        optimization_level = int(context.scene.naf_helper_props.optimization_level)
        output_file = self.filepath

        print(optimization_level)
        
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
                return obj_name[:idx]
            return obj_name
        
        # Function to recursively process objects and bones
        def process_hierarchy(existing_obj, imported_obj):
            for child in imported_obj.children_recursive:
                    if child.name == existing_obj.name or no_numbers_name(child.name) == existing_obj.name:
                        child["original_name"] = existing_obj.name
                        constraint = child.constraints.new('COPY_TRANSFORMS')
                        constraint.target = existing_obj
                        constraint.target_space = 'WORLD'
                        constraint.owner_space = 'WORLD'
                        break

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
            use_selection=True,
            export_animations=True,
            export_animation_mode='ACTIVE_ACTIONS',
            export_nla_strips_merged_animation_name='Animation',
            export_extras=True,
            export_morph=True,
            export_morph_normal=False,
            export_morph_tangent=False,
            export_morph_animation=True,
            export_skins=False,
            export_lights=False,
            export_cameras=False,
            export_yup=True,
            export_apply=False,
            export_texcoords=False,
            export_normals=False,
            export_materials='NONE',
            export_colors=False,
            export_attributes=False,
            use_mesh_edges=False,
            use_mesh_vertices=False
        )
        
        # Delete the imported skeleton
        bpy.ops.object.select_all(action='DESELECT')
        select_hierarchy(imported_armature)
        bpy.ops.object.delete()

        # Get the path to the optimizer DLL
        dll_path = get_dll_path()
        
        # Load the DLL
        optimizer_lib = ctypes.CDLL(dll_path)
        optimizer_lib.OptimizeAnimation.argtypes = [ctypes.c_char_p, ctypes.c_int]
        optimizer_lib.OptimizeAnimation.restype = ctypes.c_bool
        
        # Optimize the animation
        result = optimizer_lib.OptimizeAnimation(output_file.encode('utf-8'), optimization_level)
        if not result:
            self.report({'ERROR'}, "Failed to optimize animation.")
            return {'CANCELLED'}
        
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
        layout.prop(props, "optimization_level")
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