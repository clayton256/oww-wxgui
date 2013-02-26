"-------------------------------------------------------------------------------
" Set wxWidget makefiles - platform specific 
"-------------------------------------------------------------------------------
if has('win32')
  set makeprg=nmake\ -f\ makefile.vc
elseif has('mac')
  set makeprg=make\ -f\ makefile.mac
elseif has('unix')
  set makeprg=make\ -f\ makefile.unx
endif

"-------------------------------------------------------------------------------
" Load syntax enhancements for wxWindows, if installed
"-------------------------------------------------------------------------------
" if filereadable("syntax/wxwin.vim")
  runtime syntax/wxwin.vim
" endif




