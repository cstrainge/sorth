
( Some user environment words. )


: user.home  description: "The user's 'home' path."
            signature: " -- home_path"
    "HOME"  user.env@
;


: user.name  description: "The name of the current user."
            signature: " -- user_name"
    "USER"  user.env@
;


: user.shell description: "The default shell of the current user."
            signature: " -- shell_path"
    "SHELL" user.env@
;


: user.term  description: "The terminal we are running in."
            signature: " -- term_name"
    "TERM"  user.env@
;


"/" constant user.path_sep
