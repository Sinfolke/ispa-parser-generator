-- Add this directory to 'runtimepath' to enable ISPA grammar files.
vim.filetype.add({ extension = { isc = 'ispa' } })

vim.api.nvim_create_autocmd('FileType', {
  pattern = 'ispa',
  callback = function(args)
    vim.lsp.start({
      name = 'ispa',
      cmd = { vim.g.ispa_lsp_command or 'ispa-lsp' },
      root_dir = vim.fs.root(args.buf, { 'ispa-package.yaml', '.git' }) or vim.fn.getcwd(),
    }, { bufnr = args.buf })
  end,
})
