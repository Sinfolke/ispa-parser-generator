// Runs the whole pipeline off the main thread: the ISPA generator (WebAssembly) turns the
// grammar into Python, then Pyodide imports that parser and parses the test input.
import createIspa from './runtime/ispa.mjs';

const PYODIDE = 'https://cdn.jsdelivr.net/pyodide/v0.27.7/full/';
const say = (type, payload = {}) => postMessage({type, ...payload});

let generator, python;

async function loadGenerator() {
  say('status', {text: 'Downloading the ISPA generator…'});
  return {module: await WebAssembly.compileStreaming(fetch('runtime/ispa.wasm'))};
}

async function loadPython() {
  say('status', {text: 'Starting Python…'});
  const {loadPyodide} = await import(PYODIDE + 'pyodide.mjs');
  const pyodide = await loadPyodide({indexURL: PYODIDE});
  const sources = await (await fetch('runtime/python-runtime.json')).json();
  for (const [path, text] of Object.entries(sources)) {
    pyodide.FS.mkdirTree(`/ispa_py/${path.split('/').slice(0, -1).join('/')}`);
    pyodide.FS.writeFile(`/ispa_py/${path}`, text);
  }
  for (const name of ['runner.py', 'serialize.py'])
    pyodide.FS.writeFile(`/ispa_py/${name}`, await (await fetch(name)).text());
  pyodide.FS.mkdirTree('/gen');
  pyodide.runPython("import sys; sys.path[:0] = ['/ispa_py', '/gen']");
  return pyodide;
}

// A fresh generator instance per run: the C++ side keeps process-wide state and may call exit().
async function generate(files, logLine) {
  const module = await createIspa({
    noInitialRun: true,
    print: line => logLine('out', line),
    printErr: line => logLine('err', line),
    instantiateWasm(imports, done) {
      WebAssembly.instantiate(generator.module, imports).then(instance => done(instance, generator.module));
      return {};
    },
  });
  const {FS} = module;
  FS.mkdir('/work');
  for (const [name, text] of Object.entries(files)) FS.writeFile(`/work/${name}`, text);
  let code;
  try {
    code = module.callMain(['-d', '/work', '-o', '/out/Generated', '-l', 'python', '-a', 'LL', '--no-ai-recovery']);
  } catch (error) {
    if (error?.name === 'ExitStatus') code = error.status;
    else { logLine('err', String(error?.message ?? error)); code = -1; }
  }
  const outputs = {};
  try {
    for (const name of FS.readdir('/out')) {
      if (name.endsWith('.py')) outputs[name] = FS.readFile(`/out/${name}`, {encoding: 'utf8'});
    }
  } catch { /* nothing was generated */ }
  return {code, outputs};
}

async function run({id, files, input, script}) {
  const log = [];
  const logLine = (stream, text) => { log.push({stream, text}); say('log', {id, stream, text}); };
  generator ??= await loadGenerator();
  const started = performance.now();
  say('status', {text: 'Generating parser…'});
  const {code, outputs} = await generate(files, logLine);
  const modules = Object.keys(outputs);
  if (code !== 0 || !modules.length) {
    say('result', {id, ok: false, stage: 'generate', error: 'The grammar could not be compiled. See the console for details.', log});
    return;
  }
  const generatedMs = performance.now() - started;
  python ??= await loadPython();
  say('status', {text: 'Parsing…'});
  for (const name of python.FS.readdir('/gen').filter(n => n.endsWith('.py'))) python.FS.unlink(`/gen/${name}`);
  for (const [name, text] of Object.entries(outputs)) python.FS.writeFile(`/gen/${name}`, text);
  const entry = modules.includes('Generated.py') ? 'Generated.py' : modules[0];
  const runner = python.pyimport('runner');
  const outcome = JSON.parse(runner.run(entry.slice(0, -3), input, script));
  runner.destroy();
  say('result', {id, ...outcome, outputs, log, timing: {generated: generatedMs, total: performance.now() - started}});
}

let queue = Promise.resolve();
onmessage = ({data}) => {
  if (data.type !== 'run') return;
  queue = queue.then(() => run(data)).catch(error =>
    say('result', {id: data.id, ok: false, stage: 'internal', error: String(error?.stack ?? error)}));
};
say('ready');
