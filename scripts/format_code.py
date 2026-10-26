"""Форматирование C99: Allman, один оператор на строку, две пустые строки после функций.

Использует clang-format из PATH или указанный через --formatter файл.
Шрифты и заставка содержат растры, поэтому остаются без изменений.
"""

import argparse
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parent.parent
TOKEN = re.compile(
    r'(?P<space>\s+)|(?P<comment>//[^\n]*|/\*[\s\S]*?\*/)'
    r'|(?:u8|u|U|L)?"(?:\\[\s\S]|[^"\\])*"'
    r"|(?:u|U|L)?'(?:\\[\s\S]|[^'\\])*'"
    r'|[A-Za-z_][A-Za-z_0-9]*'
    r'|(?:\d|\.\d)(?:[A-Za-z_0-9.]|[eEpP][+-])*'
    r'|>>=|<<=|\.\.\.|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\|'
    r'|\*=|/=|%=|\+=|-=|&=|\^=|\|=|##|[^\s]'
)


def c_tokens(text):
    """Сравнивает токены, сохраняя строки и учитывая склейку строк препроцессора."""
    text = re.sub(r'\\\r?\n', '', text)
    return [
        match.group()
        for match in TOKEN.finditer(text)
        if match.lastgroup not in ('space', 'comment')
    ]


def separate_functions(text):
    """После clang-format отдельная '}' в нулевой колонке завершает функцию C."""
    text = text.replace('\r\n', '\n')
    pattern = re.compile(r'(?m)^}(?:[ \t]*//[^\n]*)?[ \t]*\n(?:[ \t]*\n)*')

    def replace(match):
        last_line = match.group().splitlines()[0].rstrip()
        return last_line + ('\n\n\n' if text[match.end():].strip() else '\n')

    return pattern.sub(replace, text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--formatter', default=shutil.which('clang-format'))
    parser.add_argument('--check', action='store_true', help='Проверить без изменения файлов')
    args = parser.parse_args()
    if not args.formatter:
        parser.error('Укажи путь к clang-format.exe через --formatter')

    sources = sorted(
        path
        for directory in ('src', 'tests')
        for path in (ROOT / directory).iterdir()
        if path.suffix in ('.c', '.h') and path.name != 'startup_splash.h'
    )
    changes = []
    for path in sources:
        before_bytes = path.read_bytes()
        before = before_bytes.decode('utf-8')
        result = subprocess.run(
            [args.formatter, '-style=file', str(path)],
            check=True, capture_output=True
        )
        after = separate_functions(result.stdout.decode('utf-8'))
        if c_tokens(before) != c_tokens(after):
            raise SystemExit(f'Отказ: форматтер изменил токены {path.relative_to(ROOT)}')
        if before_bytes != after.encode('utf-8'):
            changes.append((path, after))

    for path, text in changes:
        print(('NEEDS FORMAT ' if args.check else 'FORMAT ') + str(path.relative_to(ROOT)))
        if not args.check:
            path.write_bytes(text.encode('utf-8'))
    print(f'Checked {len(sources)} C files; {len(changes)} need formatting.')
    if args.check and changes:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
