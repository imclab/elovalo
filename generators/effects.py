#
# Copyright 2012 Elovalo project group
#
# This file is part of Elovalo.
#
# Elovalo is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Elovalo is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Elovalo.  If not, see <http://www.gnu.org/licenses/>.
#

import os
import re
from glob import glob

TICK_GRANULARITY = 0.008
DEFAULT_FPS = 30
DEFAULT_MINIMUM_TICKS = str(int(1.0 / (TICK_GRANULARITY * DEFAULT_FPS)))

file_start = '''/* GENERATED FILE! DON'T MODIFY!!!
 * Led cube effects
 */

#include <stdlib.h>
#include <stdint.h>
#include "../common/pgmspace.h"
#include "../common/env.h"
#include "../common/effects.h"
#include "../effects/common.h"
'''


def generate(source, target):
    inp = SourceFiles(glob(source))

    parent_dir = os.path.split(target)[0]

    if not os.path.exists(parent_dir):
        os.mkdir(parent_dir)

    with open(target, 'w') as t:
        t.write(file_start)
        t.write('\n')
        t.write(inp.init_definitions)
        t.write('\n')
        t.write(inp.effect_definitions)
        t.write('\n')
        t.write(inp.function_names)
        t.write('\n')
        t.write(inp.defines)
        t.write('\n')
        t.write(inp.function_declarations)
        t.write('\n')
        t.write(inp.union)
        t.write('\n')
        t.write(inp.effects)
        t.write('\n')
        t.write('const uint8_t effects_len = sizeof(effects) / ' +
                'sizeof(effect_t);\n')
        t.write('\n')
        t.write(inp.functions)

def generate_defines(source, target):
    # FIXME Not effective, reads all contents to get just file names
    inp = SourceFiles(glob(source))

    with open(target, 'w') as t:
        t.write('''/* GENERATED FILE! DON'T MODIFY!!!
 * Effect related constants
 */

#define EFFECT_JSON_LEN ''')
        t.write(str(inp.effect_json_len))
        t.write('\n')

class SourceFiles(object):

    def __init__(self, files):
        self._files = self._read(files)

    def _read(self, files):
        return [SourceFile(f) for f in files]

    @property
    def defines(self):
        return '\n'.join([f.defines for f in self._files]) + '\n'

    @property
    def function_declarations(self):
        return '\n'.join([f.function_declarations for f in self._files]) + '\n'

    @property
    def init_definitions(self):
        return self._definitions('init')

    @property
    def effect_definitions(self):
        return self._definitions('effect')

    def _definitions(self, k):
        init = lambda n: 'static void ' + k + '_' + n + '(void);'

        return '\n'.join([
                init(f.name) for f in self._files if getattr(f, k)
            ]) + '\n'

    @property
    def effect_json_len(self):
        return 1+sum([f.name.__len__()+3 for f in self._files])

    @property
    def function_names(self):
        name = lambda n: 'PROGMEM const char s_' + n + '[] = "' + n + '";'

        return '\n'.join([name(f.name) for f in self._files]) + '\n'

    @property
    def union(self):
        struct = lambda f: f.variables.replace('vars', f.name)

        ret = ['static union {']

        ret.extend([struct(f) for f in self._files if f.variables])

        ret.append('} vars;')

        return '\n'.join(ret) + '\n'

    @property
    def effects(self):
        definition = lambda f: '\t{ s_' + f.name + ', ' + init(f) + ', ' + \
            effect(f) + ', ' + flip(f) + ', ' + f.max_fps + ', ' + \
            dynamic_text(f) + ' },'
        init = lambda f: '&init_' + f.name if f.init else 'NULL'
        effect = lambda f: '&effect_' + f.name if f.effect else 'NULL'
        flip = lambda f: 'FLIP' if f.flip else 'NO_FLIP'
        dynamic_text = lambda f: 'true' if f.dynamic_text else 'false'

        ret = ['const effect_t effects[] PROGMEM = {']

        ret.extend([definition(f) for f in self._files])

        ret.append('};')

        return '\n'.join(ret) + '\n'

    @property
    def functions(self):
        merge = lambda f: f.typedefs + f.globs + \
            f.functions + f.init + f.effect

        return '\n'.join(merge(f) for f in self._files) + '\n'


