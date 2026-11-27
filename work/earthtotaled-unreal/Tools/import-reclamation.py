import runpy
from pathlib import Path
import unreal

root=Path(unreal.Paths.project_dir()).resolve()
runpy.run_path(str(root/'Tools/import-outposts.py'), init_globals={'reclamation': True})
