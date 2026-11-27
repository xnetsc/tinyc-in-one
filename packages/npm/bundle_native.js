#!/usr/bin/env node
/* Stage the unified (host x target) layout into this package and into
 * the esbuild-style platform subpackage @xnetx/tinyc-native-<host>.
 *
 *   node bundle_native.js [host-triple]     (default: current host)
 *   node bundle_native.js --merge-darwin    (stage both macOS hosts
 *                                            into @xnetx/tinyc-native-apple-darwin)
 *
 * Produces, in both the main package and the subpackage:
 *   packaging-manifest.json
 *   natives/<host>/toolchain/<target>/tinyc
 *   sysroot/<target>/...
 * and refreshes node_modules/@xnetx/tinyc-native-<host> so the subpackage
 * resolution path works in this repo. Run after ../../build.sh.
 *
 * The two macOS hosts share one subpackage: npm selects optional
 * dependencies by os/cpu, and one package declaring os darwin with
 * cpu arm64+x64 serves both. --merge-darwin stages only the merged
 * subpackage (the main package keeps whatever its last single-host
 * bundle left); the release aggregation job runs it after collecting
 * both darwin dists. The old per-arch subpackages
 * (@xnetx/tinyc-native-arm64-apple-darwin,
 * @xnetx/tinyc-native-x86_64-apple-darwin) are no longer published.
 */
const fs = require('fs');
const path = require('path');
const { execFileSync } = require('child_process');

function detectHost() {
  const p = process.platform, a = process.arch;
  if (p === 'linux' && a === 'x64') return 'x86_64-linux-gnu';
  if (p === 'linux' && a === 'arm64') return 'aarch64-linux-gnu';
  if (p === 'linux' && a === 'riscv64') return 'riscv64-linux-gnu';
  if (p === 'win32' && a === 'x64') return 'x86_64-w64-mingw32';
  if (p === 'darwin' && a === 'arm64') return 'arm64-apple-darwin';
  if (p === 'darwin' && a === 'x64') return 'x86_64-apple-darwin';
  throw new Error(`cannot detect a 64-bit host triple for ${p}/${a}`);
}

const NPM_OS_CPU = {
  'x86_64-linux-gnu': { os: ['linux'], cpu: ['x64'] },
  'aarch64-linux-gnu': { os: ['linux'], cpu: ['arm64'] },
  'riscv64-linux-gnu': { os: ['linux'], cpu: ['riscv64'] },
  'x86_64-w64-mingw32': { os: ['win32'], cpu: ['x64'] },
  'arm64-apple-darwin': { os: ['darwin'], cpu: ['arm64'] },
  'x86_64-apple-darwin': { os: ['darwin'], cpu: ['x64'] },
};

/* The merged macOS subpackage: one package, both darwin hosts. The
 * name keeps the -apple-darwin suffix both old names shared and drops
 * the arch, so it is distinct from both old names (no registry
 * version/semantics confusion with the per-arch packages). */
const MERGE_DARWIN = {
  hosts: ['arm64-apple-darwin', 'x86_64-apple-darwin'],
  subName: '@xnetx/tinyc-native-apple-darwin',
  osCpu: { os: ['darwin'], cpu: ['arm64', 'x64'] },
};

const args = process.argv.slice(2);
const mergeDarwin = args.includes('--merge-darwin');
/* Dist directories: --dist (repeatable), TINYC_DIST_DIRS (space
 * separated), TINYC_DIST_DIR (single value, as stage_layout reads
 * it). The merged darwin subpackage can only be assembled where
 * both darwin dists are present, so the release aggregation job
 * passes them here; with none given, stage_layout falls back to its
 * own default (TINYC_DIST_DIR or build/dist). */
const distArgs = [];
for (let i = 0; i < args.length; i++) {
  if (args[i] === '--dist') {
    if (i + 1 >= args.length) throw new Error('--dist needs a directory');
    distArgs.push(args[++i]);
  }
}
if (process.env.TINYC_DIST_DIRS) distArgs.push(...process.env.TINYC_DIST_DIRS.split(/\s+/).filter(Boolean));
if (process.env.TINYC_DIST_DIR) distArgs.push(process.env.TINYC_DIST_DIR);
const positional = args.filter((a, i) => a !== '--merge-darwin' && a !== '--dist' &&
  !(i > 0 && args[i - 1] === '--dist'));
