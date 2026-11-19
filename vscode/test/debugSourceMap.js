'use strict';

/** 源码双向映射的目录边界、最长前缀、宿主大小写和歧义拒绝。 */
const assert = require('node:assert/strict');
const path = require('node:path');
const { DebugSourceMap } = require('../src/debugSourceMap');
const local = path.resolve('tmp/source-map-workspace');
const map = new DebugSourceMap({ '/build/src': local, '/build/src/lib': path.join(local, 'vendor'), 'C:/Builder/中文': path.join(local, 'windows') });
assert.equal(map.toLocal('/build/src/main.lep'), path.join(local, 'main.lep'));
assert.equal(map.toLocal('/build/src/lib/row.lep'), path.join(local, 'vendor/row.lep'));
assert.equal(map.toTarget(path.join(local, 'vendor/row.lep')), '/build/src/lib/row.lep');
assert.equal(map.toTarget(path.join(local, 'main.lep')), '/build/src/main.lep');
assert.equal(map.toLocal('c:\\builder\\中文\\测试.lep'), path.join(local, 'windows/测试.lep'));
assert.equal(map.toLocal('/build/src2/other.lep'), '/build/src2/other.lep');
assert.equal(map.toLocal('/build/SRC/main.lep'), '/build/SRC/main.lep');
assert.equal(map.toLocal('/build/src/../outside.lep'), '/build/src/../outside.lep');
assert.throws(() => map.toTarget(path.join(local, 'lib/row.lep')), /覆盖/);
for (const input of [null, [], { relative: local }, { '/src': '' }, { '/src\n': local }, { '/src': 1 }, { '/a': local, '/b': local }]) assert.throws(() => new DebugSourceMap(input));
const root = new DebugSourceMap({ '/': local });
assert.throws(() => new DebugSourceMap({ '/a/../b': local, '/b': path.join(local, 'other') }), /目标.*不唯一/);
assert.throws(() => new DebugSourceMap({ 'C:/Build': local, 'c:/build': path.join(local, 'other') }), /目标.*不唯一/);
assert.equal(root.toTarget(path.join(local, 'a.lep')), '/a.lep');
assert.equal(root.toLocal('/a.lep'), path.join(local, 'a.lep'));
assert.equal(new DebugSourceMap().toLocal('<std::core>'), '<std::core>');
assert.equal(new DebugSourceMap({ '/builder': process.cwd() }).toTarget('<std::core>'), '<std::core>');
console.log('源码映射：双向最长前缀、目录边界、中文、大小写、根路径与歧义拒绝通过');
