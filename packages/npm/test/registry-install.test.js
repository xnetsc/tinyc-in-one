'use strict';
/* Registry-form install test.
 *
 * The package as published to the npm registry carries no native
 * staging: no natives/, no sysroot/, no packaging-manifest.json. The
 * platform subpackage @xnetx/tinyc-native-<host> must arrive on its
 * own through the main package's optionalDependencies (npm selects
 * exactly one by the subpackages' os/cpu fields) and the loader must
 * resolve through it. The tarball consumer tests cannot see this
 * path: they always install the main tarball and the subpackage
 * tarball side by side, explicitly.
 *
 * Simulated locally and offline: the main package is packed from a
 * staging-free copy; npm overrides redirect the current host's
 * subpackage to the locally bundled one, the other four subpackages
 * to stubs carrying their hosts' os/cpu (npm must skip those), and
 * koffi to the in-repo copy. The consumer project declares only the
 * main package, so the subpackage can only have arrived through
 * optionalDependencies.
 */
const { test } = require('node:test');
const assert = require('node:assert');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { execFileSync, spawnSync } = require('child_process');
const tcc = require('../index.js');

const PKG_ROOT = path.join(__dirname, '..');
const HOST = tcc.hostTriple();
const SUB_PREFIX = '@xnetx/tinyc-native-';

/* The npm host matrix (design doc 12.2/12.3): five hosts, one
 * subpackage each, told apart only by the subpackage os/cpu. */
const HOST_OS_CPU = {
  'x86_64-linux-gnu': { os: ['linux'], cpu: ['x64'] },
  'aarch64-linux-gnu': { os: ['linux'], cpu: ['arm64'] },
  'x86_64-w64-mingw32': { os: ['win32'], cpu: ['x64'] },
  'arm64-apple-darwin': { os: ['darwin'], cpu: ['arm64'] },
  'x86_64-apple-darwin': { os: ['darwin'], cpu: ['x64'] },
};

function run(cmd, args, opts = {}) {
  const r = spawnSync(cmd, args, { encoding: 'utf8', ...opts });
  assert.strictEqual(r.status, 0,
    `${cmd} ${args.join(' ')} failed (${r.status}):\n${r.stdout}\n${r.stderr}`);
  return r.stdout;
}

function pack(dir, dest) {
  const out = run('npm', ['pack', '--silent', '--pack-destination', dest], { cwd: dir });
  return path.join(dest, out.trim().split('\n').pop());
}

