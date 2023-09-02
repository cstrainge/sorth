
( Some user environment words. )
: user.home  ( -- home_path  ) "HOME"  user.env@ ;
: user.name  ( -- user_name  ) "USER"  user.env@ ;
: user.shell ( -- shell_path ) "SHELL" user.env@ ;
: user.term  ( -- term_name  ) "TERM"  user.env@ ;
