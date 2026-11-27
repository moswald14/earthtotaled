"""Reimport just the alert sound waves after running make-alerts.py."""
from pathlib import Path
import unreal

project = Path(unreal.Paths.project_dir()).resolve()
tasks = []
for source in sorted((project / 'ArtSource').glob('S_Alert*.wav')):
    task = unreal.AssetImportTask()
    task.filename = str(source)
    task.destination_path = '/Game/EARTHtotaled/Art'
    task.destination_name = source.stem
    task.automated = task.replace_existing = task.save = True
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    if not task.imported_object_paths:
        raise RuntimeError('Alert import failed: ' + task.filename)
unreal.log('Imported %d combat alerts' % len(tasks))