class SourceFile(object):

    def __init__(self, path):
        self.path = ''.join(path.rpartition('src')[1:])
        self.name = os.path.splitext(os.path.basename(path))[0]

        with open(path, 'r') as f:
            content = analyze(self.name, f.readlines())

        self.defines = self._block(content, 'define')
        self.typedefs = self._block(content, 'typedef')
        self.globs = self._globals(content)
        self.function_declarations = self._function_declarations(content)
        self.functions = self._functions(content)
        self.init = self._block(content, 'init')
        self.effect = self._block(content, 'effect')
        self.flip = self._flip(content)
        self.max_fps = self._max_fps(content)
        self.dynamic_text = self._dynamic_text(content)
        self.variables = self._variables(content)

    def _globals(self, c):
        return '\n'.join(find_globals(c))

    def _function_declarations(self, c):
        return ''.join(line['content'] for line in c
            if 'function_declaration' in line['types']
        )

    def _functions(self, c):
        return ''.join(block_with_meta(line, self.path) for line in c
            if 'function' in line['types'] and len(line['types']) == 1
        )

    def _block(self, c, name):
        return ''.join(block_with_meta(line, self.path) for line in c
                if name in line['types'])

    def _flip(self, c):
        return filter(lambda line: 'flip' in line['types'], c)

    def _max_fps(self, c):
        max_fps = filter(lambda line: 'max_fps' in line['types'], c)

        if len(max_fps):
            i = int(max_fps[0]['content'].split()[-1].strip('\n'))

            return str(int(1.0 / (TICK_GRANULARITY * i)))

        return DEFAULT_MINIMUM_TICKS

    def _dynamic_text(self, c):
        return filter(lambda line: 'dynamic_text' in line['types'], c)

    def _variables(self, c):
        a = filter(lambda line: 'struct' in line['types'], c)

        if a and a[0]['block'].endswith('vars;\n'):
            return a[0]['block']


def block_with_meta(line, path):
    line_number = line['line_number']
    return '\n'.join(['#line ' + str(line_number + i) + ' ' +
        '"' + path + '"' + '\n' + p
        for i, p in enumerate(line['block'].split('\n')) if p.strip()]) + '\n'


def find_globals(content):
    ret = []

    i = 0
    while i < len(content):
        c = content[i]

        if 'assignment' in c['types']:
            ret.append(c['content'])
        if 'function' in c['types']:
            i += len(c['block'].split('\n'))
        else:
            i += 1

    return ret


