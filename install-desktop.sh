#!/usr/bin/env bash
# User-local Crostini desktop entries. No sudo or ChromeOS configuration.
set -Eeuo pipefail
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
python3 - "$PROJECT_ROOT" <<'PY'
import os,shutil,subprocess,sys,time
from pathlib import Path
root=Path(sys.argv[1]); prefix=root/'build-current/native-stage'
if not (prefix/'bin/cuddle-gui').is_file():
 raise SystemExit('Build first: bash '+str(root/'rebuild-cuddle.sh'))
data=Path(os.environ.get('XDG_DATA_HOME',str(Path.home()/'.local/share')))
apps=data/'applications';icons=data/'icons/hicolor/scalable/apps'
apps.mkdir(parents=True,exist_ok=True);icons.mkdir(parents=True,exist_ok=True)
def quote(path):
 return '"'+str(path).replace('\\','\\\\').replace('"','\\"').replace('`','\\`').replace('$','\\$')+'"'
for name,binary,title,terminal in [
 ('org.softnessasstrength.Cuddle','cuddle-gui','Cuddle',False),
 ('org.softnessasstrength.Cuddle.Terminal','cuddle-tui','Cuddle Terminal',True)]:
 p=apps/(name+'.desktop')
 if p.exists():
  backup=root/'backups'/('desktop-'+time.strftime('%Y%m%d-%H%M%S')+'-'+p.name)
  shutil.copy2(p,backup)
 p.write_text('[Desktop Entry]\nType=Application\nName='+title+'\nComment=Offline Litmo rooms and Soft Signal\nExec='+quote(prefix/'bin'/binary)+'\nTryExec='+str(prefix/'bin'/binary)+'\nIcon=org.softnessasstrength.Cuddle\nTerminal='+str(terminal).lower()+'\nCategories=Utility;\nKeywords=Litmo;Soft Signal;consent;local;\nStartupNotify=true\n')
 p.chmod(0o644)
 validator=shutil.which('desktop-file-validate')
 if validator:subprocess.run([validator,str(p)],check=True)
 print('Installed:',p)
shutil.copy2(root/'data/icons/hicolor/scalable/apps/org.softnessasstrength.Cuddle.svg',icons/'org.softnessasstrength.Cuddle.svg')
cache=shutil.which('update-desktop-database')
if cache:subprocess.run([cache,str(apps)],check=True)
print('Look for Cuddle and Cuddle Terminal in the Linux apps launcher. Garcon may take a moment to refresh.')
PY
