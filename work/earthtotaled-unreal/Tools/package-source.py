"""Export the complete tracked source layout without local work logs or player data."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import subprocess
import tempfile
import zipfile

PROJECT = Path(__file__).resolve().parents[1]
WORKSPACE = PROJECT.parents[1]
PREFIX = 'EARTHtotaled/'
BLOCKED_PARTS = {'outputs', '.git', 'saved', 'intermediate', 'binaries',
                 'deriveddatacache', '__pycache__', '.vs', 'node_modules'}
BLOCKED_SUFFIXES = {'.sav', '.log', '.pem', '.key', '.pfx', '.p12', '.exe', '.dll', '.zip', '.pdb'}
REQUIRED = {'README.md', 'docs/RELEASES.md', 'docs/RELEASE-CHECKLIST.md',
            'work/earthtotaled/dist/game.js', 'work/earthtotaled/dist/vendor/THREE-LICENSE.txt',
            'work/earthtotaled-unreal/EARTHtotaled.uproject',
            'work/earthtotaled-unreal/ArtSource/manifest.json',
            'work/earthtotaled-unreal/Content/Data/design.json'}


def git(*arguments):
    result = subprocess.run(['git', '-C', str(WORKSPACE), *arguments],
                            capture_output=True, check=False)
    if result.returncode:
        raise RuntimeError('Source export requires the complete EARTHtotaled Git checkout.')
    return result.stdout.decode('utf-8').strip() if '-z' not in arguments else result.stdout.decode('utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=WORKSPACE / 'outputs/EARTHtotaled-complete-source.zip')
    output = parser.parse_args().output.resolve()
    if Path(git('rev-parse', '--show-toplevel')).resolve() != WORKSPACE:
        raise RuntimeError('Run this exporter from the complete EARTHtotaled Git checkout.')
    if git('ls-files', '--unmerged'):
        raise RuntimeError('Resolve merge conflicts before exporting source.')
    if git('ls-files', '--others', '--exclude-standard', '-z'):
        raise RuntimeError('Review and stage intended new source files, or ignore local files, before exporting.')
    files = sorted(filter(None, git('ls-files', '-z').split('\0')))
    missing = REQUIRED.difference(files)
    if missing:
        raise RuntimeError('Required source is missing: ' + ', '.join(sorted(missing)))

    for name in files:
        relative = PurePosixPath(name)
        parts = {part.lower() for part in relative.parts}
        if (BLOCKED_PARTS.intersection(parts) or relative.suffix.lower() in BLOCKED_SUFFIXES
                or relative.name.lower().startswith('.env')
                or relative.name.lower() == 'migration-status.md'):
            raise RuntimeError('Local/private or generated file is tracked; exclude it before export: ' + name)
        source = WORKSPACE / relative
        if source.is_symlink() or not source.is_file() or not source.resolve().is_relative_to(WORKSPACE):
            raise RuntimeError('Source must be a regular file inside the checkout: ' + name)
        if source.resolve() == output:
            raise RuntimeError('The ZIP cannot overwrite a tracked source file.')

    manifest = {'schema': 1, 'base_commit': git('rev-parse', 'HEAD'),
                'has_uncommitted_changes': bool(git('status', '--porcelain', '--untracked-files=no')),
                'files': {}}
    output.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(prefix='earthtotaled-source-', suffix='.zip', dir=output.parent)
    os.close(descriptor)
    temporary = Path(temporary_name)
    try:
        with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
            for name in files:
                content = (WORKSPACE / name).read_bytes()
                manifest['files'][name] = hashlib.sha256(content).hexdigest()
                archive.writestr(PREFIX + name, content)
            archive.writestr(PREFIX + 'SOURCE-MANIFEST.json', json.dumps(manifest, indent=2) + '\n')
        with zipfile.ZipFile(temporary) as archive:
            if archive.testzip() is not None:
                raise RuntimeError('Source archive CRC verification failed.')
            expected = {PREFIX + name for name in files} | {PREFIX + 'SOURCE-MANIFEST.json'}
            if set(archive.namelist()) != expected:
                raise RuntimeError('Source archive does not contain the exact reviewed file selection.')
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    with output.open('rb') as stream:
        digest = hashlib.file_digest(stream, 'sha256').hexdigest()
    print(json.dumps({'archive': str(output), 'source_files': len(files), 'bytes': output.stat().st_size,
                      'sha256': digest, 'base_commit': manifest['base_commit'],
                      'has_uncommitted_changes': manifest['has_uncommitted_changes']}, indent=2))


if __name__ == '__main__':
    main()
