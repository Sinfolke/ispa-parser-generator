const $ = id => document.getElementById(id);
const SCRIPT = 'script.py';
const DEFAULT_SCRIPT = `# Runs after your grammar compiles.
# Available: Parser, grammar (the generated module), text (the test input), to_json.
# Assign what you want to see to \`result\`; print() output goes to the console.
tree = Parser().parse(text).data()
result = tree
`;
const initial = {'main.isc': 'module main -> main;\n\nNUMBER:\n    @ [0-9]+ {@};\n\nmain:\n    @ NUMBER {@};\n', [SCRIPT]: DEFAULT_SCRIPT};
const KEY = 'ispa-playground';

let files = {...initial}, active = 'main.isc', editor;
try {
  const saved = JSON.parse(localStorage.getItem(KEY));
  if (saved && typeof saved.files === 'object' && Object.values(saved.files).every(v => typeof v === 'string')) {
    files = {...initial, ...saved.files};
    if (typeof saved.input === 'string') $('input').value = saved.input;
  }
} catch {}
const isGrammar = name => name.endsWith('.isc');
if (!isGrammar(active) || !(active in files)) active = Object.keys(files).find(isGrammar) ?? SCRIPT;

let storageWarned = false;
function save() {
  try { localStorage.setItem(KEY, JSON.stringify({files, input: $('input').value})); }
  catch { if (!storageWarned) { storageWarned = true; setStatus('Browser storage is unavailable. Keep a copy of your work.', 'warn'); } }
}
function setStatus(text, state = 'idle') {
  $('status').dataset.state = state;
  $('status').lastElementChild.textContent = text;
}

if (globalThis.CodeMirror) {
  CodeMirror.defineMode('ispa', () => ({token(stream) {
    if (stream.eatSpace()) return null;
    if (stream.match('//')) { stream.skipToEnd(); return 'comment'; }
    if (stream.match(/"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*'/)) return 'string';
    if (stream.match(/\b(module|import|from|as)\b/)) return 'keyword';
    if (stream.match(/[A-Z][A-Z_0-9]*/)) return 'atom';
    if (stream.match(/[@&$]|->/)) return 'operator';
    stream.next(); return null;
  }}));
  editor = CodeMirror.fromTextArea($('source'), {theme: 'material-darker', lineNumbers: true, indentUnit: 4, lineWrapping: true,
    extraKeys: {Tab: cm => cm.somethingSelected() ? cm.indentSelection('add') : cm.replaceSelection('    ', 'end')}});
  editor.setSize(null, 470);
  editor.on('change', (_, change) => { if (change.origin !== 'setValue') { files[active] = editor.getValue(); save(); } });
}
const modeOf = name => isGrammar(name) ? 'ispa' : 'python';

function render() {
  $('files').replaceChildren(...Object.keys(files).map(name => {
    const button = document.createElement('button');
    button.textContent = name; button.setAttribute('aria-current', String(name === active));
    if (!isGrammar(name)) button.classList.add('script');
    button.onclick = () => { active = name; render(); };
    return button;
  }));
  $('hint').textContent = isGrammar(active)
    ? 'ISPA grammar · Changes saved in this browser'
    : 'Python · runs against your generated parser · Changes saved in this browser';
  $('source').value = files[active];
  if (editor) { editor.setOption('mode', modeOf(active)); editor.setValue(files[active]); }
}
$('source').oninput = () => { files[active] = $('source').value; save(); };
$('input').oninput = save;
$('add').onclick = () => {
  const name = prompt('File name (for example tokens.isc)');
  if (!name) return;
  if (!/^[\w-]+\.isc$/.test(name) || Object.hasOwn(files, name)) { setStatus('Choose a unique .isc filename using letters, numbers, underscores or hyphens.', 'warn'); return; }
  files[name] = ''; active = name; save(); render();
};

// ---- output tabs ----
function showTab(name) {
  for (const tab of document.querySelectorAll('#tabs button')) tab.setAttribute('aria-selected', String(tab.dataset.tab === name));
  for (const id of ['ast', 'generated', 'console']) $(id).hidden = id !== name;
}
for (const tab of document.querySelectorAll('#tabs button')) tab.onclick = () => showTab(tab.dataset.tab);
$('copy').onclick = async () => {
  const visible = ['ast', 'generated', 'console'].find(id => !$(id).hidden);
  try { await navigator.clipboard.writeText($(visible).textContent); setStatus('Copied.', 'ok'); }
  catch { setStatus('Select the output and copy it manually.', 'warn'); }
};

// ---- worker ----
let worker, runId = 0, running = false;
function startWorker() {
  worker = new Worker('worker.js', {type: 'module'});
  worker.onmessage = ({data}) => {
    if (data.type === 'ready') { $('run').disabled = false; setStatus('Ready. Press Ctrl+Enter to generate and parse.'); }
    else if (data.type === 'status') setStatus(data.text, 'busy');
    else if (data.type === 'log' && data.id === runId) appendConsole(data.stream, data.text);
    else if (data.type === 'result' && data.id === runId) finish(data);
  };
  worker.onerror = event => { finishRun(); setStatus('The playground runtime failed to load: ' + (event.message || 'see the browser console'), 'error'); };
}
function appendConsole(stream, text) {
  const line = document.createElement('span');
  line.className = stream; line.textContent = text + '\n';
  $('console').append(line);
}
function setRunning(state) {
  running = state;
  $('run').disabled = state; $('stop').hidden = !state;
}
function finishRun() { setRunning(false); }

function run() {
  if (running) return;
  const grammar = Object.fromEntries(Object.entries(files).filter(([name]) => isGrammar(name)));
  if (!Object.keys(grammar).length) { setStatus('Add a .isc grammar file first.', 'warn'); return; }
  runId += 1; setRunning(true);
  $('console').replaceChildren();
  setStatus('Starting…', 'busy');
  worker.postMessage({type: 'run', id: runId, files: grammar, input: $('input').value,
    script: files[SCRIPT] ?? DEFAULT_SCRIPT});
}
function finish(data) {
  finishRun();
  if (data.outputs) $('generated').textContent = Object.entries(data.outputs).map(([n, t]) => `# ${n}\n${t}`).join('\n\n');
  if (data.printed) appendConsole('out', data.printed.replace(/\n$/, ''));
  if (data.ok) {
    $('ast').textContent = JSON.stringify(data.ast, null, 2) ?? 'null';
    showTab('ast');
    setStatus(`Parsed in ${Math.round(data.timing.total)} ms (generator ${Math.round(data.timing.generated)} ms).`, 'ok');
  } else {
    const where = {generate: 'Grammar error', import: 'Generated parser failed to import', parse: 'Parse error', script: 'Script error'}[data.stage] ?? 'Error';
    $('ast').textContent = [data.error, data.trace].filter(Boolean).join('\n\n');
    showTab(data.stage === 'generate' ? 'console' : 'ast');
    setStatus(`${where}.`, 'error');
  }
}
$('run').onclick = run;
$('stop').onclick = () => {
  worker.terminate(); finishRun(); $('run').disabled = true;
  setStatus('Stopped. Restarting the runtime…', 'busy'); startWorker();
};
addEventListener('keydown', event => {
  if (event.key === 'Enter' && (event.ctrlKey || event.metaKey)) { event.preventDefault(); run(); }
});

render();
startWorker();
