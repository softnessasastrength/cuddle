#!/usr/bin/env python3
"""Local Litmo rooms. Navigation, answers and stop behavior live in libcuddle."""
import argparse
import ctypes as C
import curses
import sys
import textwrap

try:
    import room_bridge as core
except RuntimeError as exc:
    raise SystemExit(str(exc))

class State:
    def __init__(self, start='hub'):
        self.session = core.Session()
        core.stop(C.byref(self.session))
        self.hub = start == 'hub'
        self.selected = 0
        self.choice = 0
        self.scroll = 0
        if start not in ('hub', 'signal') and core.begin(C.byref(self.session), start.encode()):
            raise ValueError('Unknown room: ' + start)

    def rows(self):
        if self.hub:
            rows = ['What would help right now?', 'Touch Language is a map, never consent.', '']
            rows += [('> ' if i == self.selected else '  ') + core.decode(r.title) for i, r in enumerate(core.rooms)]
            return rows + ['', core.decode(core.rooms[self.selected].summary)]
        if self.session.status != 1:
            return (['You can land here.', 'Completing a room proves nothing. Nothing is saved.'] if self.session.status == 2
                    else core.decode(core.soft_signal()).splitlines()) + ['', 'Enter returns to rooms. q leaves.']
        r = core.current_room(C.byref(self.session)).contents
        rows = [core.decode(r.title), core.decode(r.summary), '']
        if r.kind == 1:
            rows += [('> ' if i == self.choice else '  ') + core.decode(core.zone(i)) + ': ' + core.decode(core.status(self.session.zones[i])) for i in range(12)]
            return rows + ['', 'Enter / space cycles the selected map label. Never permission.']
        step = core.current_step(C.byref(self.session)).contents
        rows += [core.decode(step.title), core.decode(step.body), '']
        if core.decode(r.id) in ('morning', 'spoon') and self.session.step_index + 1 == r.step_count:
            buffer = C.create_string_buffer(4096)
            if not core.summary(C.byref(self.session), buffer, len(buffer)):
                rows += [core.decode(buffer.value), '']

        if r.kind == 2 and self.session.step_index == 6:
            result = core.reading(self.session.signal_bits, self.session.answered, self.session.unknown)
            rows += [core.decode(result.label), core.decode(result.advice), '']
        labels = [core.decode(step.choices[i].label) for i in range(step.choice_count)] or ['Next / skip']
        return rows + [('> ' if i == self.choice else '  ') + f'{i+1}) {label}' for i, label in enumerate(labels)]

    def key(self, key):
        if key in (ord('q'), ord('Q'), 27):
            if not self.hub and self.session.status != 1:
                return False
            core.stop(C.byref(self.session))
            self.hub = False
            self.choice = self.scroll = 0
            return True
        if key in (curses.KEY_NPAGE, curses.KEY_PPAGE):
            self.scroll += 8 if key == curses.KEY_NPAGE else -8
            self.scroll = max(0, self.scroll)
            return True
        enter = key in (10, 13, curses.KEY_ENTER, ord(' '))
        if not self.hub and self.session.status != 1:
            if enter:
                self.hub = True
                self.scroll = 0
            return True
        if self.hub:
            count = len(core.rooms)
        else:
            room = core.current_room(C.byref(self.session)).contents
            count = 12 if room.kind == 1 else max(1, core.current_step(C.byref(self.session)).contents.choice_count)
        if key in (curses.KEY_UP, ord('k'), curses.KEY_DOWN, ord('j')):
            delta = -1 if key in (curses.KEY_UP, ord('k')) else 1
            if self.hub:
                self.selected = (self.selected + delta) % count
            else:
                self.choice = (self.choice + delta) % count
        elif key == ord('b') and not self.hub:
            core.back(C.byref(self.session))
            self.choice = self.scroll = 0
        elif enter or (not self.hub and ord('1') <= key <= ord('6')):
            if self.hub:
                core.begin(C.byref(self.session), core.rooms[self.selected].id)
                self.hub = False
            elif room.kind == 1:
                core.cycle_zone(C.byref(self.session), self.choice)
            else:
                choice = key - ord('1') if ord('1') <= key <= ord('6') else self.choice
                core.choose(C.byref(self.session), choice)
                self.choice = 0
            self.scroll = 0
        return True

def wrapped(rows, width):
    result = []
    for row in rows:
        for line in row.split('\n'):
            result.extend(textwrap.wrap(line, max(1, width), replace_whitespace=False) or [''])
    return result

def loop(screen, start):
    state = State(start)
    curses.curs_set(0)
    screen.keypad(True)
    if curses.has_colors() and core.preferences.appearance:
        curses.start_color()
        foreground, background = (curses.COLOR_WHITE, curses.COLOR_BLACK) if core.preferences.appearance == 2 else (curses.COLOR_BLACK, curses.COLOR_WHITE)
        curses.init_pair(1, foreground, background)
        screen.bkgd(' ', curses.color_pair(1))
    try:
        while True:
            height, width = screen.getmaxyx()
            content = state.rows()
            if core.preferences.large_text:
                content = [part for line in content for part in (line, '')]
            rows = wrapped(content, width - 2)
            visible = max(1, height - 3)
            state.scroll = min(state.scroll, max(0, len(rows) - visible))
            # Keep the current menu choice reachable in a small terminal.
            if state.hub or (state.session.status == 1 and core.current_room(C.byref(state.session)).contents.kind == 1):
                focused = next((i for i, row in enumerate(rows) if row.startswith('> ')), 0)
                if focused < state.scroll or focused >= state.scroll + visible:
                    state.scroll = max(0, focused - visible + 1)
            screen.erase()
            def draw(y, text, style=0):
                try:
                    screen.addnstr(y, 0, text, max(0, width - 1), style)
                except curses.error:
                    pass
            draw(0, 'Cuddle · local only · nothing saved', curses.A_REVERSE)
            for y, row in enumerate(rows[state.scroll:state.scroll + visible], 2):
                draw(y, row, curses.A_BOLD if row.startswith('> ') else 0)
            draw(height - 1, 'up/down · enter choose · b back · PgDn scroll · q STOP', curses.A_REVERSE)
            screen.refresh()
            try:
                key = screen.getch()
            except KeyboardInterrupt:
                key = ord('q')
            if not state.key(key):
                break
    finally:
        core.stop(C.byref(state.session))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('screen', nargs='?', default=core.decode(core.preferences.start_room), choices=['hub', 'signal'] + [core.decode(r.id) for r in core.rooms])
    parser.add_argument('--version', action='version', version='cuddle-tui ' + core.decode(core.version()))
    parser.add_argument('--dump', action='store_true', help='render without a terminal')
    parser.add_argument('--width', type=int, default=80)
    parser.add_argument('--height', type=int, default=24)
    args = parser.parse_args()
    if args.dump:
        state = State(args.screen)
        print('\n'.join(wrapped(state.rows(), max(1, args.width))[:max(1, args.height)]))
        core.stop(C.byref(state.session))
        return 0
    if not sys.stdin.isatty() or not sys.stdout.isatty():
        print('cuddle-tui needs an interactive terminal.', file=sys.stderr)
        return 2
    curses.wrapper(lambda screen: loop(screen, args.screen))
    return 0

if __name__ == '__main__':
    sys.exit(main())
