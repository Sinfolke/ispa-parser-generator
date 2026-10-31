// Runs the packaged WebAssembly generator on a tiny grammar and checks it produced a Python parser.
// usage: node scripts/test-wasm.mjs <playground-dir>
import path from 'node:path';
import {pathToFileURL} from 'node:url';

const playground = path.resolve(process.argv[2] ?? 'playground');
const {default: createIspa} = await import(pathToFileURL(path.join(playground, 'runtime', 'ispa.mjs')).href);
const output = [];
const ispa = await createIspa({noInitialRun: true, print: line => output.push(line), printErr: line => output.push(line)});
ispa.FS.mkdir('/work');
ispa.FS.writeFile('/work/main.isc', 'module main -> main;\n\nNUMBER:\n    @ [0-9]+ {@};\n\nmain:\n    @ NUMBER {@};\n');
let code;
try { code = ispa.callMain(['-d', '/work', '-o', '/out/Generated', '-l', 'python', '-a', 'LL', '--no-ai-recovery']); }
catch (error) { code = error?.name === 'ExitStatus' ? error.status : `threw: ${error?.message ?? error}`; }
const files = (() => { try { return ispa.FS.readdir('/out').filter(n => n.endsWith('.py')); } catch { return []; } })();
if (code !== 0 || !files.length) {
  console.error(output.join('\n'));
  console.error(`generator failed (exit ${code}, files: ${files})`);
  process.exit(1);
}
console.log(`generator ok: ${files.join(', ')}`);