test('registry form: main package alone installs, its host subpackage auto-resolves, the compiler works',
  { timeout: 180000 }, () => {
    const pkg = JSON.parse(fs.readFileSync(path.join(PKG_ROOT, 'package.json'), 'utf8'));

    // The declaration itself is part of the contract: every host in
    // the matrix is an optional dependency, nothing more, nothing less.
    const declared = Object.keys(pkg.optionalDependencies || {})
      .map((n) => n.replace(SUB_PREFIX, '')).sort();
    assert.deepStrictEqual(declared, Object.keys(HOST_OS_CPU).sort(),
      'optionalDependencies must declare exactly the five platform subpackages');
    for (const v of Object.values(pkg.optionalDependencies)) {
      assert.strictEqual(v, pkg.version, 'subpackage versions track the main package version');
    }

    const subDir = path.join(PKG_ROOT, 'native-packages', '@xnetx', `tinyc-native-${HOST}`);
    assert.ok(fs.existsSync(path.join(subDir, 'packaging-manifest.json')),
      `locally bundled subpackage missing for host ${HOST}; run node bundle_native.js first`);
    const koffiDir = path.join(PKG_ROOT, 'node_modules', 'koffi');
    assert.ok(fs.existsSync(koffiDir), 'in-repo koffi missing; run npm install in packages/npm first');

    const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'tinyc-registry-'));
    try {
      // Main package, registry form: code and docs, no staging.
      const mainStage = path.join(tmp, 'main');
      fs.mkdirSync(mainStage);
      for (const f of ['package.json', 'index.js', 'README.md', 'README.zh-CN.md']) {
        fs.copyFileSync(path.join(PKG_ROOT, f), path.join(mainStage, f));
      }
      const mainTgz = pack(mainStage, tmp);
      const listing = execFileSync('tar', ['-tzf', mainTgz], { encoding: 'utf8' });
      assert.ok(!listing.includes('packaging-manifest.json'),
        'registry main tarball must not carry a packaging manifest');
      assert.ok(!/\/natives\//.test(listing) && !/\/sysroot\//.test(listing),
        'registry main tarball must not carry native staging');

      const subTgz = pack(subDir, tmp);

      // Stubs for the other four hosts: name, version and that host's
      // os/cpu, nothing else - npm must skip every one of them here.
      const overrides = { koffi: `file:${koffiDir}` };
      overrides[`${SUB_PREFIX}${HOST}`] = `file:${subTgz}`;
      for (const [h, oc] of Object.entries(HOST_OS_CPU)) {
        if (h === HOST) continue;
        const stub = path.join(tmp, `stub-${h}`);
        fs.mkdirSync(stub);
        fs.writeFileSync(path.join(stub, 'package.json'), JSON.stringify({
          name: `${SUB_PREFIX}${h}`, version: pkg.version, os: oc.os, cpu: oc.cpu,
        }));
        overrides[`${SUB_PREFIX}${h}`] = `file:${stub}`;
      }

      const consumer = path.join(tmp, 'consumer');
      fs.mkdirSync(consumer);
      fs.writeFileSync(path.join(consumer, 'package.json'), JSON.stringify({
        name: 'tinyc-registry-consumer', version: '1.0.0', private: true,
        dependencies: { '@xnetx/tinyc': `file:${mainTgz}` },
        overrides,
      }, null, 2));
      run('npm', ['install', '--ignore-scripts', '--no-audit', '--no-fund', '--loglevel=error'],
        { cwd: consumer });

      // Exactly the matching subpackage arrived; the main package is
      // the staging-free registry form.
      const installedSubs = fs.readdirSync(path.join(consumer, 'node_modules', '@xnetx'))
        .filter((s) => s.startsWith('tinyc-native-'));
      assert.deepStrictEqual(installedSubs, [`tinyc-native-${HOST}`],
        'npm must install exactly the subpackage for this host');
      const installedMain = path.join(consumer, 'node_modules', '@xnetx', 'tinyc');
      assert.ok(!fs.existsSync(path.join(installedMain, 'packaging-manifest.json')));
      assert.ok(!fs.existsSync(path.join(installedMain, 'natives')));

      // And the installed pair actually compiles and runs, resolving
      // through the subpackage root. TINYC_* overrides are scrubbed
      // so only the registry resolution path can be at work.
      const env = { ...process.env };
      for (const k of Object.keys(env)) if (k.startsWith('TINYC_')) delete env[k];
      const script = `
        const assert = require('node:assert');
        const fs = require('fs'), os = require('os'), path = require('path');
        const { execFileSync } = require('child_process');
        const tinyc = require('@xnetx/tinyc');
        const seg = '@xnetx' + path.sep + 'tinyc-native-${HOST}';
        assert.ok(tinyc.distDir().includes(seg),
          'resolution must go through the subpackage, got ' + tinyc.distDir());
        const [vc, vout] = tinyc.runCliCapture(['--version']);
        assert.strictEqual(vc, 0);
        assert.match(vout.toString(), /tcc version/);
        const d = fs.mkdtempSync(path.join(os.tmpdir(), 'tinyc-registry-hello-'));
        const src = path.join(d, 'hello.c');
        fs.writeFileSync(src, '#include <stdio.h>\\nint main(void) { printf("hello from tinyc\\\\n"); return 0; }\\n');
        const exe = path.join(d, process.platform === 'win32' ? 'hello.exe' : 'hello');
        const [cc] = tinyc.runCliCapture([src, '-o', exe]);
        assert.strictEqual(cc, 0);
        assert.strictEqual(execFileSync(exe).toString().replace(/\\r\\n/g, '\\n'), 'hello from tinyc\\n');
        const [rc, rout] = tinyc.runCliCapture(['-run', src]);
        assert.strictEqual(rc, 0);
        assert.match(rout.toString(), /hello from tinyc/);
        const s = new tinyc.State();
        s.compileString('int add(int a, int b) { return a + b; }');
        assert.strictEqual(s.function('add', 'int', ['int', 'int'])(19, 23), 42);
        s.close();
        console.log('REGISTRY-FORM OK');
      `;
      const r = spawnSync(process.execPath, ['-e', script],
        { cwd: consumer, env, encoding: 'utf8' });
      assert.strictEqual(r.status, 0, `consumer script failed:\n${r.stdout}\n${r.stderr}`);
      assert.match(r.stdout, /REGISTRY-FORM OK/);
    } finally {
      fs.rmSync(tmp, { recursive: true, force: true });
    }
  });
