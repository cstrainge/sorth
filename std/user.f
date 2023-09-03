
( Some user environment words. )
: user.home  ( -- home_path  ) description: "The user's 'home' path."
    "HOME"  user.env@
;


: user.name  ( -- user_name  ) description: "The name of the current user."
    "USER"  user.env@
;


: user.shell ( -- shell_path ) description: "The default shell of the current user."
    "SHELL" user.env@
;


: user.term  ( -- term_name  ) description: "The terminal we are running in."
    "TERM"  user.env@
;
