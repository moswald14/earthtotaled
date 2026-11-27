from pathlib import Path
import runpy
import unreal

tools = Path(unreal.Paths.project_dir()).resolve()/'Tools'
runpy.run_path(str(tools/'import-outposts.py'), init_globals={'ringline': True})