if (positional.length > 1) throw new Error('bundle_native.js takes at most one host triple');
if (mergeDarwin && positional.length > 0) {
  throw new Error('--merge-darwin takes no host triple');
}
const host = mergeDarwin ? null : (positional[0] || detectHost());
const stage = path.join(__dirname, '..', '..', 'packaging', 'stage_layout.py');

/* Paths handed to the python3 child must also work when that child is
 * an MSYS2 build (the Windows CI step runs this script from an MSYS2
 * shell): MSYS2 Python treats a Windows-form absolute path (D:\...)
 * as a relative name. Pass paths relative to the CWD the child
 * inherits, with forward slashes - Windows Python and MSYS2 Python
 * resolve those identically, and stage_layout.py anchors --root with
 * os.path.abspath() against that same CWD, so the staged tree and its
 * manifest come out exactly as with the absolute forms. */
function childPath(abs) {
  const rel = path.relative(process.cwd(), abs);
  return (rel === '' ? '.' : rel).split(path.sep).join('/');
}

function stageInto(root, hosts) {
  for (const name of ['natives', 'sysroot', 'packaging-manifest.json', 'native']) {
    fs.rmSync(path.join(root, name), { recursive: true, force: true });
  }
  const hostArgs = [];
  for (const h of hosts) hostArgs.push('--host', h);
  const distFlags = [];
  for (const d of [...new Set(distArgs)]) distFlags.push('--dist', childPath(path.resolve(d)));
  execFileSync('python3', [childPath(stage), '--root', childPath(root), ...hostArgs, ...distFlags],
               { stdio: 'inherit' });
}

/* The merged manifest must carry both darwin hosts, each with a
 * host == target native combination; otherwise the subpackage would
 * silently serve only one arch. Fail naming the missing hosts
 * (TINYC_MERGE_ALLOW_PARTIAL=1 relaxes it for local dry runs). */
function checkMergedManifest(subDir, hosts) {
  const mf = JSON.parse(fs.readFileSync(
    path.join(subDir, 'packaging-manifest.json'), 'utf8'));
  const missing = hosts.filter((h) => !(mf.hosts || []).includes(h));
  const noNative = hosts.filter((h) => !(mf.combinations || [])
    .some((c) => c.host === h && c.target === h));
  console.log(`merged subpackage: hosts staged ${JSON.stringify(mf.hosts)} ` +
    `(${(mf.combinations || []).length} combinations)`);
  if (missing.length || noNative.length) {
    console.error(`ERROR: merged darwin subpackage is missing hosts ` +
      `${JSON.stringify(missing)} / hosts without a native combination ` +
      `${JSON.stringify(noNative)}`);
    if (process.env.TINYC_MERGE_ALLOW_PARTIAL !== '1') process.exit(1);
    console.error('WARNING: partial merged subpackage allowed by ' +
      'TINYC_MERGE_ALLOW_PARTIAL=1');
  }
}

const HOST_BLURB = {
  'x86_64-linux-gnu': { en: 'Linux on x86_64 (glibc)', zh: 'x86_64 Linux（glibc）' },
  'aarch64-linux-gnu': { en: 'Linux on arm64 (glibc)', zh: 'arm64 Linux（glibc）' },
  'riscv64-linux-gnu': { en: 'Linux on riscv64 (glibc)', zh: 'riscv64 Linux（glibc）' },
  'x86_64-w64-mingw32': { en: 'Windows on x86_64', zh: 'x86_64 Windows' },
  'arm64-apple-darwin': { en: 'macOS on arm64 (Apple Silicon)', zh: 'arm64 macOS（Apple Silicon）' },
  'x86_64-apple-darwin': { en: 'macOS on x86_64 (Intel)', zh: 'x86_64 macOS（Intel）' },
};

/* Write a platform subpackage: package.json, READMEs, and the
 * node_modules copy that makes it resolvable from this repo.
 * `hosts` is the list of host triples the subpackage serves (one for
 * the per-host subpackages, both darwin triples for the merged one);
 * the per-mode wording (host intro, layout line) is passed in. */