def analyze(name, content):
    def analyze_line(i, line, line_offset):
        types = '(void|uint8_t|uint16_t|float|int|char|double)'
        patterns = (
            ('flip', '#\s*pragma\s+FLIP\s*'),
            ('dynamic_text', '#\s*pragma\s+DYNAMIC_TEXT\s*'),
            ('max_fps', '#\s*pragma\s+MAX_FPS\s+[0-9]+\s*'),
            ('init', 'void\s+init\s*[(]'),
            ('effect', '\s*effect\s*'),
            ('define', '\s*#define\s+[a-zA-Z_0-9]+\s*[a-zA-Z0-9]*\s*'),
            ('typedef', '\s*typedef\s+struct\s*[{]'),
            ('struct', '\s*struct\s*[{]'),
            ('function', '\s*' + types + '(\s+\w+\s*[(])'),
            ('assignment', '[a-zA-Z]+(\s+\w+)'),
        )
        t = [n for n, p in patterns if len(re.compile(p).findall(line)) > 0]

        ret = {
            'content': line,
            'types': t,  # TODO: might be nicer to use a set for this
            'index': i,
        }

        # TODO: fix the regex. matches too much
        if 'assignment' in ret['types'] and 'function' in ret['types']:
            ret['types'].remove('assignment')

        # TODO: fix the regex. matches too much
        if 'assignment' in ret['types'] and ret['content'].startswith('\t'):
            ret['types'].remove('assignment')

        # TODO: fix the regex, matches too much
        if 'flip' in ret['types'] or 'max_fps' in ret['types'] or 'dynamic_text' in ret['types']:
            ret['types'].remove('assignment')

        # TODO: fix the regex, matches too much
        if ret['content'].startswith('typedef ') \
                and 'assignment' in ret['types']:
            ret['types'].remove('assignment')

        # TODO: fix the regex, matches too much
        if ret['content'].startswith('struct ') \
                and 'assignment' in ret['types']:
            ret['types'].remove('assignment')

        # TODO: fix the regex, matches too much
        if 'define' in ret['types']:
            ret['types'].remove('assignment')

        block = ''
        # TODO: fix the regex. does not match enough
        if 'effect' in ret['types'] and 'function' not in ret['types']:
            ret['types'].append('function')

            if 'XY(' in ret['content']:
                block = parse_xy(name, content, ret, i)
            if 'XYZ(' in ret['content']:
                block = parse_xyz(name, content, ret, i)
        elif 'function' in ret['types']:
            if line.strip()[-1] == ';':
                ret['types'].remove('function')
                ret['types'].append('function_declaration')
            else:
                block = parse_function(name, content, ret, i)
        elif 'typedef' in ret['types']:
            block = parse_generic(name, content, ret, i)
        elif 'struct' in ret['types']:
            block = parse_generic(name, content, ret, i)
        elif 'define' in ret['types']:
            block = parse_generic(name, content, ret, i)

        ret['block'] = block

        ret['line_number'] = line_offset + i + 1

        return ret

    content_len = len(content)
    content = remove_comments(content)
    line_offset = content_len - len(content)
    content = replace_variables(content, 'vars.', 'vars.' + name + '.')

    return [analyze_line(i, line, line_offset) for i, line in enumerate(content)]


def remove_comments(text):
    # http://stackoverflow.com/a/241506
    def replacer(match):
        s = match.group(0)

        return '' if s.startswith('/') else s

    pattern = re.compile(
        r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"',
        re.DOTALL | re.MULTILINE
    )

    id = lambda a: a
    add_newline = lambda a: a + '\n'
    return map(add_newline, filter(id, re.sub(pattern, replacer,
        ''.join(text)).split('\n')))


def replace_variables(text, source, target):
    return [line.replace(source, target) for line in text]


def generic_definition(name, line):
    return line['content']


def parse_generic(name, lines, line, index):
    return _parse(name, lines, line, index, generic_definition)


def parse_xy(name, lines, line, index):
    return _parse(name, lines, line, index, xy_definition)


def xy_definition(name, line):
    ret = 'XY(effect_' + name + ') '

    if line['content'].strip()[-1] == '{':
        ret += ' {\n}'

    return ret


def parse_xyz(name, lines, line, index):
    return _parse(name, lines, line, index, xyz_definition)


def xyz_definition(name, line):
    ret = 'XYZ(effect_' + name + ') '

    if line['content'].strip()[-1] == '{':
        ret += ' {\n}'

    return ret


def parse_function(name, lines, line, index):
    return _parse(name, lines, line, index, function_definition)


def _parse(name, lines, line, index, definition):
    ret = definition(name, line)
    cc = line['content']

    if cc.startswith('#'):
        return ret

    begin_braces = cc.count('{')
    end_braces = 0
    offset = 0
    if not begin_braces:
        begin_braces, end_braces, offset = find_begin_braces(
            cc, index + 1
        )

    for i in range(index + 1 + offset, len(lines)):
        cc = lines[i]
        ret += cc
        begin_braces += cc.count('{')
        end_braces += cc.count('}')

        if begin_braces == end_braces:
            return ret

    return ret


def function_definition(name, line):
    ret = None

    if 'init' in line['types']:
        ret = 'static void init_' + name + '(void)'
    elif 'effect' in line['types']:
        ret = 'void effect_' + name + '(void)'
    else:
        return line['content']

    if '{' in line['content']:
        ret += ' {\n'

    return ret


def find_begin_braces(content, i):
    for j in range(i, len(content)):
        b = content.count('{')

        if b:
            return b, content.count('}'), j - i

    return 0, 0, 0
