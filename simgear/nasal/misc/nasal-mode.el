;;; nasal-mode.el --- A major mode for writing Nasal code. -*- lexical-binding: t; -*-
;;
;; Copyright (C) 2003, 2005, 2006 Andrew Ross
;;
;;; Commentary:
;;
;; A major mode for writing Nasal code.
;;
;; It should be sufficient to drop this into your Emacs site-lisp
;; directory (/usr/share/emacs/site-lisp on most linux distributions)
;; and add a line:
;;
;; (require 'nasal-mode)
;;
;; ...to your .emacs file.  All files with a .nas extension should
;; then be associated with nasal mode automatically.  I am *not* an
;; elisp hacker, though, so YMMV.
;;
;;; Code:

(defvar nasal-mode-syntax-table nil
  "Syntax table in use in Nasal-mode buffers.")

(if nasal-mode-syntax-table ()
  (setq nasal-mode-syntax-table (make-syntax-table))
  ;; Operator characters are "punctuation"
  (modify-syntax-entry ?!  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?*  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?+  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?-  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?/  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?~  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?:  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?.  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?,  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?\; "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?=  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?<  "."  nasal-mode-syntax-table)
  (modify-syntax-entry ?>  "."  nasal-mode-syntax-table)
  ;; Underscores are allowed as "symbol constituent"
  (modify-syntax-entry ?_  "_"  nasal-mode-syntax-table)
  ;; "m" corresponds to the matching operator in Perl. Mark it as "symbol
  ;; constituent" in Nasal.
  (modify-syntax-entry ?m  "_"  nasal-mode-syntax-table)
  ;; Backslash escapes; pound sign starts comments
  (modify-syntax-entry ?\\ "\\" nasal-mode-syntax-table)
  (modify-syntax-entry ?\# "<"  nasal-mode-syntax-table)
  (modify-syntax-entry ?\n ">#"  nasal-mode-syntax-table)
  ;; Square brackets act as parentheses
  (modify-syntax-entry ?\[ "(]"  nasal-mode-syntax-table)
  (modify-syntax-entry ?\] ")["  nasal-mode-syntax-table))

(defconst nasal-font-lock-keywords
  (eval-when-compile
    (list
     (cons (regexp-opt '("parents" "me" "arg") 'words)
           'font-lock-variable-name-face)
     (regexp-opt '("and" "break" "continue" "else" "elsif" "false" "for"
                   "foreach" "forindex" "func" "if" "nil" "or" "return"
                   "true" "var" "while")
                 'words)
     (list (regexp-opt '("append" "bind" "call" "caller" "chr" "closure"
                         "cmp" "compile" "contains" "delete" "die" "find"
                         "int" "keys" "num" "pop" "rand" "setsize" "size"
                         "split" "sprintf" "streq" "substr" "subvec" "typeof")
                       'words)
           1 'font-lock-builtin-face)))
  "Nasal-specific syntax to be hilighted.")

(define-derived-mode nasal-mode perl-mode "Nasal"
  "Major mode for editing Nasal code.
This is a Perl mode variant customized for Nasal's syntax.  It shares most of
perl-mode's features.  Turning on Nasal mode runs `nasal-mode-hook'."
  (set (make-local-variable 'comment-start) "# ")
  (set (make-local-variable 'comment-end) "")
  (set (make-local-variable 'comment-start-skip) "#+ *")
  (set (make-local-variable 'c-assignment-op-regexp) "nevermatch")
  (setq font-lock-defaults '(nasal-font-lock-keywords nil nil ((?_ . "w")))))

;; Set us up to load by default for .nas files
(setq auto-mode-alist (append '(("\\.nas$" . nasal-mode)) auto-mode-alist))

(provide 'nasal-mode)

;;; nasal-mode.el ends here
