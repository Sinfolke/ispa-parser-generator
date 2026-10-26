;;; ispa-mode.el --- ISPA grammar mode with Eglot -*- lexical-binding: t; -*-

(require 'prog-mode)

;;;###autoload
(define-derived-mode ispa-mode prog-mode "ISPA"
  "Major mode for ISPA .isc grammar files."
  (setq-local comment-start "// ")
  (setq-local comment-end "")
  (when (and (require 'eglot nil t) (executable-find "ispa-lsp"))
    (eglot-ensure)))

;;;###autoload
(add-to-list 'auto-mode-alist '("\\.isc\\'" . ispa-mode))

(with-eval-after-load 'eglot
  (add-to-list 'eglot-server-programs '(ispa-mode . ("ispa-lsp"))))

(provide 'ispa-mode)
;;; ispa-mode.el ends here
