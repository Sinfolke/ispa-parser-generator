# ISPA Grammar for VS Code

This extension recognizes `.isc` files, highlights grammar syntax, and starts
`ispa-lsp` for diagnostics, local rule completion, document symbols, and local
go to definition.

Install the server and compiler first. From a source checkout, run
`python3 -m pip install .` and put the built `ispa` executable on `PATH`.
The server is also available as `python3 -m ispa_pkg.lsp`.

To build a VSIX, run `npm install` and `npx @vscode/vsce package` in this
directory, then install the resulting VSIX with VS Code's **Install from VSIX**
command. Set `ispa.compiler.path` if the compiler is outside `PATH`. Set
`ispa.server.command` if `ispa-lsp` is outside `PATH`.

The server currently resolves definitions and completion within the open file.
Compiler diagnostics cover the unsaved buffer and local imports.
