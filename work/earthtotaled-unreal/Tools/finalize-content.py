"""Reapply materials and bake the map after art has already been successfully imported."""
from pathlib import Path
script=(Path(__file__).parent/'bootstrap-content.py').read_text()
scope={}
exec(compile(script[:script.index('tasks=[]')],'bootstrap-setup','exec'),scope)
exec(compile(script[script.index('materials=['):],'bootstrap-finalize','exec'),scope)
