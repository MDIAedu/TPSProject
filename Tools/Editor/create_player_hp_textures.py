import os

import unreal


PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
SOURCE_DIR = os.path.join(PROJECT_DIR, "Content", "Texture")
DESTINATION_PATH = "/Game/Texture"
TEXTURE_NAMES = ("T_PlayerHpFrame", "T_PlayerHpGauge")


def import_texture(texture_name):
    filename = os.path.join(SOURCE_DIR, texture_name + ".png")
    if not os.path.exists(filename):
        raise RuntimeError("Missing source image: " + filename)

    task = unreal.AssetImportTask()
    task.filename = filename
    task.destination_path = DESTINATION_PATH
    task.destination_name = texture_name
    task.automated = True
    task.replace_existing = True
    task.save = True

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported_path = DESTINATION_PATH + "/" + texture_name
    imported_asset = unreal.EditorAssetLibrary.load_asset(imported_path)
    if not imported_asset:
        raise RuntimeError("Failed to import texture: " + imported_path)

    imported_asset.compression_settings = unreal.TextureCompressionSettings.TC_EDITOR_ICON
    imported_asset.mip_gen_settings = unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    imported_asset.srgb = True
    imported_asset.modify()
    unreal.EditorAssetLibrary.save_asset(imported_path)


def main():
    if not unreal.EditorAssetLibrary.does_directory_exist(DESTINATION_PATH):
        unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)

    for texture_name in TEXTURE_NAMES:
        import_texture(texture_name)

    unreal.EditorAssetLibrary.save_directory(DESTINATION_PATH)


main()
