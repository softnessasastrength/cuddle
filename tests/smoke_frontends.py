#!/usr/bin/env python3
"""Exercise staged dispatch and actual terminal key paths; never open a browser."""
import argparse
import errno
import fcntl
import os
from pathlib import Path
import pty
import select
import struct
import subprocess
import tempfile
import termios
import time


def terminal(argv, env, initial, interactions):
    master, slave = pty.openpty()
    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack('HHHH', 30, 100, 0, 0))
    proc = subprocess.Popen(list(map(str, argv)), stdin=slave, stdout=slave, stderr=slave, env=dict(env, TERM='xterm-256color'))
    os.close(slave)
    received = bytearray()
    def wait_for(text):
        deadline = time.monotonic() + 6
        needle = text.encode()
        while time.monotonic() < deadline:
            if needle in received:
                received.clear()
                return
            readable, _, _ = select.select([master], [], [], 0.1)
            if readable:
                try:
                    chunk = os.read(master, 65536)
                except OSError as exc:
                    if exc.errno == errno.EIO:
                        break
                    raise
                if not chunk:
                    break
                received.extend(chunk)
        raise AssertionError(f'{argv}: missing {text!r}; output={bytes(received)!r}')
    try:
        wait_for(initial)
        for keys, expected in interactions:
            os.write(master, keys)
            if expected:
                wait_for(expected)
        assert proc.wait(timeout=4) == 0, argv
    finally:
        if proc.poll() is None:
            proc.kill()
        proc.wait()
        os.close(master)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--tui', choices=['python', 'curses'], required=True)
    parser.add_argument('--gui', choices=['html', 'gtk', 'qt'], required=True)
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    cli, tui, gui = [prefix / 'bin' / name for name in ('cuddle', 'cuddle-tui', 'cuddle-gui')]
    with tempfile.TemporaryDirectory(prefix='cuddle-smoke-') as temp:
        env = dict(os.environ, XDG_CONFIG_HOME=temp, CUDDLE_BINDIR=str(prefix / 'bin'), CUDDLE_SHARE=str(prefix / 'share/cuddle'))
        def run(argv, code=0, contains=None, localenv=None):
            result = subprocess.run(list(map(str, argv)), stdin=subprocess.DEVNULL, capture_output=True, text=True, env=localenv or env, timeout=8)
            assert result.returncode == code, (argv, result.returncode, result.stderr)
            if contains:
                assert contains in result.stdout, (argv, result.stdout)
        for name in ('conflict', 'interest', 'repair', 'touch', 'aftercare'):
            run([cli, '--rooms'], contains=name)
        run([cli, '--room', 'unknown-room'], code=2)
        run([cli, '--room', 'conflict'], code=2)
        run([cli, '--tui', 'interest'], code=2)
        run([tui, 'invalid-room'], code=2)
        for binary in (cli, tui, gui):
            run([binary, '--version'], contains='1.4.0')
        if args.gui == 'html':
            # Exercise config loading and CLI -> GUI -> browser dispatch without launching ChromeOS.
            cfg = Path(temp) / 'cuddle'
            cfg.mkdir()
            (cfg / 'cuddle.conf').write_text('browser=/bin/echo\n')
            run([gui], contains='gui/stop.html')
            run([cli, '--gui'], contains='gui/stop.html')
        if args.tui == 'python':
            for room in ('hub', 'flood', 'touch', 'signal', 'conflict', 'interest', 'morning', 'spoon', 'too-much', 'parallel', 'aftercare', 'repair'):
                run(['python3', prefix / 'share/cuddle/tui.py', room, '--dump'])
        terminal([cli, '--flood'], env, 'STOP', [(b'q\n', 'Ended immediately.')])
        terminal([tui, 'touch'], env, 'off limits', [(b' ', 'ask first'), (b'q', 'Ended immediately.'), (b'q', None)])
        terminal([tui, 'conflict'], env, 'Choose a rehearsal', [(b'2', 'Repair script'), (b'b', 'Choose a rehearsal'), (b'q', 'Ended immediately.'), (b'q', None)])
        terminal([tui, 'interest'], env, 'Body want', [
            (b'3', 'Mind'), (b'2', 'Should'), (b'2', 'Fear if no'),
            (b'2', 'looded'), (b'2', 'Performing'), (b'2', 'Unknown / knowing can wait'),
            (b'q', 'Ended immediately.'), (b'q', None)])
        terminal([tui, 'morning'], env, 'Morning capacity', [
            (b'1', 'Duration'), (b'1', 'Morning style'), (b'6', 'Advanced gremlin tech'),
            (b'1', 'Body boundaries'), (b'1', 'Check-in phrase'), (b'1', 'Your morning discussion map'),
            (b'q', 'Ended immediately.'), (b'q', None)])
        cfg = Path(temp) / 'cuddle'
        cfg.mkdir(exist_ok=True)
        (cfg / 'cuddle.conf').write_text('start_room=morning\nappearance=dark\nlarge_text=true\ntheme=dim\n')
        terminal([tui], env, 'Morning capacity', [(b'q', 'Ended immediately.'), (b'q', None)])
        terminal([cli, '--tui'], env, 'Morning capacity', [(b'q', 'Ended immediately.'), (b'q', None)])
        terminal([cli], env, 'Morning capacity', [(b'q\n', 'Ended immediately.')])
        if args.tui == 'python':
            run(['python3', prefix / 'share/cuddle/tui.py', '--dump'], contains='Morning capacity')
    print(f'PASS: {args.tui}/{args.gui}: staged dispatch, room catalog, terminal stop, touch, conflict branches, unknown interest, rich Morning choices and shared startup preferences')

if __name__ == '__main__':
    main()
