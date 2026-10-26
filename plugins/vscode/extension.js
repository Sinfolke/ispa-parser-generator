const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');

let client;

function activate(context) {
  const config = vscode.workspace.getConfiguration('ispa');
  const command = config.get('server.command');
  const python = config.get('server.python');
  const compiler = config.get('compiler.path');
  const executable = command.endsWith('.py') ? python : command;
  const args = command.endsWith('.py') ? [command, '--ispa', compiler] : ['--ispa', compiler];
  const serverOptions = { command: executable, args };
  const clientOptions = { documentSelector: [{ scheme: 'file', language: 'ispa' }] };
  client = new LanguageClient('ispa', 'ISPA Language Server', serverOptions, clientOptions);
  client.start();
  context.subscriptions.push({ dispose: () => client.stop() });
}

async function deactivate() {
  if (client) await client.stop();
}

module.exports = { activate, deactivate };
