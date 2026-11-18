from pathlib import Path
import zipfile
root=Path(__file__).resolve().parents[1]
output=root.parents[1]/'outputs/EARTHtotaled-Unreal-source.zip'
excluded={'Saved','Intermediate','Binaries','DerivedDataCache','__pycache__','.vs'}
with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
    for path in sorted(root.rglob('*')):
        if path.is_file() and not excluded.intersection(path.relative_to(root).parts):
            archive.write(path,Path('EARTHtotaled-Unreal')/path.relative_to(root))
with zipfile.ZipFile(output) as archive:
    assert archive.testzip() is None
    assert 'EARTHtotaled-Unreal/EARTHtotaled.uproject' in archive.namelist()
print(output)
