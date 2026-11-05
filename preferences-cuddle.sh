#!/usr/bin/env bash
# Edit only appearance/start preferences, never room answers. The app reads this file.
set -Eeuo pipefail
PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
python3 - "$PROJECT_ROOT" "$@" <<'PY'
import argparse,os,shutil,sys,time
from pathlib import Path
root=Path(sys.argv[1]);sys.argv=sys.argv[1:]
p=argparse.ArgumentParser(description='Set shared Cuddle preferences. Omitted options stay unchanged.')
p.add_argument('--appearance',choices=['system','light','dark'])
p.add_argument('--theme',choices=['default','dim'])
p.add_argument('--large-text',choices=['true','false'])
p.add_argument('--start-room',choices=['hub','flood','touch','conflict','interest','morning','spoon','too-much','parallel','aftercare','repair'])
a=p.parse_args()
config=Path(os.environ.get('XDG_CONFIG_HOME',str(Path.home()/'.config')))/'cuddle/cuddle.conf'
updates={k.replace('-','_'):v for k,v in vars(a).items() if v is not None}
if not updates:
 print('Config:',config)
 print(config.read_text() if config.exists() else 'No file; defaults: system appearance, normal text, hub.')
 p.print_help();raise SystemExit(0)
config.parent.mkdir(parents=True,exist_ok=True)
lines=config.read_text().splitlines() if config.exists() else ['# Cuddle appearance preferences. Room answers are never written here.']
if config.exists():
 backup=root/'backups'/('preferences-'+time.strftime('%Y%m%d-%H%M%S')+'-'+str(os.getpid())+'.conf')
 backup.parent.mkdir(exist_ok=True);shutil.copy2(config,backup)
result=[]
for line in lines:
 key=line.split('=',1)[0].strip() if '=' in line and not line.lstrip().startswith('#') else None
 if key not in updates:result.append(line)
result.extend(k+' = '+v for k,v in updates.items())
tmp=config.with_name('cuddle.conf.tmp-'+str(os.getpid()))
try:
 with tmp.open('x') as stream:stream.write('\n'.join(result)+'\n')
 tmp.chmod(0o600);tmp.replace(config)
finally:
 if tmp.exists():tmp.unlink()
print('Saved shared preferences:',config)
print('Restart Cuddle to apply them. Room answers were not saved.')
PY