function writeSubpackage({ subName, subDir, hosts, osCpu, desc, introEn, introZh,
                            targetIntroEn, targetIntroZh, layoutEn, layoutZh }) {
  const mainPkg = JSON.parse(fs.readFileSync(path.join(__dirname, 'package.json'), 'utf8'));
  fs.mkdirSync(subDir, { recursive: true });
  fs.writeFileSync(path.join(subDir, 'package.json'), JSON.stringify({
    name: subName,
    version: mainPkg.version,
    description: desc,
    license: mainPkg.license,
    repository: {
      type: 'git',
      url: 'git+https://github.com/xnetsc/tinyc-in-one.git',
      directory: 'packages/npm',
    },
    os: osCpu.os,
    cpu: osCpu.cpu,
    files: ['natives', 'sysroot', 'packaging-manifest.json', 'README.md', 'README.zh-CN.md'],
  }, null, 2) + '\n');
  console.log(`subpackage ${subName} -> ${subDir}`);

  // ---- subpackage READMEs (the staging directory is generated, so the
  // READMEs are written here rather than kept in the repo) ----
  const stagedTargets = fs.readdirSync(path.join(subDir, 'sysroot'), { withFileTypes: true })
    .filter((e) => e.isDirectory()).map((e) => e.name).sort();
  const targetList = (selfMark) => stagedTargets
    .map((t) => `- \`${t}\`${hosts.includes(t) ? selfMark : ''}`).join('\n');
  const osText = (osCpu.os || []).join(', ');
  const cpuText = (osCpu.cpu || []).join(', ');
  fs.writeFileSync(path.join(subDir, 'README.md'), `# ${subName}

**English** | [简体中文](README.zh-CN.md)

Native toolchain package for the [\`@xnetx/tinyc\`](https://www.npmjs.com/package/@xnetx/tinyc)
Node.js package, ${introEn}

\`@xnetx/tinyc\` embeds the TinyC (TCC) compiler in Node.js. This subpackage
carries the host-specific half of that distribution: the dual-form
compiler binaries — each one is both the \`tcc\` command-line executable
and a library exporting the libtcc \`tcc_*\` API — and the bundled
headers and libraries (sysroots) ${targetIntroEn}

${targetList(' (this host)')}

## Installation

You do not normally install this package directly. The main \`@xnetx/tinyc\`
package declares every platform subpackage in its
\`optionalDependencies\`, and npm installs the one whose \`os\` / \`cpu\`
fields match your machine; this package matches
\`os: ${osText}\` / \`cpu: ${cpuText}\`, so \`npm install @xnetx/tinyc\` brings
it in automatically on such a host.

To install it explicitly anyway:

\`\`\`sh
npm install ${subName}
\`\`\`

## Layout

${layoutEn}
- \`sysroot/<target>/\` — bundled headers (\`include/\`) and libraries
  (\`lib/\`) for \`<target>\`.
- \`packaging-manifest.json\` — the host/target combinations and the
  output formats this package carries.

## See also

- Main package: [\`@xnetx/tinyc\`](https://www.npmjs.com/package/@xnetx/tinyc)
- Repository: <https://github.com/xnetsc/tinyc-in-one>
- License: ${mainPkg.license}
`);
  fs.writeFileSync(path.join(subDir, 'README.zh-CN.md'), `# ${subName}

[English](README.md) | **简体中文**

[\`@xnetx/tinyc\`](https://www.npmjs.com/package/@xnetx/tinyc)（Node.js 包）${introZh}

\`@xnetx/tinyc\` 把 TinyC（TCC）编译器嵌入 Node.js。本子包是其中与宿主相关的部分：双形态编译器本体——同一个文件既是 \`tcc\` 命令行可执行文件，也是导出 libtcc \`tcc_*\` API 的库——${targetIntroZh}

${targetList('（本宿主）')}

## 安装

通常不需要手动安装本包。主包 \`@xnetx/tinyc\` 把全部平台子包声明在 \`optionalDependencies\` 里，npm 按各子包的 \`os\` / \`cpu\` 字段自动装上与本机匹配的那一个；本包匹配 \`os: ${osText}\` / \`cpu: ${cpuText}\`，在这类宿主上 \`npm install @xnetx/tinyc\` 会自动带上它。

如需显式安装：

\`\`\`sh
npm install ${subName}
\`\`\`

## 包内布局

${layoutZh}
- \`sysroot/<target>/\`——该目标的头文件（\`include/\`）与库文件（\`lib/\`）。
- \`packaging-manifest.json\`——本包携带的宿主/目标组合与各组合的产物格式。

## 相关链接

- 主包：[\`@xnetx/tinyc\`](https://www.npmjs.com/package/@xnetx/tinyc)
- 仓库：<https://github.com/xnetsc/tinyc-in-one>
- 许可证：${mainPkg.license}
`);

  // ---- make the subpackage resolvable from this repo ----
  const shortName = subName.replace(/^@xnetx\//, '');
  const nmDir = path.join(__dirname, 'node_modules', '@xnetx', shortName);
  fs.rmSync(nmDir, { recursive: true, force: true });
  fs.mkdirSync(path.dirname(nmDir), { recursive: true });
  fs.cpSync(subDir, nmDir, { recursive: true });
  console.log(`linked ${subName} into node_modules for in-repo resolution`);
}

if (mergeDarwin) {
  const subDir = path.join(__dirname, 'native-packages', '@xnetx', 'tinyc-native-apple-darwin');
  stageInto(subDir, MERGE_DARWIN.hosts);
  checkMergedManifest(subDir, MERGE_DARWIN.hosts);
  writeSubpackage({
    subName: MERGE_DARWIN.subName,
    subDir,
    hosts: MERGE_DARWIN.hosts,
    osCpu: MERGE_DARWIN.osCpu,
    desc: 'TinyC native binaries for macOS hosts arm64-apple-darwin and ' +
      'x86_64-apple-darwin (dual-form TCC builds and target sysroots).',
    introEn: 'for hosts running **macOS on arm64 (Apple Silicon) and ' +
      'macOS on x86_64 (Intel)** (host triples `arm64-apple-darwin` and ' +
      '`x86_64-apple-darwin`).',
    introZh: '在本宿主上的原生工具链包，宿主为 **arm64 macOS（Apple Silicon）' +
      '与 x86_64 macOS（Intel）**（宿主三元组 `arm64-apple-darwin` 与 ' +
      '`x86_64-apple-darwin`）。',
    targetIntroEn: 'for every target these hosts build\nfor:',
    targetIntroZh: '以及这两个宿主可编译的全部目标的头文件与库文件（sysroot）：',
    layoutEn: '- `natives/{arm64-apple-darwin,x86_64-apple-darwin}/toolchain/<target>/tinyc` ' +
      '— dual-form compiler\n  binary: runs on the matching macOS host, generates code for `<target>`.',
    layoutZh: '- `natives/{arm64-apple-darwin,x86_64-apple-darwin}/toolchain/<target>/tinyc`' +
      '——双形态编译器本体：在对应 macOS 宿主上运行、为 `<target>` 生成代码。',
  });
} else {
  stageInto(__dirname, [host]);
  const subName = `@xnetx/tinyc-native-${host}`;
  const subDir = path.join(__dirname, 'native-packages', '@xnetx', `tinyc-native-${host}`);
  stageInto(subDir, [host]);
  const osCpu = NPM_OS_CPU[host] || {};
  const blurb = HOST_BLURB[host] || { en: host, zh: host };
  const exeName = host.includes('mingw') ? 'tinyc.exe' : 'tinyc';
  writeSubpackage({
    subName,
    subDir,
    hosts: [host],
    osCpu,
    desc: `TinyC native binaries for host ${host} (dual-form TCC builds and target sysroots).`,
    introEn: `for hosts running **${blurb.en}** (host triple\n\`${host}\`).`,
    introZh: `在本宿主上的原生工具链包，宿主为 **${blurb.zh}**（宿主三元组 \`${host}\`）。`,
    targetIntroEn: 'for every target this host builds\nfor:',
    targetIntroZh: '以及本宿主可编译的全部目标的头文件与库文件（sysroot）：',
    layoutEn: `- \`natives/${host}/toolchain/<target>/${exeName}\` — dual-form compiler\n` +
      '  binary: runs on this host, generates code for `<target>`.',
    layoutZh: `- \`natives/${host}/toolchain/<target>/${exeName}\`——双形态编译器本体：在本宿主上运行、为 \`<target>\` 生成代码。`,
  });
}
