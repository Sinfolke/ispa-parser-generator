// Drives the real playground page in headless Chrome: clicks "Generate & parse" and checks the result.
// usage: node scripts/test-playground.mjs <playground-dir> [chrome-binary]
import {spawn} from 'node:child_process';
import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';

const root = path.resolve(process.argv[2] ?? 'playground');
const chromeBin = process.argv[3] ?? 'google-chrome';
const types = {'.html': 'text/html', '.js': 'text/javascript', '.mjs': 'text/javascript', '.css': 'text/css',
  '.wasm': 'application/wasm', '.json': 'application/json', '.py': 'text/plain'};
const server = http.createServer((req, res) => {
  const file = path.join(root, decodeURIComponent(req.url.split('?')[0]).replace(/^\/$/, '/index.html'));
  if (!file.startsWith(root) || !fs.existsSync(file)) { res.writeHead(404).end(); return; }
  res.writeHead(200, {'content-type': types[path.extname(file)] ?? 'application/octet-stream'});
  fs.createReadStream(file).pipe(res);
}).listen(0);
const port = server.address().port;

const chrome = spawn(chromeBin, ['--headless=new', '--remote-debugging-port=0', '--no-sandbox', '--disable-gpu',
  '--user-data-dir=' + fs.mkdtempSync('/tmp/ispa-chrome-'), 'about:blank'], {stdio: ['ignore', 'ignore', 'pipe']});
const endpoint = await new Promise((resolve, reject) => {
  let log = '';
  chrome.stderr.on('data', d => { log += d; const m = /ws:\/\/\S+/.exec(log); if (m) resolve(m[0]); });
  chrome.on('exit', () => reject(new Error('chrome exited: ' + log)));
});
const ws = new WebSocket(endpoint);
await new Promise(r => ws.addEventListener('open', r));
let nextId = 0; const pending = new Map();
ws.addEventListener('message', ({data}) => { const m = JSON.parse(data); pending.get(m.id)?.(m); });
const send = (method, params = {}, sessionId) => new Promise(resolve => {
  const id = ++nextId; pending.set(id, resolve); ws.send(JSON.stringify({id, method, params, sessionId}));
});
const {result: {targetId}} = await send('Target.createTarget', {url: 'about:blank'});
const {result: {sessionId}} = await send('Target.attachToTarget', {targetId, flatten: true});
const evaluate = async expression => (await send('Runtime.evaluate', {expression, returnByValue: true, awaitPromise: true}, sessionId)).result?.result?.value;
const until = async (expression, timeoutMs, label) => {
  const end = Date.now() + timeoutMs;
  while (Date.now() < end) { const v = await evaluate(expression); if (v) return v; await new Promise(r => setTimeout(r, 500)); }
  throw new Error('timed out waiting for ' + label + '; status: ' + await evaluate("document.getElementById('status').textContent"));
};
let failure;
try {
  await send('Page.navigate', {url: `http://localhost:${port}/`}, sessionId);
  await until("!document.getElementById('run')?.disabled", 60000, 'the runtime to become ready');
  await evaluate("document.getElementById('run').click()");
  await until("document.getElementById('status').dataset.state === 'ok' || document.getElementById('status').dataset.state === 'error'", 240000, 'the run to finish');
  const state = await evaluate("document.getElementById('status').dataset.state");
  const ast = await evaluate("document.getElementById('ast').textContent");
  if (state !== 'ok' || !ast.includes('"123"')) throw new Error(`run failed (${state}): ${ast.slice(0, 800)}`);
  console.log('playground ok:', await evaluate("document.getElementById('status').textContent"));
} catch (error) { failure = error; }
chrome.kill(); server.close();
if (failure) { console.error(failure.message); process.exit(1); }
process.exit(0);
