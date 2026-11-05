"""Thin ctypes binding to libcuddle's room engine; no independent room logic."""
import ctypes as C
import ctypes.util
import os
from pathlib import Path

class Choice(C.Structure):
    _fields_ = [('label', C.c_char_p), ('target', C.c_int), ('signal', C.c_uint), ('value', C.c_uint), ('navigation', C.c_int)]
class Step(C.Structure):
    _fields_ = [('title', C.c_char_p), ('body', C.c_char_p), ('choices', C.POINTER(Choice)), ('choice_count', C.c_size_t), ('record', C.c_int)]
class Room(C.Structure):
    _fields_ = [('id', C.c_char_p), ('title', C.c_char_p), ('summary', C.c_char_p), ('kind', C.c_int), ('steps', C.POINTER(Step)), ('step_count', C.c_size_t)]
class Session(C.Structure):
    _fields_ = [('room_index', C.c_size_t), ('step_index', C.c_size_t), ('status', C.c_int), ('signal_bits', C.c_uint), ('answered', C.c_uint), ('unknown', C.c_uint), ('zones', C.c_uint * 12), ('history', C.c_size_t * 32), ('history_count', C.c_size_t), ('selections', C.c_size_t * 32)]
class Reading(C.Structure):
    _fields_ = [('label', C.c_char_p), ('advice', C.c_char_p)]

def decode(value):
    return value.decode('utf-8') if value else ''

def _load():
    prefix = Path(__file__).resolve().parents[2]
    candidates = [os.environ.get('CUDDLE_LIB')]
    for folder in ('lib', 'lib64'):
        candidates.append(str(prefix / folder / 'libcuddle.so.0'))
        candidates.extend(str(p) for p in (prefix / folder).glob('*/libcuddle.so.0'))
    candidates.append(ctypes.util.find_library('cuddle'))
    for name in candidates:
        if not name:
            continue
        try:
            library = C.CDLL(name)
            library.cuddle_room_count
            return library
        except (OSError, AttributeError):
            pass
    raise RuntimeError('Matching libcuddle rooms library not found. Rebuild/install Cuddle, or set CUDDLE_LIB to the new libcuddle.so.0.')

class Config(C.Structure):
    _fields_ = [('datadir_override', C.c_char * 512), ('browser', C.c_char * 256),
                ('tui_screen', C.c_char * 64), ('theme', C.c_char * 64),
                ('start_room', C.c_char * 64), ('large_text', C.c_int), ('appearance', C.c_int)]
class Preferences(C.Structure):
    _fields_ = [('theme', C.c_char * 64), ('start_room', C.c_char * 64),
                ('large_text', C.c_int), ('appearance', C.c_int)]

lib = _load()
def bind(name, result, *args):
    fn = getattr(lib, name)
    fn.restype, fn.argtypes = result, list(args)
    return fn
count = bind('cuddle_room_count', C.c_size_t)
at = bind('cuddle_room_at', C.POINTER(Room), C.c_size_t)
begin = bind('cuddle_session_begin', C.c_int, C.POINTER(Session), C.c_char_p)
current_room = bind('cuddle_session_room', C.POINTER(Room), C.POINTER(Session))
current_step = bind('cuddle_session_step', C.POINTER(Step), C.POINTER(Session))
choose = bind('cuddle_session_choose', C.c_int, C.POINTER(Session), C.c_size_t)
back = bind('cuddle_session_back', C.c_int, C.POINTER(Session))
stop = bind('cuddle_session_stop', None, C.POINTER(Session))
cycle_zone = bind('cuddle_session_cycle_zone', C.c_int, C.POINTER(Session), C.c_size_t)
zone = bind('cuddle_touch_zone', C.c_char_p, C.c_size_t)
status = bind('cuddle_touch_status', C.c_char_p, C.c_uint)
reading = bind('cuddle_interest_read', Reading, C.c_uint, C.c_uint, C.c_uint)
summary = bind('cuddle_session_summary', C.c_int, C.POINTER(Session), C.c_char_p, C.c_size_t)
soft_signal = bind('cuddle_soft_signal_text', C.c_char_p)
rooms = [at(i).contents for i in range(count())]

config_load = bind('cuddle_config_load', C.c_int, C.POINTER(Config))
config_apply = bind('cuddle_config_apply', None, C.POINTER(Config))
preferences_get = bind('cuddle_preferences_get', C.POINTER(Preferences))
config = Config()
config_load(C.byref(config))
config_apply(C.byref(config))
preferences = preferences_get().contents

version = bind('cuddle_version', C.c_char_p)
