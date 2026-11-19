'use strict';

/**
 * `.lep` 折叠块的配对（纯计算，不依赖 VS Code，便于直接测）。
 *
 * 与 LEPUS 编译器的"定义期配对检查"同口径：
 *   - 宏模板由花括号配对；
 *   - 配对用一个栈，交叉写错时不会把层次折乱；
 *   - 收尾还没写完（编辑到一半）时按文件末尾收，块不会突然消失。
 *
 * 指令名按**完整词面**判断：`$match_if` 不是 `$match`。
 * 行号是 0 基、闭区间，和 VS Code 的 `FoldingRange` 一致。
 */

/** 开标签 → 折叠后显示的提示文字。顺序就是判断顺序。 */
const OPENERS = [
  ['$macro', '宏定义'],
  ['$for', 'for 展开'],
  ['$if', '条件展开'],
  ['$while', 'while 展开'],
  ['$loop', 'loop 展开'],
  ['$match', '语法匹配'],
  ['$regex', '正则匹配'],
];

/**
 * 这一行是不是完整的 `word` 指令（后面不能紧跟标识符字符）。
 *
 * @param {string} text 去掉首尾空白的整行。
 * @param {string} word 指令名，例如 `$macro`。
 * @returns {boolean} 是返回 true。
 */
function isDirective(text, word) {
  if (text.length < word.length || !text.startsWith(word)) return false;
  if (text.length === word.length) return true;
  const next = text[word.length];
  return !/[A-Za-z0-9_]/.test(next);
}

/**
 * 找出同行宏头后开始模板的左花括号。
 * @param {string} text 已屏蔽注释和字面量的行。
 * @param {string} directive 匹配到的开标签。
 * @returns {number} 模板起点，旧式写法返回 -1。
 */
function braceOpening(text, directive) {
  if (directive === '$macro') {
    const arrow = /->\s*(?:(?:expr|stmt|item|member)\b)?\s*(?=\{)/.exec(text);
    return arrow ? arrow.index + arrow[0].length : -1;
  }
  const tail = text.slice(directive.length);
  let at = tail.search(/\S/);
  if (at < 0) return -1;
  if (tail[at] === '(') {
    let depth = 1;
    while (++at < tail.length) {
      if (tail[at] === '(') depth++;
      if (tail[at] === ')' && --depth === 0) break;
    }
    do { at++; } while (/\s/.test(tail[at] || ''));
  }
  return tail[at] === '{' ? directive.length + at : -1;
}

/**
 * 算出所有可折叠块。
 *
 * @param {string[]} lines 按行拆好的源码（不含换行符）。
 * @returns {{start: number, end: number, name: string}[]} 按起始行递增、只含跨行的块。
 */
function planFoldingRanges(lines) {
  const blocks = [];
  const open = [];
  let depth = 0;
  let inComment = false;
  const close = (index) => {
    const start = open.pop();
    if (index > start.line) blocks.push({ start: start.line, end: index, name: start.name });
  };
  for (let index = 0; index < lines.length; index++) {
    const raw = String(lines[index]);
    let code = '';
    for (let at = 0; at < raw.length;) {
      const start = at;
      if (inComment || raw.startsWith('/*', at)) {
        const end = raw.indexOf('*/', inComment ? at : at + 2);
        inComment = end < 0;
        at = inComment ? raw.length : end + 2;
      } else if (raw.startsWith('//', at)) {
        break;
      } else if (raw[at] === '"' || raw[at] === "'") {
        const quote = raw[at++];
        while (at < raw.length) {
          const ch = raw[at++];
          if (ch === '\\' && at < raw.length) at++;
          else if (ch === quote) break;
        }
      } else {
        code += raw[at++];
        continue;
      }
      code += ' '.repeat(at - start);
    }
    const text = code.trimStart();
    const opener = OPENERS.find(([word]) => isDirective(text, word));
    let brace = -1;
    if (opener) {
      const [directive] = opener;
      brace = braceOpening(text, directive);
    }
    for (let at = 0; at < text.length; at++) {
      if (text[at] === '{') {
        if (at === brace) open.push({ line: index, name: opener[1], depth });
        depth++;
      }
      if (text[at] === '}') {
        depth--;
        if (open.length && open[open.length - 1].depth === depth) close(index);
      }
    }
  }

  // 收尾没写完：按到文件末尾收，编辑中的代码也能折。
  const last = lines.length - 1;
  while (open.length > 0) {
    const start = open.pop();
    if (last > start.line) blocks.push({ start: start.line, end: last, name: start.name });
  }

  blocks.sort((a, b) => (a.start !== b.start ? a.start - b.start : a.end - b.end));
  return blocks;
}

module.exports = { planFoldingRanges };
